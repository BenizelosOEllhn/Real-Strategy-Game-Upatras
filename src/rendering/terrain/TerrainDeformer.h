#pragma once

#include "Terrain.h"
#include <cstddef>
#include <functional>
#include <glm/glm.hpp>
#include <vector>

class TerrainDeformer
{
public:
    TerrainDeformer(Terrain* terrain);
    ~TerrainDeformer();

    // Pull latest terrain vertex/index data into the deformer cache.
    void RefreshFromTerrain();

    /// Apply a parabolic crater to the terrain
    /// Uses the formula: y_new = y_old - depth * (1 - (distance^2 / radius^2))
    /// Clamps to bedrock level
    void ApplyCrater(const glm::vec3& center, float radius, float depth);

    /// Apply a custom deformation function to all vertices within a region
    /// deformFunc: takes (x, y, z) and returns the new y value
    void ApplyDeformation(const glm::vec3& center, float radius,
                         std::function<float(float, float, float)> deformFunc);

    /// Sync terrain data to GPU after modifications
    /// Must be called after any vertex manipulation
    void SyncToGPU();

    /// Get vertices for custom processing (advanced)
    std::vector<Vertex>& GetVertices() { return vertices_; }
    const std::vector<Vertex>& GetVertices() const { return vertices_; }

    /// Sample height at any point (accounts for deformations)
    float GetHeightAtPoint(float x, float z) const;

    /// Check the slope angle at a point (for walkability)
    float GetSlopeAngle(float x, float z) const;

private:
    Terrain* terrain_;
    std::vector<Vertex> vertices_;
    std::vector<unsigned int> indices_;
    bool needsGPUSync_ = false;
    int width_ = 0;
    int depth_ = 0;
    int vertexStride_ = 0;
    float halfWidth_ = 0.0f;
    float halfDepth_ = 0.0f;

    static constexpr float BEDROCK_LEVEL = -14.0f;

    void recalculateNormals();
    void recalculateNormalsInRadius(const glm::vec3& center, float radius);
    glm::vec3 calculateVertexNormal(size_t vertexIndex);
    float sampleHeightBilinear(float x, float z) const;
};
