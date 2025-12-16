#include "Scene.h"
#include "SceneConstants.h"
#include <queue>
#include <limits>
#include <cmath>
#include <algorithm>

void Scene::initPathfindingGrid()
{
    navCellSize_ = 3.0f;
    navGridCols_ = static_cast<int>(SceneConst::kTerrainWidth / navCellSize_);
    navGridRows_ = static_cast<int>(SceneConst::kTerrainDepth / navCellSize_);
    navOrigin_.x = -SceneConst::kTerrainWidth * 0.5f;
    navOrigin_.y = -SceneConst::kTerrainDepth * 0.5f;
    navWalkable_.assign(navGridCols_ * navGridRows_, 1);
    refreshNavObstacles();
}

void Scene::refreshNavObstacles()
{
    if (navGridCols_ <= 0 || navGridRows_ <= 0)
        return;

    navWalkable_.assign(navGridCols_ * navGridRows_, 1);

    for (int row = 0; row < navGridRows_; ++row)
    {
        for (int col = 0; col < navGridCols_; ++col)
        {
            glm::vec3 world = navToWorld(col, row);
            int idx = row * navGridCols_ + col;
            if (isWaterArea(world.x, world.z))
                navWalkable_[idx] = 0;
        }
    }

    for (GameEntity* entity : entities_)
    {
        Building* building = dynamic_cast<Building*>(entity);
        if (!building) continue;
        float radius = buildingNavRadius(building->type);
        if (radius > 0.0f)
            markObstacleDisc(building->position, radius);
    }
}

bool Scene::commandUnitTo(Unit* unit, const glm::vec3& destination)
{
    if (!unit)
        return false;

    std::vector<glm::vec3> path;
    if (findPath(unit->position, destination, path) && !path.empty())
    {
        unit->SetPath(path);
        unit->SetTaskState(Unit::TaskState::Moving);
        return true;
    }

    unit->SetMoveTarget(destination);
    unit->SetTaskState(Unit::TaskState::Moving);
    return false;
}

bool Scene::findPath(const glm::vec3& start, const glm::vec3& goal, std::vector<glm::vec3>& outPath) const
{
    outPath.clear();
    if (navGridCols_ <= 0 || navGridRows_ <= 0)
        return false;

    int startCol = 0, startRow = 0;
    int goalCol = 0, goalRow = 0;
    if (!worldToNav(start, startCol, startRow))
        return false;
    if (!worldToNav(goal, goalCol, goalRow))
        return false;

    int startIdx = startRow * navGridCols_ + startCol;
    int goalIdx = goalRow * navGridCols_ + goalCol;

    const int nodeCount = navGridCols_ * navGridRows_;
    std::vector<float> gScore(nodeCount, std::numeric_limits<float>::infinity());
    std::vector<int> cameFrom(nodeCount, -1);
    std::vector<uint8_t> closed(nodeCount, 0);

    auto heuristic = [&](int col, int row) -> float
    {
        float dx = static_cast<float>(col - goalCol);
        float dz = static_cast<float>(row - goalRow);
        return std::sqrt(dx * dx + dz * dz);
    };

    struct Node
    {
        float f;
        int idx;
    };
    auto cmp = [](const Node& a, const Node& b) { return a.f > b.f; };
    std::priority_queue<Node, std::vector<Node>, decltype(cmp)> open(cmp);

    gScore[startIdx] = 0.0f;
    open.push({ heuristic(startCol, startRow), startIdx });

    auto isWalkableIdx = [&](int idx) -> bool
    {
        if (idx == startIdx || idx == goalIdx)
            return true;
        return idx >= 0 && idx < nodeCount && navWalkable_[idx] != 0;
    };
    auto hasLineOfSight = [&](const glm::vec3& a, const glm::vec3& b) -> bool
    {
        int aCol = 0, aRow = 0;
        int bCol = 0, bRow = 0;
        if (!worldToNav(a, aCol, aRow) || !worldToNav(b, bCol, bRow))
            return false;

        int dCol = std::abs(bCol - aCol);
        int dRow = std::abs(bRow - aRow);
        int colStep = (bCol > aCol) ? 1 : (bCol < aCol ? -1 : 0);
        int rowStep = (bRow > aRow) ? 1 : (bRow < aRow ? -1 : 0);
        int col = aCol;
        int row = aRow;
        int n = 1 + dCol + dRow;
        int error = dCol - dRow;
        dCol *= 2;
        dRow *= 2;

        while (n-- > 0)
        {
            int idx = row * navGridCols_ + col;
            if (!isWalkableIdx(idx))
                return false;
            if (col == bCol && row == bRow)
                break;
            if (error > 0)
            {
                col += colStep;
                error -= dRow;
            }
            else if (error < 0)
            {
                row += rowStep;
                error += dCol;
            }
            else
            {
                col += colStep;
                row += rowStep;
                error -= dRow;
                error += dCol;
            }
        }
        return true;
    };

    const int offsets[8][2] = {
        { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 },
        { 1, 1 }, { 1, -1 }, { -1, 1 }, { -1, -1 }
    };

    while (!open.empty())
    {
        Node current = open.top();
        open.pop();

        if (closed[current.idx])
            continue;
        closed[current.idx] = 1;

        if (current.idx == goalIdx)
            break;

        int currentRow = current.idx / navGridCols_;
        int currentCol = current.idx % navGridCols_;

        for (const auto& offset : offsets)
        {
            int nCol = currentCol + offset[0];
            int nRow = currentRow + offset[1];
            if (nCol < 0 || nCol >= navGridCols_ || nRow < 0 || nRow >= navGridRows_)
                continue;

            int neighborIdx = nRow * navGridCols_ + nCol;
            if (!isWalkableIdx(neighborIdx))
                continue;

            float cost = (offset[0] == 0 || offset[1] == 0) ? navCellSize_ : navCellSize_ * 1.4142f;
            float tentative = gScore[current.idx] + cost;
            if (tentative < gScore[neighborIdx])
            {
                cameFrom[neighborIdx] = current.idx;
                gScore[neighborIdx] = tentative;
                float fScore = tentative + heuristic(nCol, nRow);
                open.push({ fScore, neighborIdx });
            }
        }
    }

    if (cameFrom[goalIdx] == -1 && goalIdx != startIdx)
        return false;

    std::vector<int> chain;
    int current = goalIdx;
    chain.push_back(goalIdx);
    while (current != startIdx && current != -1)
    {
        current = cameFrom[current];
        if (current != -1 && current != startIdx)
            chain.push_back(current);
    }

    std::reverse(chain.begin(), chain.end());
    outPath.reserve(chain.size());
    for (int idx : chain)
    {
        int row = idx / navGridCols_;
        int col = idx % navGridCols_;
        outPath.push_back(navToWorld(col, row));
    }

    if (outPath.empty())
    {
        outPath.push_back(goal);
    }

    if (outPath.size() >= 3)
    {
        std::vector<glm::vec3> simplified;
        simplified.reserve(outPath.size());
        size_t anchor = 0;
        simplified.push_back(outPath.front());
        while (anchor < outPath.size() - 1)
        {
            size_t next = anchor + 1;
            for (size_t candidate = outPath.size() - 1; candidate > anchor; --candidate)
            {
                if (hasLineOfSight(outPath[anchor], outPath[candidate]))
                {
                    next = candidate;
                    break;
                }
            }
            if (next == anchor)
                break;
            simplified.push_back(outPath[next]);
            anchor = next;
        }
        if (simplified.back() != outPath.back())
            simplified.push_back(outPath.back());
        outPath.swap(simplified);
    }

    return true;
}

bool Scene::worldToNav(const glm::vec3& pos, int& col, int& row) const
{
    if (navGridCols_ <= 0 || navGridRows_ <= 0)
        return false;

    float fx = (pos.x - navOrigin_.x) / navCellSize_;
    float fz = (pos.z - navOrigin_.y) / navCellSize_;

    if (!std::isfinite(fx) || !std::isfinite(fz))
        return false;

    col = static_cast<int>(std::floor(fx));
    row = static_cast<int>(std::floor(fz));

    if (col < 0) col = 0;
    if (col >= navGridCols_) col = navGridCols_ - 1;
    if (row < 0) row = 0;
    if (row >= navGridRows_) row = navGridRows_ - 1;

    return true;
}

glm::vec3 Scene::navToWorld(int col, int row) const
{
    float x = navOrigin_.x + (static_cast<float>(col) + 0.5f) * navCellSize_;
    float z = navOrigin_.y + (static_cast<float>(row) + 0.5f) * navCellSize_;
    float y = Terrain::getHeight(x, z);
    return glm::vec3(x, y, z);
}

void Scene::markObstacleDisc(const glm::vec3& center, float radius)
{
    if (radius <= 0.0f)
        return;

    int minCol = static_cast<int>((center.x - radius - navOrigin_.x) / navCellSize_);
    int maxCol = static_cast<int>((center.x + radius - navOrigin_.x) / navCellSize_);
    int minRow = static_cast<int>((center.z - radius - navOrigin_.y) / navCellSize_);
    int maxRow = static_cast<int>((center.z + radius - navOrigin_.y) / navCellSize_);

    minCol = std::max(0, minCol);
    minRow = std::max(0, minRow);
    maxCol = std::min(navGridCols_ - 1, maxCol);
    maxRow = std::min(navGridRows_ - 1, maxRow);

    float radiusSq = radius * radius;

    for (int row = minRow; row <= maxRow; ++row)
    {
        for (int col = minCol; col <= maxCol; ++col)
        {
            glm::vec3 world = navToWorld(col, row);
            glm::vec2 delta(world.x - center.x, world.z - center.z);
            if (glm::dot(delta, delta) <= radiusSq)
            {
                int idx = row * navGridCols_ + col;
                navWalkable_[idx] = 0;
            }
        }
    }
}

float Scene::buildingNavRadius(EntityType type) const
{
    switch (type)
    {
    case EntityType::TownCenter: return 32.0f;
    case EntityType::Barracks:   return 28.0f;
    case EntityType::Storage:    return 24.0f;
    case EntityType::Market:     return 26.0f;
    case EntityType::Farm:       return 18.0f;
    case EntityType::House:      return 18.0f;
    default:
        return 20.0f;
    }
}
