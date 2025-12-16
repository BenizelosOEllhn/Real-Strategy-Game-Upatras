#pragma once
#include "Building.h"
#include "Resources.h"

class House : public Building {
public:
    House(glm::vec3 pos, Model* foundation, Model* finalModel, int ownerID, Resources* resources);

    void Update(float dt) override;
    void SpawnUnit(std::vector<GameEntity*>& entities) override;

private:
    Resources* ownerResources_;
    bool grantedPopulation_ = false;
};
