#include "Scene.h"
#include "SceneConstants.h"

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
    delete selectionRingTex;

    delete farmerModel;
    delete archerUnitModel;
    delete knightUnitModel;

    for (GameEntity* e : entities_)
        delete e;

    if (selectionCircleVAO) glDeleteVertexArrays(1, &selectionCircleVAO);
    if (selectionCircleVBO) glDeleteBuffers(1, &selectionCircleVBO);

    delete waterShader;
    delete previewShader;
    delete selectionShader;

    if (waterVAO) glDeleteVertexArrays(1, &waterVAO);
    if (lakeVAO) glDeleteVertexArrays(1, &lakeVAO);
    if (riverVAO) glDeleteVertexArrays(1, &riverVAO);
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
    selectionRingTex = new Texture((base + "units/unitspngs/ring.png").c_str());
    soundManager_.SetWoodChopPath(base + "audio/wood_chopping.wav");
    soundManager_.SetStoneMinePath(base + "audio/stone_mining.wav");


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
    if (archerUnitModel)
    {
        archerUnitModel->SetOverrideTexture(base + "units/Archer.png");
    }
    if (knightUnitModel)
    {
        knightUnitModel->SetOverrideTexture(base + "units/Knight.png");
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
    buildingManager_.setPreviewModel(BuildType::TownCenter, townCenterModel);
    buildingManager_.setPreviewModel(BuildType::Barracks,   barracksModel);
    buildingManager_.setPreviewModel(BuildType::Farm,       farmModel);
    buildingManager_.setPreviewModel(BuildType::House,      houseModel);
    buildingManager_.setPreviewModel(BuildType::Market,     marketModel);
    buildingManager_.setPreviewModel(BuildType::Storage,    storageModel);

    UnitManager::UnitAssets unitAssets;
    unitAssets.farmer       = farmerModel;
    unitAssets.archer       = archerUnitModel;
    unitAssets.knight       = knightUnitModel;
    unitManager_.init(activePlayerPtr(), &entities_, unitAssets);
    
    // 5. Generate Procedural Content
    GenerateWaterGeometry(); // big ocean plane
    generateTrees();
    generateRocks();
    generateLakeWater();     // local lake mesh
    generateRiverWater();    // local river mesh
    initPathfindingGrid();
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
    buildingManager_.onPlaceBuilding = [this](BuildType type, glm::vec3 pos)
    {
        std::cout << "Placing building at " 
                << pos.x << ", " << pos.y << ", " << pos.z << std::endl;
        Resources* ownerRes = activePlayerPtr();
        if (!ownerRes)
            return;

        UnitCost buildCost = getBuildingCost(type);
        if (!ownerRes->Spend(buildCost))
        {
            std::cout << "Insufficient resources for building." << std::endl;
            return;
        }

        int ownerId = activePlayerIndex_ + 1;

        switch (type)
        {
        case BuildType::TownCenter:
        {
            TownCenter* tc = new TownCenter(pos, townCenterModel, townCenterModel, ownerId);
            entities_.push_back(tc);
            registerTownCenter(tc);
            spawnInitialVillager(tc);
            break;
        }
        case BuildType::Barracks:
        {
            Barracks* b = new Barracks(pos, barracksModel, barracksModel, ownerId);
            entities_.push_back(b);
            registerBarracks(b);
            break;
        }
        case BuildType::Farm:
        {
            Farm* f = new Farm(pos, farmModel, farmModel, ownerId, ownerRes);
            entities_.push_back(f);
            break;
        }
        case BuildType::House:
        {
            House* h = new House(pos, houseModel, houseModel, ownerId, ownerRes);
            entities_.push_back(h);
            break;
        }
        case BuildType::Market:
        {
            Market* m = new Market(pos, marketModel, marketModel, ownerId, ownerRes);
            entities_.push_back(m);
            break;
        }
        case BuildType::Storage:
        {
            Storage* s = new Storage(pos, storageModel, storageModel, ownerId, ownerRes);
            entities_.push_back(s);
            break;
        }
        default:
            break;
        }

    updateResourceTexts();
    refreshNavObstacles();

    };

    initSelectionCircle();
}
