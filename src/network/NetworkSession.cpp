#include "NetworkSession.h"

#include <cstring>
#include <iostream>

#ifdef _WIN32
#    include <winsock2.h>
#    include <ws2tcpip.h>
#    pragma comment(lib, "ws2_32.lib")
using SocketHandle = SOCKET;
#else
#    include <arpa/inet.h>
#    include <netinet/in.h>
#    include <sys/socket.h>
#    include <unistd.h>
using SocketHandle = int;
#    define INVALID_SOCKET (-1)
#    define SOCKET_ERROR   (-1)
#endif

namespace
{
    constexpr const char* kHelloMsg = "cin-hello";

#ifdef _WIN32
    struct WinSockGuard
    {
        WinSockGuard()
        {
            WSADATA data;
            initialized = (WSAStartup(MAKEWORD(2, 2), &data) == 0);
        }
        ~WinSockGuard()
        {
            if (initialized)
                WSACleanup();
        }
        bool initialized = false;
    };
#endif
}

NetworkSession::NetworkSession()
{
#ifdef _WIN32
    static WinSockGuard guard;
    if (!guard.initialized)
        updateStatus("Winsock initialization failed.");
#endif
    updateStatus("Idle");
}

NetworkSession::~NetworkSession()
{
    Shutdown();
}

bool NetworkSession::StartHosting(uint16_t port)
{
#ifdef _WIN32
    static WinSockGuard guard;
    if (!guard.initialized)
        return false;
#endif
    Shutdown();
    stop_ = false;
    mode_ = Mode::Host;
    running_ = true;
    worker_ = std::thread(&NetworkSession::hostThread, this, port);
    updateStatus("Hosting on port " + std::to_string(port) + " ...");
    return true;
}

bool NetworkSession::ConnectToHost(const std::string& host, uint16_t port)
{
#ifdef _WIN32
    static WinSockGuard guard;
    if (!guard.initialized)
        return false;
#endif
    Shutdown();
    stop_ = false;
    mode_ = Mode::Client;
    running_ = true;
    worker_ = std::thread(&NetworkSession::clientThread, this, host, port);
    updateStatus("Connecting to " + host + ":" + std::to_string(port) + " ...");
    return true;
}

void NetworkSession::Shutdown()
{
    stop_ = true;
    running_ = false;
    setConnected(false);
    closeSocket(listenSocket_);
    closeSocket(peerSocket_);
    if (worker_.joinable())
        worker_.join();
    if (recvThread_.joinable())
        recvThread_.join();
    mode_ = Mode::None;
    updateStatus("Idle");
}

bool NetworkSession::SendMessage(const std::string& message)
{
    if (!IsConnected() || peerSocket_ == -1)
        return false;

    std::string payload = message;
    payload.push_back('\n');
    int result = ::send(peerSocket_, payload.c_str(), static_cast<int>(payload.size()), 0);
    if (result == SOCKET_ERROR)
    {
        updateStatus("Failed to send message.");
        return false;
    }
    return true;
}

bool NetworkSession::PollMessage(std::string& outMessage)
{
    std::lock_guard<std::mutex> lock(queueMutex_);
    if (incomingMessages_.empty())
        return false;
    outMessage = std::move(incomingMessages_.front());
    incomingMessages_.pop();
    return true;
}

std::string NetworkSession::GetStatus() const
{
    std::lock_guard<std::mutex> lock(statusMutex_);
    return statusMessage_;
}

void NetworkSession::hostThread(uint16_t port)
{
    SocketHandle server = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server == INVALID_SOCKET)
    {
        updateStatus("Failed to create host socket.");
        running_ = false;
        return;
    }
    listenSocket_ = static_cast<int>(server);

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    int enable = 1;
#ifndef _WIN32
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
#endif

    if (::bind(server, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR)
    {
        updateStatus("Failed to bind host socket.");
        closeSocket(listenSocket_);
        running_ = false;
        return;
    }

    if (::listen(server, 1) == SOCKET_ERROR)
    {
        updateStatus("Listen failed.");
        closeSocket(listenSocket_);
        running_ = false;
        return;
    }

    updateStatus("Waiting for LAN client on port " + std::to_string(port) + " ...");
    while (!stop_)
    {
        sockaddr_in clientAddr;
        socklen_t len = sizeof(clientAddr);
        SocketHandle client = ::accept(server, reinterpret_cast<sockaddr*>(&clientAddr), &len);
        if (client == INVALID_SOCKET)
        {
            continue;
        }
        peerSocket_ = static_cast<int>(client);
        setConnected(true);
        char addrStr[64] = {};
        inet_ntop(AF_INET, &clientAddr.sin_addr, addrStr, sizeof(addrStr));
        updateStatus(std::string("Client connected from ") + addrStr + ".");
        ::send(client, kHelloMsg, static_cast<int>(std::strlen(kHelloMsg)), 0);
        char buffer[32] = {};
        ::recv(client, buffer, sizeof(buffer), 0);
        updateStatus("LAN session ready (host).");
        startReceiveLoop();
        break;
    }

    running_ = false;
}

void NetworkSession::clientThread(std::string host, uint16_t port)
{
    SocketHandle sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
    {
        updateStatus("Failed to create client socket.");
        running_ = false;
        return;
    }
    peerSocket_ = static_cast<int>(sock);

    sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0)
    {
        updateStatus("Invalid host IP.");
        closeSocket(peerSocket_);
        running_ = false;
        return;
    }

    if (::connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR)
    {
        updateStatus("Unable to connect to host.");
        closeSocket(peerSocket_);
        running_ = false;
        return;
    }

    char buffer[32] = {};
    ::recv(sock, buffer, sizeof(buffer), 0);
    ::send(sock, kHelloMsg, static_cast<int>(std::strlen(kHelloMsg)), 0);

    setConnected(true);
    updateStatus("LAN session ready (client).");
    startReceiveLoop();

    running_ = false;
}

void NetworkSession::startReceiveLoop()
{
    if (recvThread_.joinable())
        recvThread_.join();
    recvThread_ = std::thread(&NetworkSession::receiveLoop, this);
}

void NetworkSession::updateStatus(const std::string& text)
{
    std::lock_guard<std::mutex> lock(statusMutex_);
    statusMessage_ = text;
    std::cout << "[Network] " << text << std::endl;
}

void NetworkSession::setConnected(bool value)
{
    connected_.store(value);
}

void NetworkSession::closeSocket(int& sock)
{
    if (sock == -1)
        return;
#ifdef _WIN32
    closesocket(sock);
#else
    ::close(sock);
#endif
    sock = -1;
}

void NetworkSession::receiveLoop()
{
    std::string pending;
    char buffer[512];
    while (!stop_)
    {
        if (peerSocket_ == -1)
            break;

        int received = ::recv(peerSocket_, buffer, sizeof(buffer), 0);
        if (received <= 0)
        {
            updateStatus("Connection closed.");
            setConnected(false);
            break;
        }
        pending.append(buffer, received);
        size_t newlinePos = 0;
        while ((newlinePos = pending.find('\n')) != std::string::npos)
        {
            std::string line = pending.substr(0, newlinePos);
            pending.erase(0, newlinePos + 1);
            if (!line.empty())
            {
                std::lock_guard<std::mutex> lock(queueMutex_);
                incomingMessages_.push(line);
            }
        }
    }
}
