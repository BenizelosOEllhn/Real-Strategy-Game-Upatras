#pragma once
#include <string>
#include <thread>
#include <atomic>

class SoundManager
{
public:
    ~SoundManager();
    void SetWoodChopPath(const std::string& path) { woodPath_ = path; }
    void SetStoneMinePath(const std::string& path) { stonePath_ = path; }
    void SetAmbiencePath(const std::string& path) { ambiencePath_ = path; }

    void PlayWoodChop() const;
    void PlayStoneMine() const;
    void StartAmbience();
    void StopAmbience();
    void Shutdown() { StopAmbience(); }

private:
    std::string woodPath_;
    std::string stonePath_;
    std::string ambiencePath_;
    std::atomic<bool> ambienceRunning_{false};
    std::thread ambienceThread_;

    void playClip(const std::string& path) const;
};
