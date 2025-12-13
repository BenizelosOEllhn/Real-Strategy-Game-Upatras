#pragma once
#include "GameEntity.h"
#include "Resources.h"

class Storage : public GameEntity {
public:
    Storage(glm::vec3 pos, Model* model, int ownerID, Resources* resources);

    void Update(float dt) override {}

private:
    Resources* ownerResources_;
};
