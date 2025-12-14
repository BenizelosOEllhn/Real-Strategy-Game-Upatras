#pragma once
#include "GameEntity.h"

class Building : public GameEntity {
public:
    Model* foundationModel = nullptr;
    Model* finalModel      = nullptr;

    bool  isUnderConstruction = true;
    float buildProgress = 0.0f;
    float buildTime = 6.0f;

    Building(glm::vec3 pos, EntityType t, Model* foundation, Model* final, int owner)
        : GameEntity(pos, t, final, owner),
          foundationModel(foundation),
          finalModel(final)
    {}

    void Update(float dt) override
    {
        if (!isUnderConstruction) return;
        buildProgress = glm::clamp(buildProgress + dt / buildTime, 0.0f, 1.0f);
        if (buildProgress >= 1.0f) {
            isUnderConstruction = false;
        }
    }

    void Draw(Shader& shader) override;           
    virtual void SpawnUnit(std::vector<GameEntity*>& entities) = 0;
};
