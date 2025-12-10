#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>

#include "Terrain.h"
#include "../common/Model.h"
#include "../common/Shader.h"
#include "../common/Texture.h"

// --- Game Entities ---
// (Ensure these header files exist in src/game/ or code will fail to compile)
#include "game/GameEntity.h"
#include "game/Resources.h"
#include "game/Worker.h"
#include "game/Knight.h"
#include "game/Archer.h"
#include "game/TownCenter.h"
#include "game/Barracks.h"

class Scene {
public:
    Scene();
    ~Scene();

    void Init();
    void Update(float dt); // Update logic for units/buildings

    // Depth-only pass for shadow mapping
    void DrawDepth(Shader& depthShader, const glm::mat4& lightSpaceMatrix);

    // Main render pass
    void Draw(Shader& terrainShader,
              Shader& objectShader,
              glm::mat4 view,
              glm::mat4 proj,
              glm::vec3 lightPos,
              glm::vec3 viewPos,
              const glm::mat4& lightSpaceMatrix,
              unsigned int shadowMap);

    // Terrain needs to be public for Camera collision/height checks
    Terrain* terrain = nullptr;

private:
    // ===========================
    //         ASSETS
    // ===========================
    
    // -- Environment Models --
    Model* treeModel = nullptr;
    Model* rockModel = nullptr;

    // -- Unit/Building Models --
    Model* workerModel     = nullptr;
    Model* knightModel     = nullptr;
    Model* archerModel     = nullptr;
    Model* townCenterModel = nullptr;
    Model* barracksModel   = nullptr;

    // -- Textures --
    Texture* grass1Tex;        
    Texture* grass2Tex;      
    Texture* grass3Tex;       
    Texture* rockTex    = nullptr;
    Texture* peakTex    = nullptr;
    Texture* treeTex    = nullptr;
    Texture* boulderTex = nullptr;
    Texture* sandTex    = nullptr;

    // ===========================
    //         WATER
    // ===========================
    Shader* waterShader = nullptr;
    Texture* waterTex    = nullptr;
    Texture* noiseTex    = nullptr;
    Texture* overlayTex  = nullptr;

    GLuint waterVAO = 0;
    GLuint waterVBO = 0;
    GLuint waterEBO = 0;

    void GenerateWaterGeometry();
    void DrawWater(const glm::mat4& view, const glm::mat4& proj);

    // ===========================
    //      SCENE GENERATION
    // ===========================
    
    // Instancing Data (Static scenery)
    std::vector<glm::mat4> treeTransforms;
    std::vector<glm::mat4> rockTransforms;

    void generateTrees();
    void generateRocks();

    // ===========================
    //      GAMEPLAY / ENTITIES
    // ===========================
    
    // Dynamic Game Objects
    std::vector<GameEntity*> entities;

    Resources player1;
    Resources player2;

    void SpawnInitialBuildings();
    
    // Helpers to render the dynamic list of entities
    void DrawEntities(Shader& shader);
    void DrawEntitiesDepth(Shader& depthShader, const glm::mat4& lightSpaceMatrix);
};