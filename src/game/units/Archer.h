#pragma once
#include "Unit.h"

class Archer : public Unit {
public:
    Archer(glm::vec3 pos, Model* m, int owner)
        : Unit(pos, EntityType::Archer, m, owner, owner == 2 ? 3.4f : 0.65f)
    {
        if (owner == 2)
        {
            SetAnimationNames("CharacterArmature|Idle", "CharacterArmature|Walk");
            SetBaseHeightOffset(3.6f);
        }
        else
        {
            SetAnimationNames("Attack", "Attack");
            SetBaseHeightOffset(4.7f);
        }
    }

    void Update(float dt) override;
};
