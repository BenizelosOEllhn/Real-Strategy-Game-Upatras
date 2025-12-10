#include "Scene.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <random>
#include <iostream>
#include <string>
#include <cmath>

#ifndef ASSET_PATH
#define ASSET_PATH "assets/"
#endif

// ------------------------------------------------------------
// Terrain & Generation Constants
// ------------------------------------------------------------
namespace {
    constexpr int   kTerrainWidth     = 270;
    constexpr int   kTerrainDepth     = 300;
    constexpr int   kTreeCount        = 360;
    constexpr int   kRockCount        = 110;

    constexpr float kMountainStart    = -90.0f;
    constexpr float kMountainAvoidZ   = -70.0f;
    constexpr float kLakeCenterZ      = 80.0f;
    constexpr float kLakeRadius       = 55.0f;
    constexpr float kRiverTreeBuffer  = 14.0f;
    constexpr float kSouthForestBias  = 0.75f;
    constexpr float kMountainTreeBias = 0.2f;
    constexpr float kCornerPlainZ     = 135.0f;
    constexpr float kCornerPlainX     = 95.0f;

    constexpr float kRockZoneMinZ     = kMountainStart - 80.0f;
    constexpr float kRockZoneMaxZ     = -45.0f;
    constexpr float kRockMinHeight    = 5.0f;

    // Helper to check if a point is too close to the river
    bool nearRiver(float x, float z) {
        if (!(z < kLakeCenterZ && z > kMountainStart - 20.0f)) return false;
        
        float symX = std::abs(x);
        float distFromLake = kLakeCenterZ - z;
        float basePathX = 25.0f + (distFromLake * 0.4f);
        float snakeWiggle = std::sin(z * 0.08f) * 12.0f;
        float riverIdealX = basePathX + snakeWiggle;
        float distToRiver = std::abs(symX - riverIdealX);
        
        return distToRiver < kRiverTreeBuffer;
    }
}

// ------------------------------------------------------------
// Constructor / Destructor
// ------------------------------------------------------------
Scene::Scene()
    : terrain(nullptr),
      treeModel(nullptr), rockModel(nullptr),
      grass1Tex(nullptr), grass2Tex(nullptr), grass3Tex(nullptr),
      rockTex(nullptr), 
      treeTex(nullptr), boulderTex(nullptr),
      waterTex(nullptr), noiseTex(nullptr), overlayTex(nullptr),
      waterShader(nullptr),
      waterVAO(0), waterVBO(0), waterEBO(0)
{
}

Scene::~Scene() {
    // Cleanup Models & Terrain
    delete terrain;
    delete treeModel;
    delete rockModel;

    // Cleanup Textures
    delete rockTex;
    delete boulderTex;
    delete treeTex;
    delete waterTex;
    delete noiseTex;
    delete overlayTex;

    // Cleanup Water Shader & Buffers
    delete waterShader;
    if (waterVAO) glDeleteVertexArrays(1, &waterVAO);
    if (waterVBO) glDeleteBuffers(1, &waterVBO);
    if (waterEBO) glDeleteBuffers(1, &waterEBO);
}

// ------------------------------------------------------------
// Initialization
// ------------------------------------------------------------
void Scene::Init() {
    // 1. Generate Terrain Mesh
    terrain = new Terrain(kTerrainWidth, kTerrainDepth);

    const std::string base = ASSET_PATH;
    

    // 2. Load Textures
    grass1Tex   = new Texture((base + "textures/grass.png").c_str());
    grass2Tex   = new Texture((base + "textures/grass2.jpeg").c_str());
    grass3Tex   = new Texture((base + "textures/grass3.jpeg").c_str());
    rockTex    = new Texture((base + "textures/rock.jpeg").c_str());
    treeTex    = new Texture((base + "textures/leaf.png").c_str());
    boulderTex = new Texture((base + "textures/smallRockTexture.jpg").c_str());
    waterTex   = new Texture((base + "textures/water.jpeg").c_str());
    noiseTex   = new Texture((base + "textures/perlin.png").c_str());
    overlayTex = new Texture((base + "textures/overlay.png").c_str());

    // 3. Load Models
    treeModel  = new Model((base + "models/tree.obj").c_str());
    rockModel  = new Model((base + "models/Rock.obj").c_str());

    // 4. Load Water Shader
    waterShader = new Shader(base + "shaders/water.vert", base + "shaders/water.frag");

    // 5. Generate Procedural Content
    GenerateWaterGeometry();
    generateTrees();
    generateRocks();
}

// ------------------------------------------------------------
// DEPTH PASS (Shadow Mapping)
// ------------------------------------------------------------
void Scene::DrawDepth(Shader& depthShader, const glm::mat4& lightSpaceMatrix) {
    if (!terrain) return;

    depthShader.Use();
    depthShader.SetMat4("lightSpaceMatrix", lightSpaceMatrix);

    // 1. Draw Terrain
    depthShader.SetMat4("model", glm::mat4(1.0f));
    terrain->Draw(depthShader.ID);

    // 2. Draw Trees (Instanced)
    if (treeModel && !treeTransforms.empty()) {
        treeModel->DrawInstanced(depthShader, treeTransforms);
    }

    // 3. Draw Rocks (Instanced)
    if (rockModel && !rockTransforms.empty()) {
        rockModel->DrawInstanced(depthShader, rockTransforms);
    }
}

// ------------------------------------------------------------
// MAIN RENDER PASS
// ------------------------------------------------------------
void Scene::Draw(Shader& terrainShader,
                 Shader& objectShader,
                 glm::mat4 view,
                 glm::mat4 projection,
                 glm::vec3 lightPos,
                 glm::vec3 viewPos,
                 const glm::mat4& lightSpaceMatrix,
                 unsigned int shadowMap)
{
    const glm::vec3 lightColor(1.0f, 0.97f, 0.92f);

    // ===============================
    // 1. TERRAIN RENDERING
    // ===============================
    if (terrain && grass1Tex && grass2Tex && grass3Tex)
    {
        terrainShader.Use();

        terrainShader.SetMat4("view", view);
        terrainShader.SetMat4("projection", projection);
        terrainShader.SetMat4("model", glm::mat4(1.0f));
        terrainShader.SetMat4("lightSpaceMatrix", lightSpaceMatrix);

        grass1Tex->Bind(0);
        grass2Tex->Bind(1);
        grass3Tex->Bind(2);
        noiseTex->Bind(3);

        rockTex->Bind(4);

        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, shadowMap);

        // Shader uniform mapping
        terrainShader.SetInt("grass1", 0);
        terrainShader.SetInt("grass2", 1);
        terrainShader.SetInt("grass3", 2);
        terrainShader.SetInt("noiseDetail", 3);
        terrainShader.SetInt("rockTex", 4);
        terrainShader.SetInt("shadowMap", 7);


        terrainShader.SetVec3("lightPos", lightPos);
        terrainShader.SetVec3("viewPos", viewPos);
        terrainShader.SetVec3("lightColor", lightColor);

        terrain->Draw(terrainShader.ID);
    }

    // ===============================
    // 2. INSTANCED NATURAL OBJECTS
    // ===============================
    bool hasTrees = treeModel && treeTex && !treeTransforms.empty();
    bool hasRocks = rockModel && boulderTex && !rockTransforms.empty();

    if (hasTrees || hasRocks)
    {
        objectShader.Use();
        objectShader.SetMat4("view", view);
        objectShader.SetMat4("projection", projection);
        objectShader.SetMat4("lightSpaceMatrix", lightSpaceMatrix);
        objectShader.SetMat4("model", glm::mat4(1.0f));

        objectShader.SetVec3("lightPos", lightPos);
        objectShader.SetVec3("viewPos", viewPos);
        objectShader.SetVec3("lightColor", lightColor);

        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, shadowMap);
        objectShader.SetInt("shadowMap", 7);
    }

    if (hasTrees)
    {
        treeTex->Bind(0);
        objectShader.SetInt("texture_diffuse1", 0);
        treeModel->DrawInstanced(objectShader, treeTransforms);
    }

    if (hasRocks)
    {
        boulderTex->Bind(0);
        objectShader.SetInt("texture_diffuse1", 0);
        rockModel->DrawInstanced(objectShader, rockTransforms);
    }

    // ===============================
    // 3. WATER DRAW
    // ===============================
    DrawWater(view, projection);
}


// ------------------------------------------------------------
// Procedural Generation: Trees
// ------------------------------------------------------------
void Scene::generateTrees() {
    treeTransforms.clear();
    if (!terrain) return;

    std::mt19937 rng(1337);
    std::bernoulli_distribution preferSouth(kSouthForestBias);
    std::bernoulli_distribution mountainChance(kMountainTreeBias);
    std::uniform_real_distribution<float> forestX(-kTerrainWidth * 0.4f, kTerrainWidth * 0.4f);
    std::uniform_real_distribution<float> generalX(-kTerrainWidth * 0.45f, kTerrainWidth * 0.45f);
    std::uniform_real_distribution<float> forestZ(kLakeCenterZ + 10.0f, kTerrainDepth * 0.5f);
    std::uniform_real_distribution<float> generalZ(-kTerrainDepth * 0.35f, kLakeCenterZ + 20.0f);
    std::uniform_real_distribution<float> mountainX(-kTerrainWidth * 0.35f, kTerrainWidth * 0.35f);
    std::uniform_real_distribution<float> mountainZ(kMountainStart - 55.0f, kMountainStart + 10.0f);
    std::uniform_real_distribution<float> scaleDist(0.65f, 1.45f);
    std::uniform_real_distribution<float> rotDist(0.0f, glm::two_pi<float>());

    treeTransforms.reserve(kTreeCount);

    int attempts = 0;
    const int maxAttempts = kTreeCount * 15;
    while (treeTransforms.size() < kTreeCount && attempts < maxAttempts) {
        attempts++;
        bool mountainBand = mountainChance(rng);
        bool southBand = !mountainBand && preferSouth(rng);
        
        float x = 0.0f, z = 0.0f;
        if (mountainBand) { x = mountainX(rng); z = mountainZ(rng); } 
        else if (southBand) { x = forestX(rng); z = forestZ(rng); } 
        else { x = generalX(rng); z = generalZ(rng); }

        if (!mountainBand && z < kMountainAvoidZ) continue;
        float distToLake = std::sqrt(x * x + std::pow(z - kLakeCenterZ, 2));
        if (distToLake < kLakeRadius + 6.0f) continue;
        if (nearRiver(x, z)) continue;
        if (z > kCornerPlainZ && std::abs(x) > kCornerPlainX) continue;

        float height = Terrain::getHeight(x, z);
        if (height < -2.0f) continue;
        if (mountainBand && height < 8.0f) continue;

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(x, height, z));
        model = glm::rotate(model, rotDist(rng), glm::vec3(0.0f, 1.0f, 0.0f));
        float scale = scaleDist(rng);
        model = glm::scale(model, glm::vec3(scale));

        treeTransforms.push_back(model);
    }
    std::cout << "Generated " << treeTransforms.size() << " trees." << std::endl;
}

// ------------------------------------------------------------
// Procedural Generation: Rocks
// ------------------------------------------------------------
void Scene::generateRocks() {
    rockTransforms.clear();
    if (!terrain) return;

    std::mt19937 rng(42);

    // Spread rocks across the entire map interior
    std::uniform_real_distribution<float> rockX(-kTerrainWidth * 0.45f, kTerrainWidth * 0.45f);
    std::uniform_real_distribution<float> rockZ(-kTerrainDepth * 0.45f, kTerrainDepth * 0.45f);

    // Bigger rocks so they are visible in your RTS view
    std::uniform_real_distribution<float> scaleDist(1.0f, 3.5f);
    std::uniform_real_distribution<float> rotDist(0.0f, glm::two_pi<float>());

    rockTransforms.reserve(kRockCount);

    int attempts = 0;
    const int maxAttempts = kRockCount * 80; // very generous

    while (rockTransforms.size() < kRockCount && attempts < maxAttempts) {
        attempts++;

        float x = rockX(rng);
        float z = rockZ(rng);

        // Terrain height at this location
        float height = Terrain::getHeight(x, z);

        // ---------- RULES FOR ROCK PLACEMENT ----------

        // 1. Avoid lake area
        float distToLake = std::sqrt(x*x + std::pow(z - kLakeCenterZ, 2));
        if (distToLake < kLakeRadius + 8.0f) continue;

        // 2. Avoid rivers
        if (nearRiver(x, z)) continue;

        // 3. Avoid underwater or deep trenches
        if (height < -1.0f) continue;

        // 4. Avoid extreme slopes (almost vertical)
        glm::vec3 normal = terrain->getNormal(x, z);
        float slope = glm::dot(normal, glm::vec3(0, 1, 0));
        if (slope < 0.25f) continue; // too steep

        // 5. Avoid bottom-most southern area (your base zones)
        if (z > 130.0f) continue;

        // -------------------------------------------------

        // Create rock transform matrix
        glm::mat4 model = glm::mat4(1.0f);

        // Slight lift to avoid Z-fighting or burial
        model = glm::translate(model, glm::vec3(x, height + 5.0f, z));

        // Random rotation
        model = glm::rotate(model, rotDist(rng), glm::vec3(0.0f, 1.0f, 0.0f));

        // Visible rock size
        float scale = scaleDist(rng);
        glm::vec3 nonUniform(scale, scale * 1.25f, scale);
        model = glm::scale(model, nonUniform);

        rockTransforms.push_back(model);
    }

    // Optional: force 1 debug rock dead center for sanity check
    {
        float x = 0.0f;
        float z = 0.0f;
        float h = Terrain::getHeight(x, z);
        glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(x, h + 1.0f, z));
        m = glm::scale(m, glm::vec3(8.0f));
        rockTransforms.push_back(m);
    }

    std::cout << "Generated " << rockTransforms.size() << " rocks." << std::endl;
}

void Scene::GenerateWaterGeometry() {
    // 128x128 grid for vertex waves
    int resolution = 128; 
    float size = 600.0f;  // Size of water plane
    
    // HEIGHT ADJUSTMENT: 
    // Your Terrain.cpp clamps rivers at -2.0f. 
    // We set water at -1.2f so it sits "inside" the river trench 
    // but not high enough to flood the plains.
    float y = -1.2f;      
    
    float uvScale = 20.0f;

    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    float step = (size * 2.0f) / resolution;

    for (int z = 0; z <= resolution; ++z) {
        for (int x = 0; x <= resolution; ++x) {
            float xPos = -size + x * step;
            float zPos = -size + z * step;
            
            vertices.push_back(xPos);
            vertices.push_back(y);
            vertices.push_back(zPos);

            float u = (float)x / resolution * uvScale;
            float v = (float)z / resolution * uvScale;
            vertices.push_back(u);
            vertices.push_back(v);
        }
    }

    for (int z = 0; z < resolution; ++z) {
        for (int x = 0; x < resolution; ++x) {
            int topLeft     = z * (resolution + 1) + x;
            int topRight    = topLeft + 1;
            int bottomLeft  = (z + 1) * (resolution + 1) + x;
            int bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    if (waterVAO) glDeleteVertexArrays(1, &waterVAO);
    if (waterVBO) glDeleteBuffers(1, &waterVBO);
    if (waterEBO) glDeleteBuffers(1, &waterEBO);

    glGenVertexArrays(1, &waterVAO);
    glGenBuffers(1, &waterVBO);
    glGenBuffers(1, &waterEBO);

    glBindVertexArray(waterVAO);

    glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, waterEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);
}

// Ensure DrawWater uses the new index count
void Scene::DrawWater(const glm::mat4& view, const glm::mat4& proj) {
    if (!waterShader || !waterTex || !waterVAO) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    waterShader->Use();
    waterShader->SetMat4("model", glm::mat4(1.0f));
    waterShader->SetMat4("view", view);
    waterShader->SetMat4("projection", proj);
    waterShader->SetFloat("time", static_cast<float>(glfwGetTime()));

    waterShader->SetInt("textureSampler", 0);
    waterShader->SetInt("noiseSampler",   1);
    waterShader->SetInt("overlaySampler", 2);

    waterTex->Bind(0);
    noiseTex->Bind(1);
    overlayTex->Bind(2);

    glBindVertexArray(waterVAO);
    // 128*128 quads * 6 indices
    glDrawElements(GL_TRIANGLES, 128 * 128 * 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
}