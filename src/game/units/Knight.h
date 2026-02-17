#pragma once
#include "Unit.h"
#include <glm/gtc/constants.hpp>

class Knight : public Unit {
public:
    Knight(glm::vec3 pos, Model* m, int owner)
        : Unit(pos, EntityType::Knight, m, owner, owner == 2 ? 1.8f : 0.8f)
    {
        if (owner == 2)
        {
            SetAnimationNames("Walk", "Walk");
            SetBaseHeightOffset(3.5f);
        }
        else
        {
            SetAnimationNames("Idle", "Walk");
            SetBaseHeightOffset(9.2f);
        }
    }

    void Update(float dt) override;

    bool ReadyToStrike() const { return attackCooldown_ <= 0.0f; }
    void ResetAttackTimer() { attackCooldown_ = attackInterval_; }
    float AttackRange() const { return attackRange_; }
    float AttackDamage() const { return attackDamage_; }

private:
    float attackCooldown_ = 0.0f;
    const float attackInterval_ = 1.0f;
    const float attackRange_ = 5.5f;
    const float attackDamage_ = 35.0f;
    float skeletonAnimClock_ = 0.0f;
    bool skeletonWasMoving_ = false;
};

