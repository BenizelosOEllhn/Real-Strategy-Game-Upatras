#include "Scene.h"

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
      riverVAO(0), riverVBO(0), riverEBO(0)
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
    delete ghostShader;
    delete previewShader;

    if (waterVAO) glDeleteVertexArrays(1, &waterVAO);
    if (lakeVAO) glDeleteVertexArrays(1, &lakeVAO);
    if (riverVAO) glDeleteVertexArrays(1, &riverVAO);
}

void Scene::Init()
{
    // --- framebuffer sizing ---
    int w, h;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &w, &h);
    fbWidth = (float)w;
    fbHeight = (float)h;

    std::cout << "Framebuffer: " << fbWidth << " × " << fbHeight << "\n";

    // --- terrain + camera ---
    terrain = new Terrain(kTerrainWidth, kTerrainDepth);
    camera  = new Camera(glm::vec3(0, 50, 100));

    const std::string base = ASSET_PATH;

    // --- load textures ---
    grass1Tex = new Texture((base + "textures/grass.png").c_str());
    grass2Tex = new Texture((base + "textures/grass2.jpeg").c_str());
    grass3Tex = new Texture((base + "textures/grass3.jpeg").c_str());
    sandTex   = new Texture((base + "textures/sand1.jpg").c_str());
    rockTex   = new Texture((base + "textures/smallRockTexture.jpg").c_str());
    boulderTex= new Texture((base + "textures/smallRockTexture.jpg").c_str());
    treeTex   = new Texture((base + "textures/leaf.png").c_str());
    peakTex   = new Texture((base + "textures/peak.jpeg").c_str());
    waterTex  = new Texture((base + "textures/water.jpeg").c_str());
    noiseTex  = new Texture((base + "textures/perlin.png").c_str());
    overlayTex= new Texture((base + "textures/overlay.png").c_str());

    // --- load models ---
    treeModel     = new Model((base + "models/tree.obj").c_str());
    rockModel     = new Model((base + "models/Rock.obj").c_str());
    farmModel     = new Model((base + "buildings/Farm_FirstAge_Level1_Wheat.obj").c_str());
    houseModel    = new Model((base + "buildings/Houses_FirstAge_1_Level1.obj").c_str());
    marketModel   = new Model((base + "buildings/Market_FirstAge_Level1.obj").c_str());
    storageModel  = new Model((base + "buildings/Storage_FirstAge_Level1.obj").c_str());
    townCenterModel = new Model((base + "buildings/TownCenter_FirstAge_Level1.obj").c_str());
    barracksModel   = new Model((base + "buildings/Barracks_FirstAge_Level1.obj").c_str());

    // --- shaders ---
    waterShader   = new Shader(base + "shaders/water.vert",
                               base + "shaders/water.frag");

    ghostShader   = new Shader(base + "shaders/ghost.vert",
                               base + "shaders/ghost.frag");

    previewShader = new Shader(base + "shaders/preview.vert",
                               base + "shaders/preview.frag");

    // --- ghost cube for footprint ---
    ghostCube = new Model((base + "models/ghost_cube.obj").c_str());

    // --- UI setup ---
    Shader* uiShader = new Shader(base + "shaders/ui.vert",
                                  base + "shaders/ui.frag");

    Texture* fontTex = new Texture((base + "gui/UIFont_16x16.png").c_str());
    uiManager_.setFontTexture(fontTex->ID, 16, 16, 8.0f, 8.0f);
    uiManager_.init(uiShader, fbWidth, fbHeight);

    // --- building manager ---
    buildingManager_.init(terrain, camera, fbWidth, fbHeight);

    buildingManager_.onPlaceBuilding =
        [this](BuildType type, glm::vec3 pos)
    {
        // same callback body you had before
        // (spawn building instances)
    };

    // --- procedural world generation ---
    GenerateWaterGeometry();
    generateTrees();
    generateRocks();
    generateLakeWater();
    generateRiverWater();
    setupBuildingBar();
}