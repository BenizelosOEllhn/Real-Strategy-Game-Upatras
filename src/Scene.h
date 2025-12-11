#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>

#include "Terrain.h"
#include "Model.h"
#include "Texture.h"
#include "Shader.h"

class Scene
{
public:
    Scene();
    ~Scene();

    void Init();

    void DrawDepth(Shader& depthShader, const glm::mat4& lightSpaceMatrix);

    void Draw(Shader& terrainShader,
              Shader& objectShader,
              glm::mat4 view,
              glm::mat4 projection,
              glm::vec3 lightPos,
              glm::vec3 viewPos,
              const glm::mat4& lightSpaceMatrix,
              unsigned int shadowMap);

private:
    // --------------------------------------------------------
    // Core world data
    // --------------------------------------------------------
    Terrain* terrain;

    Model* treeModel;
    Model* rockModel;

    // Textures
    Texture* grass1Tex;
    Texture* grass2Tex;
    Texture* grass3Tex;
    Texture* rockTex;
    Texture* sandTex;
    Texture* treeTex;
    Texture* peakTex;
    Texture* boulderTex;
    Texture* waterTex;
    Texture* noiseTex;
    Texture* overlayTex;

    // Water shader (used by ocean, lake and river)
    Shader* waterShader;

    // --------------------------------------------------------
    // Instanced foliage / rocks
    // --------------------------------------------------------
    std::vector<glm::mat4> treeTransforms;
    std::vector<glm::mat4> rockTransforms;

    void generateTrees();
    void generateRocks();

    // --------------------------------------------------------
    // BIG OCEAN WATER PLANE
    // --------------------------------------------------------
    GLuint waterVAO;
    GLuint waterVBO;
    GLuint waterEBO;

    void GenerateWaterGeometry();
    void DrawWater(const glm::mat4& view, const glm::mat4& proj);

    // --------------------------------------------------------
    // LOCAL WATER MESHES (LAKE + RIVERS)
    // --------------------------------------------------------
    struct WaterVertex
    {
        glm::vec3 position;
        glm::vec2 uv;
    };

    // --- Lake water mesh ---
    std::vector<WaterVertex>    lakeWaterVerts;
    std::vector<unsigned int>   lakeWaterIndices;
    GLuint                      lakeVAO;
    GLuint                      lakeVBO;
    GLuint                      lakeEBO;

    // --- River water mesh ---
    std::vector<WaterVertex>    riverWaterVerts;
    std::vector<unsigned int>   riverWaterIndices;
    GLuint                      riverVAO;
    GLuint                      riverVBO;
    GLuint                      riverEBO;

    void generateLakeWater();
    void uploadLakeWaterMesh();
    void DrawLakeWater(const glm::mat4& view, const glm::mat4& proj);

    void generateRiverWater();
    void uploadRiverWaterMesh();
    void DrawRiverWater(const glm::mat4& view, const glm::mat4& proj);
};
    