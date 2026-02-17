#pragma once
#include <cmath>

class FastNoiseLite
{
public:
    enum NoiseType
    {
        NoiseType_Perlin = 0
    };

    FastNoiseLite()
    {
        frequency = 0.01f;
        noiseType = NoiseType_Perlin;
    }

    void SetFrequency(float f) { frequency = f; }
    void SetNoiseType(NoiseType type) { noiseType = type; }

    float GetNoise(float x, float y) const
    {
        // Simple Perlin-like hashed noise
        int xi = (int)floor(x * frequency);
        int yi = (int)floor(y * frequency);

        int hash = (xi * 49632) ^ (yi * 325176) ^ 0x9e3779b9;
        hash = (hash << 13) ^ hash;

        float n = (1.0f - ((hash * (hash * hash * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);

        return n; // -1..1
    }

private:
    float frequency;
    NoiseType noiseType;
};
