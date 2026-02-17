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
#include <array>
#include <memory>

// ============================================================
// Engine / Rendering
// ============================================================
#include "../rendering/terrain/Terrain.h"
#include "../../common/Model.h"
#include "../../common/Texture.h"
#include "../../common/Shader.h"
#include "RTSCamera.h"
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
#include "../vfx/UnitExplosionSystem.h"
#include "../easteregg/EasterEggSystem.h"
#include "../game/vfx/Projectile.h"
#include "../rendering/terrain/TerrainDeformer.h"
#include "../core/AStarGridUpdater.h"

// ============================================================
// Entities
// ============================================================
#include "../game/data/EntityType.h"
#include "../game/entities/GameEntity.h"
#include "../game/entities/Unit.h"
#include "../game/units/Knight.h"
#include "../game/units/Archer.h"
#include "../game/buildings/TownCenter.h"
#include "../game/buildings/Barracks.h"
#include "../game/buildings/Farm.h"
#include "../game/buildings/House.h"
#include "../game/buildings/Market.h"
#include "../game/buildings/Storage.h"
#include "../game/buildings/Bridge.h"

#include "AssetPath.h"


// ============================================================
// Frustum Culling
// ============================================================
struct Frustum {
    glm::vec4 planes[6]; // left, right, bottom, top, near, far
    
    void extractFromMatrix(const glm::mat4& viewProj) {
        // Left
        planes[0] = glm::vec4(
            viewProj[0][3] + viewProj[0][0],
            viewProj[1][3] + viewProj[1][0],
            viewProj[2][3] + viewProj[2][0],
            viewProj[3][3] + viewProj[3][0]
        );
        // Right
        planes[1] = glm::vec4(
            viewProj[0][3] - viewProj[0][0],
            viewProj[1][3] - viewProj[1][0],
            viewProj[2][3] - viewProj[2][0],
            viewProj[3][3] - viewProj[3][0]
        );
        // Bottom
        planes[2] = glm::vec4(
            viewProj[0][3] + viewProj[0][1],
            viewProj[1][3] + viewProj[1][1],
            viewProj[2][3] + viewProj[2][1],
            viewProj[3][3] + viewProj[3][1]
        );
        // Top
        planes[3] = glm::vec4(
            viewProj[0][3] - viewProj[0][1],
            viewProj[1][3] - viewProj[1][1],
            viewProj[2][3] - viewProj[2][1],
            viewProj[3][3] - viewProj[3][1]
        );
        // Near
        planes[4] = glm::vec4(
            viewProj[0][3] + viewProj[0][2],
            viewProj[1][3] + viewProj[1][2],
            viewProj[2][3] + viewProj[2][2],
            viewProj[3][3] + viewProj[3][2]
        );
        // Far
        planes[5] = glm::vec4(
            viewProj[0][3] - viewProj[0][2],
            viewProj[1][3] - viewProj[1][2],
            viewProj[2][3] - viewProj[2][2],
            viewProj[3][3] - viewProj[3][2]
        );
        
        // Normalize planes
        for (int i = 0; i < 6; i++) {
            float length = glm::length(glm::vec3(planes[i]));
            planes[i] /= length;
        }
    }
    
    bool sphereInFrustum(const glm::vec3& center, float radius) const {
        for (int i = 0; i < 6; i++) {
            float dist = planes[i].x * center.x + 
                        planes[i].y * center.y + 
                        planes[i].z * center.z + 
                        planes[i].w;
            if (dist < -radius)
                return false;
        }
        return true;
    }
};

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
    bool handleUpgradeRequest();
    std::string getBuildingName(BuildType type) const;
    std::string buildingNameForOwner(BuildType type, int ownerId) const;
    void updateBuildingButtonTexturesForOwner(int ownerId);
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
    void configureBuildingPreviewsForOwner(int ownerId);
    void updateBuildingBarLabels();
    Model* modelForBuildType(BuildType type, int ownerId) const;
    Model* modelForBuildType(BuildType type, int ownerId, int level) const;
    Model* unitModelForType(EntityType type, int ownerId) const;
    const std::unordered_map<BuildType, std::string>& buildingInfoMapForOwner(int ownerId) const;
    float buildingScaleForOwner(BuildType type, int ownerId) const;
    glm::vec3 buildingRotationForOwner(BuildType type, int ownerId) const;
    glm::vec3 buildingOffsetForOwner(BuildType type, int ownerId) const;
    void applyBuildingVisualTweaks(Building* building, BuildType type, int ownerId, const glm::vec3* forcedRotation = nullptr);
    void addBridgeSpan(const glm::vec3& pos, float yawRadians);
    bool pointOnBridge(float x, float z) const;
    void initFogOfWar();
    void resetFogOfWar();
    void updateFogOfWar();
    bool updatePlayerFog(int playerId);
    void spawnStartingTownCenters();
    void spawnObjectiveTemple();
    void clearUnitSelection();
    void drawCaptureRadius(const glm::mat4& view, const glm::mat4& projection);
    void selectSingleUnit(const glm::vec2& screenPos, bool additive);
    void selectUnitsInRect(const glm::vec2& a, const glm::vec2& b, bool additive);
    glm::vec2 worldToScreen(const glm::vec3& worldPos) const;
    void issueMoveCommand();
    bool handleProductionRequest(EntityType unitType);
    bool canAffordBuilding(BuildType type) const;
    UnitCost getUpgradeCost(BuildType type, int currentLevel) const;
    bool canAffordUpgrade(const Building* building) const;
    BuildType buildTypeFromEntityType(EntityType type) const;
    UnitCost getBuildingCost(BuildType type) const;
    void selectBuildingAtScreen(const glm::vec2& screenPos);
    void handleDeleteCurrentUnit();
    void deleteUnit(Unit* unit);
    
    // Unit death explosion parameters
    struct UnitExplosionParams {
        float radius = 5.0f;
        float depth = 1.0f;
        float force = 100.0f;
    };
    UnitExplosionParams getExplosionParamsForUnit(EntityType unitType) const;
    Unit* spawnInitialVillager(TownCenter* tc, int forcedNetworkId = -1);
    void startSinglePlayerGame();
    void startLanHostGame();
    void startLanJoinGame();
    void beginGameplay(bool enableLanMode);
    void updateMainMenu(float dt);
    void setMainMenuVisible(bool visible);
    std::string readLanAddress() const;
    void processNetworkMessages();
    void handleNetworkMessage(const std::string& message);
    bool applyBuildCommand(int ownerId, BuildType type, const glm::vec3& pos, int buildingNetId, int initialWorkerNetId, const glm::vec3& rotation);
    void sendBuildCommand(BuildType type, int ownerId, const glm::vec3& pos, int buildingNetId, int initialWorkerNetId, const glm::vec3& rotation);
    void sendUpgradeCommand(int ownerId, int buildingNetId, int targetLevel);
    bool applyUpgradeCommand(int ownerId, int buildingNetId, int targetLevel);
    void sendGatherCommand(int ownerId, int workerNetId, int resourceType, int resourceIndex);
    bool applyGatherCommand(int ownerId, int workerNetId, int resourceType, int resourceIndex);
    void sendAttackCommand(int ownerId, int attackerNetId, int targetNetId);
    bool applyAttackCommand(int ownerId, int attackerNetId, int targetNetId);
    void sendStopCommand(int ownerId, int unitNetId);
    bool applyStopCommand(int ownerId, int unitNetId);
    void sendResourceRemoveCommand(int resourceType, int resourceIndex);
    bool applyResourceRemoveCommand(int resourceType, int resourceIndex);
    void sendResourceGainCommand(int ownerId, int resourceType, int amount);
    bool applyResourceGainCommand(int ownerId, int resourceType, int amount);
    void sendUnitHealthCommand(int unitNetId, float health);
    bool applyUnitHealthCommand(int unitNetId, float health);
    void sendBuildingHealthCommand(int buildingNetId, float health);
    bool applyBuildingHealthCommand(int buildingNetId, float health);
    void sendUnitDeleteCommand(int unitNetId);
    bool applyUnitDeleteCommand(int unitNetId);
    void sendBuildingDeleteCommand(int buildingNetId);
    bool applyBuildingDeleteCommand(int buildingNetId);
    Building* placeBuildingForOwner(BuildType type, const glm::vec3& pos, int ownerId, Resources* ownerRes, bool spendResources, int forcedNetworkId = -1, const glm::vec3* forcedRotation = nullptr);
    void sendTrainCommand(EntityType type, int ownerId, const glm::vec3& pos, int unitNetId);
    bool applyTrainCommand(int ownerId, EntityType type, const glm::vec3& pos, int unitNetId);
    void sendMoveCommand(int networkId, int ownerId, const glm::vec3& pos);
    bool applyMoveCommand(int ownerId, int networkId, const glm::vec3& pos);
    Unit* spawnUnitForOwner(EntityType type, const glm::vec3& pos, int ownerId, bool adjustEconomy, int forcedNetworkId = -1);
    UnitCost getUnitCost(EntityType type) const;
    int findEntityIndex(const GameEntity* entity) const;
    int allocateNetworkId();
    int registerEntity(GameEntity* entity, int requestedId = -1);
    void unregisterEntity(GameEntity* entity);
    GameEntity* findEntityByNetworkId(int networkId) const;
    bool findClosestLandPoint(const glm::vec3& desired, glm::vec3& out) const;
    bool isPositionVisibleToPlayer(const glm::vec3& pos, int playerId) const;
    bool isPositionExploredByPlayer(const glm::vec3& pos, int playerId) const;
    void rebuildFogMeshForPlayer(int playerId);
    void DrawFogOfWar(const glm::mat4& view, const glm::mat4& projection);
    void SetWaterLevels(float ocean, float lake, float river, bool rebuildMeshes);
    float GetOceanY() const { return oceanY; }
    float GetLakeY() const { return lakeY; }
    float GetRiverY() const { return riverY; }
    float visibilityRadiusForEntity(const GameEntity* entity) const;
    bool segmentCrossesWater(const glm::vec3& start, const glm::vec3& end) const;
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
    bool gatherSelectedWorkersNearby(float radius);
    void updateGatherTasks(float dt);
    void updateCombat(float dt);
    void updateProjectiles(float dt);
    std::shared_ptr<Projectile> spawnProjectile(ProjectileType type, const glm::vec3& start, Unit* source, Unit* targetUnit, float damage);
    std::shared_ptr<Projectile> spawnProjectile(ProjectileType type, const glm::vec3& start, Unit* source, Building* targetBuilding, float damage);
    std::shared_ptr<Projectile> spawnProjectile(ProjectileType type, const glm::vec3& start, const glm::vec3& targetPos, Unit* source, float damage);
    void clearGatherTasksFor(Unit* worker);
    void clearGatherTasksFor(ResourceNodeType type, size_t resourceIndex);
    void toggleUnitCamera();
    void updateUnitCameraView();
    bool IsUnitCameraActive() const { return unitCameraActive_; }
    void RotateUnitCamera(float yawDeltaDeg, float pitchDeltaDeg);
    void focusCameraOnTownCenter();
    void rotatePlacementPreview(float radians);
    void initPathfindingGrid();
    void refreshNavObstacles();
    bool commandUnitTo(Unit* unit, const glm::vec3& destination);
    bool findPath(const glm::vec3& start, const glm::vec3& goal, std::vector<glm::vec3>& outPath) const;
    void toggleFogReveal();
    bool isFogRevealed() const { return fogRevealOverride_; }
    float getEntityCullRadius(const GameEntity* entity) const;
    void toggleAmbience();
    UnitExplosionSystem* GetExplosionSystem() { return explosionSystem_.get(); }
    float GetExplosionCameraShake() const { return explosionSystem_ ? explosionSystem_->GetCameraShakeMagnitude() : 0.0f; }
    
    // Navigation grid accessors for pathfinding updates
    int GetNavGridCols() const { return navGridCols_; }
    int GetNavGridRows() const { return navGridRows_; }
    float GetNavCellSize() const { return navCellSize_; }
    glm::vec2 GetNavOrigin() const { return navOrigin_; }
    std::vector<uint8_t>& GetNavWalkable() { return navWalkable_; }
    const std::vector<uint8_t>& GetNavWalkable() const { return navWalkable_; }
    const std::vector<GameEntity*>& GetEntities() const { return entities_; }
    const std::vector<Unit*>& GetSelectedUnits() const { return selectedUnits_; }
    
    // Update navigation grid after terrain deformation (called by UnitExplosionSystem)
    void UpdateNavigationGridAfterDeformation(const glm::vec3& centerWorld, float radiusWorld)
    {
        if (terrainDeformer_)
            terrainDeformer_->RefreshFromTerrain();
        if (gridUpdater_)
            gridUpdater_->UpdateGridAfterDeformation(centerWorld, radiusWorld);
    }

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
    Model* farmModelUpgraded = nullptr;
    Model* houseModel       = nullptr;
    Model* houseModelUpgraded = nullptr;
    Model* marketModel      = nullptr;
    Model* marketModelUpgraded = nullptr;
    Model* storageModel     = nullptr;
    Model* storageModelUpgraded = nullptr;
    Model* townCenterModel  = nullptr;
    Model* townCenterModelUpgraded = nullptr;
    Model* barracksModel    = nullptr;
    Model* barracksModelUpgraded = nullptr;
    Model* farmerModel      = nullptr;
    Model* archerUnitModel  = nullptr;
    Model* knightUnitModel  = nullptr;
    Model* evilFarmerModel  = nullptr;
    Model* wizardUnitModel  = nullptr;
    Model* skeletonUnitModel = nullptr;
    Model* altarModel       = nullptr;
    Model* graveyardModel   = nullptr;
    Model* hutModel         = nullptr;
    Model* smithyModel      = nullptr;
    Model* hangmanModel     = nullptr;
    Model* stoneTempleModel = nullptr;
    Model* objectiveTempleModel = nullptr;
    Model* bridgeModel      = nullptr;

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
    Texture* evilVillagerIconTex = nullptr;
    Texture* evilArcherIconTex = nullptr;
    Texture* evilKnightIconTex = nullptr;
    Texture* selectionRingTex = nullptr;
    Texture* neutralFlagTex = nullptr;
    Texture* blueFlagTex = nullptr;
    Texture* redFlagTex = nullptr;
    Texture* blueRingTex = nullptr;
    Texture* redRingTex = nullptr;
    Texture* greyRingTex = nullptr;

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

    // Projectile rendering (arrow cube + fireball sphere)
    void initProjectileMesh();
    GLuint projectileVAO_ = 0;
    GLuint projectileVBO_ = 0;
    int projectileVertexCount_ = 0;
    GLuint projectileSphereVAO_ = 0;
    GLuint projectileSphereVBO_ = 0;
    GLuint projectileSphereEBO_ = 0;
    int projectileSphereIndexCount_ = 0;

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
    size_t timerLabelIndex_ = SIZE_MAX;
    size_t playerLabelIndex_ = SIZE_MAX;
    size_t victoryLabelIndex_ = SIZE_MAX;
    float timerElapsed_ = 0.0f;
    size_t neutralFlagRingIndex_ = SIZE_MAX;
    size_t neutralFlagRingInnerIndex_ = SIZE_MAX;
    size_t neutralFlagProgressRingIndex_ = SIZE_MAX;
    size_t neutralFlagIconIndex_ = SIZE_MAX;
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
    std::vector<std::shared_ptr<Projectile>> projectiles_;
    std::vector<TownCenter*> townCenters_;
    std::vector<Barracks*>   barracks_;
    std::vector<Unit*>       selectedUnits_;
    Building* selectedBuilding_ = nullptr;
    Building* objectiveTemple_ = nullptr;
    std::unordered_map<int, GameEntity*> networkEntities_;
    int nextNetworkId_ = 1;

    Resources player1;
    Resources player2;
    int activePlayerIndex_ = 0;
    Resources* activeResources_ = nullptr;
    bool mainMenuActive_ = true;
    bool lanSessionPending_ = false;
    bool lanModeActive_ = false;
    bool lanIsHost_ = false;
    std::string lanStatusText_;
    bool suppressNetworkSend_ = false;
    bool objectiveTempleSpawned_ = false;

    enum class CaptureState { Neutral, Player1Capturing, Player2Capturing };
    CaptureState captureState_ = CaptureState::Neutral;
    float captureProgress_ = 0.0f;
    const float captureRadius_ = 40.0f;
    const float captureTime_ = 60.0f;
    void updateCaptureLogic(float dt);
    void updateCaptureUI();

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
    std::vector<BuildType> buildingButtonTypes_;
    struct BuildingButtonIcons
    {
        GLuint friendlyTex = 0;
        GLuint evilTex = 0;
    };
    std::vector<BuildingButtonIcons> buildingButtonIcons_;
    std::vector<std::unique_ptr<Texture>> buildingBarTextures_;
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
    size_t productionUpgradeButtonIndex_ = SIZE_MAX;
    size_t productionUpgradeLabelIndex_ = SIZE_MAX;
    size_t productionUpgradeCostLabelIndex_ = SIZE_MAX;
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
    std::unordered_map<BuildType, std::string> evilBuildingInfoText_;
    struct BridgeSpan
    {
        glm::vec3 center{0.0f};
        float halfLength = 0.0f;
        float halfWidth = 0.0f;
        float yawRadians = 0.0f;
        float cosYaw = 1.0f;
        float sinYaw = 0.0f;
    };
    std::vector<BridgeSpan> bridgeSpans_;

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

    // Fog of war
    std::vector<uint8_t> fogStates_[2];
    std::vector<uint8_t> fogVisibility_[2];
    bool fogDirty_ = true;
    GLuint fogVAO_ = 0;
    GLuint fogVBO_ = 0;
    size_t fogVertexCount_ = 0;
    std::vector<float> fogVertexBuffer_;
    Shader* fogShader = nullptr;
    float fogPlaneY_ = 12.0f;
    bool fogRevealOverride_ = false;
    bool startingBasesSpawned_ = false;
    
    // Frustum culling
    Frustum viewFrustum_;

    // Explosion system
    std::unique_ptr<UnitExplosionSystem> explosionSystem_;
    std::unique_ptr<EasterEggSystem> easterEgg_;
    std::unique_ptr<TerrainDeformer> terrainDeformer_;
    std::unique_ptr<AStarGridUpdater> gridUpdater_;
};
