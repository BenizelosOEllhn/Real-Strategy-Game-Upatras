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


}

Scene::~Scene()
{
    delete terrain;
    delete treeModel;
    delete rockModel;

    delete grass1Tex; delete grass2Tex; delete grass3Tex;
    delete rockTex; delete sandTex; delete boulderTex;
    delete treeTex; delete peakTex; delete waterTex;
    delete noiseTex; delete overlayTex;

    delete waterShader;
    delete previewShader;

    if (waterVAO) glDeleteVertexArrays(1, &waterVAO);
    if (lakeVAO) glDeleteVertexArrays(1, &lakeVAO);
    if (riverVAO) glDeleteVertexArrays(1, &riverVAO);
}
void Scene::Init() {
    int w, h; // 1. Temporary integers
    glfwGetFramebufferSize(glfwGetCurrentContext(), &w, &h);
    initWaterRenderTargets(w, h);

    // 2. Assign to your float variables
    fbWidth = (w);
    fbHeight = (h);

    std::cout << "Framebuffer: " << fbWidth << " x " << fbHeight << std::endl;
    // 1. Generate Terrain Mesh
    terrain = new Terrain(SceneConst::kTerrainWidth, SceneConst::kTerrainDepth);
    camera = new Camera(glm::vec3(0, 50, 100)); // example

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


    // 3. Load Models
    treeModel   = new Model((base + "models/tree.obj").c_str());
    rockModel   = new Model((base + "models/Rock.obj").c_str());

    farmModel    = new Model((base + "buildings/Farm_FirstAge_Level1_Wheat.obj").c_str());    
    houseModel   = new Model((base + "buildings/Houses_FirstAge_1_Level1.obj").c_str());
    marketModel  = new Model((base + "buildings/Market_FirstAge_Level1.obj").c_str());
    storageModel = new Model((base + "buildings/Storage_FirstAge_Level1.obj").c_str());
    townCenterModel = new Model((base + "buildings/TownCenter_FirstAge_Level1.obj").c_str());
    barracksModel   = new Model((base + "buildings/Barracks_FirstAge_Level1.obj").c_str());

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

    // 2. Init UIManager with shader + window size
    uiManager_.init(uiShader, fbWidth, fbHeight);
    buildingManager_.init(terrain, camera, fbWidth, fbHeight);

    buildingManager_.setModels(
    townCenterModel,
    barracksModel
    );
    
    // 5. Generate Procedural Content
    GenerateWaterGeometry(); // big ocean plane
    generateTrees();
    generateRocks();
    generateLakeWater();     // local lake mesh
    generateRiverWater();    // local river mesh
    setupBuildingBar();

    previewShader = new Shader(
    std::string(ASSET_PATH) + "shaders/preview.vert",
    std::string(ASSET_PATH) + "shaders/preview.frag"
    );
    buildingManager_.onPlaceBuilding = [this](BuildType type, glm::vec3 pos)
    {
        std::cout << "Placing building at " 
                << pos.x << ", " << pos.y << ", " << pos.z << std::endl;

// In Scene.cpp inside your callback or update loop
    foundationModel = townCenterModel;
    if (type == BuildType::TownCenter)
    {   foundationModel = townCenterModel;
       GameEntity* tc = new TownCenter(pos, foundationModel,townCenterModel, 1);
        entities_.push_back(tc);
    }
    else if (type == BuildType::Barracks)
    {   foundationModel = barracksModel;
        GameEntity* b = new Barracks(pos, foundationModel, barracksModel, 1);
        entities_.push_back(b);
    }
    else if (type == BuildType::Farm)
    {
        // Pass player resources so the farm can add food
        GameEntity* f = new Farm(pos, farmModel, 1, &player1);
        entities_.push_back(f);
    }
    else if (type == BuildType::House)
    {
        GameEntity* h = new House(pos, houseModel, 1, &player1);
        entities_.push_back(h);
    }
    else if (type == BuildType::Market)
    {
        GameEntity* m = new Market(pos, marketModel, 1, &player1);
        entities_.push_back(m);
    }
    else if (type == BuildType::Storage)
    {
        GameEntity* s = new Storage(pos, storageModel, 1, &player1);
        entities_.push_back(s);
    }

    };

}