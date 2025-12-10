#pragma once
#include "Unit.h"

class Knight : public Unit {
public:
    Knight(glm::vec3 pos, Model* m, int owner)
        : Unit(pos, EntityType::KNIGHT, m, owner) {}

    void Update(float dt) override;
};
