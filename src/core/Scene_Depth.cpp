#include "Scene.h"

void Scene::DrawDepth(Shader& depthShader,
                      const glm::mat4& lightSpaceMatrix)
{
    if (!terrain) return;

    depthShader.Use();
    depthShader.SetMat4("lightSpaceMatrix", lightSpaceMatrix);

    // ============================================================
    // 1) TERRAIN (static)
    // ============================================================
    depthShader.SetBool("isInstanced", false);
    depthShader.SetBool("uUnderConstruction", false);
    depthShader.SetFloat("uBuildProgress", 1.0f);

    depthShader.SetMat4("model", glm::mat4(1.0f));
    terrain->Draw(depthShader.ID);

    // ============================================================
    // 2) TREES (instanced)
    // ============================================================
    if (treeModel && !treeTransforms.empty())
    {
        depthShader.SetBool("isInstanced", true);
        depthShader.SetBool("uUnderConstruction", false);
        depthShader.SetFloat("uBuildProgress", 1.0f);

        treeModel->DrawInstanced(depthShader, treeTransforms);
    }

    // ============================================================
    // 3) ROCKS (instanced)
    // ============================================================
    if (rockModel && !rockTransforms.empty())
    {
        depthShader.SetBool("isInstanced", true);
        depthShader.SetBool("uUnderConstruction", false);
        depthShader.SetFloat("uBuildProgress", 1.0f);

        rockModel->DrawInstanced(depthShader, rockTransforms);
    }

    // ============================================================
    // 4) BUILDINGS (non-instanced)
    // ============================================================
    if (!entities_.empty())
    {
    depthShader.SetBool("isInstanced", false);

    for (GameEntity* e : entities_)
    {
        if (!e) continue;

        depthShader.SetMat4("model", e->transform);

        if (Building* b = dynamic_cast<Building*>(e))
        {
            if (b->isUnderConstruction && b->foundationModel)
                b->foundationModel->Draw(depthShader);
            else if (b->finalModel)
                b->finalModel->Draw(depthShader);
        }
        else
        {
            if (e->model) e->model->Draw(depthShader);
        }
    }
    }

}
