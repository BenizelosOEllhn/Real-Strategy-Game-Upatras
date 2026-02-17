#pragma once
#include <GL/glew.h>

class Texture {
public:
    unsigned int ID;
    Texture(const char* path, bool flipY = true);
    void Bind(unsigned int slot = 0); // Slot 0 = Grass, Slot 1 = Rock
    bool LoadedFromFile() const { return loadedFromFile_; }
    int Width() const { return width_; }
    int Height() const { return height_; }
    int Channels() const { return channels_; }

private:
    bool loadedFromFile_ = false;
    int width_ = 0;
    int height_ = 0;
    int channels_ = 0;
};
