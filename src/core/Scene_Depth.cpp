#include "Scene.h"
void Scene::DrawDepth(Shader& depthShader, const glm::mat4& lightSpaceMatrix)
{
    if (!terrain) return;

    depthShader.Use();
    depthShader.SetMat4("lightSpaceMatrix", lightSpaceMatrix);

    // Terrain
    depthShader.SetBool("isInstanced", false);
    depthShader.SetMat4("model", glm::mat4(1.0f));
    terrain->Draw(depthShader.ID);

    // Trees
    if (treeModel && !treeTransforms.empty())
    {
        depthShader.SetBool("isInstanced", true);
        treeModel->DrawInstanced(depthShader, treeTransforms);
    }

    // Rocks
    if (rockModel && !rockTransforms.empty())
    {
        depthShader.SetBool("isInstanced", true);
        rockModel->DrawInstanced(depthShader, rockTransforms);
    }

    // Buildings
    if (!entities_.empty())
    {
        depthShader.SetBool("isInstanced", false);
        for (GameEntity* e : entities_)
        {
            if (!e || !e->model) continue;
            depthShader.SetMat4("model", e->transform);
            e->model->Draw(depthShader); 
        }
    }
}
