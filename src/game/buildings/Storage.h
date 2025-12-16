#pragma once
#include "Building.h"
#include "Resources.h"

class Storage : public Building {
public:
    Storage(glm::vec3 pos, Model* foundation, Model* finalModel, int ownerID, Resources* resources);

    void Update(float dt) override;
    void SpawnUnit(std::vector<GameEntity*>& entities) override;

private:
    Resources* ownerResources_;
    bool capacityGranted_ = false;
};
