/// ============================================================
/// A* Grid Dynamic Update Module
/// ============================================================
/// Handles real-time updates to the navigation grid when
/// terrain is deformed (craters, holes, etc.)
/// Manages node state changes, cost recalculation, and
/// unit repath forcing.
/// ============================================================

#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <unordered_set>

class Scene;
class TerrainDeformer;
class Unit;

// Navigation node state
enum class NavNodeState : uint8_t
{
    Walkable = 1,
    Unwalkable = 0,
    HighCost = 2   // Increased traversal cost
};

struct NavNodeUpdate
{
    int gridIndex;
    NavNodeState newState;
    float costMultiplier;  // 1.0 = normal, 2.0 = twice cost, etc.
};

class AStarGridUpdater
{
public:
    AStarGridUpdater(Scene* scene, TerrainDeformer* deformer);
    ~AStarGridUpdater();

    /// Process crater and update all affected A* nodes
    /// Identifies nodes with significant slope changes and marks unwalkable/high-cost areas
    void UpdateGridAfterDeformation(const glm::vec3& centerWorld, float radiusWorld);

    /// Get list of nodes that changed state (for UI visualization, debugging)
    const std::vector<NavNodeUpdate>& GetLastNodeUpdates() const { return lastUpdates_; }

    /// Force repathfor all units currently using affected nodes
    void ForceRepathUnitsInRadius(const glm::vec3& center, float radius);

    /// Handle units standing on new unwalkable terrain
    /// Either slide them to safe nearby position or damage them
    void HandleUnitsOnUnwalkable(const glm::vec3& center, float radius);

private:
    Scene* scene_;
    TerrainDeformer* deformer_;
    std::vector<NavNodeUpdate> lastUpdates_;

    static constexpr float MAX_WALKABLE_ANGLE_RAD = 0.785398f;  // ~45 degrees
    static constexpr float HIGH_COST_ANGLE_RAD = 0.436332f;     // ~25 degrees
    static constexpr float CRATER_CORE_BLOCK_RATIO = 0.42f;
    static constexpr float CRATER_EDGE_COST_RATIO = 0.92f;

    struct AffectedNode
    {
        int gridIndex;
        float slopeAngle;
        float avgHeight;
        float distanceToCenter;
    };

    void queryAffectedNodes(const glm::vec3& center, float radius,
                           std::vector<AffectedNode>& outNodes);

    void updateNodeStates(const std::vector<AffectedNode>& affectedNodes, float craterRadius);

    void findUnitsOnNodes(const std::vector<int>& nodeIndices,
                         std::vector<Unit*>& outUnits);

    int worldToGridIndex(const glm::vec3& worldPos);
    glm::vec3 gridToWorldPos(int index);
};
