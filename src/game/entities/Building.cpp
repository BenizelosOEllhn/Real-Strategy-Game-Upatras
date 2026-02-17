#include "Building.h"

void Building::Draw(Shader& shader)
{
    if (!finalModel) return;

    shader.Use();
    shader.SetMat4("model", transform);
    shader.SetBool("uUseSkinning", false);
    shader.BindBoneTexture(0, 0);

    if (isUnderConstruction)
    {
        const bool hasDistinctFoundation = foundationModel && foundationModel != finalModel;

        // Fade the foundation out as progress approaches 1.
        if (hasDistinctFoundation)
        {
            shader.SetFloat("uAlpha", 1.0f - buildProgress);
            foundationModel->Draw(shader);
        }

        // Fade the finished building in.
        shader.SetFloat("uAlpha", buildProgress);
        finalModel->Draw(shader);
    }
    else if (upgradeInProgress_ && upgradeFromModel_ && upgradeFromModel_ != finalModel)
    {
        float t = glm::clamp(upgradeProgress_, 0.0f, 1.0f);
        shader.SetFloat("uAlpha", 1.0f - t);
        upgradeFromModel_->Draw(shader);
        shader.SetFloat("uAlpha", t);
        finalModel->Draw(shader);
    }
    else
    {
        shader.SetFloat("uAlpha", 1.0f);
        finalModel->Draw(shader);
    }
}

void Building::Update(float dt)
{
    if (!isUnderConstruction)
    {
        if (upgradeInProgress_)
        {
            upgradeProgress_ = glm::clamp(upgradeProgress_ + dt / upgradeDuration_, 0.0f, 1.0f);
            if (upgradeProgress_ >= 1.0f)
            {
                upgradeInProgress_ = false;
                upgradeFromModel_ = nullptr;
            }
        }
        return;
    }

    buildProgress = glm::clamp(buildProgress + dt / buildTime, 0.0f, 1.0f);
    if (buildProgress >= 1.0f) {
        isUnderConstruction = false;
    }
}
