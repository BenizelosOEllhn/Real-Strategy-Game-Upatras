#pragma once
#include <string>

// Returns the folder where runtime assets are located (resolved relative to the executable).
const std::string& GetAssetBasePath();

// Helper to build a full path for a relative asset reference.
inline std::string AssetPath(const std::string& relative)
{
    return GetAssetBasePath() + relative;
}
