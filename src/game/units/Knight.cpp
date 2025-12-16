#include "Knight.h"
#include <algorithm>

void Knight::Update(float dt)
{
    attackCooldown_ = std::max(0.0f, attackCooldown_ - dt);
    Unit::Update(dt);
}
