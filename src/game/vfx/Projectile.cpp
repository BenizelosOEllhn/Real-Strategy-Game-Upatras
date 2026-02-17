#include "Projectile.h"

glm::vec3 Projectile::GetDirection() const
{
    glm::vec3 dir = targetPos - position;
    float dist = glm::length(dir);
    if (dist > 0.001f)
        return dir / dist;
    return glm::vec3(0.0f, 0.0f, 1.0f);
}

bool Projectile::HasReachedTarget() const
{
    float dist = glm::distance(position, targetPos);
    // Different proximity thresholds for different projectile types
    float threshold = (type == ProjectileType::Arrow) ? 1.5f : 2.0f;
    return dist <= threshold;
}

bool Projectile::Update(float dt)
{
    if (hasHit || IsExpired())
        return false;
    
    elapsedTime += dt;
    
    // Update target position if targeting a moving unit
    if (targetUnit)
    {
        targetPos = targetUnit->position;
    }
    else if (targetBuilding)
    {
        targetPos = targetBuilding->position;
    }
    
    // Move towards target
    glm::vec3 dir = GetDirection();
    velocity = dir * speed;  // Store velocity for rendering orientation
    position += velocity * dt;
    
    // Update rotation for arrow (point towards direction of travel)
    if (type == ProjectileType::Arrow)
    {
        // Calculate yaw to face movement direction
        if (glm::length(dir) > 0.001f)
        {
            rotation = atan2(dir.x, dir.z);
        }
    }
    
    // Check if reached target
    if (HasReachedTarget())
    {
        hasHit = true;
        position = targetPos;  // Snap to exact target position
        
        // Deal damage
        if (targetUnit)
        {
            float newHealth = targetUnit->GetHealth() - damage;
            targetUnit->SetHealth(newHealth);
        }
        else if (targetBuilding)
        {
            targetBuilding->ApplyDamage(damage);
        }
        
        return true;  // Signal that projectile hit
    }
    
    return false;
}
