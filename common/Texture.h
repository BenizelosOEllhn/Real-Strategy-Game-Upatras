#pragma once
#include <GL/glew.h>

class Texture {
public:
    unsigned int ID;
    Texture(const char* path);
    void Bind(unsigned int slot = 0); // Slot 0 = Grass, Slot 1 = Rock
};