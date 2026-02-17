#include "Archer.h"

void Archer::Update(float dt)
{
    Unit::Update(dt);
    
    // Decrement attack cooldown
    if (attackCooldown_ > 0.0f)
        attackCooldown_ -= dt;
    
    // Calculate evil status once during init
    if (!isEvil_ && ownerID == 2)
        isEvil_ = true;
}
