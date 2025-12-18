#include "SoundManager.h"
#include <thread>
#include <cstdlib>
#ifdef __APPLE__
#include <unistd.h>
#endif

SoundManager::~SoundManager()
{
    StopAmbience();
}

void SoundManager::PlayWoodChop() const
{
    playClip(woodPath_);
}

void SoundManager::PlayStoneMine() const
{
    playClip(stonePath_);
}

void SoundManager::playClip(const std::string& path) const
{
    if (path.empty())
        return;
#ifdef __APPLE__
    std::thread([path]() {
        std::string command = "afplay -t 2 \"" + path + "\" >/dev/null 2>&1";
        std::system(command.c_str());
    }).detach();
#endif
}

void SoundManager::StartAmbience()
{
#ifdef __APPLE__
    if (ambiencePath_.empty())
        return;
    StopAmbience();
    ambienceRunning_ = true;
    ambienceThread_ = std::thread([this]() {
        while (ambienceRunning_)
        {
            std::string command = "afplay -v 0.2 \"" + ambiencePath_ + "\" >/dev/null 2>&1";
            std::system(command.c_str());
        }
    });
#endif
}

void SoundManager::StopAmbience()
{
#ifdef __APPLE__
    if (!ambienceRunning_)
        return;
    ambienceRunning_ = false;
    if (ambienceThread_.joinable())
        ambienceThread_.join();
#endif
}
