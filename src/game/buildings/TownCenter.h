#pragma once
#include "Building.h"
#include "Worker.h"

class TownCenter : public Building {
public:
    TownCenter(glm::vec3 pos, Model* m, int owner)
        : Building(pos, EntityType::TownCenter, m, owner) {}

    void Update(float dt) override {}

    void SpawnUnit(std::vector<GameEntity*>& list) override;
};
