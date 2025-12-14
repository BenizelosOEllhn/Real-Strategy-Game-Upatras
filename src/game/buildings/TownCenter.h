#pragma once
#include "Building.h"
#include "Worker.h"

class TownCenter : public Building {
public:
    TownCenter(glm::vec3 pos,
               Model* foundation,
               Model* finalModel,
               int owner)
        : Building(
              pos,
              EntityType::TownCenter,
              foundation,     // foundation mesh
              finalModel,     // final building mesh
              owner)
    {}

    void SpawnUnit(std::vector<GameEntity*>& list) override;
};
