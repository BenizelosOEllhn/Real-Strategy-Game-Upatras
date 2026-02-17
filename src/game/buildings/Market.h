#pragma once
#include "Building.h"
#include "Resources.h"

class Market : public Building {
public:
    Market(glm::vec3 pos, Model* foundation, Model* finalModel, int ownerID, Resources* resources);

    void Update(float dt) override;
    void SpawnUnit(std::vector<GameEntity*>& entities) override;
    int GoldTickAmount() const { return goldTickAmount_; }
    float GoldInterval() const { return goldInterval_; }

private:
    Resources* ownerResources_;
    float goldTimer_ = 0.0f;
    float goldInterval_ = 5.0f;
    int goldTickAmount_ = 5;

protected:
    void OnUpgradeApplied(int newLevel) override
    {
        if (newLevel >= 2)
            goldTickAmount_ *= 2;
    }
};
