#include "Unit.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <GL/glew.h>
#include <iostream>

#include <algorithm>
#include <cmath>

Unit::~Unit()
{
    if (boneTexture_ != 0)
        glDeleteTextures(1, &boneTexture_);
    if (boneBuffer_ != 0)
        glDeleteBuffers(1, &boneBuffer_);
}

void Unit::Update(float dt)
{
    animationTime_ += dt;

    if (hasMoveTarget_)
    {
        glm::vec3 toTarget = targetPosition - position;
        float dist = glm::length(toTarget);
        if (dist < 0.2f)
        {
            position = targetPosition;
            advanceToNextPathPoint();
        }
        else
        {
            glm::vec3 dir = glm::normalize(toTarget);
            float desiredSpeed = speed;
            if (dist < arrivalRadius_)
                desiredSpeed *= (dist / arrivalRadius_);
            glm::vec3 desiredVel = dir * desiredSpeed;
            glm::vec3 steering = desiredVel - velocity_;
            float maxStep = maxAcceleration_ * dt;
            if (glm::length(steering) > maxStep)
                steering = glm::normalize(steering) * maxStep;
            velocity_ += steering;
            float vLen = glm::length(velocity_);
            if (vLen > speed)
                velocity_ = (velocity_ / vLen) * speed;

            position += velocity_ * dt;

            if (glm::length(velocity_) > 0.01f)
            {
                glm::vec3 flat = glm::normalize(glm::vec3(velocity_.x, 0.0f, velocity_.z));
                float desiredYaw = std::atan2(flat.x, flat.z);
                SetYaw(desiredYaw);
            }
        }
    }
    else if (taskState_ == TaskState::Moving)
    {
        SetTaskState(TaskState::Idle);
        velocity_ = glm::vec3(0.0f);
    }

    float animSpeed = hasMoveTarget_ ? 8.0f : 4.0f;
    float bobAmount = hasMoveTarget_ ? 0.35f : 0.2f;
    float bob = std::sin(animationTime_ * animSpeed) * bobAmount;
    SetVisualOffset(glm::vec3(0.0f, baseHeightOffset_ + bob, 0.0f));
    RebuildTransform();

    updateAnimation(dt);
}

void Unit::SetMoveTarget(const glm::vec3& target)
{
    targetPosition = target;
    hasMoveTarget_ = true;
    followingPath_ = false;
    pathPoints_.clear();
    pathCursor_ = 0;
    velocity_ = glm::vec3(0.0f);
}

void Unit::SetHealth(float value)
{
    health_ = std::clamp(value, 0.0f, maxHealth_);
}

void Unit::ClearMoveTarget()
{
    hasMoveTarget_ = false;
    followingPath_ = false;
    pathPoints_.clear();
    pathCursor_ = 0;
    velocity_ = glm::vec3(0.0f);
    if (taskState_ == TaskState::Moving)
        SetTaskState(TaskState::Idle);
}

void Unit::SetPath(const std::vector<glm::vec3>& path)
{
    pathPoints_ = path;
    if (!pathPoints_.empty() && glm::distance(pathPoints_.front(), position) < 0.25f)
    {
        pathPoints_.erase(pathPoints_.begin());
    }

    pathCursor_ = 0;
    followingPath_ = !pathPoints_.empty();
    if (followingPath_)
    {
        targetPosition = pathPoints_[0];
        hasMoveTarget_ = true;
    }
    else
    {
        hasMoveTarget_ = false;
    }
}

void Unit::SetTaskState(TaskState state)
{
    if (taskState_ == state)
        return;
    taskState_ = state;
    updateActiveAnimationForState();
}

void Unit::advanceToNextPathPoint()
{
    if (followingPath_ && pathCursor_ + 1 < pathPoints_.size())
    {
        ++pathCursor_;
        targetPosition = pathPoints_[pathCursor_];
        hasMoveTarget_ = true;
    }
    else
    {
        pathPoints_.clear();
        pathCursor_ = 0;
        followingPath_ = false;
        hasMoveTarget_ = false;
        handleArrival();
        velocity_ = glm::vec3(0.0f);
    }
}

void Unit::handleArrival()
{
    if (taskState_ == TaskState::Moving)
        SetTaskState(TaskState::Idle);
}

void Unit::updateAnimation(float dt)
{
    if (!model || !model->HasAnimations())
    {
        useSkinning_ = false;
        return;
    }

    if (!useSkinning_)
    {
        useSkinning_ = true;
        ensureBoneCapacity();
    }

    animationTimeSeconds_ += static_cast<double>(dt);
    model->EvaluateAnimation(activeAnimationIndex_, animationTimeSeconds_, boneTransforms_);
    uploadBonePalette();
}

void Unit::ensureBoneCapacity()
{
    if (!model) return;
    size_t count = model->GetBoneCount();
    if (boneTransforms_.size() != count)
        boneTransforms_.assign(count, glm::mat4(1.0f));
    ensureBoneGPUCapacity(count);
}

void Unit::selectAnimationIndices()
{
    if (!model || !model->HasAnimations())
    {
        idleAnimIndex_ = -1;
        walkAnimIndex_ = -1;
        useSkinning_ = false;
        return;
    }

    idleAnimIndex_ = model->FindAnimationIndex(idleAnimName_);
    walkAnimIndex_ = model->FindAnimationIndex(walkAnimName_);
    if (idleAnimIndex_ < 0 && model->GetAnimationCount() > 0)
        idleAnimIndex_ = 0;
    if (walkAnimIndex_ < 0)
        walkAnimIndex_ = idleAnimIndex_;

    activeAnimationIndex_ = static_cast<size_t>(std::max(0, idleAnimIndex_));
}

void Unit::updateActiveAnimationForState()
{
    if (!model || !model->HasAnimations())
        return;

    int desired = idleAnimIndex_;
    if (!actionAnimName_.empty() && actionAnimIndex_ >= 0)
    {
        desired = actionAnimIndex_;
    }
    else if ((taskState_ == TaskState::Moving || hasMoveTarget_) && walkAnimIndex_ >= 0)
    {
        desired = walkAnimIndex_;
    }

    if (desired >= 0 && static_cast<size_t>(desired) != activeAnimationIndex_)
    {
        activeAnimationIndex_ = static_cast<size_t>(desired);
        animationTimeSeconds_ = 0.0;
    }
}

void Unit::SetAnimationNames(const std::string& idle, const std::string& walk)
{
    idleAnimName_ = idle;
    walkAnimName_ = walk;
    selectAnimationIndices();
    updateActiveAnimationForState();
}

void Unit::SetActionAnimation(const std::string& name)
{
    actionAnimName_ = name;
    if (model)
        actionAnimIndex_ = model->FindAnimationIndex(name);
    else
        actionAnimIndex_ = -1;
    updateActiveAnimationForState();
}

void Unit::ClearActionAnimation()
{
    actionAnimName_.clear();
    actionAnimIndex_ = -1;
    updateActiveAnimationForState();
}

void Unit::Draw(Shader& shader)
{
    shader.Use();
    shader.SetMat4("model", transform);
    bool canSkin = useSkinning_ && boneTexture_ != 0 && !boneTransforms_.empty();
    shader.SetFloat("uAlpha", 1.0f);
    shader.SetBool("uUseSkinning", canSkin);
    if (canSkin)
    {
        shader.BindBoneTexture(boneTexture_, static_cast<int>(boneTransforms_.size()));
    }
    else
    {
        shader.BindBoneTexture(0, 0);
        if (useSkinning_)
        {
            std::cout << "[Unit] Warning: skinning requested but bone texture missing for entity type "
                      << static_cast<int>(type) << "\n";
        }
    }

    model->Draw(shader);
}

void Unit::ensureBoneGPUCapacity(size_t count)
{
    if (count == 0)
        return;
    if (boneCapacity_ >= count)
        return;

    boneCapacity_ = count;
    if (boneBuffer_ == 0)
        glGenBuffers(1, &boneBuffer_);
    glBindBuffer(GL_TEXTURE_BUFFER, boneBuffer_);
    glBufferData(GL_TEXTURE_BUFFER, boneCapacity_ * sizeof(glm::mat4), nullptr, GL_DYNAMIC_DRAW);
    if (boneTexture_ == 0)
    {
        glGenTextures(1, &boneTexture_);
        std::cout << "[Unit] Created bone texture for entity type " << static_cast<int>(type)
                  << " with capacity " << boneCapacity_ << " matrices.\n";
    }
    glBindTexture(GL_TEXTURE_BUFFER, boneTexture_);
    glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, boneBuffer_);
}

void Unit::uploadBonePalette()
{
    if (!useSkinning_ || boneTransforms_.empty())
        return;
    ensureBoneGPUCapacity(boneTransforms_.size());
    glBindBuffer(GL_TEXTURE_BUFFER, boneBuffer_);
    glBufferSubData(GL_TEXTURE_BUFFER, 0,
                    boneTransforms_.size() * sizeof(glm::mat4),
                    boneTransforms_.data());
    if (boneTexture_ == 0)
    {
        std::cout << "[Unit] ERROR: Failed to create bone texture buffer for entity type "
                  << static_cast<int>(type) << "\n";
    }
}
