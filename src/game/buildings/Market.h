#pragma once
#include "Building.h"
#include "Resources.h"

class Market : public Building {
public:
    Market(glm::vec3 pos, Model* foundation, Model* finalModel, int ownerID, Resources* resources);

    void Update(float dt) override;
    void SpawnUnit(std::vector<GameEntity*>& entities) override;

private:
    Resources* ownerResources_;
    float goldTimer_ = 0.0f;
    float goldInterval_ = 5.0f;
    int goldTickAmount_ = 5;
};
