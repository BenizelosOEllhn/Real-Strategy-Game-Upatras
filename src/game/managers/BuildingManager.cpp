#include "BuildingManager.h"

BuildingManager::BuildingManager()
{
}

void BuildingManager::init(Terrain* t, Camera* c, int screenW, int screenH)
{
    terrain_ = t;
    camera_  = c;
    screenW_ = screenW;
    screenH_ = screenH;
}

void BuildingManager::startPlacing(BuildType type)
{
    currentType_ = type;
    isPlacing_   = (type != BuildType::None);
    hasPreview_  = false;
    validHit_    = false;

    // Assign preview model
    switch (type)
    {
        case BuildType::TownCenter:
            previewModel_ = townCenterModel_;
            break;

        case BuildType::Barracks:
            previewModel_ = barracksModel_;
            break;

        default:
            previewModel_ = nullptr;
            break;
    }
}

void BuildingManager::update(double mouseX, double mouseY)
{
    if (!isPlacing_ || !camera_ || !terrain_)
        return;

    if (!previewModel_)
        return;

    // --------- IMPORTANT: Fix macOS Retina & bottom-left UI mismatch ---------
    float flippedY = screenH_ - (float)mouseY;

    // Generate ray from mouse
    Ray ray = Raycaster::screenPointToRay(
        (float)mouseX,
        flippedY,
        screenW_,
        screenH_,
        *camera_
    );

    // Raycast to terrain
    glm::vec3 hit;
    if (Raycaster::raycastTerrain(ray, *terrain_, hit))
    {
        previewPos_ = hit;
        hasPreview_ = true;
        validHit_   = true;
    }
    else
    {
        hasPreview_ = false;
        validHit_   = false;
    }
}

void BuildingManager::confirmPlacement(double mouseX, double mouseY)
{
    if (!isPlacing_ || !validHit_ || !hasPreview_)
        return;

    if (onPlaceBuilding)
        onPlaceBuilding(currentType_, previewPos_);

    // Reset placement mode
    isPlacing_   = false;
    hasPreview_  = false;
    validHit_    = false;
    currentType_ = BuildType::None;
    previewModel_ = nullptr;
}
