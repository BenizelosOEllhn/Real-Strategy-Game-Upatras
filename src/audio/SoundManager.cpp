#include "SoundManager.h"
#include <thread>
#include <cstdlib>
#ifdef __APPLE__
#include <unistd.h>
#endif

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
