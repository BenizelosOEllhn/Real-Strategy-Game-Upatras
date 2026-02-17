#pragma once
#include "Building.h"

class Bridge : public Building
{
public:
    Bridge(glm::vec3 pos, Model* foundation, Model* finalModel, int owner)
        : Building(pos, EntityType::Bridge, foundation, finalModel, owner)
    {
        buildTime = 4.0f;
    }

    void SpawnUnit(std::vector<GameEntity*>& /*entities*/) override {}
};
