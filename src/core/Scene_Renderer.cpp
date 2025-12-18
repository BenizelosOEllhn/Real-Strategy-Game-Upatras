#include "Scene.h"
#include "SceneConstants.h"
#include <cmath>
#include <limits>
#include <glm/gtx/euler_angles.hpp>

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
            if (e->ownerID > 0 &&
                e->ownerID != activePlayerIndex_ + 1 &&
                !isPositionVisibleToPlayer(e->position, activePlayerIndex_ + 1))
                continue;
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

        glm::vec3 previewPos = buildingManager_.getPreviewPos() + buildingManager_.getPreviewOffset();
        glm::vec3 previewRotation = buildingManager_.getPreviewRotation();
        float previewScale = buildingManager_.getPreviewScale();
        glm::mat4 m = glm::translate(glm::mat4(1.0f), previewPos);
        if (glm::length(previewRotation) > std::numeric_limits<float>::epsilon())
        {
            glm::mat4 rot = glm::yawPitchRoll(previewRotation.y, previewRotation.x, previewRotation.z);
            m *= rot;
        }
        m = glm::scale(m, glm::vec3(previewScale));
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

    if (!isFogRevealed())
        DrawFogOfWar(view, projection);

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

void Scene::rebuildFogMeshForPlayer(int playerId)
{
    fogVertexBuffer_.clear();
    fogVertexCount_ = 0;

    if (playerId < 1 || playerId > 2)
        return;
    if (navGridCols_ <= 0 || navGridRows_ <= 0)
        return;

    const auto& fog = fogStates_[playerId - 1];
    if (fog.empty() || fogVAO_ == 0 || fogVBO_ == 0)
        return;

    const size_t cellCount = static_cast<size_t>(navGridCols_) * static_cast<size_t>(navGridRows_);
    if (fog.size() < cellCount)
        return;

    fogVertexBuffer_.reserve(cellCount * 6 * 7);
    const float half = navCellSize_ * 0.5f;
    const float pad = navCellSize_ * 0.08f;

    auto pushVertex = [&](const glm::vec3& p, const glm::vec4& c)
    {
        fogVertexBuffer_.push_back(p.x);
        fogVertexBuffer_.push_back(p.y);
        fogVertexBuffer_.push_back(p.z);
        fogVertexBuffer_.push_back(c.r);
        fogVertexBuffer_.push_back(c.g);
        fogVertexBuffer_.push_back(c.b);
        fogVertexBuffer_.push_back(c.a);
    };

    for (int row = 0; row < navGridRows_; ++row)
    {
        for (int col = 0; col < navGridCols_; ++col)
        {
            size_t idx = static_cast<size_t>(row) * static_cast<size_t>(navGridCols_) + static_cast<size_t>(col);
            uint8_t state = fog[idx];
            if (state == 2)
                continue;

            glm::vec3 center = navToWorld(col, row);
            float y = fogPlaneY_;
            glm::vec3 p0(center.x - half - pad, y, center.z - half - pad);
            glm::vec3 p1(center.x + half + pad, y, center.z - half - pad);
            glm::vec3 p2(center.x + half + pad, y, center.z + half + pad);
            glm::vec3 p3(center.x - half - pad, y, center.z + half + pad);

            glm::vec4 color = (state == 0)
                ? glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
                : glm::vec4(0.0f, 0.0f, 0.0f, 0.35f);

            pushVertex(p0, color);
            pushVertex(p1, color);
            pushVertex(p2, color);

            pushVertex(p0, color);
            pushVertex(p2, color);
            pushVertex(p3, color);
        }
    }

    fogVertexCount_ = fogVertexBuffer_.size() / 7;
    glBindBuffer(GL_ARRAY_BUFFER, fogVBO_);
    glBufferData(GL_ARRAY_BUFFER,
                 fogVertexBuffer_.size() * sizeof(float),
                 fogVertexBuffer_.data(),
                 GL_DYNAMIC_DRAW);
    fogDirty_ = false;
}

void Scene::DrawFogOfWar(const glm::mat4& view, const glm::mat4& projection)
{
    if (isFogRevealed())
        return;
    if (!fogShader || fogVAO_ == 0 || fogVBO_ == 0)
        return;

    if (fogDirty_)
        rebuildFogMeshForPlayer(activePlayerIndex_ + 1);

    if (fogVertexCount_ == 0)
        return;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    fogShader->Use();
    fogShader->SetMat4("view", view);
    fogShader->SetMat4("projection", projection);

    glBindVertexArray(fogVAO_);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(fogVertexCount_));
    glBindVertexArray(0);

    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}
