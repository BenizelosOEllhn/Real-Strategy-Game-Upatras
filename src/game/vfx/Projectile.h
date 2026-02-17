#pragma once

#include <glm/glm.hpp>
#include <memory>

#include "../entities/Unit.h"
#include "../entities/Building.h"
#include <glm/gtc/constants.hpp>

// Forward declarations
class Unit;
class Building;

enum class ProjectileType {
    Arrow,
    Fireball
};

struct Projectile {
    ProjectileType type;
    glm::vec3 position;
    glm::vec3 startPos;
    glm::vec3 targetPos;
    glm::vec3 velocity;
    Unit* sourceUnit = nullptr;           // The unit that fired this projectile
    Unit* targetUnit = nullptr;            // The unit being targeted (if any)
    Building* targetBuilding = nullptr;    // The building being targeted (if any)
    float speed = 30.0f;                   // Units per second
    float lifetime = 10.0f;                // Max lifetime in seconds
    float elapsedTime = 0.0f;              // Time elapsed
    float damage = 0.0f;                   // Damage to apply
    bool hasHit = false;                   // Whether projectile has already dealt damage
    
    // Animation/visual properties
    float scale = 1.0f;
    float rotation = 0.0f;                 // Yaw rotation for arrow
    
    Projectile(ProjectileType t, const glm::vec3& start, Unit* src, float dmg)
        : type(t), position(start), startPos(start), sourceUnit(src), damage(dmg)
    {
        if (t == ProjectileType::Arrow)
            speed = 40.0f;
        else if (t == ProjectileType::Fireball)
            speed = 25.0f;
    }
    
    // Update projectile position and check for hits
    bool Update(float dt, bool applyDamage);
    
    // Set target unit
    void SetTargetUnit(Unit* target) { targetUnit = target; }
    
    // Set target building
    void SetTargetBuilding(Building* target) { targetBuilding = target; }
    
    // Set explicit target position
    void SetTargetPosition(const glm::vec3& target) { targetPos = target; }
    
    // Check if projectile is expired
    bool IsExpired() const { return elapsedTime >= lifetime || hasHit; }
    
    // Get direction towards target
    glm::vec3 GetDirection() const;
    
    // Check if reached target
    bool HasReachedTarget() const;
};
