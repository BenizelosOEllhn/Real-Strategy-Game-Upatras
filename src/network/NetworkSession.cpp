#include "NetworkSession.h"

#include <cstring>
#include <iostream>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    using SocketHandle = SOCKET;
#else
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
    #include <unistd.h>
    #include <csignal>
    using SocketHandle = int;
    #define INVALID_SOCKET (-1)
    #define SOCKET_ERROR   (-1)
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

// ------------------------------------------------------------

NetworkSession::NetworkSession()
{
#ifdef _WIN32
    static WinSockGuard guard;
    if (!guard.initialized)
        updateStatus("Winsock initialization failed.");
#else
    // Ignore SIGPIPE so writing to a broken socket doesn't kill the process
    std::signal(SIGPIPE, SIG_IGN);
#endif
    updateStatus("Idle");
}

NetworkSession::~NetworkSession()
{
    Shutdown();
}

// ------------------------------------------------------------

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

// ------------------------------------------------------------

bool NetworkSession::SendPacket(const std::string& message)
{
    if (!IsConnected() || peerSocket_ == -1)
        return false;

    std::string payload = message + "\n";
    int result = ::send(peerSocket_, payload.c_str(),
                        static_cast<int>(payload.size()), 0);
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

// ------------------------------------------------------------
// Internal threads
// ------------------------------------------------------------

void NetworkSession::hostThread(uint16_t port)
{
    SocketHandle server = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server == INVALID_SOCKET)
    {
        updateStatus("Failed to create host socket.");
        return;
    }
    listenSocket_ = static_cast<int>(server);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

#ifndef _WIN32
    int enable = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
#endif

    if (::bind(server, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR)
    {
        updateStatus("Bind failed.");
        closeSocket(listenSocket_);
        return;
    }

    ::listen(server, 1);
    updateStatus("Waiting for client...");

    sockaddr_in clientAddr{};
    socklen_t len = sizeof(clientAddr);
    SocketHandle client = ::accept(server, reinterpret_cast<sockaddr*>(&clientAddr), &len);
    if (client == INVALID_SOCKET)
        return;

    peerSocket_ = static_cast<int>(client);
#ifdef __APPLE__
    { int on = 1; setsockopt(client, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on)); }
#endif
    setConnected(true);
    updateStatus("Client connected.");
    startReceiveLoop();
}

void NetworkSession::clientThread(std::string host, uint16_t port)
{
    SocketHandle sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
        return;

    peerSocket_ = static_cast<int>(sock);
#ifdef __APPLE__
    { int on = 1; setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on)); }
#endif

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &addr.sin_addr);

    if (::connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == SOCKET_ERROR)
    {
        updateStatus("Connection failed.");
        closeSocket(peerSocket_);
        return;
    }

    setConnected(true);
    updateStatus("Connected to host.");
    startReceiveLoop();
}

// ------------------------------------------------------------

void NetworkSession::startReceiveLoop()
{
    if (recvThread_.joinable())
        recvThread_.join();
    recvThread_ = std::thread(&NetworkSession::receiveLoop, this);
}

void NetworkSession::receiveLoop()
{
    char buffer[512];
    while (!stop_)
    {
        int received = ::recv(peerSocket_, buffer, sizeof(buffer), 0);
        if (received <= 0)
        {
            updateStatus("Disconnected.");
            setConnected(false);
            break;
        }

        std::lock_guard<std::mutex> lock(queueMutex_);
        recvBuffer_.append(buffer, buffer + received);
        size_t pos = 0;
        while ((pos = recvBuffer_.find('\n')) != std::string::npos)
        {
            std::string line = recvBuffer_.substr(0, pos);
            recvBuffer_.erase(0, pos + 1);
            if (!line.empty())
                incomingMessages_.push(std::move(line));
        }
    }
}

// ------------------------------------------------------------

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
