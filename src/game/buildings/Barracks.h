#pragma once
#include "Building.h"
#include "Knight.h"
#include "Archer.h"

class Barracks : public Building {
public:
    Barracks(glm::vec3 pos,
             Model* foundation,
             Model* finalModel,
             int owner)
        : Building(
              pos,
              EntityType::Barracks,
              foundation,
              finalModel,
              owner)
    {
        SetMaxHealth(900.0f);
    }

    void SpawnUnit(std::vector<GameEntity*>& list) override;
    bool HasEmpoweredTraining() const { return empoweredTraining_; }

protected:
    void OnUpgradeApplied(int newLevel) override
    {
        if (newLevel >= 2)
            empoweredTraining_ = true;
    }

private:
    bool empoweredTraining_ = false;
};
