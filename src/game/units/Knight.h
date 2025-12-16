#pragma once
#include "Unit.h"

class Knight : public Unit {
public:
    Knight(glm::vec3 pos, Model* m, int owner)
        : Unit(pos, EntityType::Knight, m, owner, 0.8f)
    {
        SetAnimationNames("Idle", "Walk");
        SetBaseHeightOffset(4.5f);
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
};
