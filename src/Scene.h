#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "Terrain.h"
#include "../common/Model.h"
#include "../common/Shader.h"
#include "../common/Texture.h"

class Scene {
public:
    Scene();
    ~Scene();

    void Init(); // Spawns trees, loads assets
    void Draw(Shader& terrainShader, Shader& objectShader, glm::mat4 view, glm::mat4 proj, glm::vec3 lightPos, glm::vec3 viewPos);

    // We keep Terrain public so Camera can access getHeight if needed
    Terrain* terrain;

private:
    // Assets
    Model* treeModel;
    Model* rockModel;
    Texture* grassTex;
    Texture* rockTex;
    Texture* peakTex;
    Texture* treeTex;
    Texture* boulderTex;

    // Data
    std::vector<glm::mat4> treeTransforms;
    std::vector<glm::mat4> rockTransforms;

    void generateTrees();
    void generateRocks();
};
