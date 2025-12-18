#pragma once
#include <array>
#include <functional>
#include <glm/glm.hpp>
#include "Raycaster.h"
#include "Terrain.h"
#include "Camera.h"
#include "../common/Model.h"
#include "Building.h"


class Terrain;
class Camera;
class Model;

// What building type the player is placing
enum class BuildType {
    None = 0,
    TownCenter,
    Barracks,
    Farm,
    House,
    Market,
    Storage,
    Bridge
};

class BuildingManager
{
public:
    BuildingManager();

    // Call this ONCE in Scene::Init()
    void init(Terrain* t, Camera* c, int screenW, int screenH);

    // UI tells us which building we are placing
    void startPlacing(BuildType type);
    bool isValidPlacement() const { return validPlacement_; }


    // Called every frame from Scene::Update
    void update(double mouseX, double mouseY, int screenW, int screenH, const Camera& cam);

    // Called when the user left-clicks
    void confirmPlacement(double mouseX, double mouseY);

    // For Scene::Draw to know when to draw preview
    bool isPlacing() const { return isPlacing_; }
    bool hasPreview() const { return hasPreview_; }
    BuildType currentType() const { return currentType_; }
    glm::vec3 getPreviewPos() const { return previewPos_; }
    Model* getPreviewModel() const { return previewModel_; }
    float getPreviewScale() const;
    glm::vec3 getPreviewRotation() const;
    glm::vec3 getPreviewOffset() const;
    void rotatePreviewYaw(float radians);
    

    // Scene registers this callback so it can spawn a real building
    std::function<void(BuildType, glm::vec3, glm::vec3)> onPlaceBuilding;

    // Provide real building models to use for previews
    void setPreviewModel(BuildType type, Model* model);
    void setPreviewScale(BuildType type, float scale);
    void setPreviewRotation(BuildType type, const glm::vec3& rotation);
    void setPreviewOffset(BuildType type, const glm::vec3& offset);
    void setPlacementValidator(std::function<bool(BuildType, const glm::vec3&)> validator);

private:
    static constexpr std::size_t kBuildTypeCount =
        static_cast<std::size_t>(BuildType::Bridge) + 1;

    Terrain* terrain_ = nullptr;
    Camera*  camera_  = nullptr;

    int screenW_ = 0;
    int screenH_ = 0;

    bool isPlacing_  = false;
    bool hasPreview_ = false;
    bool validHit_   = false;

    BuildType currentType_ = BuildType::None;
    bool validPlacement_ = false;

    glm::vec3 previewPos_ = glm::vec3(0);
    glm::vec3 finalPos = glm::vec3(0);
    float previewYawDelta_ = 0.0f;

    Model* previewModel_ = nullptr;
    std::array<Model*, kBuildTypeCount> previewModels_{};
    std::array<float, kBuildTypeCount> previewScales_{};
    std::array<glm::vec3, kBuildTypeCount> previewRotations_{};
    std::array<glm::vec3, kBuildTypeCount> previewOffsets_{};
    std::function<bool(BuildType, const glm::vec3&)> placementValidator_;
};
