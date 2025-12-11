#include "Scene.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <cmath>
#include <random>
#include <iostream>
#include <string>

#ifndef ASSET_PATH
#define ASSET_PATH "assets/"
#endif

// ------------------------------------------------------------
// Terrain & Generation Constants
// ------------------------------------------------------------
namespace {
    constexpr int   kTerrainWidth     = 600;
    constexpr int   kTerrainDepth     = 600;
    constexpr int   kTreeCount        = 360;
    constexpr int   kRockCount        = 110;

    constexpr float kMountainStart    = -90.0f;
    constexpr float kMountainAvoidZ   = -70.0f;

    // NOTE: these should match your Terrain lake
    constexpr float kLakeCenterZ      = -200.0f;
    constexpr float kLakeRadius       = 60.0f;

    constexpr float kRiverTreeBuffer  = 14.0f;
    constexpr float kSouthForestBias  = 0.75f;
    constexpr float kMountainTreeBias = 0.2f;
    constexpr float kCornerPlainZ     = 135.0f;
    constexpr float kCornerPlainX     = 95.0f;

    constexpr float kRockZoneMinZ     = kMountainStart - 80.0f;
    constexpr float kRockZoneMaxZ     = -45.0f;
    constexpr float kRockMinHeight    = 5.0f;

    // Helper to check if a point is too close to the river
// Same path & width as Terrain::carveRiver(), but a bit wider for safety.
bool nearRiver(float x, float z)
{
    // Must match Terrain constants
    const float lakeZ      = -200.0f;
    const float riverStartZ = lakeZ + 12.0f;   // -188
    const float riverEndZ   = 260.0f;

    if (z < riverStartZ || z > riverEndZ)
        return false;

    auto evalPath = [&](float startX, float dir) -> std::pair<float,float>
    {
        float t = (z - riverStartZ) / (riverEndZ - riverStartZ);

        float endX  = dir * 180.0f;
        float pathX = glm::mix(startX, endX, t);

        pathX += dir * (30.0f * std::sin(z * 0.035f));
        pathX +=        (12.0f * std::cos(z * 0.02f));

        float halfWidth = glm::mix(28.0f, 18.0f, t);
        float fade = glm::clamp((z - 150.0f) / 30.0f, 0.0f, 1.0f);
        halfWidth *= (1.0f - fade);

        // slightly padded for foliage rejection
        halfWidth += 4.0f;

        return { pathX - halfWidth, pathX + halfWidth };
    };

    // left & right rivers
    auto [lMin, lMax] = evalPath(-15.0f, -1.0f);
    auto [rMin, rMax] = evalPath(+15.0f, +1.0f);

    if (x >= lMin && x <= lMax) return true;
    if (x >= rMin && x <= rMax) return true;

    return false;
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
      sandTex(nullptr),
      treeTex(nullptr), peakTex(nullptr), boulderTex(nullptr),
      waterTex(nullptr), noiseTex(nullptr), overlayTex(nullptr),
      waterShader(nullptr),
      waterVAO(0), waterVBO(0), waterEBO(0),
      lakeVAO(0), lakeVBO(0), lakeEBO(0),
      riverVAO(0), riverVBO(0), riverEBO(0)
{
}

Scene::~Scene() {
    // Cleanup Models & Terrain
    delete terrain;
    delete treeModel;
    delete rockModel;

    // Cleanup Textures
    delete rockTex;
    delete sandTex;
    delete boulderTex;
    delete treeTex;
    delete peakTex;
    delete waterTex;
    delete noiseTex;
    delete overlayTex;
    delete grass1Tex;
    delete grass2Tex;
    delete grass3Tex;

    // Cleanup Water Shader & Buffers
    delete waterShader;

    if (waterVAO) glDeleteVertexArrays(1, &waterVAO);
    if (waterVBO) glDeleteBuffers(1, &waterVBO);
    if (waterEBO) glDeleteBuffers(1, &waterEBO);

    if (lakeVAO) glDeleteVertexArrays(1, &lakeVAO);
    if (lakeVBO) glDeleteBuffers(1, &lakeVBO);
    if (lakeEBO) glDeleteBuffers(1, &lakeEBO);

    if (riverVAO) glDeleteVertexArrays(1, &riverVAO);
    if (riverVBO) glDeleteBuffers(1, &riverVBO);
    if (riverEBO) glDeleteBuffers(1, &riverEBO);
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
    rockTex     = new Texture((base + "textures/smallRockTexture.jpg").c_str());
    sandTex     = new Texture((base + "textures/sand1.jpg").c_str());
    treeTex     = new Texture((base + "textures/leaf.png").c_str());
    peakTex     = new Texture((base + "textures/peak.jpeg").c_str());
    boulderTex  = new Texture((base + "textures/smallRockTexture.jpg").c_str());
    waterTex    = new Texture((base + "textures/water.jpeg").c_str());
    noiseTex    = new Texture((base + "textures/perlin.png").c_str());
    overlayTex  = new Texture((base + "textures/overlay.png").c_str());

    // 3. Load Models
    treeModel   = new Model((base + "models/tree.obj").c_str());
    rockModel   = new Model((base + "models/Rock.obj").c_str());

    // 4. Load Water Shader
    waterShader = new Shader(base + "shaders/water.vert", base + "shaders/water.frag");

    // 5. Generate Procedural Content
    GenerateWaterGeometry(); // big ocean plane
    generateTrees();
    generateRocks();
    generateLakeWater();     // local lake mesh
    generateRiverWater();    // local river mesh
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
        sandTex->Bind(5);
        peakTex->Bind(6);

        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, shadowMap);

        terrainShader.SetInt("grass1",       0);
        terrainShader.SetInt("grass2",       1);
        terrainShader.SetInt("grass3",       2);
        terrainShader.SetInt("noiseDetail",  3);
        terrainShader.SetInt("textureRock",  4);
        terrainShader.SetInt("sandTex",      5);
        terrainShader.SetInt("texturePeak",  6);
        terrainShader.SetInt("shadowMap",    7);

        terrainShader.SetVec3("lightPos",  lightPos);
        terrainShader.SetVec3("viewPos",   viewPos);
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

        objectShader.SetVec3("lightPos",  lightPos);
        objectShader.SetVec3("viewPos",   viewPos);
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
    // Big ocean plane
    DrawWater(view, projection);
    // Local meshes (lake & rivers)
    DrawLakeWater(view, projection);
    DrawRiverWater(view, projection);
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
    std::uniform_real_distribution<float> forestX(-kTerrainWidth * 0.4f,  kTerrainWidth * 0.4f);
    std::uniform_real_distribution<float> generalX(-kTerrainWidth * 0.45f, kTerrainWidth * 0.45f);
    std::uniform_real_distribution<float> forestZ(60.0f + 10.0f,            kTerrainDepth * 0.5f);
    std::uniform_real_distribution<float> generalZ(-kTerrainDepth * 0.35f,  60.0f + 20.0f);
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
        if (mountainBand) {
            x = mountainX(rng);
            z = mountainZ(rng);
        } else if (southBand) {
            x = forestX(rng);
            z = forestZ(rng);
        } else {
            x = generalX(rng);
            z = generalZ(rng);
        }

        if (!mountainBand && z < kMountainAvoidZ) continue;

        // Avoid lake (use real lake pos)
        float distToLake = std::sqrt(x * x + std::pow(z - kLakeCenterZ, 2));
        if (distToLake < kLakeRadius + 6.0f) continue;

        // Avoid rivers (approx)
        if (nearRiver(x, z)) continue;

        if (z > kCornerPlainZ && std::abs(x) > kCornerPlainX) continue;

        float height = Terrain::getHeight(x, z);
        if (height < 1.0f) continue;
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

    std::uniform_real_distribution<float> rockX(-kTerrainWidth * 0.45f, kTerrainWidth * 0.45f);
    std::uniform_real_distribution<float> rockZ(-kTerrainDepth * 0.45f, kTerrainDepth * 0.45f);

    std::uniform_real_distribution<float> scaleDist(1.0f, 3.5f);
    std::uniform_real_distribution<float> rotDist(0.0f, glm::two_pi<float>());

    rockTransforms.reserve(kRockCount);

    int attempts = 0;
    const int maxAttempts = kRockCount * 80;

    while (rockTransforms.size() < kRockCount && attempts < maxAttempts) {
        attempts++;

        float x = rockX(rng);
        float z = rockZ(rng);

        float height = Terrain::getHeight(x, z);

        float distToLake = std::sqrt(x*x + std::pow(z - kLakeCenterZ, 2));
        if (distToLake < kLakeRadius + 8.0f) continue;

        if (nearRiver(x, z)) continue;
        if (height < 1.0f) continue;

        glm::vec3 normal = terrain->getNormal(x, z);
        float slope = glm::dot(normal, glm::vec3(0, 1, 0));
        if (slope < 0.25f) continue;

        if (z > 130.0f) continue;

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(x, height + 5.0f, z));
        model = glm::rotate(model, rotDist(rng), glm::vec3(0.0f, 1.0f, 0.0f));

        float scale = scaleDist(rng);
        glm::vec3 nonUniform(scale, scale * 1.25f, scale);
        model = glm::scale(model, nonUniform);

        rockTransforms.push_back(model);
    }

    // Debug rock in center
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

// ------------------------------------------------------------
// BIG OCEAN WATER PLANE
// ------------------------------------------------------------
void Scene::GenerateWaterGeometry() {
    int resolution = 128;
    float size = 600.0f;
    float y = -1.2f;       // global water height
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
    glDrawElements(GL_TRIANGLES, 128 * 128 * 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
}

// ------------------------------------------------------------
// LAKE WATER MESH
// ------------------------------------------------------------
void Scene::generateLakeWater()
{
    // Must match Terrain constants
    const float lakeX       = 0.0f;
    const float lakeZ       = -200.0f;
    const float lakeOuterR  = 90.0f;
    const float lakeInnerR  = 55.0f;

    // Pick whatever you like visually; just keep it <= rim height
    const float waterY      = 4.5f;   

    const int segments = 96;

    lakeWaterVerts.clear();
    lakeWaterIndices.clear();

    // Center vertex
    lakeWaterVerts.push_back({
        glm::vec3(lakeX, waterY, lakeZ),
        glm::vec2(0.5f, 0.5f)
    });

    for (int i = 0; i <= segments; ++i)
    {
        float a = (float)i / segments * 2.0f * glm::pi<float>();

        // Same irregular function as Terrain
        float irregular =
              1.0f
            + 0.18f * std::cos(3.0f * a)
            + 0.08f * std::cos(5.0f * a);

        float innerR = lakeInnerR  * irregular;
        float outerR = lakeOuterR  * irregular;

        // Water radius: safely between inner & outer rim
        float waterR = innerR + (outerR - innerR) * 0.55f;

        float x = lakeX + std::cos(a) * waterR;
        float z = lakeZ + std::sin(a) * waterR;

        lakeWaterVerts.push_back({
            glm::vec3(x, waterY, z),
            glm::vec2((std::cos(a) + 1.0f) * 0.5f,
                      (std::sin(a) + 1.0f) * 0.5f)
        });
    }

    // Fan indices
    for (int i = 1; i <= segments; ++i)
    {
        lakeWaterIndices.push_back(0);
        lakeWaterIndices.push_back(i);
        lakeWaterIndices.push_back(i + 1);
    }

    uploadLakeWaterMesh();
}

void Scene::uploadLakeWaterMesh()
{
    if (lakeVAO) glDeleteVertexArrays(1, &lakeVAO);
    if (lakeVBO) glDeleteBuffers(1, &lakeVBO);
    if (lakeEBO) glDeleteBuffers(1, &lakeEBO);

    glGenVertexArrays(1, &lakeVAO);
    glGenBuffers(1, &lakeVBO);
    glGenBuffers(1, &lakeEBO);

    glBindVertexArray(lakeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, lakeVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 lakeWaterVerts.size() * sizeof(WaterVertex),
                 lakeWaterVerts.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lakeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 lakeWaterIndices.size() * sizeof(unsigned int),
                 lakeWaterIndices.data(),
                 GL_STATIC_DRAW);

    // layout: 0 = position, 1 = uv
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          sizeof(WaterVertex),
                          (void*)offsetof(WaterVertex, position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                          sizeof(WaterVertex),
                          (void*)offsetof(WaterVertex, uv));

    glBindVertexArray(0);
}

void Scene::DrawLakeWater(const glm::mat4& view, const glm::mat4& proj)
{
    if (!waterShader || !waterTex || !lakeVAO || lakeWaterIndices.empty())
        return;

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

    glBindVertexArray(lakeVAO);
    glDrawElements(GL_TRIANGLES,
                   static_cast<GLsizei>(lakeWaterIndices.size()),
                   GL_UNSIGNED_INT,
                   0);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
}

// ------------------------------------------------------------
// RIVER WATER MESH
// ------------------------------------------------------------
void Scene::generateRiverWater()
{
    // Must match Terrain
    const float lakeZ       = -200.0f;
    const float riverStartZ = lakeZ + 12.0f;   // -188
    const float riverEndZ   = 260.0f;

    // Where Terrain's halfWidth has fully faded to 0 (see carveRiver)
    const float visualEndZ  = 180.0f;          // stop mesh here

    const float waterY = 1.5f;   // your chosen river surface height
    const float step   = 3.0f;   // smaller step = smoother mesh

    riverWaterVerts.clear();
    riverWaterIndices.clear();

    auto evalSlice = [&](float z, float startX, float dir,
                         float& outLeft, float& outRight) -> bool
    {
        if (z < riverStartZ || z > riverEndZ)
            return false;

        float t = (z - riverStartZ) / (riverEndZ - riverStartZ);

        float endX  = dir * 180.0f;
        float pathX = glm::mix(startX, endX, t);

        pathX += dir * (30.0f * std::sin(z * 0.035f));
        pathX +=        (12.0f * std::cos(z * 0.02f));

        float halfWidth = glm::mix(28.0f, 18.0f, t);
        float fade = glm::clamp((z - 150.0f) / 30.0f, 0.0f, 1.0f);
        halfWidth *= (1.0f - fade);

        if (halfWidth <= 0.1f)
            return false; // channel basically gone

        outLeft  = pathX - halfWidth;
        outRight = pathX + halfWidth;
        return true;
    };

    // Start a little below the lake to avoid the nasty fan at the join
    const float meshStartZ = riverStartZ + 18.0f;

    for (float z = meshStartZ; z <= visualEndZ; z += step)
    {
        float lLeft, lRight, rLeft, rRight;
        bool okL = evalSlice(z, -15.0f, -1.0f, lLeft, lRight);
        bool okR = evalSlice(z, +15.0f, +1.0f, rLeft, rRight);

        // If either river vanished here, skip this slice
        if (!okL && !okR) continue;

        // Store 4 verts per slice (left inner/outer, right inner/outer)
        WaterVertex v0 { glm::vec3(lLeft,  waterY, z), glm::vec2(0.0f, 0.0f) };
        WaterVertex v1 { glm::vec3(lRight, waterY, z), glm::vec2(1.0f, 0.0f) };
        WaterVertex v2 { glm::vec3(rLeft,  waterY, z), glm::vec2(0.0f, 1.0f) };
        WaterVertex v3 { glm::vec3(rRight, waterY, z), glm::vec2(1.0f, 1.0f) };

        riverWaterVerts.push_back(v0);
        riverWaterVerts.push_back(v1);
        riverWaterVerts.push_back(v2);
        riverWaterVerts.push_back(v3);
    }

    // Build indices (two strips, just like before)
    int count = static_cast<int>(riverWaterVerts.size());
    for (int i = 0; i < count - 4; i += 4)
    {
        // left strip
        riverWaterIndices.push_back(i);
        riverWaterIndices.push_back(i + 1);
        riverWaterIndices.push_back(i + 4);

        riverWaterIndices.push_back(i + 1);
        riverWaterIndices.push_back(i + 5);
        riverWaterIndices.push_back(i + 4);

        // right strip
        riverWaterIndices.push_back(i + 2);
        riverWaterIndices.push_back(i + 3);
        riverWaterIndices.push_back(i + 6);

        riverWaterIndices.push_back(i + 3);
        riverWaterIndices.push_back(i + 7);
        riverWaterIndices.push_back(i + 6);
    }

    uploadRiverWaterMesh();
}



void Scene::uploadRiverWaterMesh()
{
    if (riverVAO) glDeleteVertexArrays(1, &riverVAO);
    if (riverVBO) glDeleteBuffers(1, &riverVBO);
    if (riverEBO) glDeleteBuffers(1, &riverEBO);

    glGenVertexArrays(1, &riverVAO);
    glGenBuffers(1, &riverVBO);
    glGenBuffers(1, &riverEBO);

    glBindVertexArray(riverVAO);

    glBindBuffer(GL_ARRAY_BUFFER, riverVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 riverWaterVerts.size() * sizeof(WaterVertex),
                 riverWaterVerts.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, riverEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 riverWaterIndices.size() * sizeof(unsigned int),
                 riverWaterIndices.data(),
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          sizeof(WaterVertex),
                          (void*)offsetof(WaterVertex, position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
                          sizeof(WaterVertex),
                          (void*)offsetof(WaterVertex, uv));

    glBindVertexArray(0);
}

void Scene::DrawRiverWater(const glm::mat4& view, const glm::mat4& proj)
{
    if (!waterShader || !waterTex || !riverVAO || riverWaterIndices.empty())
        return;

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

    glBindVertexArray(riverVAO);
    glDrawElements(GL_TRIANGLES,
                   static_cast<GLsizei>(riverWaterIndices.size()),
                   GL_UNSIGNED_INT,
                   0);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
}

void Scene::Update(float dt)
{
    uiManager.update(mouseX, mouseY);
    buildingManager.update(dt, mouseX, mouseY);
    entityManager.update(dt);
}

void Scene::Render()
{
    terrain.render();
    entityManager.render();
    buildingManager.renderGhost(camera);
    uiManager.render();
}
