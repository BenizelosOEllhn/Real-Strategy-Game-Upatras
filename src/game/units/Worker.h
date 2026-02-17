#pragma once
#include "Unit.h"
#include <glm/gtc/constants.hpp>

class Worker : public Unit {
public:
    Worker(glm::vec3 pos, Model* m, int owner)
        : Unit(pos, EntityType::Worker, m, owner, owner == 2 ? 5.8f : 6.2f)
    {
        if (owner == 2)
        {
            SetAnimationNames("Stand1", "Run");
            SetBaseHeightOffset(3.5f);
            SetRotationEuler(glm::vec3(-glm::half_pi<float>(), 0.0f, 0.0f));
        }
        else
        {
            SetAnimationNames("CharacterArmature|Idle_Neutral", "CharacterArmature|Walk");
            SetBaseHeightOffset(1.6f);
        }
    }

    void Update(float dt) override;
};
