#include "AStarGridUpdater.h"
#include "Scene.h"
#include "../rendering/terrain/TerrainDeformer.h"
#include "../game/entities/Unit.h"
#include <cmath>
#include <iostream>
#include <algorithm>

AStarGridUpdater::AStarGridUpdater(Scene* scene, TerrainDeformer* deformer)
    : scene_(scene), deformer_(deformer)
{
}

AStarGridUpdater::~AStarGridUpdater()
{
}

void AStarGridUpdater::UpdateGridAfterDeformation(const glm::vec3& centerWorld, float radiusWorld)
{
    if (!scene_ || !deformer_)
        return;

    lastUpdates_.clear();

    std::vector<AffectedNode> affectedNodes;
    queryAffectedNodes(centerWorld, radiusWorld, affectedNodes);

    updateNodeStates(affectedNodes, radiusWorld);

    std::vector<int> affectedIndices;
    affectedIndices.reserve(affectedNodes.size());
    for (const auto& node : affectedNodes)
        affectedIndices.push_back(node.gridIndex);

    std::vector<Unit*> affectedUnits;
    findUnitsOnNodes(affectedIndices, affectedUnits);

    for (Unit* unit : affectedUnits)
    {
        if (unit)
            unit->ClearMoveTarget();
    }

    ForceRepathUnitsInRadius(centerWorld, radiusWorld);
}

void AStarGridUpdater::queryAffectedNodes(const glm::vec3& center, float radius,
                                          std::vector<AffectedNode>& outNodes)
{
    if (!scene_)
        return;

    // Get grid parameters
    int gridCols = scene_->GetNavGridCols();
    int gridRows = scene_->GetNavGridRows();
    float cellSize = scene_->GetNavCellSize();
    glm::vec2 navOrigin = scene_->GetNavOrigin();
    float radiusSq = radius * radius;

    // Iterate through all grid cells
    for (int row = 0; row < gridRows; ++row)
    {
        for (int col = 0; col < gridCols; ++col)
        {
            int index = row * gridCols + col;

            // Get center of grid cell in world space
            glm::vec3 cellCenter(
                navOrigin.x + col * cellSize + cellSize * 0.5f,
                0.0f,
                navOrigin.y + row * cellSize + cellSize * 0.5f
            );

            // Check if cell is within deformation radius
            float dx = cellCenter.x - center.x;
            float dz = cellCenter.z - center.z;
            float distSq = dx * dx + dz * dz;

            if (distSq > radiusSq)
                continue;  // Cell too far away

            // Get height at cell center
            float height = deformer_->GetHeightAtPoint(cellCenter.x, cellCenter.z);
            cellCenter.y = height;

            // Calculate slope angle at this cell
            float slopeAngle = deformer_->GetSlopeAngle(cellCenter.x, cellCenter.z);

            // Record affected node
            AffectedNode node;
            node.gridIndex = index;
            node.slopeAngle = slopeAngle;
            node.avgHeight = height;
            node.distanceToCenter = std::sqrt(distSq);
            outNodes.push_back(node);
        }
    }
}

void AStarGridUpdater::updateNodeStates(const std::vector<AffectedNode>& affectedNodes, float craterRadius)
{
    if (!scene_)
        return;

    auto& navWalkable = scene_->GetNavWalkable();

    for (const auto& node : affectedNodes)
    {
        if (node.gridIndex < 0 || node.gridIndex >= static_cast<int>(navWalkable.size()))
            continue;

        uint8_t previousState = navWalkable[node.gridIndex];

        NavNodeUpdate update;
        update.gridIndex = node.gridIndex;
        update.costMultiplier = 1.0f;
        float craterRatio = craterRadius > 0.001f ? (node.distanceToCenter / craterRadius) : 1.0f;

        // Preserve static blockers from the base nav grid (water/buildings).
        if (previousState == static_cast<uint8_t>(NavNodeState::Unwalkable))
        {
            update.newState = NavNodeState::Unwalkable;
            lastUpdates_.push_back(update);
            continue;
        }

        if (node.slopeAngle > MAX_WALKABLE_ANGLE_RAD || craterRatio <= CRATER_CORE_BLOCK_RATIO)
        {
            update.newState = NavNodeState::Unwalkable;
            navWalkable[node.gridIndex] = static_cast<uint8_t>(NavNodeState::Unwalkable);
        }
        else if (node.slopeAngle > HIGH_COST_ANGLE_RAD || craterRatio <= CRATER_EDGE_COST_RATIO)
        {
            update.newState = NavNodeState::HighCost;
            update.costMultiplier = 2.0f;
            navWalkable[node.gridIndex] = static_cast<uint8_t>(NavNodeState::HighCost);
        }
        else
        {
            update.newState = NavNodeState::Walkable;
            navWalkable[node.gridIndex] = static_cast<uint8_t>(NavNodeState::Walkable);
        }

        lastUpdates_.push_back(update);
    }
}

void AStarGridUpdater::findUnitsOnNodes(const std::vector<int>& nodeIndices,
                                       std::vector<Unit*>& outUnits)
{
    if (!scene_ || nodeIndices.empty())
        return;

    std::unordered_set<int> affectedNodeSet(nodeIndices.begin(), nodeIndices.end());
    const auto& entities = scene_->GetEntities();
    outUnits.reserve(8);

    for (GameEntity* entity : entities)
    {
        Unit* unit = dynamic_cast<Unit*>(entity);
        if (!unit)
            continue;

        int idx = worldToGridIndex(unit->position);
        if (idx >= 0 && affectedNodeSet.find(idx) != affectedNodeSet.end())
            outUnits.push_back(unit);
    }
}

void AStarGridUpdater::ForceRepathUnitsInRadius(const glm::vec3& center, float radius)
{
    if (!scene_)
        return;

    float radiusSq = radius * radius;
    const auto& entities = scene_->GetEntities();
    for (GameEntity* entity : entities)
    {
        Unit* unit = dynamic_cast<Unit*>(entity);
        if (!unit)
            continue;

        glm::vec2 delta(unit->position.x - center.x, unit->position.z - center.z);
        if (glm::dot(delta, delta) <= radiusSq)
            unit->ClearMoveTarget();
    }
}

void AStarGridUpdater::HandleUnitsOnUnwalkable(const glm::vec3& center, float radius)
{
    (void)center;
    (void)radius;
}

int AStarGridUpdater::worldToGridIndex(const glm::vec3& worldPos)
{
    if (!scene_)
        return -1;

    int gridCols = scene_->GetNavGridCols();
    int gridRows = scene_->GetNavGridRows();
    float cellSize = scene_->GetNavCellSize();
    glm::vec2 navOrigin = scene_->GetNavOrigin();

    // Convert world position to grid coordinates
    float relX = worldPos.x - navOrigin.x;
    float relZ = worldPos.z - navOrigin.y;

    int col = static_cast<int>(relX / cellSize);
    int row = static_cast<int>(relZ / cellSize);

    // Clamp to valid grid bounds
    col = glm::clamp(col, 0, gridCols - 1);
    row = glm::clamp(row, 0, gridRows - 1);

    return row * gridCols + col;
}

glm::vec3 AStarGridUpdater::gridToWorldPos(int index)
{
    if (!scene_)
        return glm::vec3(0.0f);

    int gridCols = scene_->GetNavGridCols();
    float cellSize = scene_->GetNavCellSize();
    glm::vec2 navOrigin = scene_->GetNavOrigin();

    if (gridCols <= 0)
        return glm::vec3(0.0f);

    int col = index % gridCols;
    int row = index / gridCols;

    glm::vec3 worldPos(
        navOrigin.x + col * cellSize + cellSize * 0.5f,
        0.0f,
        navOrigin.y + row * cellSize + cellSize * 0.5f
    );

    if (deformer_)
    {
        worldPos.y = deformer_->GetHeightAtPoint(worldPos.x, worldPos.z);
    }

    return worldPos;
}
