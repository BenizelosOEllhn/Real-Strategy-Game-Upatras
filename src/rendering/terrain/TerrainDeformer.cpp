#include "TerrainDeformer.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

TerrainDeformer::TerrainDeformer(Terrain* terrain)
    : terrain_(terrain)
{
    if (!terrain_)
    {
        std::cerr << "[TerrainDeformer] ERROR: Terrain pointer is null!\n";
        return;
    }

    RefreshFromTerrain();
    std::cout << "[TerrainDeformer] Initialized for terrain sampling\n";
}

TerrainDeformer::~TerrainDeformer()
{
}

void TerrainDeformer::RefreshFromTerrain()
{
    if (!terrain_)
        return;

    width_ = terrain_->GetWidth();
    depth_ = terrain_->GetDepth();
    vertexStride_ = width_ + 1;
    halfWidth_ = static_cast<float>(width_) * 0.5f;
    halfDepth_ = static_cast<float>(depth_) * 0.5f;

    // Keep CPU copies empty for the query-only path; A* reads directly from Terrain.
    vertices_.clear();
    indices_.clear();
}

void TerrainDeformer::ApplyCrater(const glm::vec3& center, float radius, float depth)
{
    std::vector<Vertex>* targetVertices = &vertices_;
    if (targetVertices->empty() && terrain_)
        targetVertices = &terrain_->GetVertices();
    if (targetVertices->empty())
        return;

    const float radiusSq = radius * radius;

    for (auto& vertex : *targetVertices)
    {
        float dx = vertex.Position.x - center.x;
        float dz = vertex.Position.z - center.z;
        float distSq = dx * dx + dz * dz;

        if (distSq <= radiusSq)
        {
            float falloff = 1.0f - (distSq / radiusSq);
            falloff *= falloff;

            vertex.Position.y -= depth * falloff;
            if (vertex.Position.y < BEDROCK_LEVEL)
                vertex.Position.y = BEDROCK_LEVEL;
        }
    }

    if (targetVertices == &vertices_)
    {
        recalculateNormalsInRadius(center, radius);
        needsGPUSync_ = true;
    }
    else if (terrain_)
    {
        terrain_->RecalculateNormalsInRadius(center, radius);
        terrain_->SyncVerticesToGPU();
    }
}

void TerrainDeformer::ApplyDeformation(const glm::vec3& center, float radius,
                                      std::function<float(float, float, float)> deformFunc)
{
    std::vector<Vertex>* targetVertices = &vertices_;
    if (targetVertices->empty() && terrain_)
        targetVertices = &terrain_->GetVertices();
    if (targetVertices->empty())
        return;

    const float radiusSq = radius * radius;

    for (auto& vertex : *targetVertices)
    {
        float dx = vertex.Position.x - center.x;
        float dz = vertex.Position.z - center.z;
        float distSq = dx * dx + dz * dz;

        if (distSq <= radiusSq)
        {
            float newY = deformFunc(vertex.Position.x, vertex.Position.y, vertex.Position.z);
            vertex.Position.y = std::max(newY, BEDROCK_LEVEL);
        }
    }

    if (targetVertices == &vertices_)
    {
        recalculateNormalsInRadius(center, radius);
        needsGPUSync_ = true;
    }
    else if (terrain_)
    {
        terrain_->RecalculateNormalsInRadius(center, radius);
        terrain_->SyncVerticesToGPU();
    }
}

void TerrainDeformer::SyncToGPU()
{
    if (!needsGPUSync_ || !terrain_)
        return;

    auto& terrainVertices = terrain_->GetVertices();
    if (terrainVertices.size() == vertices_.size())
    {
        terrainVertices = vertices_;
    }
    else
    {
        RefreshFromTerrain();
    }
    terrain_->SyncVerticesToGPU();
    needsGPUSync_ = false;
}

float TerrainDeformer::GetHeightAtPoint(float x, float z) const
{
    return sampleHeightBilinear(x, z);
}

float TerrainDeformer::GetSlopeAngle(float x, float z) const
{
    const float epsilon = 1.0f;
    float hL = GetHeightAtPoint(x - epsilon, z);
    float hR = GetHeightAtPoint(x + epsilon, z);
    float hD = GetHeightAtPoint(x, z - epsilon);
    float hU = GetHeightAtPoint(x, z + epsilon);

    glm::vec3 dx(2.0f * epsilon, hR - hL, 0.0f);
    glm::vec3 dz(0.0f, hU - hD, 2.0f * epsilon);
    glm::vec3 normal = glm::normalize(glm::cross(dz, dx));

    float y = glm::clamp(normal.y, -1.0f, 1.0f);
    return std::acos(y);
}

void TerrainDeformer::recalculateNormals()
{
    if (vertices_.empty())
        return;

    for (size_t i = 0; i < vertices_.size(); ++i)
    {
        vertices_[i].Normal = calculateVertexNormal(i);
    }
}

void TerrainDeformer::recalculateNormalsInRadius(const glm::vec3& center, float radius)
{
    if (vertices_.empty() || width_ <= 0 || depth_ <= 0)
        return;

    int minX = std::max(0, static_cast<int>(std::floor(center.x + halfWidth_ - radius)) - 1);
    int maxX = std::min(width_, static_cast<int>(std::ceil(center.x + halfWidth_ + radius)) + 1);
    int minZ = std::max(0, static_cast<int>(std::floor(center.z + halfDepth_ - radius)) - 1);
    int maxZ = std::min(depth_, static_cast<int>(std::ceil(center.z + halfDepth_ + radius)) + 1);
    float radiusSq = radius * radius;

    for (int z = minZ; z <= maxZ; ++z)
    {
        for (int x = minX; x <= maxX; ++x)
        {
            size_t idx = static_cast<size_t>(z) * static_cast<size_t>(vertexStride_) + static_cast<size_t>(x);
            float dx = vertices_[idx].Position.x - center.x;
            float dz = vertices_[idx].Position.z - center.z;
            if (dx * dx + dz * dz <= radiusSq)
            {
                vertices_[idx].Normal = calculateVertexNormal(idx);
            }
        }
    }
}

glm::vec3 TerrainDeformer::calculateVertexNormal(size_t vertexIndex)
{
    if (vertices_.empty() || width_ <= 0 || depth_ <= 0 || vertexStride_ <= 0)
        return glm::vec3(0.0f, 1.0f, 0.0f);

    int z = static_cast<int>(vertexIndex / static_cast<size_t>(vertexStride_));
    int x = static_cast<int>(vertexIndex % static_cast<size_t>(vertexStride_));

    int xL = std::max(0, x - 1);
    int xR = std::min(width_, x + 1);
    int zD = std::max(0, z - 1);
    int zU = std::min(depth_, z + 1);

    auto h = [&](int vx, int vz) -> float
    {
        size_t idx = static_cast<size_t>(vz) * static_cast<size_t>(vertexStride_) + static_cast<size_t>(vx);
        return vertices_[idx].Position.y;
    };

    glm::vec3 dx(
        static_cast<float>(xR - xL),
        h(xR, z) - h(xL, z),
        0.0f
    );
    glm::vec3 dz(
        0.0f,
        h(x, zU) - h(x, zD),
        static_cast<float>(zU - zD)
    );

    glm::vec3 normal = glm::cross(dz, dx);
    if (glm::dot(normal, normal) < 1e-6f)
        return glm::vec3(0.0f, 1.0f, 0.0f);
    return glm::normalize(normal);
}

float TerrainDeformer::sampleHeightBilinear(float x, float z) const
{
    if (width_ <= 0 || depth_ <= 0 || vertexStride_ <= 0 || !terrain_)
        return Terrain::getHeight(x, z);

    const auto& source = terrain_->GetVertices();
    if (source.empty())
        return Terrain::getHeight(x, z);

    float fx = glm::clamp(x + halfWidth_, 0.0f, static_cast<float>(width_));
    float fz = glm::clamp(z + halfDepth_, 0.0f, static_cast<float>(depth_));

    int x0 = static_cast<int>(std::floor(fx));
    int z0 = static_cast<int>(std::floor(fz));
    int x1 = std::min(x0 + 1, width_);
    int z1 = std::min(z0 + 1, depth_);
    x0 = std::max(0, std::min(x0, width_));
    z0 = std::max(0, std::min(z0, depth_));

    float tx = fx - static_cast<float>(x0);
    float tz = fz - static_cast<float>(z0);

    auto sample = [&](int sx, int sz) -> float
    {
        size_t idx = static_cast<size_t>(sz) * static_cast<size_t>(vertexStride_) + static_cast<size_t>(sx);
        return source[idx].Position.y;
    };

    float h00 = sample(x0, z0);
    float h10 = sample(x1, z0);
    float h01 = sample(x0, z1);
    float h11 = sample(x1, z1);

    float hx0 = h00 + (h10 - h00) * tx;
    float hx1 = h01 + (h11 - h01) * tx;
    return hx0 + (hx1 - hx0) * tz;
}
