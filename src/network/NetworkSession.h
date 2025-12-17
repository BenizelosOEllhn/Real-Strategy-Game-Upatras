#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

class NetworkSession
{
public:
    enum class Mode
    {
        None,
        Host,
        Client
    };

    NetworkSession();
    ~NetworkSession();

    bool StartHosting(uint16_t port);
    bool ConnectToHost(const std::string& host, uint16_t port);
    void Shutdown();
    bool SendMessage(const std::string& message);
    bool PollMessage(std::string& outMessage);

    bool IsConnected() const { return connected_.load(); }
    bool IsRunning() const { return running_.load(); }
    Mode GetMode() const { return mode_; }
    std::string GetStatus() const;

private:
    void hostThread(uint16_t port);
    void clientThread(std::string host, uint16_t port);
    void startReceiveLoop();
    void receiveLoop();
    void updateStatus(const std::string& text);
    void setConnected(bool value);
    void closeSocket(int& sock);

    std::thread worker_;
    std::atomic<bool> running_{false};
    std::atomic<bool> connected_{false};
    std::atomic<bool> stop_{false};
    Mode mode_ = Mode::None;
    int listenSocket_ = -1;
    int peerSocket_ = -1;
    mutable std::mutex statusMutex_;
    std::string statusMessage_;
    std::thread recvThread_;
    std::mutex queueMutex_;
    std::queue<std::string> incomingMessages_;
};
