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
    depthShader.SetBool("uUseSkinning", false);
    depthShader.BindBoneTexture(0, 0);

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
        depthShader.SetBool("uUseSkinning", false);
        depthShader.BindBoneTexture(0, 0);

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
        depthShader.SetBool("uUseSkinning", false);
        depthShader.BindBoneTexture(0, 0);

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
        if (e->ownerID > 0 &&
            e->ownerID != activePlayerIndex_ + 1 &&
            !isPositionVisibleToPlayer(e->position, activePlayerIndex_ + 1))
            continue;

        depthShader.SetMat4("model", e->transform);

        if (Unit* unit = dynamic_cast<Unit*>(e))
        {
            bool useSkin = unit->UsesSkinning();
            depthShader.SetBool("uUseSkinning", useSkin);
            if (useSkin)
                depthShader.BindBoneTexture(unit->GetBoneTexture(), unit->GetBoneCount());
            else
                depthShader.BindBoneTexture(0, 0);
            if (unit->model)
                unit->model->Draw(depthShader);
        }
        else if (Building* b = dynamic_cast<Building*>(e))
        {
            depthShader.BindBoneTexture(0, 0);
            if (b->isUnderConstruction && b->foundationModel)
            {
                depthShader.SetBool("uUseSkinning", false);
                b->foundationModel->Draw(depthShader);
            }
            else if (b->finalModel)
            {
                depthShader.SetBool("uUseSkinning", false);
                b->finalModel->Draw(depthShader);
            }
        }
        else
        {
            depthShader.SetBool("uUseSkinning", false);
            depthShader.BindBoneTexture(0, 0);
            if (e->model) e->model->Draw(depthShader);
        }
    }
    }

}
