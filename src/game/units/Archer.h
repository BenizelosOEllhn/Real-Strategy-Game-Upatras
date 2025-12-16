#pragma once
#include "Unit.h"

class Archer : public Unit {
public:
    Archer(glm::vec3 pos, Model* m, int owner)
        : Unit(pos, EntityType::Archer, m, owner, 0.65f)
    {
        SetAnimationNames("Attack", "Attack");
        SetBaseHeightOffset(4.0f);
    }

    void Update(float dt) override;
};
