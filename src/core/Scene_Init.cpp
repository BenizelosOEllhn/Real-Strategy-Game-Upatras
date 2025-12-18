#include "Scene.h"
#include "SceneConstants.h"
#include "../game/units/Worker.h"
#include "../game/units/Archer.h"
#include "../game/units/Knight.h"
#include <glm/gtc/constants.hpp>

Scene::Scene()
    : terrain(nullptr),
      treeModel(nullptr), rockModel(nullptr),
      grass1Tex(nullptr), grass2Tex(nullptr), grass3Tex(nullptr),
      rockTex(nullptr), sandTex(nullptr),
      treeTex(nullptr), peakTex(nullptr), boulderTex(nullptr),
      waterTex(nullptr), noiseTex(nullptr), overlayTex(nullptr),
      waterShader(nullptr),
      waterVAO(0), waterVBO(0), waterEBO(0),
      lakeVAO(0), lakeVBO(0), lakeEBO(0),
      riverVAO(0), riverVBO(0), riverEBO(0),
      reflectionFBO(0),
      refractionFBO(0),
      reflectionColorTex(0),
      refractionColorTex(0),
      refractionDepthTex(0),
      reflectionDepthRBO(0),
      waterRTWidth(0),
      waterRTHeight(0)
{
    activeResources_ = &player1;
}

void Scene::registerTownCenter(TownCenter* tc)
{
    if (!tc) return;
    townCenters_.push_back(tc);
    unitManager_.registerTownCenter(tc);
}

void Scene::registerBarracks(Barracks* barracks)
{
    if (!barracks) return;
    barracks_.push_back(barracks);
    unitManager_.registerBarracks(barracks);
}

void Scene::initSelectionCircle()
{
    if (selectionCircleVAO != 0)
        return;

    const float verts[] = {
        // positions          // uv
        -1.0f, 0.0f, -1.0f,   0.0f, 1.0f,
         1.0f, 0.0f, -1.0f,   1.0f, 1.0f,
         1.0f, 0.0f,  1.0f,   1.0f, 0.0f,

        -1.0f, 0.0f, -1.0f,   0.0f, 1.0f,
         1.0f, 0.0f,  1.0f,   1.0f, 0.0f,
        -1.0f, 0.0f,  1.0f,   0.0f, 0.0f
    };

    glGenVertexArrays(1, &selectionCircleVAO);
    glGenBuffers(1, &selectionCircleVBO);
    glBindVertexArray(selectionCircleVAO);
    glBindBuffer(GL_ARRAY_BUFFER, selectionCircleVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);
}

Scene::~Scene()
{
    soundManager_.Shutdown();
    networkSession_.Shutdown();
    delete terrain;
    delete treeModel;
    delete rockModel;

    delete grass1Tex; delete grass2Tex; delete grass3Tex;
    delete rockTex; delete sandTex; delete boulderTex;
    delete treeTex; delete peakTex; delete waterTex;
    delete noiseTex; delete overlayTex;
    delete cornIconTex; delete woodIconTex; delete goldIconTex;
    delete oreIconTex; delete populationIconTex;
    delete villagerIconTex; delete archerIconTex; delete knightIconTex;
    delete evilVillagerIconTex; delete evilArcherIconTex; delete evilKnightIconTex;
    delete selectionRingTex;

    delete farmerModel;
    delete archerUnitModel;
    delete knightUnitModel;
    delete evilFarmerModel;
    delete wizardUnitModel;
    delete skeletonUnitModel;
    delete altarModel;
    delete graveyardModel;
    delete hutModel;
    delete smithyModel;
    delete hangmanModel;
    delete stoneTempleModel;
    delete bridgeModel;

    for (GameEntity* e : entities_)
        delete e;

    if (selectionCircleVAO) glDeleteVertexArrays(1, &selectionCircleVAO);
    if (selectionCircleVBO) glDeleteBuffers(1, &selectionCircleVBO);

    delete waterShader;
    delete previewShader;
    delete selectionShader;
    delete fogShader;

    if (waterVAO) glDeleteVertexArrays(1, &waterVAO);
    if (lakeVAO) glDeleteVertexArrays(1, &lakeVAO);
    if (riverVAO) glDeleteVertexArrays(1, &riverVAO);
    if (fogVAO_) glDeleteVertexArrays(1, &fogVAO_);
    if (fogVBO_) glDeleteBuffers(1, &fogVBO_);
}
void Scene::Init(Camera* activeCamera) {
    camera = activeCamera;
    if (!camera)
    {
        std::cerr << "Scene::Init requires a valid camera." << std::endl;
        return;
    }
    int w, h; // 1. Temporary integers
    glfwGetFramebufferSize(glfwGetCurrentContext(), &w, &h);
    initWaterRenderTargets(w, h);

    // 2. Assign to your float variables
    fbWidth = (w);
    fbHeight = (h);

    std::cout << "Framebuffer: " << fbWidth << " x " << fbHeight << std::endl;
    // 1. Generate Terrain Mesh
    terrain = new Terrain(SceneConst::kTerrainWidth, SceneConst::kTerrainDepth);

    const std::string base = ASSET_PATH;

    // 2. Load Textures
    grass1Tex   = new Texture((base + "textures/grass.png").c_str());
    grass2Tex   = new Texture((base + "textures/grass2.jpeg").c_str());
    grass3Tex   = new Texture((base + "textures/grass3.jpeg").c_str());
    rockTex     = new Texture((base + "textures/smallRockTexture.jpg").c_str());
    sandTex     = new Texture((base + "textures/sand1.jpg").c_str());
    treeTex     = new Texture((base + "textures/leaf.png").c_str());
    peakTex     = new Texture((base + "textures/peak.jpeg").c_str());
    boulderTex  = new Texture((base + "textures/smallRockTexture.jpg").c_str());
    waterTex    = new Texture((base + "textures/water.jpeg").c_str());
    noiseTex    = new Texture((base + "textures/perlin.png").c_str());
    overlayTex  = new Texture((base + "textures/overlay.png").c_str());
    foamTex = new Texture((base + "textures/foam_noise.png").c_str());
    villagerIconTex = new Texture((base + "units/unitspngs/farmer.png").c_str());
    archerIconTex   = new Texture((base + "units/unitspngs/archer.png").c_str());
    knightIconTex   = new Texture((base + "units/unitspngs/swordman.png").c_str());
    evilVillagerIconTex = new Texture((base + "evilunits/evilunitspngs/orc-head.png").c_str());
    evilArcherIconTex   = new Texture((base + "evilunits/evilunitspngs/wizard-face.png").c_str());
    evilKnightIconTex   = new Texture((base + "evilunits/evilunitspngs/skeleton.png").c_str());
    selectionRingTex = new Texture((base + "units/unitspngs/ring.png").c_str());
    soundManager_.SetWoodChopPath(base + "audio/wood_chopping.wav");
    soundManager_.SetStoneMinePath(base + "audio/stone_mining.wav");
    soundManager_.SetAmbiencePath(base + "audio/ambience.mp3");
    soundManager_.StartAmbience();


    // 3. Load Models
    treeModel   = new Model((base + "models/tree.obj").c_str());
    rockModel   = new Model((base + "models/Rock.obj").c_str());

    farmModel    = new Model((base + "buildings/Farm_FirstAge_Level1_Wheat.obj").c_str());    
    houseModel   = new Model((base + "buildings/Houses_FirstAge_1_Level1.obj").c_str());
    marketModel  = new Model((base + "buildings/Market_FirstAge_Level1.obj").c_str());
    storageModel = new Model((base + "buildings/Storage_FirstAge_Level1.obj").c_str());
    townCenterModel = new Model((base + "buildings/TownCenter_FirstAge_Level1.obj").c_str());
    barracksModel   = new Model((base + "buildings/Barracks_FirstAge_Level1.obj").c_str());
    farmerModel      = new Model((base + "units/Farmer.glb").c_str());
    archerUnitModel  = new Model((base + "units/archer_version_3.glb").c_str());
    knightUnitModel  = new Model((base + "units/knight.glb").c_str());
    evilFarmerModel  = new Model((base + "evilunits/evil_farmer.glb").c_str());
    wizardUnitModel  = new Model((base + "evilunits/evil_wizard.glb").c_str());
    skeletonUnitModel = new Model((base + "evilunits/skeleton_knight.glb").c_str());
    altarModel       = new Model((base + "evilbuildings/altar.glb").c_str());
    graveyardModel   = new Model((base + "evilbuildings/Demo_Scene.obj").c_str());
    hutModel         = new Model((base + "evilbuildings/hut.fbx").c_str());
    smithyModel      = new Model((base + "evilbuildings/smithy.fbx").c_str());
    hangmanModel     = new Model((base + "evilbuildings/hangman_wooden_structure.glb").c_str());
    stoneTempleModel = new Model((base + "evilbuildings/old_ruined_temple.glb").c_str());
    bridgeModel      = new Model((base + "buildings/stylized_bridge_low_poly.glb").c_str());
    if (archerUnitModel)
    {
        archerUnitModel->SetOverrideTexture(base + "units/Archer.png");
    }
    if (knightUnitModel)
    {
        knightUnitModel->SetOverrideTexture(base + "units/Knight.png");
    }
    if (graveyardModel)
    {
        graveyardModel->SetOverrideTexture(base + "evilbuildings/gravetex.png");
    }
    if (hutModel)
    {
        hutModel->SetOverrideTexture(base + "evilbuildings/proto_orc_RTS_color.tga.png");
    }
    if (smithyModel)
    {
        smithyModel->SetOverrideTexture(base + "evilbuildings/proto_orc_RTS_color.tga.png");
    }

    // 4. Load Water Shader
    waterShader = new Shader(
        std::string(ASSET_PATH) + "shaders/water.vert",
        std::string(ASSET_PATH) + "shaders/water.frag"
    );

    // 1. Load UI shader
    Shader* uiShader = new Shader(
        std::string(ASSET_PATH) + "shaders/ui.vert",
        std::string(ASSET_PATH) + "shaders/ui.frag"
    );
    std::string fontPath = std::string(ASSET_PATH) + "gui/UIFont_16x16.png";
    Texture* fontTex = new Texture(fontPath.c_str());
    uiManager_.setFontTexture(fontTex->ID, 16, 16, 8.0f, 8.0f);
    uiManager_.setTextScale(1.35f);

    // 2. Init UIManager with shader + window size
    uiManager_.init(uiShader, fbWidth, fbHeight);
    buildingManager_.init(terrain, camera, fbWidth, fbHeight);
    buildingManager_.setPlacementValidator([this](BuildType, const glm::vec3& pos)
    {
        return isPositionExploredByPlayer(pos, activePlayerIndex_ + 1);
    });
    configureBuildingPreviewsForOwner(activePlayerIndex_ + 1);

    UnitManager::UnitAssets unitAssets;
    unitAssets.farmer       = farmerModel;
    unitAssets.archer       = archerUnitModel;
    unitAssets.knight       = knightUnitModel;
    unitAssets.evilFarmer   = evilFarmerModel;
    unitAssets.wizard       = wizardUnitModel;
    unitAssets.skeleton     = skeletonUnitModel;
    unitManager_.init(activePlayerPtr(), &entities_, unitAssets);
    
    // 5. Generate Procedural Content
    GenerateWaterGeometry(); // big ocean plane
    generateTrees();
    generateRocks();
    generateLakeWater();     // local lake mesh
    generateRiverWater();    // local river mesh
    initPathfindingGrid();
    initFogOfWar();
    setupBuildingBar();
    setupBuildingInfoPanel();
    setupResourceBar();
    setupUnitPanel();
    setupProductionPanel();
    setupTabButtons();
    setupUnitInfoPanel();
    setupMainMenu();
    setMainMenuVisible(true);
    setActiveTab(UITab::Buildings);
    refreshUnitListUI();
    updateProductionPanel();
    updateUnitInfoPanel();

    previewShader = new Shader(
    std::string(ASSET_PATH) + "shaders/preview.vert",
    std::string(ASSET_PATH) + "shaders/preview.frag"
    );
    selectionShader = new Shader(
    std::string(ASSET_PATH) + "shaders/selection.vert",
    std::string(ASSET_PATH) + "shaders/selection.frag"
    );
    fogShader = new Shader(
    std::string(ASSET_PATH) + "shaders/fog.vert",
    std::string(ASSET_PATH) + "shaders/fog.frag"
    );
    buildingManager_.onPlaceBuilding = [this](BuildType type, glm::vec3 pos, glm::vec3 rotation)
    {
        Resources* ownerRes = resourcesForOwner(activePlayerIndex_ + 1);
        if (!ownerRes)
            return;

        int ownerId = activePlayerIndex_ + 1;
        Building* building = placeBuildingForOwner(type, pos, ownerId, ownerRes, true, -1, &rotation);
        if (!building)
            return;

        int buildingNetId = building->GetNetworkId();
        int initialVillagerId = -1;
        if (type == BuildType::TownCenter)
        {
            if (TownCenter* tc = dynamic_cast<TownCenter*>(building))
            {
                if (Unit* villager = spawnInitialVillager(tc))
                    initialVillagerId = villager->GetNetworkId();
            }
        }

        if (lanModeActive_ && !suppressNetworkSend_ && networkSession_.IsConnected())
            sendBuildCommand(type, ownerId, pos, buildingNetId, initialVillagerId, rotation);
    };

    initSelectionCircle();
}

Building* Scene::placeBuildingForOwner(BuildType type,
                                       const glm::vec3& pos,
                                       int ownerId,
                                       Resources* ownerRes,
                                       bool spendResources,
                                       int forcedNetworkId,
                                       const glm::vec3* forcedRotation)
{
    if (!ownerRes)
        return nullptr;

    if (spendResources)
    {
        UnitCost cost = getBuildingCost(type);
        if (!ownerRes->Spend(cost))
        {
            std::cout << "Insufficient resources for building." << std::endl;
            return nullptr;
        }
    }

    Model* finalModel = modelForBuildType(type, ownerId);
    if (!finalModel)
        finalModel = modelForBuildType(type, 1);
    Model* foundation = finalModel;
    Building* newBuilding = nullptr;
    switch (type)
    {
    case BuildType::TownCenter:
        newBuilding = new TownCenter(pos, foundation, finalModel, ownerId);
        break;
    case BuildType::Barracks:
        newBuilding = new Barracks(pos, foundation, finalModel, ownerId);
        break;
    case BuildType::Farm:
        newBuilding = new Farm(pos, foundation, finalModel, ownerId, ownerRes);
        break;
    case BuildType::House:
        newBuilding = new House(pos, foundation, finalModel, ownerId, ownerRes);
        break;
    case BuildType::Market:
        newBuilding = new Market(pos, foundation, finalModel, ownerId, ownerRes);
        break;
    case BuildType::Storage:
        newBuilding = new Storage(pos, foundation, finalModel, ownerId, ownerRes);
        break;
    case BuildType::Bridge:
        newBuilding = new Bridge(pos, foundation, finalModel, ownerId);
        break;
    default:
        break;
    }

    if (!newBuilding)
        return nullptr;

    glm::vec3 rotationApplied = forcedRotation
        ? *forcedRotation
        : buildingRotationForOwner(type, ownerId);
    applyBuildingVisualTweaks(newBuilding, type, ownerId, &rotationApplied);

    if (type == BuildType::Bridge)
        addBridgeSpan(pos, rotationApplied.y);

    registerEntity(newBuilding, forcedNetworkId);
    entities_.push_back(newBuilding);
    if (type == BuildType::TownCenter)
    {
        registerTownCenter(static_cast<TownCenter*>(newBuilding));
    }
    else if (type == BuildType::Barracks)
    {
        registerBarracks(static_cast<Barracks*>(newBuilding));
    }

    updateResourceTexts();
    refreshNavObstacles();
    return newBuilding;
}

Unit* Scene::spawnUnitForOwner(EntityType type, const glm::vec3& pos, int ownerId, bool adjustEconomy, int forcedNetworkId)
{
    Unit* unit = nullptr;
    Model* unitModel = unitModelForType(type, ownerId);
    switch (type)
    {
    case EntityType::Worker:
        if (unitModel)
            unit = new Worker(pos, unitModel, ownerId);
        break;
    case EntityType::Archer:
        if (unitModel)
            unit = new Archer(pos, unitModel, ownerId);
        break;
    case EntityType::Knight:
        if (unitModel)
            unit = new Knight(pos, unitModel, ownerId);
        break;
    default:
        break;
    }

    if (!unit)
        return nullptr;

    registerEntity(unit, forcedNetworkId);
    entities_.push_back(unit);
    if (adjustEconomy)
    {
        Resources* res = resourcesForOwner(ownerId);
        if (res)
        {
            res->AddPopulation(1);
            if (type == EntityType::Worker)
                res->AddVillager(1);
        }
    }

    refreshUnitListUI();
    updateResourceTexts();
    return unit;
}

Model* Scene::modelForBuildType(BuildType type, int ownerId) const
{
    const bool evil = (ownerId == 2);
    switch (type)
    {
    case BuildType::TownCenter: return evil && altarModel ? altarModel : townCenterModel;
    case BuildType::Barracks:   return evil && graveyardModel ? graveyardModel : barracksModel;
    case BuildType::Farm:       return evil && hangmanModel ? hangmanModel : farmModel;
    case BuildType::House:      return evil && hutModel ? hutModel : houseModel;
    case BuildType::Market:     return evil && smithyModel ? smithyModel : marketModel;
    case BuildType::Storage:    return evil && stoneTempleModel ? stoneTempleModel : storageModel;
    case BuildType::Bridge:     return bridgeModel;
    default:                    return nullptr;
    }
}

Model* Scene::unitModelForType(EntityType type, int ownerId) const
{
    const bool evil = (ownerId == 2);
    switch (type)
    {
    case EntityType::Worker:
        if (evil && evilFarmerModel) return evilFarmerModel;
        return farmerModel;
    case EntityType::Archer:
        if (evil && wizardUnitModel) return wizardUnitModel;
        return archerUnitModel;
    case EntityType::Knight:
        if (evil && skeletonUnitModel) return skeletonUnitModel;
        return knightUnitModel;
    default:
        return nullptr;
    }
}

void Scene::configureBuildingPreviewsForOwner(int ownerId)
{
    auto configure = [&](BuildType type)
    {
        buildingManager_.setPreviewModel(type, modelForBuildType(type, ownerId));
        buildingManager_.setPreviewScale(type, buildingScaleForOwner(type, ownerId));
        buildingManager_.setPreviewRotation(type, buildingRotationForOwner(type, ownerId));
        buildingManager_.setPreviewOffset(type, buildingOffsetForOwner(type, ownerId));
    };

    configure(BuildType::TownCenter);
    configure(BuildType::Barracks);
    configure(BuildType::Farm);
    configure(BuildType::House);
    configure(BuildType::Market);
    configure(BuildType::Storage);
    configure(BuildType::Bridge);
}

float Scene::buildingScaleForOwner(BuildType type, int ownerId) const
{
    const bool evil = (ownerId == 2);
    switch (type)
    {
    case BuildType::Bridge:
        return 12.0f;
    case BuildType::Barracks:
        return evil ? 0.06f : 20.0f;
    case BuildType::Farm:
        return evil ? 1.2f : 20.0f;
    default:
        return 20.0f;
    }
}

glm::vec3 Scene::buildingRotationForOwner(BuildType type, int /*ownerId*/) const
{
    switch (type)
    {
    case BuildType::Bridge:
        return glm::vec3(-glm::half_pi<float>(), 0.0f, 0.0f);
    default:
        return glm::vec3(0.0f);
    }
}

glm::vec3 Scene::buildingOffsetForOwner(BuildType type, int ownerId) const
{
    const bool evil = (ownerId == 2);
    if (type == BuildType::Bridge)
        return glm::vec3(0.0f, 5.0f, 0.0f);
    if (evil && type == BuildType::TownCenter)
        return glm::vec3(0.0f, 5.0f, 0.0f);
    return glm::vec3(0.0f);
}

void Scene::applyBuildingVisualTweaks(Building* building, BuildType type, int ownerId, const glm::vec3* forcedRotation)
{
    if (!building)
        return;

    building->SetUniformScale(buildingScaleForOwner(type, ownerId));
    building->SetVisualOffset(buildingOffsetForOwner(type, ownerId));
    glm::vec3 rotation = forcedRotation
        ? *forcedRotation
        : buildingRotationForOwner(type, ownerId);
    building->SetRotationEuler(rotation);
}
