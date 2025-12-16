#pragma once
#include "Unit.h"

class Worker : public Unit {
public:
    Worker(glm::vec3 pos, Model* m, int owner)
        : Unit(pos, EntityType::Worker, m, owner, 6.2f)
    {
        SetAnimationNames("CharacterArmature|Idle_Neutral", "CharacterArmature|Walk");
        SetBaseHeightOffset(1.6f);
    }

    void Update(float dt) override;
};
