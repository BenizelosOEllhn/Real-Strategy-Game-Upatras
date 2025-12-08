#pragma once
#include "FastNoiseLite.h"

class NoiseGenerator
{
public:
    NoiseGenerator()
    {
        noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
        noise.SetFrequency(0.05f);
    }

    float get(float x, float z)
    {
        float v = noise.GetNoise(x, z);
        return (v + 1.0f) * 0.5f; 
    }

private:
    FastNoiseLite noise;
};

