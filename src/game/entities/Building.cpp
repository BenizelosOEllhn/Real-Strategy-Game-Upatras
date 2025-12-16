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
    else
    {
        shader.SetFloat("uAlpha", 1.0f);
        finalModel->Draw(shader);
    }
}
