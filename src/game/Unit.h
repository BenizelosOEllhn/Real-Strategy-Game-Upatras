#pragma once
#include "GameEntity.h"

class Unit : public GameEntity {
public:
    float speed = 5.0f;

    Unit(glm::vec3 pos, EntityType t, Model* model, int owner)
        : GameEntity(pos, t, model, owner) {}

    virtual void Update(float dt) override;

    virtual ~Unit();
};
