#pragma once
#include "Unit.h"

class Worker : public Unit {
public:
    Worker(glm::vec3 pos, Model* m, int owner)
        : Unit(pos, EntityType::WORKER, m, owner) {}

    void Update(float dt) override;
};
