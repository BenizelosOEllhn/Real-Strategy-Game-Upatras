#include "Knight.h"
#include <algorithm>
#include <cmath>

void Knight::Update(float dt)
{
    attackCooldown_ = std::max(0.0f, attackCooldown_ - dt);

    if (ownerID == 2 && model && model->HasAnimations())
    {
        const float ticksPerSecond = 25.0f;
        const float walkStart = 0.0f / ticksPerSecond;
        const float walkEnd = 120.0f / ticksPerSecond;
        const float idleStart = 150.0f / ticksPerSecond;
        const float idleEnd = 250.0f / ticksPerSecond;

        const bool moving = (taskState_ == TaskState::Moving) || hasMoveTarget_;
        const bool hasAction = !actionAnimName_.empty() && actionAnimIndex_ >= 0;

        if (!hasAction)
        {
            if (moving != skeletonWasMoving_)
            {
                skeletonAnimClock_ = 0.0f;
                skeletonWasMoving_ = moving;
            }

            const float rangeStart = moving ? walkStart : idleStart;
            const float rangeEnd = moving ? walkEnd : idleEnd;
            const float rangeDuration = std::max(0.001f, rangeEnd - rangeStart);

            skeletonAnimClock_ = std::fmod(skeletonAnimClock_ + dt, rangeDuration);
            FreezeAnimation(true, rangeStart + skeletonAnimClock_);
        }
        else
        {
            FreezeAnimation(false);
        }
    }

    Unit::Update(dt);
}
