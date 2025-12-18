#include "SoundManager.h"
#include <thread>
#include <cstdlib>
#ifdef __APPLE__
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
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
    if (ambiencePath_.empty() || ambienceRunning_)
        return;
    StopAmbience();
    ambienceRunning_ = true;
    ambienceThread_ = std::thread([this]() {
        while (ambienceRunning_)
        {
            pid_t pid = fork();
            if (pid == 0)
            {
                execlp("afplay", "afplay", "-v", "0.2", ambiencePath_.c_str(), (char*)nullptr);
                _exit(0);
            }
            if (pid < 0)
            {
                break;
            }
            ambiencePid_.store(pid);
            int status = 0;
            waitpid(pid, &status, 0);
            ambiencePid_.store(-1);
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
    pid_t pid = ambiencePid_.load();
    if (pid > 0)
        kill(pid, SIGTERM);
    if (ambienceThread_.joinable())
        ambienceThread_.join();
    ambiencePid_.store(-1);
#endif
}
