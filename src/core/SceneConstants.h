#pragma once

#include <cmath>
#include <utility>
#include <glm/glm.hpp>
#include <glm/common.hpp> // mix, clamp

// ============================================================
// Shared Scene Constants (PROJECT-WIDE)
// ============================================================
namespace SceneConst {

inline constexpr int   kTerrainWidth     = 600;
inline constexpr int   kTerrainDepth     = 600;
inline constexpr int   kTreeCount        = 360;
inline constexpr int   kRockCount        = 110;

inline constexpr float kMountainStart    = -90.0f;
inline constexpr float kMountainAvoidZ   = -70.0f;

// NOTE: must match Terrain lake
inline constexpr float kLakeCenterZ      = -200.0f;
inline constexpr float kLakeRadius       = 60.0f;

inline constexpr float kRiverTreeBuffer  = 14.0f;
inline constexpr float kSouthForestBias  = 0.75f;
inline constexpr float kMountainTreeBias = 0.2f;
inline constexpr float kCornerPlainZ     = 135.0f;
inline constexpr float kCornerPlainX     = 95.0f;

inline constexpr float kRockZoneMinZ     = kMountainStart - 80.0f;
inline constexpr float kRockZoneMaxZ     = -45.0f;
inline constexpr float kRockMinHeight    = 5.0f;

// Same path & width as Terrain::carveRiver(), but a bit wider for safety.
inline bool nearRiver(float x, float z)
{
    constexpr float lakeZ       = kLakeCenterZ;
    constexpr float riverStartZ = lakeZ + 12.0f;  // -188
    constexpr float riverEndZ   = 260.0f;

    if (z < riverStartZ || z > riverEndZ)
        return false;

    auto evalPath = [&](float startX, float dir) -> std::pair<float, float>
    {
        float t = (z - riverStartZ) / (riverEndZ - riverStartZ);

        float endX  = dir * 180.0f;
        float pathX = glm::mix(startX, endX, t);

        pathX += dir * (30.0f * std::sin(z * 0.035f));
        pathX +=        (12.0f * std::cos(z * 0.02f));

        float halfWidth = glm::mix(28.0f, 18.0f, t);
        float fade = glm::clamp((z - 150.0f) / 30.0f, 0.0f, 1.0f);
        halfWidth *= (1.0f - fade);

        halfWidth += 4.0f; // padding for foliage rejection

        return { pathX - halfWidth, pathX + halfWidth };
    };

    auto [lMin, lMax] = evalPath(-15.0f, -1.0f);
    auto [rMin, rMax] = evalPath(+15.0f, +1.0f);

    return (x >= lMin && x <= lMax) || (x >= rMin && x <= rMax);
}

} 
