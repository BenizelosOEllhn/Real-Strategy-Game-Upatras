#pragma once

#include <glm/glm.hpp>
#include <cstdint>
#include <vector>
#include <memory>
#include <queue>
#include <GL/glew.h>

// Forward declarations
class Terrain;
class Scene;
class Shader;
class Texture;

// ============================================================
// VFX Particle (for debris)
// ============================================================
struct ExplosionParticle
{
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 scale;
    glm::vec3 color;
    float lifetime = 0.0f;
    float maxLifetime = 0.0f;
    float rotation = 0.0f;
    float angularVelocity = 0.0f;
    float drag = 0.0f;
    float gravityScale = 1.0f;
    float growthRate = 0.0f;
};

// ============================================================
// Crater Info (for terrain deformation)
// ============================================================
struct CraterInfo
{
    glm::vec3 center;
    float radius;
    float depth;
    float creationTime;
};

// ============================================================
// Explosion Event (queued for processing)
// ============================================================
struct ExplosionEvent
{
    glm::vec3 epicenter;
    float radius;
    float depth;
    float force;              // for shockwave/camera shake
    double timestamp;
};

// ============================================================
// UnitExplosionSystem
// ============================================================
class UnitExplosionSystem
{
public:
    UnitExplosionSystem();
    ~UnitExplosionSystem();

    void Init(Terrain* terrain, Scene* scene);
    void Update(float dt, const glm::vec3& cameraPos);
    void Render(Shader& particleShader, const glm::mat4& view, const glm::mat4& projection);

    // Trigger an explosion at the given location
    void TriggerExplosion(const glm::vec3& epicenter, float radius, float depth, float force);

    // Query craters
    const std::vector<CraterInfo>& GetCraters() const { return craters_; }
    
    // Check if a point is in a crater and get falloff factor [0=not in crater, 1=center]
    float GetCraterInfluence(const glm::vec3& point) const;
    
    // Get camera shake magnitude for this frame (used by the main camera)
    float GetCameraShakeMagnitude() const { return cameraSakeMagnitude_; }
    float GetGlobalLightFlash() const;
    
    // Get scorched earth texture for shader binding
    Texture* GetScorchedEarthTexture() const { return scorchedEarthTex_.get(); }
    GLuint GetCraterMapTexture() const { return craterMapTexture_; }

private:
    // References
    Terrain* terrain_ = nullptr;
    Scene* scene_ = nullptr;

    // Textures
    std::unique_ptr<Texture> scorchedEarthTex_;
    GLuint craterMapTexture_ = 0;
    int craterMapResolution_ = 512;
    std::vector<uint8_t> craterMapData_;

    // Crater tracking
    std::vector<CraterInfo> craters_;
    std::queue<ExplosionEvent> explosionQueue_;

    // Particles (debris)
    std::vector<ExplosionParticle> particles_;
    
    // Particle rendering (simple cube mesh)
    GLuint particleVAO_ = 0;
    GLuint particleVBO_ = 0;
    GLuint particleEBO_ = 0;
    int particleIndexCount_ = 0;
    static constexpr size_t kMaxActiveParticles = 1200;

    // Lighting (point lights spawned during explosions)
    struct ExplosionLight
    {
        glm::vec3 position;
        float intensity;
        float maxIntensity;
        float decayRate;
        float lifetime;
        float totalLifetime;
    };
    std::vector<ExplosionLight> explosionLights_;

    // VFX timing
    float cameraSakeMagnitude_ = 0.0f;

    // Private methods
    void processExplosionQueue();
    void applyCrater(const glm::vec3& center, float radius, float depth);
    void updateNavGridAfterDeformation(const glm::vec3& center, float radius);
    void spawnExplosionVFX(const glm::vec3& epicenter, float radius, float depth, float force);
    void spawnDebrisParticles(const glm::vec3& epicenter, float radius, float force, int particleCount);
    void updateParticles(float dt);
    void updateLights(float dt);
    void renderParticles(Shader& particleShader, const glm::mat4& view, const glm::mat4& projection);
    
    // Particle mesh setup
    void initializeParticleMesh();

    // Terrain modification
    void deformTerrainVertices(const glm::vec3& center, float radius, float depth);
    void updateTerrainTexture(const glm::vec3& center, float radius);
    void initializeCraterMap();
    void uploadCraterMap();
};
