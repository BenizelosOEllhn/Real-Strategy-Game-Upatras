#pragma once

#include <cstddef>
#include <string>
#include <vector>
#include <random>
#include <glm/glm.hpp>

class Scene;
class Terrain;
class UIManager;
class GameEntity;

class EasterEggSystem
{
public:
    EasterEggSystem();
    void Init(Scene* scene, Terrain* terrain, UIManager* ui, int screenW, int screenH);
    void Update(float dt, const std::vector<GameEntity*>& entities);

private:
    void findPeak();
    void setupUI(int screenW, int screenH);
    void loadPoemText();
    void showPoem();
    void updateFlood(float dt);
    void updateThunder(float dt);
    void triggerThunderFlash();
    void ensureFlashTexture();
    void playBirdSound() const;

    Scene* scene_ = nullptr;
    Terrain* terrain_ = nullptr;
    UIManager* ui_ = nullptr;

    glm::vec3 peakPos_{0.0f};
    float peakHeight_ = 0.0f;
    float peakTriggerRadius_ = 18.0f;

    bool triggered_ = false;
    bool floodComplete_ = false;
    bool poemShown_ = false;

    float targetSeaLevel_ = 0.0f;
    float floodSpeed_ = 3.0f;
    float originalOceanY_ = 0.0f;
    float originalLakeY_ = 0.0f;
    float originalRiverY_ = 0.0f;

    float waterRebuildCooldown_ = 0.0f;
    float lastWaterRebuildY_ = 0.0f;

    size_t leftBlockIndex_ = SIZE_MAX;
    size_t rightBlockIndex_ = SIZE_MAX;
    size_t leftLabelIndex_ = SIZE_MAX;
    size_t rightLabelIndex_ = SIZE_MAX;
    size_t flashIndex_ = SIZE_MAX;

    unsigned int flashTexture_ = 0;
    float flashAlpha_ = 0.0f;
    float flashDecay_ = 0.0f;
    float thunderTimer_ = 0.0f;
    float nextThunderDelay_ = 0.0f;

    std::string poemLeft_;
    std::string poemRight_;

    std::mt19937 rng_;
};
