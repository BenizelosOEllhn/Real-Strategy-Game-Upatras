#pragma once
#include "GameEntity.h"
#include <vector>
#include <string>
#include <glm/vec4.hpp>

class Unit : public GameEntity {
public:
    float speed = 6.0f;

    Unit(glm::vec3 pos, EntityType t, Model* model, int owner, float scale = 8.0f)
        : GameEntity(pos, t, model, owner, scale),
          targetPosition(pos)
    {
        health_ = maxHealth_;
        selectAnimationIndices();
        updateActiveAnimationForState();
    }

    virtual void Update(float dt) override;
    virtual void Draw(Shader& shader) override;

    virtual ~Unit();

    void SetMoveTarget(const glm::vec3& target);
    void SetPath(const std::vector<glm::vec3>& path);
    bool HasMoveTarget() const { return hasMoveTarget_; }
    void ClearMoveTarget();
    float GetHealth() const { return health_; }
    float GetMaxHealth() const { return maxHealth_; }
    void SetHealth(float value);
    enum class TaskState
    {
        Idle,
        Moving,
        Gathering,
        Combat
    };
    void SetTaskState(TaskState state);
    TaskState GetTaskState() const { return taskState_; }
    bool UsesSkinning() const { return useSkinning_; }
    const std::vector<glm::mat4>& GetBoneMatrices() const { return boneTransforms_; }
    unsigned int GetBoneTexture() const { return boneTexture_; }
    int GetBoneCount() const { return static_cast<int>(boneTransforms_.size()); }
    void SetAnimationNames(const std::string& idle, const std::string& walk);
    void SetActionAnimation(const std::string& name);
    void ClearActionAnimation();
    void SetBaseHeightOffset(float offset) { baseHeightOffset_ = offset; }
    void FreezeAnimation(bool freeze, double timeSeconds = 0.0);

protected:
    glm::vec3 targetPosition;
    bool hasMoveTarget_ = false;
    float maxHealth_ = 100.0f;
    float health_ = 100.0f;
    float animationTime_ = 0.0f;
    std::vector<glm::vec3> pathPoints_;
    size_t pathCursor_ = 0;
    bool followingPath_ = false;
    TaskState taskState_ = TaskState::Idle;

    void advanceToNextPathPoint();
    void handleArrival();
    void updateAnimation(float dt);
    void ensureBoneCapacity();
    void selectAnimationIndices();
    void updateActiveAnimationForState();
    void setActionAnimationInternal(const std::string& name);
    void ensureBoneGPUCapacity(size_t count);
    void uploadBonePalette();

    glm::vec3 velocity_{0.0f};
    float maxAcceleration_ = 20.0f;
    float arrivalRadius_ = 3.0f;
    bool useSkinning_ = false;
    size_t activeAnimationIndex_ = 0;
    double animationTimeSeconds_ = 0.0;
    std::vector<glm::mat4> boneTransforms_;
    int idleAnimIndex_ = -1;
    int walkAnimIndex_ = -1;
    std::string idleAnimName_ = "Idle";
    std::string walkAnimName_ = "Walk";
    std::string actionAnimName_;
    int actionAnimIndex_ = -1;
    unsigned int boneTexture_ = 0;
    unsigned int boneBuffer_ = 0;
    size_t boneCapacity_ = 0;
    float baseHeightOffset_ = 0.0f;
    bool freezeAnimation_ = false;
    double frozenAnimationTime_ = 0.0;
};
