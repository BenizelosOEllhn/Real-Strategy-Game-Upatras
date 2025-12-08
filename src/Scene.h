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

    void Init(); 

    // Depth-only pass for shadow map
    void DrawDepth(Shader& depthShader, const glm::mat4& lightSpaceMatrix);

    // Normal pass with Phong + shadows
    void Draw(Shader& terrainShader,
              Shader& objectShader,
              glm::mat4 view,
              glm::mat4 proj,
              glm::vec3 lightPos,
              glm::vec3 viewPos,
              const glm::mat4& lightSpaceMatrix,
              unsigned int shadowMap);

    // We keep Terrain public so Camera can access getHeight if needed
    Terrain* terrain;

private:
    // Assets
    Model*   treeModel;
    Model*   rockModel;

    Texture* grassTex;
    Texture* rockTex;
    Texture* peakTex;
    Texture* treeTex;
    Texture* boulderTex;

    // Instance transforms for instanced rendering
    std::vector<glm::mat4> treeTransforms;
    std::vector<glm::mat4> rockTransforms;

    void generateTrees();
    void generateRocks();
};
