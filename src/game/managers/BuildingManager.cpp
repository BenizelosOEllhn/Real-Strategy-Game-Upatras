#include "BuildingManager.h"
#include <algorithm>
#include <utility>
#include <glm/gtc/constants.hpp>

BuildingManager::BuildingManager()
{
    previewModels_.fill(nullptr);
    previewScales_.fill(20.0f);
    previewRotations_.fill(glm::vec3(0.0f));
    previewOffsets_.fill(glm::vec3(0.0f));
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
    previewYawDelta_ = 0.0f;

    const std::size_t idx = static_cast<std::size_t>(type);
    if (idx < previewModels_.size())
        previewModel_ = previewModels_[idx];
    else
        previewModel_ = nullptr;
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
        bool aboveWater = (hit.y > waterLevel + 0.1f);
        bool passesVisibility = !placementValidator_ || placementValidator_(currentType_, hit);
        validPlacement_ = aboveWater && passesVisibility;
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
        onPlaceBuilding(currentType_, previewPos_, getPreviewRotation());

    // reset
    isPlacing_ = false;
    hasPreview_ = false;
    validPlacement_ = false;
    currentType_ = BuildType::None;
    previewModel_ = nullptr;
    previewYawDelta_ = 0.0f;
}

void BuildingManager::setPreviewModel(BuildType type, Model* model)
{
    const std::size_t idx = static_cast<std::size_t>(type);
    if (idx >= previewModels_.size())
        return;
    previewModels_[idx] = model;
}

void BuildingManager::setPreviewScale(BuildType type, float scale)
{
    const std::size_t idx = static_cast<std::size_t>(type);
    if (idx >= previewScales_.size())
        return;
    previewScales_[idx] = scale;
}

void BuildingManager::setPreviewRotation(BuildType type, const glm::vec3& rotation)
{
    const std::size_t idx = static_cast<std::size_t>(type);
    if (idx >= previewRotations_.size())
        return;
    previewRotations_[idx] = rotation;
}

void BuildingManager::setPreviewOffset(BuildType type, const glm::vec3& offset)
{
    const std::size_t idx = static_cast<std::size_t>(type);
    if (idx >= previewOffsets_.size())
        return;
    previewOffsets_[idx] = offset;
}

float BuildingManager::getPreviewScale() const
{
    std::size_t idx = static_cast<std::size_t>(currentType_);
    if (idx >= previewScales_.size())
        return 20.0f;
    return previewScales_[idx];
}

glm::vec3 BuildingManager::getPreviewOffset() const
{
    std::size_t idx = static_cast<std::size_t>(currentType_);
    if (idx >= previewOffsets_.size())
        return glm::vec3(0.0f);
    return previewOffsets_[idx];
}

glm::vec3 BuildingManager::getPreviewRotation() const
{
    std::size_t idx = static_cast<std::size_t>(currentType_);
    if (idx >= previewRotations_.size())
        return glm::vec3(0.0f, previewYawDelta_, 0.0f);
    glm::vec3 rotation = previewRotations_[idx];
    rotation.y += previewYawDelta_;
    return rotation;
}

void BuildingManager::rotatePreviewYaw(float radians)
{
    if (!isPlacing_)
        return;
    previewYawDelta_ += radians;
    if (previewYawDelta_ > glm::pi<float>())
        previewYawDelta_ -= glm::two_pi<float>();
    else if (previewYawDelta_ < -glm::pi<float>())
        previewYawDelta_ += glm::two_pi<float>();
}

void BuildingManager::setPlacementValidator(std::function<bool(BuildType, const glm::vec3&)> validator)
{
    placementValidator_ = std::move(validator);
}
