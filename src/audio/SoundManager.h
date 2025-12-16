#pragma once
#include <string>

class SoundManager
{
public:
    void SetWoodChopPath(const std::string& path) { woodPath_ = path; }
    void SetStoneMinePath(const std::string& path) { stonePath_ = path; }

    void PlayWoodChop() const;
    void PlayStoneMine() const;

private:
    std::string woodPath_;
    std::string stonePath_;

    void playClip(const std::string& path) const;
};
