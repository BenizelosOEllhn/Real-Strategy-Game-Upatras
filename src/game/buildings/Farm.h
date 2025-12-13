#pragma once
#include "GameEntity.h"
#include "Resources.h"

class Farm : public GameEntity {
public:
    Farm(glm::vec3 pos, Model* model, int ownerID, Resources* resources);

    void Update(float dt) override;

private:
    Resources* ownerResources_;
    float productionTimer_ = 0.0f;
    float productionInterval_ = 3.0f;
    int productionAmount_ = 5;
};
