#pragma once
#include "Building.h"
#include "Resources.h"
#include <glm/vec4.hpp>

class Storage : public Building {
public:
    Storage(glm::vec3 pos, Model* foundation, Model* finalModel, int ownerID, Resources* resources);

    void Update(float dt) override;
    void SpawnUnit(std::vector<GameEntity*>& entities) override;
    glm::ivec4 CapacityBonus() const
    {
        return glm::ivec4(150 * GetLevel(), 150 * GetLevel(), 80 * GetLevel(), 120 * GetLevel());
    }

private:
    Resources* ownerResources_;
    bool capacityGranted_ = false;

protected:
    void OnUpgradeApplied(int newLevel) override
    {
        if (newLevel >= 2 && ownerResources_)
            ownerResources_->IncreaseStorageCapacity(150, 150, 80, 120);
    }
};
