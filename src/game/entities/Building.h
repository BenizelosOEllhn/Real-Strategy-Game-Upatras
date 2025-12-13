#pragma once
#include "GameEntity.h"

class Building : public GameEntity {
public:
    Building(glm::vec3 pos, EntityType t, Model* m, int owner)
        : GameEntity(pos, t, m, owner) {}

    virtual void SpawnUnit(std::vector<GameEntity*>& entities) = 0;
};
