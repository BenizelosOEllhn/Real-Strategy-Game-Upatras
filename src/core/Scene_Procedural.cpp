#include "Scene.h"

namespace
{
    // ------------------------------------------------------------
    // Vegetation Bias
    // ------------------------------------------------------------
    constexpr float kSouthForestBias   = 0.75f; // more trees in south
    constexpr float kMountainTreeBias  = 0.35f; // fewer trees at altitude

    // ------------------------------------------------------------
    // Plain / corner flattening
    // ------------------------------------------------------------
    constexpr float kCornerPlainX = 120.0f;
    constexpr float kCornerPlainZ = -120.0f;
}


bool Scene::nearRiver(float x, float z) 
{
    // Must match Terrain river parameters
    const float lakeZ       = kLakeCenterZ;
    const float riverStartZ = lakeZ + 12.0f;
    const float riverEndZ   = 260.0f;

    if (z < riverStartZ || z > riverEndZ)
        return false;

    auto evalPath = [&](float startX, float dir,
                        float& outMin, float& outMax)
    {
        float t = (z - riverStartZ) / (riverEndZ - riverStartZ);

        float endX  = dir * 180.0f;
        float pathX = glm::mix(startX, endX, t);

        pathX += dir * (30.0f * std::sin(z * 0.035f));
        pathX +=        (12.0f * std::cos(z * 0.02f));

        float halfWidth = glm::mix(28.0f, 18.0f, t);
        float fade = glm::clamp((z - 150.0f) / 30.0f, 0.0f, 1.0f);
        halfWidth *= (1.0f - fade);

        // Slight padding so foliage doesn’t clip water
        halfWidth += 4.0f;

        outMin = pathX - halfWidth;
        outMax = pathX + halfWidth;
    };

    float lMin, lMax, rMin, rMax;
    evalPath(-15.0f, -1.0f, lMin, lMax);
    evalPath( 15.0f,  1.0f, rMin, rMax);

    if (x >= lMin && x <= lMax) return true;
    if (x >= rMin && x <= rMax) return true;

    return false;
}

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