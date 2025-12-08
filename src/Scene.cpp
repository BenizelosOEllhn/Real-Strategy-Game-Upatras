#include "Scene.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <random>
#include <cmath>
#include <iostream>

#ifndef ASSET_PATH
#define ASSET_PATH "assets/"
#endif

namespace {
    constexpr int   kTerrainWidth   = 270;
    constexpr int   kTerrainDepth   = 300;
    constexpr int   kTreeCount      = 360;
    constexpr int   kRockCount      = 110;

    constexpr float kMountainStart      = -90.0f;
    constexpr float kMountainAvoidZ     = -70.0f;
    constexpr float kLakeCenterZ        = 80.0f;
    constexpr float kLakeRadius         = 55.0f;   
    constexpr float kRiverTreeBuffer    = 14.0f;
    constexpr float kSouthForestBias    = 0.75f;
    constexpr float kMountainTreeBias   = 0.2f;
    constexpr float kCornerPlainZ       = 135.0f;
    constexpr float kCornerPlainX       = 95.0f;

    constexpr float kRockZoneMinZ   = kMountainStart - 80.0f;
    constexpr float kRockZoneMaxZ   = -45.0f;
    constexpr float kRockMinHeight  = 5.0f;

    bool nearRiver(float x, float z)
    {
        if (!(z < kLakeCenterZ && z > kMountainStart - 20.0f))
            return false;

        float symX          = std::abs(x);
        float distFromLake  = kLakeCenterZ - z;
        float basePathX     = 25.0f + (distFromLake * 0.4f);
        float snakeWiggle   = std::sin(z * 0.08f) * 12.0f;
        float riverIdealX   = basePathX + snakeWiggle;
        float distToRiver   = std::abs(symX - riverIdealX);

        return distToRiver < kRiverTreeBuffer;
    }
}

// ------------------------------------------------------------

Scene::Scene()
    : terrain(nullptr)
    , treeModel(nullptr)
    , rockModel(nullptr)
    , grassTex(nullptr)
    , rockTex(nullptr)
    , peakTex(nullptr)
    , treeTex(nullptr)
    , boulderTex(nullptr)
{
}

Scene::~Scene()
{
    delete terrain;
    delete treeModel;
    delete rockModel;
    delete grassTex;
    delete rockTex;
    delete peakTex;
    delete boulderTex;
    delete treeTex;
}

void Scene::Init()
{
    // Terrain mesh
    terrain = new Terrain(kTerrainWidth, kTerrainDepth);

    const std::string base = ASSET_PATH;

    // Load textures and assets
    grassTex   = new Texture((base + "textures/grass.jpeg").c_str());
    rockTex    = new Texture((base + "textures/rock.jpeg").c_str());
    peakTex    = new Texture((base + "textures/peak.png").c_str());
    treeTex    = new Texture((base + "textures/leaf.png").c_str());
    boulderTex = new Texture((base + "textures/smallRockTexture.jpg").c_str());

    treeModel  = new Model((base + "models/tree.obj").c_str());
    rockModel  = new Model((base + "models/Rock.obj").c_str());

    generateTrees();
    generateRocks();
}

// ------------------------ DEPTH PASS ------------------------

void Scene::DrawDepth(Shader& depthShader, const glm::mat4& lightSpaceMatrix)
{
    if (!terrain) return;

    depthShader.Use();
    depthShader.SetMat4("lightSpaceMatrix", lightSpaceMatrix);

    // --- Terrain (non-instanced) ---
    depthShader.SetMat4("model", glm::mat4(1.0f));
    terrain->Draw(depthShader.ID);

    // --- Trees (instanced) ---
    if (treeModel && !treeTransforms.empty()) {
        // depth vertex shader must use instance mat4 at locations 3..6
        // and multiply it inside: lightSpaceMatrix * instanceMatrix * vec4(aPos,1)
        treeModel->DrawInstanced(depthShader, treeTransforms);
    }

    // --- Rocks (instanced) ---
    if (rockModel && !rockTransforms.empty()) {
        rockModel->DrawInstanced(depthShader, rockTransforms);
    }
}

// ------------------------ MAIN PASS ------------------------

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

    // --- Draw Terrain (Phong + PCF shadows) ---
    if (terrain && grassTex && rockTex && peakTex) {
        terrainShader.Use();

        terrainShader.SetMat4("view", view);
        terrainShader.SetMat4("projection", projection);
        terrainShader.SetMat4("model", glm::mat4(1.0f));
        terrainShader.SetMat4("lightSpaceMatrix", lightSpaceMatrix);

        grassTex->Bind(0);
        rockTex->Bind(1);
        peakTex->Bind(2);

        glUniform1i(glGetUniformLocation(terrainShader.ID, "textureGrass"), 0);
        glUniform1i(glGetUniformLocation(terrainShader.ID, "textureRock"), 1);
        glUniform1i(glGetUniformLocation(terrainShader.ID, "texturePeak"), 2);

        // Shadow map on texture unit 3
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, shadowMap);
        glUniform1i(glGetUniformLocation(terrainShader.ID, "shadowMap"), 3);

        glUniform3fv(glGetUniformLocation(terrainShader.ID, "lightPos"), 1, &lightPos[0]);
        glUniform3fv(glGetUniformLocation(terrainShader.ID, "viewPos"), 1, &viewPos[0]);
        glUniform3fv(glGetUniformLocation(terrainShader.ID, "lightColor"), 1, &lightColor[0]);
        glUniform2f(glGetUniformLocation(terrainShader.ID, "peakHeightRange"), 18.0f, 27.0f);

        terrain->Draw(terrainShader.ID);
    }

    bool hasTreeMeshes = treeModel && treeTex && !treeTransforms.empty();
    bool hasRockMeshes = rockModel && boulderTex && !rockTransforms.empty();

    if (hasTreeMeshes || hasRockMeshes) {
        objectShader.Use();
        objectShader.SetMat4("view", view);
        objectShader.SetMat4("projection", projection);

        // model = identity because per-instance transform is in instance matrix attribute
        objectShader.SetMat4("model", glm::mat4(1.0f));
        objectShader.SetMat4("lightSpaceMatrix", lightSpaceMatrix);

        glUniform3fv(glGetUniformLocation(objectShader.ID, "lightPos"), 1, &lightPos[0]);
        glUniform3fv(glGetUniformLocation(objectShader.ID, "viewPos"), 1, &viewPos[0]);
        glUniform3fv(glGetUniformLocation(objectShader.ID, "lightColor"), 1, &lightColor[0]);

        // Shadow map on texture unit 3, same as terrain
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, shadowMap);
        glUniform1i(glGetUniformLocation(objectShader.ID, "shadowMap"), 3);
    }

    // --- Trees (INSTANCED) ---
    if (hasTreeMeshes) {
        treeTex->Bind(0);
        glUniform1i(glGetUniformLocation(objectShader.ID, "texture_diffuse1"), 0);

        treeModel->DrawInstanced(objectShader, treeTransforms);
    }

    // --- Rocks (INSTANCED) ---
    if (hasRockMeshes) {
        boulderTex->Bind(0);
        glUniform1i(glGetUniformLocation(objectShader.ID, "texture_diffuse1"), 0);

        rockModel->DrawInstanced(objectShader, rockTransforms);
    }
}

// ------------------------ TREE & ROCK GENERATION ------------------------

void Scene::generateTrees()
{
    treeTransforms.clear();
    if (!terrain) return;

    std::mt19937 rng(1337);

    std::bernoulli_distribution preferSouth(kSouthForestBias);
    std::bernoulli_distribution mountainChance(kMountainTreeBias);

    std::uniform_real_distribution<float> forestX(-kTerrainWidth * 0.4f,  kTerrainWidth * 0.4f);
    std::uniform_real_distribution<float> generalX(-kTerrainWidth * 0.45f, kTerrainWidth * 0.45f);

    std::uniform_real_distribution<float> forestZ(kLakeCenterZ + 10.0f,      kTerrainDepth * 0.5f);
    std::uniform_real_distribution<float> generalZ(-kTerrainDepth * 0.35f,   kLakeCenterZ + 20.0f);

    std::uniform_real_distribution<float> mountainX(-kTerrainWidth * 0.35f,  kTerrainWidth * 0.35f);
    std::uniform_real_distribution<float> mountainZ(kMountainStart - 55.0f,  kMountainStart + 10.0f);

    std::uniform_real_distribution<float> scaleDist(0.65f, 1.45f);
    std::uniform_real_distribution<float> rotDist(0.0f, glm::two_pi<float>());

    treeTransforms.reserve(kTreeCount);

    int attempts = 0;
    const int maxAttempts = kTreeCount * 15;

    while (treeTransforms.size() < kTreeCount && attempts < maxAttempts) {
        attempts++;

        bool mountainBand = mountainChance(rng);
        bool southBand    = !mountainBand && preferSouth(rng);

        float x = 0.0f;
        float z = 0.0f;

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

void Scene::generateRocks()
{
    rockTransforms.clear();
    if (!terrain) return;

    std::mt19937 rng(2025);

    std::uniform_real_distribution<float> rockX(-kTerrainWidth * 0.45f, kTerrainWidth * 0.45f);
    std::uniform_real_distribution<float> rockZ(kRockZoneMinZ, kRockZoneMaxZ);
    std::uniform_real_distribution<float> scaleDist(0.65f, 1.85f);
    std::uniform_real_distribution<float> rotDist(0.0f, glm::two_pi<float>());

    rockTransforms.reserve(kRockCount);

    int attempts = 0;
    const int maxAttempts = kRockCount * 18;

    while (rockTransforms.size() < kRockCount && attempts < maxAttempts) {
        attempts++;

        float x = rockX(rng);
        float z = rockZ(rng);

        if (nearRiver(x, z)) continue;

        float height = Terrain::getHeight(x, z);
        if (z > -45.0f || height < kRockMinHeight) continue;

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(x, height, z));
        model = glm::rotate(model, rotDist(rng), glm::vec3(0.0f, 1.0f, 0.0f));

        float scale = scaleDist(rng);
        glm::vec3 nonUniform(scale, scale * 1.35f, scale);
        model = glm::scale(model, nonUniform);

        rockTransforms.push_back(model);
    }

    std::cout << "Generated " << rockTransforms.size() << " rocks." << std::endl;
}
