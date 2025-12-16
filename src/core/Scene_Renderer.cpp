#include "Scene.h"
#include "SceneConstants.h"

static inline glm::vec4 NoClip()
{
    return glm::vec4(0, 1, 0, 100000.0f);
}

void Scene::Draw(
    Shader& terrainShader,
    Shader& objectShader,
    glm::mat4 view,
    glm::mat4 projection,
    glm::vec3 lightPos,
    glm::vec3 viewPos,
    const glm::mat4& lightSpaceMatrix,
    unsigned int shadowMap)
{
    lastViewMatrix_ = view;
    lastProjMatrix_ = projection;

    // ------------------------------------------------------------
    // GLOBAL STATE RESET
    // ------------------------------------------------------------
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW); // keep default, water will disable culling anyway

    const glm::vec3 lightColor(1.0f, 0.97f, 0.92f);

    // ============================================================
    // 1) TERRAIN
    // ============================================================
    if (terrain)
    {
        terrainShader.Use();
        terrainShader.SetMat4("view", view);
        terrainShader.SetMat4("projection", projection);
        terrainShader.SetMat4("model", glm::mat4(1.0f));
        terrainShader.SetMat4("lightSpaceMatrix", lightSpaceMatrix);
        terrainShader.SetVec4("uClipPlane", NoClip());
        terrainShader.BindBoneTexture(0, 0);

        grass1Tex->Bind(0);
        grass2Tex->Bind(1);
        grass3Tex->Bind(2);
        noiseTex->Bind(3);
        rockTex->Bind(4);
        sandTex->Bind(5);
        peakTex->Bind(6);

        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, shadowMap);

        terrainShader.SetInt("grass1", 0);
        terrainShader.SetInt("grass2", 1);
        terrainShader.SetInt("grass3", 2);
        terrainShader.SetInt("noiseDetail", 3);
        terrainShader.SetInt("textureRock", 4);
        terrainShader.SetInt("sandTex", 5);
        terrainShader.SetInt("texturePeak", 6);
        terrainShader.SetInt("shadowMap", 7);

        terrainShader.SetVec3("lightPos", lightPos);
        terrainShader.SetVec3("viewPos", viewPos);
        terrainShader.SetVec3("lightColor", lightColor);

        terrain->Draw(terrainShader.ID);
    }

    // ============================================================
    // 2) TREES / ROCKS (INSTANCED)
    // ============================================================
    bool hasTrees = (treeModel && treeTex && !treeTransforms.empty());
    bool hasRocks = (rockModel && boulderTex && !rockTransforms.empty());
    if (hasTrees || hasRocks)
    {
        objectShader.Use();
        objectShader.SetMat4("view", view);
        objectShader.SetMat4("projection", projection);
        objectShader.SetMat4("lightSpaceMatrix", lightSpaceMatrix);
        objectShader.SetVec3("lightPos", lightPos);
        objectShader.SetVec3("viewPos", viewPos);
        objectShader.SetVec3("lightColor", lightColor);
        objectShader.SetVec4("uClipPlane", NoClip());
        objectShader.SetFloat("uAlpha", 1.0f);

        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, shadowMap);
        objectShader.SetInt("shadowMap", 7);
        objectShader.SetInt("texture_diffuse1", 0);

        // IMPORTANT: set these AFTER Use()
        objectShader.SetBool("isInstanced", true);
        objectShader.SetBool("useTexture", true);
        objectShader.SetVec3("uMaterialColor", glm::vec3(0,1,0));
        objectShader.SetBool("uUseSkinning", false);
        objectShader.BindBoneTexture(0, 0);
    }

    if (hasTrees)
    {
        treeTex->Bind(0);
        treeModel->DrawInstanced(objectShader, treeTransforms);
    }

    if (hasRocks)
    {
        boulderTex->Bind(0);
        rockModel->DrawInstanced(objectShader, rockTransforms);
    }

    // ============================================================
    // 3) BUILDINGS (NON-INSTANCED)
    // ============================================================
    if (!entities_.empty())
    {
        objectShader.Use();
        objectShader.SetMat4("view", view);
        objectShader.SetMat4("projection", projection);
        objectShader.SetMat4("lightSpaceMatrix", lightSpaceMatrix);
        objectShader.SetVec3("lightPos", lightPos);
        objectShader.SetVec3("viewPos", viewPos);
        objectShader.SetVec3("lightColor", lightColor);
        objectShader.SetVec4("uClipPlane", NoClip());
        objectShader.SetFloat("uAlpha", 1.0f);

        objectShader.SetBool("isInstanced", false);
        objectShader.SetBool("useTexture", false); // buildings use uMaterialColor
        objectShader.SetBool("uUseSkinning", false);
        objectShader.SetInt("texture_diffuse1", 0);
        objectShader.BindBoneTexture(0, 0);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        for (GameEntity* e : entities_)
        {
            if (!e) continue;
            e->Draw(objectShader);   // Building::Draw sets uAlpha
        }

        glDisable(GL_BLEND);
    }

    drawSelectionIndicators(view, projection);

    // ============================================================
    // 4) WATER (DISABLE CULLING OR IT VANISHES)
    // ============================================================
    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    DrawWater(view, projection, viewPos);
    DrawLakeWater(view, projection, viewPos);
    DrawRiverWater(view, projection, viewPos);

    glEnable(GL_CULL_FACE);

    // ============================================================
    // 5) PREVIEW (TRANSPARENT)
    // ============================================================
    if (buildingManager_.isPlacing() &&
        buildingManager_.hasPreview() &&
        buildingManager_.getPreviewModel())
    {
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        previewShader->Use();
        previewShader->SetMat4("view", view);
        previewShader->SetMat4("projection", projection);
        previewShader->BindBoneTexture(0, 0);

        glm::mat4 m = glm::translate(glm::mat4(1.0f), buildingManager_.getPreviewPos());
        m = glm::scale(m, glm::vec3(20.0f));
        previewShader->SetMat4("model", m);

        // Pulse alpha for fade in/out effect
        float time = (float)glfwGetTime();
        float alpha = 0.4f + 0.2f * sin(time * 2.0f); // slower pulse for longer fade

        glm::vec4 tint = buildingManager_.isValidPlacement()
            ? glm::vec4(0.1f, 1.0f, 0.1f, alpha)
            : glm::vec4(1.0f, 0.1f, 0.1f, alpha);

        // Your preview.frag uses "uTint" or "tint"? Make it match.
        previewShader->SetVec4("uTint", tint);

        buildingManager_.getPreviewModel()->Draw(*previewShader);

        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
    }

    // ============================================================
    // 6) UI LAST
    // ============================================================
    glDisable(GL_DEPTH_TEST);
    uiManager_.render();
    glEnable(GL_DEPTH_TEST);
}

void Scene::drawSelectionIndicators(const glm::mat4& view, const glm::mat4& projection)
{
    if (!selectionShader || !selectionRingTex || selectionCircleVAO == 0 || selectedUnits_.empty())
        return;

    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    selectionShader->Use();
    selectionShader->SetMat4("view", view);
    selectionShader->SetMat4("projection", projection);
    selectionRingTex->Bind(0);
    selectionShader->SetInt("uTexture", 0);
    selectionShader->SetVec4("uTint", glm::vec4(1.0f));

    glBindVertexArray(selectionCircleVAO);

    for (Unit* unit : selectedUnits_)
    {
        if (!unit) continue;

        glm::mat4 model = glm::mat4(1.0f);
        glm::vec3 pos = unit->position;
        pos.y += 0.6f;
        model = glm::translate(model, pos);
        model = glm::scale(model, glm::vec3(6.0f, 1.0f, 6.0f));

        selectionShader->SetMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
}
