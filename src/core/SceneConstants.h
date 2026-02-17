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

} 
