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

void BuildingManager::update(double mouseX, double mouseY, int currentScreenW, int currentScreenH, const Camera& cam)
{
    if (!isPlacing_ || !camera_ || !terrain_)
        return;

    if (!previewModel_)
        return;

    // Generate ray from mouse
    Ray ray = Raycaster::screenPointToRay(
        (float)mouseX,
        (float)mouseY,
        currentScreenW,
        currentScreenH,
        cam
    );

    // Raycast to terrain
glm::vec3 hit;
    if (Raycaster::raycastTerrain(ray, *terrain_, hit))
    {
        previewPos_ = hit;
        hasPreview_ = true;

        // ------------------------
        // VALIDITY CHECK
        // ------------------------
        float waterLevel = std::max({ -1.2, 4.5, 1.5 });

        if (hit.y > waterLevel + 0.1f)
        {
            validPlacement_ = true;
        }
        else
        {
            validPlacement_ = false;
        }
    }
    else
    {
        hasPreview_ = false;
        validPlacement_ = false;
    }
}

void BuildingManager::confirmPlacement(double mouseX, double mouseY)
{
    if (!isPlacing_ || !hasPreview_ || !validPlacement_)
        return;

    if (onPlaceBuilding)
        onPlaceBuilding(currentType_, previewPos_);

    // reset
    isPlacing_ = false;
    hasPreview_ = false;
    validPlacement_ = false;
    currentType_ = BuildType::None;
    previewModel_ = nullptr;
}

