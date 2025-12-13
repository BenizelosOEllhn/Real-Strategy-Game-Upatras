#pragma once
#include <functional>
#include <glm/glm.hpp>
#include "Raycaster.h"
#include "Terrain.h"
#include "Camera.h"
#include "../common/Model.h"


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
    Storage
};

class BuildingManager
{
public:
    BuildingManager();

    // Call this ONCE in Scene::Init()
    void init(Terrain* t, Camera* c, int screenW, int screenH);

    // UI tells us which building we are placing
    void startPlacing(BuildType type);

    // Called every frame from Scene::Update
    void update(double mouseX, double mouseY);

    // Called when the user left-clicks
    void confirmPlacement(double mouseX, double mouseY);

    // For Scene::Draw to know when to draw preview
    bool isPlacing() const { return isPlacing_; }
    bool hasPreview() const { return hasPreview_; }
    glm::vec3 getPreviewPos() const { return previewPos_; }
    Model* getPreviewModel() const { return previewModel_; }

    // Scene registers this callback so it can spawn a real building
    std::function<void(BuildType, glm::vec3)> onPlaceBuilding;

    // Provide real building models to use for previews
    void setModels(Model* townCenter, Model* barracks) {
        townCenterModel_ = townCenter;
        barracksModel_   = barracks;
    }

private:
    Terrain* terrain_ = nullptr;
    Camera*  camera_  = nullptr;

    int screenW_ = 0;
    int screenH_ = 0;

    bool isPlacing_  = false;
    bool hasPreview_ = false;
    bool validHit_   = false;

    BuildType currentType_ = BuildType::None;

    glm::vec3 previewPos_ = glm::vec3(0);

    Model* previewModel_ = nullptr;

    // Actual models provided by Scene
    Model* townCenterModel_ = nullptr;
    Model* barracksModel_   = nullptr;
};
