#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

#include <vector>
#include <string>
#include <iostream>
#include <random>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <unordered_map>

// ============================================================
// Engine / Rendering
// ============================================================
#include "../rendering/terrain/Terrain.h"
#include "../../common/Model.h"
#include "../../common/Texture.h"
#include "../../common/Shader.h"
#include "Camera.h"
#include "Scene.h"

// ============================================================
// RTS Systems
// ============================================================
#include "../gui/UIManager.h"
#include "../game/managers/BuildingManager.h"
#include "../game/managers/UnitManager.h"
#include "../network/NetworkSession.h"
#include "../game/data/Resources.h"
#include "../audio/SoundManager.h"

// ============================================================
// Entities
// ============================================================
#include "../game/data/EntityType.h"
#include "../game/entities/GameEntity.h"
#include "../game/entities/Unit.h"
#include "../game/buildings/TownCenter.h"
#include "../game/buildings/Barracks.h"
#include "../game/buildings/Farm.h"
#include "../game/buildings/House.h"
#include "../game/buildings/Market.h"
#include "../game/buildings/Storage.h"

#ifndef ASSET_PATH
#define ASSET_PATH "assets/"
#endif


// ============================================================
// Scene
// ============================================================
enum class UITab {
    Buildings,
    Units
};

class Scene
{
public:
    Scene();
    ~Scene();

    void Init(Camera* activeCamera);
    void Update(float dt, const Camera& cam);

    void Draw(Shader& terrainShader,
              Shader& objectShader,
              glm::mat4 view,
              glm::mat4 projection,
              glm::vec3 lightPos,
              glm::vec3 viewPos,
              const glm::mat4& lightSpaceMatrix,
              unsigned int shadowMap);

    void DrawDepth(Shader& depthShader,
                   const glm::mat4& lightSpaceMatrix);

    // Input / UI
    void setupBuildingBar();
    void setupResourceBar();
    void setupTabButtons();
    void setupUnitPanel();
    void setupProductionPanel();
    void setupUnitInfoPanel();
    void setupBuildingInfoPanel();
    void setupMainMenu();
    void setActiveTab(UITab tab);
    void refreshUnitListUI();
    void selectUnitFromList(size_t entryIndex);
    void updateProductionPanel();
    void updateUnitInfoPanel();
    void updateBuildingInfoPanel(BuildType type);
    std::string getBuildingName(BuildType type) const;
    void onMouseMove(double x, double y);
    void onMouseButton(int button, int action, int mods);
    void cancelCurrentAction();
    void updateResourceTexts();
    void switchActivePlayer();
    int GetActivePlayerIndex() const { return activePlayerIndex_; }
    Resources& activePlayer();
    const Resources& activePlayer() const;
    Resources* activePlayerPtr();
    Resources* resourcesForOwner(int ownerId);
    const Resources* resourcesForOwner(int ownerId) const;
    void destroyBuilding(Building* building);
    void showVictoryMessage(int winningPlayer);
    void checkVictoryState();
    void registerTownCenter(TownCenter* tc);
    void registerBarracks(Barracks* barracks);
    void drawSelectionIndicators(const glm::mat4& view, const glm::mat4& projection);
    void initSelectionCircle();
    void clearUnitSelection();
    void selectSingleUnit(const glm::vec2& screenPos, bool additive);
    void selectUnitsInRect(const glm::vec2& a, const glm::vec2& b, bool additive);
    glm::vec2 worldToScreen(const glm::vec3& worldPos) const;
    void issueMoveCommand();
    bool handleProductionRequest(EntityType unitType);
    bool canAffordBuilding(BuildType type) const;
    UnitCost getBuildingCost(BuildType type) const;
    void selectBuildingAtScreen(const glm::vec2& screenPos);
    void handleDeleteCurrentUnit();
    void deleteUnit(Unit* unit);
    void spawnInitialVillager(TownCenter* tc);
    void startSinglePlayerGame();
    void startLanHostGame();
    void startLanJoinGame();
    void beginGameplay(bool enableLanMode);
    void updateMainMenu(float dt);
    void setMainMenuVisible(bool visible);
    std::string readLanAddress() const;
    bool findClosestLandPoint(const glm::vec3& desired, glm::vec3& out) const;
    enum class ResourceNodeType { Tree, Rock };
    struct GatherTask {
        Unit* worker = nullptr;
        ResourceNodeType type = ResourceNodeType::Tree;
        size_t resourceIndex = 0;
        float progress = 0.0f;
        bool soundActive = false;
        bool animationActive = false;
    };

    bool findNearestTree(const glm::vec3& point, float radius, size_t& outIndex, glm::vec3& outPos) const;
    bool findNearestRock(const glm::vec3& point, float radius, size_t& outIndex, glm::vec3& outPos) const;
    void removeTree(size_t index);
    void removeRock(size_t index);
    bool handleResourceGather(const glm::vec3& point);
    void updateGatherTasks(float dt);
    void updateCombat(float dt);
    void clearGatherTasksFor(Unit* worker);
    void clearGatherTasksFor(ResourceNodeType type, size_t resourceIndex);
    void toggleUnitCamera();
    void updateUnitCameraView();
    bool IsUnitCameraActive() const { return unitCameraActive_; }
    void RotateUnitCamera(float yawDeltaDeg, float pitchDeltaDeg);
    void initPathfindingGrid();
    void refreshNavObstacles();
    bool commandUnitTo(Unit* unit, const glm::vec3& destination);
    bool findPath(const glm::vec3& start, const glm::vec3& goal, std::vector<glm::vec3>& outPath) const;

private:
    // ========================================================
    // Core world
    // ========================================================
    Terrain* terrain = nullptr;
    Camera*  camera  = nullptr;

    int fbWidth  = 0;
    int fbHeight = 0;

    // ========================================================
    // Models
    // ========================================================
    Model* treeModel       = nullptr;
    Model* rockModel       = nullptr;
    Model* farmModel        = nullptr;
    Model* houseModel       = nullptr;
    Model* marketModel      = nullptr;
    Model* storageModel     = nullptr;
    Model* townCenterModel  = nullptr;
    Model* barracksModel    = nullptr;
    Model* farmerModel      = nullptr;
    Model* archerUnitModel  = nullptr;
    Model* knightUnitModel  = nullptr;

    // ========================================================
    // Textures
    // ========================================================
    Texture* grass1Tex = nullptr;
    Texture* grass2Tex = nullptr;
    Texture* grass3Tex = nullptr;
    Texture* sandTex   = nullptr;
    Texture* rockTex   = nullptr;
    Texture* treeTex   = nullptr;
    Texture* peakTex   = nullptr;
    Texture* boulderTex = nullptr;
    Texture* waterTex   = nullptr;
    Texture* noiseTex   = nullptr;
    Texture* overlayTex = nullptr;
    Texture* cornIconTex = nullptr;
    Texture* woodIconTex = nullptr;
    Texture* goldIconTex = nullptr;
    Texture* oreIconTex  = nullptr;
    Texture* populationIconTex = nullptr;
    Texture* villagerIconTex = nullptr;
    Texture* archerIconTex = nullptr;
    Texture* knightIconTex = nullptr;
    Texture* selectionRingTex = nullptr;

    // ========================================================
    // Foliage
    // ========================================================
    std::vector<glm::mat4> treeTransforms;
    std::vector<glm::mat4> rockTransforms;
    std::vector<glm::vec3> treePositions_;
    std::vector<glm::vec3> rockPositions_;
    std::vector<GatherTask> gatherTasks_;

    void generateTrees();
    void generateRocks();

    // ========================================================
    // WATER SYSTEM
    // ========================================================
    struct WaterVertex {
        glm::vec3 position;
        glm::vec2 uv;
        float fade;
    };

    // Ocean
    GLuint waterVAO = 0;
    GLuint waterVBO = 0;
    GLuint waterEBO = 0;
    size_t waterIndexCount = 0;

    Shader* waterShader = nullptr;

    void GenerateWaterGeometry();
void DrawWater(const glm::mat4& view,
               const glm::mat4& proj,
               const glm::vec3& viewPos);

    // Lake
    std::vector<WaterVertex>  lakeWaterVerts;
    std::vector<unsigned int> lakeWaterIndices;
    GLuint lakeVAO = 0, lakeVBO = 0, lakeEBO = 0;

    void generateLakeWater();
    void uploadLakeWaterMesh();

void DrawLakeWater(const glm::mat4& view,
                   const glm::mat4& proj,
                   const glm::vec3& viewPos);

    // River
    std::vector<WaterVertex>  riverWaterVerts;
    std::vector<unsigned int> riverWaterIndices;
    GLuint riverVAO = 0, riverVBO = 0, riverEBO = 0;

    void generateRiverWater();
    void uploadRiverWaterMesh();

    void DrawRiverWater(const glm::mat4& view,
                    const glm::mat4& proj,
                    const glm::vec3& viewPos);

    bool nearRiver(float x, float z) const;
    void Resize(int fbW, int fbH);

    // --- Reflection / Refraction FBOs ---
    GLuint reflectionFBO = 0;
    GLuint reflectionColorTex = 0;
    GLuint reflectionDepthRBO = 0;

    GLuint refractionFBO = 0;
    GLuint refractionColorTex = 0;
    GLuint refractionDepthTex = 0;

    int waterRTWidth  = 0;
    int waterRTHeight = 0;

    // Foam noise texture (optional but recommended)
    Texture* foamTex = nullptr;

    // Water heights (pick values that match your meshes)
    float oceanY = -1.2f;
    float lakeY  = 4.5f;
    float riverY = 1.5f;
    
    // Helpers
    void initWaterRenderTargets(int w, int h);
    void destroyWaterRenderTargets();

    void beginReflectionPass(int w, int h);
    void beginRefractionPass(int w, int h);
    void endWaterPass(int w, int h);
    bool isWaterAt(float x, float z, float y) const;
    bool isWaterArea(float x, float z) const;
    
    //MousePlacement
    glm::vec3 GetMouseWorldPos(double mouseX, double mouseY,
                            int screenW, int screenH,
                            const glm::mat4& view,
                            const glm::mat4& projection,
                            float groundY);

    // ========================================================
    // UI + BUILDING
    // ========================================================
    UIManager       uiManager_;
    BuildingManager buildingManager_;
    UnitManager     unitManager_;
    NetworkSession  networkSession_;

    double mouseX_ = 0.0;
    double mouseY_ = 0.0;

    Shader* previewShader = nullptr;
    Shader* selectionShader = nullptr;
    size_t foodLabelIndex_ = SIZE_MAX;
    size_t woodLabelIndex_ = SIZE_MAX;
    size_t goldLabelIndex_ = SIZE_MAX;
    size_t oreLabelIndex_ = SIZE_MAX;
    size_t populationLabelIndex_ = SIZE_MAX;
    size_t playerLabelIndex_ = SIZE_MAX;
    size_t victoryLabelIndex_ = SIZE_MAX;
    size_t mainMenuBackgroundIndex_ = SIZE_MAX;
    size_t mainMenuTitleLabelIndex_ = SIZE_MAX;
    size_t mainMenuSingleBtnIndex_ = SIZE_MAX;
    size_t mainMenuHostBtnIndex_ = SIZE_MAX;
    size_t mainMenuJoinBtnIndex_ = SIZE_MAX;
    size_t mainMenuSingleLabelIndex_ = SIZE_MAX;
    size_t mainMenuHostLabelIndex_ = SIZE_MAX;
    size_t mainMenuJoinLabelIndex_ = SIZE_MAX;
    size_t mainMenuStatusLabelIndex_ = SIZE_MAX;

    // ========================================================
    // GAME STATE
    // ========================================================
    std::vector<GameEntity*> entities_;
    std::vector<TownCenter*> townCenters_;
    std::vector<Barracks*>   barracks_;
    std::vector<Unit*>       selectedUnits_;
    Building* selectedBuilding_ = nullptr;

    Resources player1;
    Resources player2;
    int activePlayerIndex_ = 0;
    Resources* activeResources_ = nullptr;
    bool mainMenuActive_ = true;
    bool lanSessionPending_ = false;
    bool lanModeActive_ = false;
    bool lanIsHost_ = false;
    std::string lanStatusText_;

    // Selection helpers
    bool draggingSelection_ = false;
    bool additiveSelection_ = false;
    glm::vec2 dragStart_{0.0f};
    glm::vec2 dragCurrent_{0.0f};
    glm::mat4 lastViewMatrix_{1.0f};
    glm::mat4 lastProjMatrix_{1.0f};
    GLuint selectionCircleVAO = 0;
    GLuint selectionCircleVBO = 0;
    UITab currentTab_ = UITab::Buildings;
    glm::vec2 buildingBarPos_{0.0f};
    glm::vec2 buildingBarSize_{0.0f};
    std::vector<size_t> buildingButtonIndices_;
    std::vector<size_t> buildingLabelIndices_;
    std::vector<size_t> unitEntryIconIndices_;
    std::vector<size_t> unitEntryLabelIndices_;
    std::vector<Unit*>  unitEntryTargets_;
    size_t buildingTabButtonIndex_ = SIZE_MAX;
    size_t unitTabButtonIndex_ = SIZE_MAX;
    size_t buildingTabLabelIndex_ = SIZE_MAX;
    size_t unitTabLabelIndex_ = SIZE_MAX;
    size_t buildingBarBackgroundIndex_ = SIZE_MAX;
    size_t unitPanelBackgroundIndex_ = SIZE_MAX;
    size_t unitPanelTitleLabelIndex_ = SIZE_MAX;
    size_t productionPanelBackgroundIndex_ = SIZE_MAX;
    std::vector<size_t> productionButtonIndices_;
    std::vector<size_t> productionLabelIndices_;
    std::vector<EntityType> productionButtonTypes_;
    size_t unitInfoPanelBackgroundIndex_ = SIZE_MAX;
    size_t unitInfoNameLabelIndex_ = SIZE_MAX;
    size_t unitInfoHealthLabelIndex_ = SIZE_MAX;
    size_t unitDeleteButtonIndex_ = SIZE_MAX;
    size_t unitDeleteLabelIndex_ = SIZE_MAX;
    Unit* unitInfoTarget_ = nullptr;
    size_t buildingInfoPanelIndex_ = SIZE_MAX;
    size_t buildingInfoTitleLabelIndex_ = SIZE_MAX;
    size_t buildingInfoTextLabelIndex_ = SIZE_MAX;
    std::unordered_map<BuildType, std::string> buildingInfoText_;

    // Pathfinding
    float navCellSize_ = 3.0f;
    int navGridCols_ = 0;
    int navGridRows_ = 0;
    glm::vec2 navOrigin_{0.0f};
    std::vector<uint8_t> navWalkable_;

    bool worldToNav(const glm::vec3& pos, int& col, int& row) const;
    glm::vec3 navToWorld(int col, int row) const;
    void markObstacleDisc(const glm::vec3& center, float radius);
    float buildingNavRadius(EntityType type) const;
    bool unitCameraActive_ = false;
    glm::vec3 savedCameraPos_{0.0f};
    float savedCameraYaw_ = -90.0f;
    float savedCameraPitch_ = -20.0f;
    Unit* unitCameraTarget_ = nullptr;
    float unitCameraYawOffset_ = 0.0f;
    float unitCameraPitchOffset_ = 0.0f;
    SoundManager soundManager_;
    bool victoryShown_ = false;
};
