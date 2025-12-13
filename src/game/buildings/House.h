#pragma once
#include "GameEntity.h"
#include "Resources.h"

class House : public GameEntity {
public:
    House(glm::vec3 pos, Model* model, int ownerID, Resources* resources);

    void Update(float dt) override {}

private:
    Resources* ownerResources_;
};
