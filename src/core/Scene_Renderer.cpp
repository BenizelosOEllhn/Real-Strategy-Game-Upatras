#include "Scene.h"
#include "SceneConstants.h"

// ------------------------------------------------------------
// MAIN RENDER PASS
// ------------------------------------------------------------
void Scene::Draw(Shader& terrainShader,
                 Shader& objectShader,
                 glm::mat4 view,
                 glm::mat4 projection,
                 glm::vec3 lightPos,
                 glm::vec3 viewPos,
                 const glm::mat4& lightSpaceMatrix,
                 unsigned int shadowMap)
{
    const glm::vec3 lightColor(1.0f, 0.97f, 0.92f);

    // ===============================
    // 1. TERRAIN RENDERING
    // ===============================
    if (terrain && grass1Tex && grass2Tex && grass3Tex)
    {
        terrainShader.Use();

        terrainShader.SetMat4("view", view);
        terrainShader.SetMat4("projection", projection);
        terrainShader.SetMat4("model", glm::mat4(1.0f));
        terrainShader.SetMat4("lightSpaceMatrix", lightSpaceMatrix);

        grass1Tex->Bind(0);
        grass2Tex->Bind(1);
        grass3Tex->Bind(2);
        noiseTex->Bind(3);
        rockTex->Bind(4);
        sandTex->Bind(5);
        peakTex->Bind(6);

        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, shadowMap);

        terrainShader.SetInt("grass1",       0);
        terrainShader.SetInt("grass2",       1);
        terrainShader.SetInt("grass3",       2);
        terrainShader.SetInt("noiseDetail",  3);
        terrainShader.SetInt("textureRock",  4);
        terrainShader.SetInt("sandTex",      5);
        terrainShader.SetInt("texturePeak",  6);
        terrainShader.SetInt("shadowMap",    7);

        terrainShader.SetVec3("lightPos",  lightPos);
        terrainShader.SetVec3("viewPos",   viewPos);
        terrainShader.SetVec3("lightColor", lightColor);

        terrain->Draw(terrainShader.ID);
    }

    // ===============================
    // 2. INSTANCED NATURAL OBJECTS
    // ===============================
    bool hasTrees = treeModel && treeTex && !treeTransforms.empty();
    bool hasRocks = rockModel && boulderTex && !rockTransforms.empty();

    if (hasTrees || hasRocks)
    {
        objectShader.Use();
        objectShader.SetMat4("view", view);
        objectShader.SetMat4("projection", projection);
        objectShader.SetMat4("lightSpaceMatrix", lightSpaceMatrix);
        objectShader.SetMat4("model", glm::mat4(1.0f));

        objectShader.SetVec3("lightPos",  lightPos);
        objectShader.SetVec3("viewPos",   viewPos);
        objectShader.SetVec3("lightColor", lightColor);

        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, shadowMap);
        objectShader.SetInt("shadowMap", 7);
    }

    if (hasTrees)
    {
        treeTex->Bind(0);
        objectShader.SetInt("texture_diffuse1", 0);
        treeModel->DrawInstanced(objectShader, treeTransforms);
    }

    if (hasRocks)
    {
        boulderTex->Bind(0);
        objectShader.SetInt("texture_diffuse1", 0);
        rockModel->DrawInstanced(objectShader, rockTransforms);
    }

    // ===============================
    // 3. WATER DRAW
    // ===============================
    // Big ocean plane
    DrawWater(view, projection, viewPos);
    // Local meshes (lake & rivers)
    DrawLakeWater(view, projection, viewPos);
    DrawRiverWater(view, projection, viewPos);

    // ===============================
    // 4. DRAW UI + BUILD PLACEMENT PREVIEW
    // ===============================
if (buildingManager_.isPlacing() &&
    buildingManager_.hasPreview() &&
    buildingManager_.getPreviewModel())
{
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    previewShader->Use();
    previewShader->SetMat4("view", view);
    previewShader->SetMat4("projection", projection);

    glm::mat4 model = glm::translate(
        glm::mat4(1.0f),
        buildingManager_.getPreviewPos()
    );
    previewShader->SetMat4("model", model);

    // FORCE visible green
    previewShader->SetVec4("uTint",
        glm::vec4(0.1f, 1.0f, 0.1f, 0.6f)
    );

    buildingManager_.getPreviewModel()->Draw(*previewShader);

    glBindVertexArray(0);
    glUseProgram(0);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glDisable(GL_BLEND);


    std::cout << "[Scene] Drawing preview at "
          << buildingManager_.getPreviewPos().x << ", "
          << buildingManager_.getPreviewPos().y << ", "
          << buildingManager_.getPreviewPos().z << std::endl;

}

    // (B) Draw UI buttons
    glDisable(GL_DEPTH_TEST);
    uiManager_.render();
    glEnable(GL_DEPTH_TEST);

}
