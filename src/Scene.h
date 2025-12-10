#pragma once
#include <vector>
#include <glm/glm.hpp>

#include "Terrain.h"
#include "../common/Model.h"
#include "../common/Shader.h"
#include "../common/Texture.h"

// --- Game Entities ---
#include "../src/game/GameEntity.h"
#include "../src/game/Resources.h"
#include "../src/game/Worker.h"
#include "../src/game/Knight.h"
#include "../src/game/Archer.h"
#include "../src/game/TownCenter.h"
#include "../src/game/Barracks.h"

class Scene {
public:
    Scene();
    ~Scene();

    void Init();
    void Update(float dt);

    void DrawDepth(Shader& depthShader, const glm::mat4& lightSpaceMatrix);

    void Draw(Shader& terrainShader,
              Shader& objectShader,
              glm::mat4 view,
              glm::mat4 proj,
              glm::vec3 lightPos,
              glm::vec3 viewPos,
              const glm::mat4& lightSpaceMatrix,
              unsigned int shadowMap);

    Terrain* terrain;

private:

    // === MODELS ===
    Model* treeModel;
    Model* rockModel;

    Model* workerModel;
    Model* knightModel;
    Model* archerModel;
    Model* townCenterModel;
    Model* barracksModel;

    // === TEXTURES ===
    Texture* grassTex;
    Texture* rockTex;
    Texture* peakTex;
    Texture* treeTex;
    Texture* boulderTex;

    // === INSTANCING ===
    std::vector<glm::mat4> treeTransforms;
    std::vector<glm::mat4> rockTransforms;

    // === GAME ENTITIES ===
    std::vector<GameEntity*> entities;

    Resources player1;
    Resources player2;

    // === Generation ===
    void generateTrees();
    void generateRocks();

    // === Entity helpers ===
    void SpawnInitialBuildings();
    void DrawEntities(Shader& shader);
    void DrawEntitiesDepth(Shader& depthShader, const glm::mat4& lightSpaceMatrix);
};
