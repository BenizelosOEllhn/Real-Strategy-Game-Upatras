#pragma once
#include "Building.h"
#include "Resources.h"

class Farm : public Building {
public:
    Farm(glm::vec3 pos, Model* foundation, Model* finalModel, int ownerID, Resources* resources);

    void Update(float dt) override;
    void SpawnUnit(std::vector<GameEntity*>& entities) override;

private:
    Resources* ownerResources_;
    float productionTimer_ = 0.0f;
    float productionInterval_ = 3.0f;
    int productionAmount_ = 5;
};
