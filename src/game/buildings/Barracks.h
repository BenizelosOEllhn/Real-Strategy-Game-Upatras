#pragma once
#include "Building.h"
#include "Knight.h"
#include "Archer.h"

class Barracks : public Building {
public:
    Barracks(glm::vec3 pos, Model* m, int owner)
        : Building(pos, EntityType::Barracks, m, owner) {}

    void Update(float dt) override {}

    void SpawnUnit(std::vector<GameEntity*>& list) override;
};
