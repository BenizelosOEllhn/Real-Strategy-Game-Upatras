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
#include "../game/data/Resources.h"

// ============================================================
// Entities
// ============================================================
#include "../game/data/EntityType.h"
#include "../game/entities/GameEntity.h"
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
class Scene
{
public:
    Scene();
    ~Scene();

    void Init();
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
    void onMouseMove(double x, double y);
    void onMouseButton(int button, int action, int mods);

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
    Model* farmModel       = nullptr;
    Model* houseModel      = nullptr;
    Model* marketModel     = nullptr;
    Model* storageModel    = nullptr;
    Model* townCenterModel = nullptr;
    Model* barracksModel   = nullptr;

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

    // ========================================================
    // Foliage
    // ========================================================
    std::vector<glm::mat4> treeTransforms;
    std::vector<glm::mat4> rockTransforms;

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

    double mouseX_ = 0.0;
    double mouseY_ = 0.0;

    Shader* previewShader = nullptr;

    // ========================================================
    // GAME STATE
    // ========================================================
    std::vector<GameEntity*> entities_;
    std::vector<TownCenter*> townCenters_;
    std::vector<Barracks*>   barracks_;

    Resources player1;
};
