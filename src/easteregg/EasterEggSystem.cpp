#include "EasterEggSystem.h"

#include <GL/glew.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <thread>
#include <cstdlib>

#include "../core/Scene.h"
#include "../core/AssetPath.h"
#include "../gui/UIManager.h"
#include "../game/entities/GameEntity.h"
#include "../game/data/EntityType.h"
#include "../rendering/terrain/Terrain.h"

EasterEggSystem::EasterEggSystem()
    : rng_(std::random_device{}())
{
}

void EasterEggSystem::Init(Scene* scene, Terrain* terrain, UIManager* ui, int screenW, int screenH)
{
    scene_ = scene;
    terrain_ = terrain;
    ui_ = ui;

    if (!scene_ || !terrain_ || !ui_)
        return;

    findPeak();

    originalOceanY_ = scene_->GetOceanY();
    originalLakeY_ = scene_->GetLakeY();
    originalRiverY_ = scene_->GetRiverY();

    targetSeaLevel_ = peakHeight_ - 0.8f;
    lastWaterRebuildY_ = originalOceanY_;

    setupUI(screenW, screenH);
    loadPoemText();

    if (leftLabelIndex_ != SIZE_MAX)
        ui_->setLabelText(leftLabelIndex_, poemLeft_);
    if (rightLabelIndex_ != SIZE_MAX)
        ui_->setLabelText(rightLabelIndex_, poemRight_);
}

void EasterEggSystem::Update(float dt, const std::vector<GameEntity*>& entities)
{
    if (!scene_ || !terrain_ || !ui_)
        return;

    if (!triggered_)
    {
        const float radiusSq = peakTriggerRadius_ * peakTriggerRadius_;
        for (const GameEntity* entity : entities)
        {
            if (!entity || entity->type != EntityType::Worker)
                continue;

            float dx = entity->position.x - peakPos_.x;
            float dz = entity->position.z - peakPos_.z;
            float distSq = dx * dx + dz * dz;
            if (distSq <= radiusSq)
            {
                triggered_ = true;
                thunderTimer_ = 0.2f;
                nextThunderDelay_ = 0.8f;
                break;
            }
        }
    }

    if (!triggered_)
        return;

    updateFlood(dt);

    if (!floodComplete_)
        updateThunder(dt);

    if (floodComplete_ && !poemShown_)
    {
        showPoem();
        playBirdSound();
        poemShown_ = true;
    }
}

void EasterEggSystem::findPeak()
{
    peakHeight_ = -1.0e6f;
    peakPos_ = glm::vec3(0.0f);

    const auto& vertices = terrain_->GetVertices();
    for (const auto& vertex : vertices)
    {
        if (vertex.Position.y > peakHeight_)
        {
            peakHeight_ = vertex.Position.y;
            peakPos_ = vertex.Position;
        }
    }
}

void EasterEggSystem::setupUI(int screenW, int screenH)
{
    const float blockW = screenW * 0.42f;
    const float blockH = screenH * 0.55f;
    const float blockY = screenH * 0.22f;
    const float gap = screenW * 0.04f;
    const float leftX = screenW * 0.06f;
    const float rightX = leftX + blockW + gap;

    UIButton leftBlock;
    leftBlock.pos = glm::vec2(leftX, blockY);
    leftBlock.size = glm::vec2(blockW, blockH);
    leftBlock.texture = 0;
    leftBlock.onClick = nullptr;
    leftBlock.clickable = false;
    leftBlock.visible = false;

    UIButton rightBlock = leftBlock;
    rightBlock.pos = glm::vec2(rightX, blockY);

    leftBlockIndex_ = ui_->addButton(leftBlock);
    rightBlockIndex_ = ui_->addButton(rightBlock);

    const float textPadding = 18.0f;
    const float labelScale = 1.35f;

    leftLabelIndex_ = ui_->addLabel("", glm::vec2(leftX + textPadding, blockY + blockH - textPadding), labelScale);
    rightLabelIndex_ = ui_->addLabel("", glm::vec2(rightX + textPadding, blockY + blockH - textPadding), labelScale);

    if (leftLabelIndex_ != SIZE_MAX)
        ui_->setLabelVisibility(leftLabelIndex_, false);
    if (rightLabelIndex_ != SIZE_MAX)
        ui_->setLabelVisibility(rightLabelIndex_, false);

    ensureFlashTexture();

    UIButton flash;
    flash.pos = glm::vec2(0.0f, 0.0f);
    flash.size = glm::vec2((float)screenW, (float)screenH);
    flash.texture = flashTexture_;
    flash.onClick = nullptr;
    flash.clickable = false;
    flash.visible = true;

    flashIndex_ = ui_->addButton(flash);
    if (flashIndex_ != SIZE_MAX)
        ui_->setButtonTint(flashIndex_, glm::vec4(1.0f, 1.0f, 1.0f, 0.0f));
}

void EasterEggSystem::loadPoemText()
{
    std::string path = AssetPath("easteregg/poem.txt");
    std::ifstream file(path);
    if (!file)
    {
        poemLeft_.clear();
        poemRight_.clear();
        return;
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line))
        lines.push_back(line);

    if (lines.size() < 50)
        lines.resize(50);

    std::ostringstream left;
    std::ostringstream right;
    for (size_t i = 0; i < 25 && i < lines.size(); ++i)
    {
        if (i) left << '\n';
        left << lines[i];
    }
    for (size_t i = 25; i < 50 && i < lines.size(); ++i)
    {
        if (i > 25) right << '\n';
        right << lines[i];
    }

    poemLeft_ = left.str();
    poemRight_ = right.str();
}

void EasterEggSystem::showPoem()
{
    if (leftBlockIndex_ != SIZE_MAX)
        ui_->setButtonVisibility(leftBlockIndex_, true);
    if (rightBlockIndex_ != SIZE_MAX)
        ui_->setButtonVisibility(rightBlockIndex_, true);

    if (leftLabelIndex_ != SIZE_MAX)
        ui_->setLabelVisibility(leftLabelIndex_, true);
    if (rightLabelIndex_ != SIZE_MAX)
        ui_->setLabelVisibility(rightLabelIndex_, true);
}

void EasterEggSystem::updateFlood(float dt)
{
    float current = scene_->GetOceanY();
    if (current >= targetSeaLevel_ - 0.02f)
    {
        if (!floodComplete_)
        {
            scene_->SetWaterLevels(targetSeaLevel_, targetSeaLevel_, targetSeaLevel_, true);
            floodComplete_ = true;
        }
        return;
    }

    float next = std::min(targetSeaLevel_, current + floodSpeed_ * dt);
    waterRebuildCooldown_ -= dt;

    bool rebuild = false;
    if (std::fabs(next - lastWaterRebuildY_) >= 0.35f || waterRebuildCooldown_ <= 0.0f)
    {
        rebuild = true;
        lastWaterRebuildY_ = next;
        waterRebuildCooldown_ = 0.25f;
    }

    scene_->SetWaterLevels(next, next, next, rebuild);
}

void EasterEggSystem::updateThunder(float dt)
{
    thunderTimer_ -= dt;
    if (thunderTimer_ <= 0.0f)
    {
        triggerThunderFlash();
        thunderTimer_ = nextThunderDelay_;
    }

    if (flashAlpha_ > 0.0f)
    {
        flashAlpha_ -= flashDecay_ * dt;
        if (flashAlpha_ < 0.0f)
            flashAlpha_ = 0.0f;
    }

    if (flashIndex_ != SIZE_MAX)
        ui_->setButtonTint(flashIndex_, glm::vec4(1.0f, 1.0f, 1.0f, flashAlpha_));
}

void EasterEggSystem::triggerThunderFlash()
{
    std::uniform_real_distribution<float> delayDist(0.35f, 1.2f);
    std::uniform_real_distribution<float> alphaDist(0.45f, 0.9f);
    std::uniform_real_distribution<float> decayDist(3.5f, 6.0f);

    nextThunderDelay_ = delayDist(rng_);
    flashAlpha_ = alphaDist(rng_);
    flashDecay_ = decayDist(rng_);
}

void EasterEggSystem::ensureFlashTexture()
{
    if (flashTexture_ != 0)
        return;

    unsigned char white[4] = { 255, 255, 255, 255 };
    glGenTextures(1, &flashTexture_);
    glBindTexture(GL_TEXTURE_2D, flashTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void EasterEggSystem::playBirdSound() const
{
#ifdef __APPLE__
    const std::string path = AssetPath("audio/Bird Flying Away - Sound Effect.mp3");
    std::thread([path]() {
        std::string command = "afplay -t 3 \"" + path + "\" >/dev/null 2>&1";
        std::system(command.c_str());
    }).detach();
#endif
}
