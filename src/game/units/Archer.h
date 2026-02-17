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
            SetBaseHeightOffset(2.1f);
        }
        else
        {
            SetAnimationNames("Attack", "Attack");
            SetBaseHeightOffset(8.7f);
            SetAutoFreezeWhenIdle(true);
            FreezeAnimation(true, 0.0);
        }
    }

    void Update(float dt) override;
    
    bool ReadyToStrike() const { return attackCooldown_ <= 0.0f; }
    void ResetAttackTimer() { attackCooldown_ = attackInterval_; }
    float AttackRange() const { return isEvil_ ? 43.0f : 55.0f; }  // Ranged, longer than knight
    float AttackDamage() const { return isEvil_ ? 30.0f : 25.0f; }
    
    // Projectile spawn offset (from unit position to fire point)
    glm::vec3 GetProjectileSpawnOffset() const
    {
        return glm::vec3(0.0f, 3.0f, 0.0f);  // Spawn at chest/bow height
    }

private:
    float attackCooldown_ = 0.0f;
    const float attackInterval_ = 1.6f;  // Slower ranged rate
    bool isEvil_ = false;
    
    void CalculateIsEvil() { isEvil_ = (ownerID == 2); }
    
    friend class Scene;  // Allow Scene to set isEvil during initialization
};
