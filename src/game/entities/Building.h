#pragma once
#include "GameEntity.h"
#include <algorithm>

class Building : public GameEntity {
public:
    Model* foundationModel = nullptr;
    Model* finalModel      = nullptr;

    bool  isUnderConstruction = true;
    float buildProgress = 0.0f;
    float buildTime = 6.0f;

    Building(glm::vec3 pos, EntityType t, Model* foundation, Model* final, int owner)
        : GameEntity(pos, t, final, owner),
          foundationModel(foundation),
          finalModel(final)
    {}

    void Update(float dt) override;

    void Draw(Shader& shader) override;           
    virtual void SpawnUnit(std::vector<GameEntity*>& entities) = 0;

    int GetLevel() const { return level_; }
    virtual int MaxLevel() const { return 2; }
    bool CanUpgrade() const { return !isUnderConstruction && level_ < MaxLevel(); }
    bool UpgradeLevel()
    {
        if (!CanUpgrade())
            return false;
        ++level_;
        OnUpgradeApplied(level_);
        return true;
    }

    void SetMaxHealth(float value)
    {
        maxHealth_ = std::max(0.0f, value);
        health_ = std::min(health_, maxHealth_);
    }
    void RestoreFullHealth() { health_ = maxHealth_; }

    float GetHealth() const { return health_; }
    float GetMaxHealth() const { return maxHealth_; }
    void SetHealth(float value)
    {
        health_ = std::clamp(value, 0.0f, maxHealth_);
    }
    void ApplyDamage(float amount)
    {
        if (amount <= 0.0f) return;
        health_ = std::max(0.0f, health_ - amount);
    }
    bool IsDestroyed() const { return health_ <= 0.0f; }

    void StartUpgradeTransition(Model* newModel, float duration = 1.0f)
    {
        if (!newModel)
            return;
        if (finalModel == newModel)
        {
            model = newModel;
            foundationModel = newModel;
            return;
        }

        upgradeFromModel_ = finalModel;
        finalModel = newModel;
        model = newModel;
        foundationModel = newModel;
        upgradeDuration_ = std::max(0.01f, duration);
        upgradeProgress_ = 0.0f;
        upgradeInProgress_ = (upgradeFromModel_ != nullptr);
    }

protected:
    virtual void OnUpgradeApplied(int /*newLevel*/) {}
    float maxHealth_ = 600.0f;
    float health_ = 600.0f;
    int level_ = 1;

    Model* upgradeFromModel_ = nullptr;
    float upgradeProgress_ = 0.0f;
    float upgradeDuration_ = 1.0f;
    bool upgradeInProgress_ = false;
};
