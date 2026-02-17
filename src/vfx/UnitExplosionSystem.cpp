#include "UnitExplosionSystem.h"
#include "../rendering/terrain/Terrain.h"
#include "../core/Scene.h"
#include "../core/AssetPath.h"
#include "../../common/Shader.h"
#include "../../common/Model.h"
#include "../../common/Texture.h"
#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <random>
#include <GLFW/glfw3.h>

namespace
{
float Saturate(float v)
{
    return std::clamp(v, 0.0f, 1.0f);
}

float Smoothstep(float edge0, float edge1, float x)
{
    if (edge0 == edge1)
        return x < edge0 ? 0.0f : 1.0f;
    float t = Saturate((x - edge0) / (edge1 - edge0));
    return t * t * (3.0f - 2.0f * t);
}

std::string ResolveScorchedEarthPath()
{
    namespace fs = std::filesystem;
    std::vector<fs::path> candidates = {
        fs::path(AssetPath("textures/scorchedearth.png")),
        fs::path("assets/textures/scorchedearth.png"),
        fs::path("../assets/textures/scorchedearth.png"),
        fs::current_path() / "assets/textures/scorchedearth.png",
        fs::current_path() / "../assets/textures/scorchedearth.png"
    };

    fs::path cur = fs::current_path();
    for (int i = 0; i < 8; ++i)
    {
        candidates.push_back(cur / "assets/textures/scorchedearth.png");
        fs::path parent = cur.parent_path();
        if (parent.empty() || parent == cur)
            break;
        cur = parent;
    }

    for (const fs::path& p : candidates)
    {
        std::error_code ec;
        if (fs::exists(p, ec))
            return fs::weakly_canonical(p, ec).string();
    }

    return candidates.front().string();
}
}

UnitExplosionSystem::UnitExplosionSystem()
{
}

UnitExplosionSystem::~UnitExplosionSystem()
{
    if (particleVAO_) glDeleteVertexArrays(1, &particleVAO_);
    if (particleVBO_) glDeleteBuffers(1, &particleVBO_);
    if (particleEBO_) glDeleteBuffers(1, &particleEBO_);
    if (craterMapTexture_) glDeleteTextures(1, &craterMapTexture_);
}

void UnitExplosionSystem::Init(Terrain* terrain, Scene* scene)
{
    terrain_ = terrain;
    scene_ = scene;

    initializeCraterMap();

    const std::string scorchedPath = ResolveScorchedEarthPath();
    const bool scorchExists = std::filesystem::exists(scorchedPath);
    std::cout << "[UnitExplosionSystem] Scorched texture path: " << scorchedPath
              << " exists=" << (scorchExists ? "yes" : "no") << "\n";

    try {
        scorchedEarthTex_ = std::make_unique<Texture>(scorchedPath.c_str());
        if (scorchedEarthTex_ && scorchedEarthTex_->LoadedFromFile())
        {
            std::cout << "[UnitExplosionSystem] Loaded scorched earth texture ("
                      << scorchedEarthTex_->Width() << "x" << scorchedEarthTex_->Height()
                      << ", channels=" << scorchedEarthTex_->Channels() << ")\n";
        }
        else
        {
            std::cout << "[UnitExplosionSystem] WARNING: scorched earth texture fallback in use.\n";
        }
    } catch (const std::exception& e) {
        std::cout << "[UnitExplosionSystem] Failed to load scorched earth texture: " << e.what() << "\n";
    }
    
    initializeParticleMesh();
}

void UnitExplosionSystem::Update(float dt, const glm::vec3& cameraPos)
{
    (void)cameraPos;

    // Process any queued explosions
    processExplosionQueue();

    // Update particles
    updateParticles(dt);

    // Update explosion lights
    updateLights(dt);

    // Decay camera shake
    cameraSakeMagnitude_ = std::max(0.0f, cameraSakeMagnitude_ - dt * 5.0f);
}

void UnitExplosionSystem::Render(Shader& particleShader, const glm::mat4& view, const glm::mat4& projection)
{
    if (particles_.empty())
        return;

    renderParticles(particleShader, view, projection);
}

void UnitExplosionSystem::TriggerExplosion(const glm::vec3& epicenter, float radius, float depth, float force)
{
    ExplosionEvent evt;
    evt.epicenter = epicenter;
    evt.radius = radius;
    evt.depth = depth;
    evt.force = force;
    evt.timestamp = 0.0; // Will be used for VFX sequencing
    explosionQueue_.push(evt);
}

void UnitExplosionSystem::processExplosionQueue()
{
    int processedThisFrame = 0;
    constexpr int kMaxExplosionsPerFrame = 2;
    while (!explosionQueue_.empty() && processedThisFrame < kMaxExplosionsPerFrame)
    {
        const ExplosionEvent& evt = explosionQueue_.front();

        applyCrater(evt.epicenter, evt.radius, evt.depth);
        spawnExplosionVFX(evt.epicenter, evt.radius, evt.depth, evt.force);
        updateNavGridAfterDeformation(evt.epicenter, evt.radius);

        explosionQueue_.pop();
        ++processedThisFrame;
    }
}

void UnitExplosionSystem::applyCrater(const glm::vec3& center, float radius, float depth)
{
    CraterInfo crater;
    crater.center = center;
    crater.radius = radius;
    crater.depth = depth;
    crater.creationTime = static_cast<float>(glfwGetTime());
    craters_.push_back(crater);

    if (terrain_)
    {
        deformTerrainVertices(center, radius, depth);
        updateTerrainTexture(center, radius);
    }
}

void UnitExplosionSystem::deformTerrainVertices(const glm::vec3& center, float radius, float depth)
{
    if (!terrain_) {
        std::cerr << "[UnitExplosionSystem] ERROR: Terrain pointer is null\n";
        return;
    }

    const float BEDROCK_LEVEL = -14.0f;
    const float radiusSq = radius * radius;

    auto& vertices = terrain_->GetVertices();

    for (auto& vertex : vertices) {
        float dx = vertex.Position.x - center.x;
        float dz = vertex.Position.z - center.z;
        float distSq = dx * dx + dz * dz;

        if (distSq <= radiusSq) {
            float falloff = 1.0f - (distSq / radiusSq);
            falloff = falloff * falloff;

            vertex.Position.y -= depth * falloff;

            if (vertex.Position.y < BEDROCK_LEVEL) {
                vertex.Position.y = BEDROCK_LEVEL;
            }
        }
    }

    terrain_->RecalculateNormalsInRadius(center, radius);
    terrain_->SyncVerticesToGPU();
}

void UnitExplosionSystem::updateTerrainTexture(const glm::vec3& center, float radius)
{
    if (!terrain_ || craterMapTexture_ == 0 || craterMapData_.empty())
        return;

    float terrainWidth = static_cast<float>(terrain_->GetWidth());
    float terrainDepth = static_cast<float>(terrain_->GetDepth());
    float halfW = terrainWidth * 0.5f;
    float halfD = terrainDepth * 0.5f;
    float worldToTexX = static_cast<float>(craterMapResolution_) / terrainWidth;
    float worldToTexY = static_cast<float>(craterMapResolution_) / terrainDepth;

    float textureRadius = radius * 1.35f;
    int minX = std::max(0, static_cast<int>(std::floor((center.x - textureRadius + halfW) * worldToTexX)));
    int maxX = std::min(craterMapResolution_ - 1, static_cast<int>(std::ceil((center.x + textureRadius + halfW) * worldToTexX)));
    int minY = std::max(0, static_cast<int>(std::floor((center.z - textureRadius + halfD) * worldToTexY)));
    int maxY = std::min(craterMapResolution_ - 1, static_cast<int>(std::ceil((center.z + textureRadius + halfD) * worldToTexY)));

    float textureRadiusSq = textureRadius * textureRadius;
    for (int y = minY; y <= maxY; ++y)
    {
        float worldZ = (static_cast<float>(y) + 0.5f) / worldToTexY - halfD;
        for (int x = minX; x <= maxX; ++x)
        {
            float worldX = (static_cast<float>(x) + 0.5f) / worldToTexX - halfW;
            float dx = worldX - center.x;
            float dz = worldZ - center.z;
            float distSq = dx * dx + dz * dz;
            if (distSq > textureRadiusSq)
                continue;

            float falloff = 1.0f - (distSq / textureRadiusSq);
            falloff = std::pow(falloff, 0.62f);
            float strength = 0.08f + falloff * 0.92f;
            uint8_t intensity = static_cast<uint8_t>(std::clamp(strength * 255.0f, 0.0f, 255.0f));

            size_t idx = static_cast<size_t>(y) * static_cast<size_t>(craterMapResolution_) + static_cast<size_t>(x);
            craterMapData_[idx] = std::max(craterMapData_[idx], intensity);
        }
    }

    uploadCraterMap();
}

void UnitExplosionSystem::updateNavGridAfterDeformation(const glm::vec3& center, float radius)
{
    if (!scene_)
        return;

    scene_->UpdateNavigationGridAfterDeformation(center, radius);
}

void UnitExplosionSystem::spawnExplosionVFX(const glm::vec3& epicenter, float radius, float depth, float force)
{
    (void)depth;

    int primaryCount = static_cast<int>(radius * 5.0f + force * 0.03f);
    primaryCount = std::clamp(primaryCount, 14, 80);
    spawnDebrisParticles(epicenter, radius, force, primaryCount);

    // Add a lighter secondary burst to avoid a single "flat" pulse.
    int secondaryCount = std::clamp(primaryCount / 3, 6, 26);
    spawnDebrisParticles(epicenter, radius * 0.55f, force * 0.6f, secondaryCount);

    ExplosionLight light;
    light.position = epicenter;
    light.intensity = force;
    light.maxIntensity = force * 1.05f;
    light.decayRate = 2.6f;
    light.lifetime = 2.4f + std::min(radius * 0.03f, 0.6f);
    light.totalLifetime = light.lifetime;
    explosionLights_.push_back(light);

    ExplosionLight afterGlow;
    afterGlow.position = epicenter + glm::vec3(0.0f, 1.5f, 0.0f);
    afterGlow.intensity = force * 0.5f;
    afterGlow.maxIntensity = force * 0.45f;
    afterGlow.decayRate = 1.9f;
    afterGlow.lifetime = light.lifetime * 1.35f;
    afterGlow.totalLifetime = afterGlow.lifetime;
    explosionLights_.push_back(afterGlow);

    cameraSakeMagnitude_ = std::min(cameraSakeMagnitude_ + force * 0.15f, 45.0f);
}

void UnitExplosionSystem::spawnDebrisParticles(const glm::vec3& epicenter, float radius, float force, int particleCount)
{
    if (particleCount <= 0)
        return;

    static thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> angleDist(0.0f, 6.28318f);  // 2*PI
    std::uniform_real_distribution<float> radiusDistInner(radius * 0.10f, radius * 0.95f);
    std::uniform_real_distribution<float> heightDist(radius * 0.18f, radius * 0.95f);
    std::uniform_real_distribution<float> scaleDist(0.20f, 1.05f);
    std::uniform_real_distribution<float> speedMulDist(0.78f, 1.42f);
    std::uniform_real_distribution<float> upwardMulDist(0.95f, 2.35f);
    std::uniform_real_distribution<float> dragDist(0.35f, 1.35f);
    std::uniform_real_distribution<float> gravityScaleDist(0.68f, 1.34f);
    std::uniform_real_distribution<float> growthDist(-0.14f, 0.30f);
    std::uniform_real_distribution<float> hotnessDist(0.0f, 1.0f);

    size_t available = kMaxActiveParticles > particles_.size() ? (kMaxActiveParticles - particles_.size()) : 0;
    int spawnCount = std::min<int>(particleCount, static_cast<int>(available));
    if (spawnCount <= 0)
        return;
    particles_.reserve(particles_.size() + static_cast<size_t>(spawnCount));

    for (int i = 0; i < spawnCount; ++i)
    {
        ExplosionParticle particle;

        float angle = angleDist(rng);
        float dist = radiusDistInner(rng);
        particle.position.x = epicenter.x + std::cos(angle) * dist;
        particle.position.z = epicenter.z + std::sin(angle) * dist;
        particle.position.y = epicenter.y + radius * 0.08f + heightDist(rng) * 0.20f;

        float velocityMag = radius * (1.5f + 0.008f * force) * speedMulDist(rng);
        particle.velocity.x = std::cos(angle) * velocityMag;
        particle.velocity.z = std::sin(angle) * velocityMag;
        particle.velocity.y = heightDist(rng) * upwardMulDist(rng);

        particle.scale = glm::vec3(scaleDist(rng));
        float hotness = hotnessDist(rng);
        if (hotness > 0.72f)
            particle.color = glm::vec3(1.0f, 0.52f, 0.16f);      // hot embers
        else if (hotness > 0.36f)
            particle.color = glm::vec3(0.56f, 0.41f, 0.29f);     // warm dust
        else
            particle.color = glm::vec3(0.28f, 0.24f, 0.20f);     // cold debris
        particle.lifetime = 0.0f;
        particle.maxLifetime = 1.6f + scaleDist(rng) * 0.9f + radius * 0.02f;

        particle.rotation = angleDist(rng);
        particle.angularVelocity = (angleDist(rng) - 3.14159f) * 3.1f;
        particle.drag = dragDist(rng);
        particle.gravityScale = gravityScaleDist(rng);
        particle.growthRate = growthDist(rng);

        particles_.push_back(particle);
    }
}

void UnitExplosionSystem::updateParticles(float dt)
{
    size_t writeIndex = 0;
    for (size_t i = 0; i < particles_.size(); ++i)
    {
        ExplosionParticle particle = particles_[i];
        particle.lifetime += dt;

        if (particle.lifetime >= particle.maxLifetime)
        {
            continue;
        }

        float dragFactor = std::max(0.0f, 1.0f - particle.drag * dt);
        particle.velocity *= dragFactor;
        particle.velocity.y -= 9.8f * particle.gravityScale * dt;
        particle.position += particle.velocity * dt;
        particle.rotation += particle.angularVelocity * dt;
        particle.scale += glm::vec3(particle.growthRate * dt);
        particle.scale = glm::max(particle.scale, glm::vec3(0.12f));

        float groundY = Terrain::getHeight(particle.position.x, particle.position.z) + 0.05f;
        if (particle.position.y < groundY)
        {
            particle.position.y = groundY;
            if (std::fabs(particle.velocity.y) > 0.45f)
            {
                particle.velocity.y *= -0.32f;
                particle.velocity.x *= 0.66f;
                particle.velocity.z *= 0.66f;
                particle.angularVelocity *= 0.7f;
            }
            else
            {
                particle.velocity.y = 0.0f;
                particle.velocity.x *= 0.86f;
                particle.velocity.z *= 0.86f;
            }
        }

        particles_[writeIndex++] = particle;
    }
    particles_.resize(writeIndex);
}

void UnitExplosionSystem::updateLights(float dt)
{
    for (auto it = explosionLights_.begin(); it != explosionLights_.end();)
    {
        it->lifetime -= dt;

        float elapsed = it->totalLifetime - std::max(0.0f, it->lifetime);
        it->intensity = it->maxIntensity * std::exp(-it->decayRate * elapsed);

        if (it->lifetime <= 0.0f)
        {
            it = explosionLights_.erase(it);
            continue;
        }

        ++it;
    }
}

void UnitExplosionSystem::renderParticles(Shader& particleShader, const glm::mat4& view, const glm::mat4& projection)
{
    if (particles_.empty() || particleVAO_ == 0)
        return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);

    particleShader.Use();
    particleShader.SetMat4("view", view);
    particleShader.SetMat4("projection", projection);
    particleShader.SetBool("isInstanced", false);
    particleShader.SetBool("useTexture", false);
    particleShader.SetVec3("uMaterialColor", glm::vec3(0.30f, 0.24f, 0.18f));
    particleShader.SetBool("uUseSkinning", false);
    particleShader.BindBoneTexture(0, 0);
    
    glBindVertexArray(particleVAO_);

    for (const auto& particle : particles_)
    {
        float lifeT = Saturate(particle.lifetime / particle.maxLifetime);
        float fadeIn = Smoothstep(0.0f, 0.12f, lifeT);
        float fadeOut = 1.0f - Smoothstep(0.26f, 1.0f, lifeT);
        float alpha = Saturate(fadeIn * fadeOut) * 0.92f;

        glm::vec3 smokeColor(0.16f, 0.16f, 0.16f);
        glm::vec3 midColor = particle.color * (1.0f - lifeT) + smokeColor * lifeT;
        float hotPulse = 1.0f - Smoothstep(0.0f, 0.28f, lifeT);
        glm::vec3 finalColor = midColor + glm::vec3(0.24f, 0.12f, 0.03f) * hotPulse;
        particleShader.SetVec3("uMaterialColor", finalColor);

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, particle.position);
        
        // Rotate based on angular velocity
        model = glm::rotate(model, particle.rotation, glm::vec3(0.0f, 1.0f, 0.0f));
        
        // Scale particle
        model = glm::scale(model, particle.scale);

        particleShader.SetMat4("model", model);
        particleShader.SetFloat("uAlpha", alpha);

        // Draw cube mesh (6 faces, 36 indices)
        glDrawElements(GL_TRIANGLES, particleIndexCount_, GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(0);
    glEnable(GL_CULL_FACE);
    glDisable(GL_BLEND);
}

void UnitExplosionSystem::initializeParticleMesh()
{
    // Create a simple unit cube for particle rendering
    // Vertices of a unit cube (-0.5 to 0.5 on each axis)
    float vertices[] = {
        // Front face
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        
        // Back face
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
        
        // Top face
        -0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        
        // Bottom face
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,
        
        // Right face
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         
        // Left face
        -0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f
    };

    unsigned int indices[] = {
        // Front face
        0, 1, 2,
        2, 3, 0,
        
        // Back face
        4, 6, 5,
        4, 7, 6,
        
        // Top face
        8, 10, 9,
        8, 11, 10,
        
        // Bottom face
        12, 13, 14,
        14, 15, 12,
        
        // Right face
        16, 18, 17,
        16, 19, 18,
        
        // Left face
        20, 21, 22,
        22, 23, 20
    };

    particleIndexCount_ = 36;

    glGenVertexArrays(1, &particleVAO_);
    glGenBuffers(1, &particleVBO_);
    glGenBuffers(1, &particleEBO_);

    glBindVertexArray(particleVAO_);

    glBindBuffer(GL_ARRAY_BUFFER, particleVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, particleEBO_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

}

void UnitExplosionSystem::initializeCraterMap()
{
    if (craterMapTexture_ != 0)
        return;

    craterMapData_.assign(static_cast<size_t>(craterMapResolution_) * static_cast<size_t>(craterMapResolution_), 0);

    glGenTextures(1, &craterMapTexture_);
    glBindTexture(GL_TEXTURE_2D, craterMapTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, craterMapResolution_, craterMapResolution_, 0, GL_RED, GL_UNSIGNED_BYTE, craterMapData_.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void UnitExplosionSystem::uploadCraterMap()
{
    if (craterMapTexture_ == 0 || craterMapData_.empty())
        return;

    glBindTexture(GL_TEXTURE_2D, craterMapTexture_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(
        GL_TEXTURE_2D,
        0,
        0,
        0,
        craterMapResolution_,
        craterMapResolution_,
        GL_RED,
        GL_UNSIGNED_BYTE,
        craterMapData_.data()
    );
    glBindTexture(GL_TEXTURE_2D, 0);
}

float UnitExplosionSystem::GetGlobalLightFlash() const
{
    if (explosionLights_.empty())
        return 0.0f;

    float maxIntensity = 0.0f;
    for (const auto& light : explosionLights_)
        maxIntensity = std::max(maxIntensity, light.intensity);

    return Saturate(maxIntensity / 280.0f) * 0.42f;
}

float UnitExplosionSystem::GetCraterInfluence(const glm::vec3& point) const
{
    float maxInfluence = 0.0f;

    for (const auto& crater : craters_) {
        float dist = glm::distance(point, crater.center);

        if (dist <= crater.radius) {
            // Smooth falloff from center (1.0) to edge (0.0)
            float falloff = 1.0f - (dist / crater.radius);
            falloff = falloff * falloff; // Quadratic falloff for smoother blending
            maxInfluence = std::max(maxInfluence, falloff);
        }
    }

    return maxInfluence;
}
