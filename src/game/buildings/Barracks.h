#pragma once
#include "Building.h"
#include "Knight.h"
#include "Archer.h"

class Barracks : public Building {
public:
    Barracks(glm::vec3 pos,
             Model* foundation,
             Model* finalModel,
             int owner)
        : Building(
              pos,
              EntityType::Barracks,
              foundation,
              finalModel,
              owner)
    {}

    void SpawnUnit(std::vector<GameEntity*>& list) override;
};
