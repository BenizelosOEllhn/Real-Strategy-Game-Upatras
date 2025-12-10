#pragma once
#include "Unit.h"

class Archer : public Unit {
public:
    Archer(glm::vec3 pos, Model* m, int owner)
        : Unit(pos, EntityType::ARCHER, m, owner) {}

    void Update(float dt) override;
};
