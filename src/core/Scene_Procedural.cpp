#include "Scene.h"
#include "SceneConstants.h"

// ------------------------------------------------------------
// Procedural Generation: Trees
// ------------------------------------------------------------
void Scene::generateTrees() {
    treeTransforms.clear();
    treePositions_.clear();
    if (!terrain) return;

    std::mt19937 rng(1337);
    std::bernoulli_distribution preferSouth(SceneConst::kSouthForestBias);
    std::bernoulli_distribution mountainChance(SceneConst::kMountainTreeBias);
    std::uniform_real_distribution<float> forestX(SceneConst::kTerrainWidth * -0.4f,  SceneConst::kTerrainWidth * 0.4f);
    std::uniform_real_distribution<float> generalX(SceneConst::kTerrainWidth * -0.45f, SceneConst::kTerrainWidth * 0.45f);
    std::uniform_real_distribution<float> forestZ(60.0f + 10.0f,  SceneConst::kTerrainDepth * 0.5f);
    std::uniform_real_distribution<float> generalZ(SceneConst::kTerrainDepth * -0.35f,  60.0f + 20.0f);
    std::uniform_real_distribution<float> mountainX(SceneConst::kTerrainWidth * -0.35f, SceneConst::kTerrainWidth * 0.35f);
    std::uniform_real_distribution<float> mountainZ(SceneConst::kMountainStart - 55.0f, SceneConst::kMountainStart + 10.0f);
    std::uniform_real_distribution<float> scaleDist(0.65f, 1.45f);
    std::uniform_real_distribution<float> rotDist(0.0f, glm::two_pi<float>());

    treeTransforms.reserve(SceneConst::kTreeCount);

    int attempts = 0;
    const int maxAttempts = SceneConst::kTreeCount * 15;
    while (treeTransforms.size() < SceneConst::kTreeCount && attempts < maxAttempts) {
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

        if (!mountainBand && z < SceneConst::kMountainAvoidZ) continue;

        // Avoid lake (use real lake pos)
        float distToLake = std::sqrt(x * x + std::pow(z - SceneConst::kLakeCenterZ, 2));
        if (distToLake < SceneConst::kLakeRadius + 6.0f) continue;

        // Avoid rivers (approx)
        if (Scene::nearRiver(x, z)) continue;

        if (z > SceneConst::kCornerPlainZ && std::abs(x) > SceneConst::kCornerPlainX) continue;

        float height = Terrain::getHeight(x, z);
        if (height < 1.0f) continue;
        if (mountainBand && height < 8.0f) continue;

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(x, height, z));
        model = glm::rotate(model, rotDist(rng), glm::vec3(0.0f, 1.0f, 0.0f));
        float scale = scaleDist(rng) * 1.25f;
        model = glm::scale(model, glm::vec3(scale));

        treeTransforms.push_back(model);
        treePositions_.push_back(glm::vec3(x, height, z));
    }
    std::cout << "Generated " << treeTransforms.size() << " trees." << std::endl;
}

// ------------------------------------------------------------
// Procedural Generation: Rocks
// ------------------------------------------------------------
void Scene::generateRocks() {
    rockTransforms.clear();
    rockPositions_.clear();
    if (!terrain) return;

    std::mt19937 rng(42);

    std::uniform_real_distribution<float> rockX(SceneConst::kTerrainWidth * -0.45f, SceneConst::kTerrainWidth * 0.45f);
    std::uniform_real_distribution<float> rockZ(SceneConst::kTerrainDepth * -0.45f, SceneConst::kTerrainDepth * 0.45f);

    std::uniform_real_distribution<float> scaleDist(1.0f, 3.5f);
    std::uniform_real_distribution<float> rotDist(0.0f, glm::two_pi<float>());

    rockTransforms.reserve(SceneConst::kRockCount);

    int attempts = 0;
    const int maxAttempts = SceneConst::kRockCount * 80;

    while (rockTransforms.size() < SceneConst::kRockCount && attempts < maxAttempts) {
        attempts++;

        float x = rockX(rng);
        float z = rockZ(rng);

        float height = Terrain::getHeight(x, z);

        float distToLake = std::sqrt(x*x + std::pow(z - SceneConst::kLakeCenterZ, 2));
        if (distToLake < SceneConst::kLakeRadius + 8.0f) continue;

        if (Scene::nearRiver(x, z)) continue;
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
        rockPositions_.push_back(glm::vec3(x, height + 5.0f, z));
    }

    // Debug rock in center
    {
        float x = 0.0f;
        float z = 0.0f;
        float h = Terrain::getHeight(x, z);
        glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(x, h + 1.0f, z));
        m = glm::scale(m, glm::vec3(8.0f));
        rockTransforms.push_back(m);
        rockPositions_.push_back(glm::vec3(x, h + 1.0f, z));
    }

    std::cout << "Generated " << rockTransforms.size() << " rocks." << std::endl;
}
