#include "Scene.h"

void Scene::DrawDepth(Shader& depthShader, const glm::mat4& lightSpaceMatrix) {
    if (!terrain) return;

    depthShader.Use();
    depthShader.SetMat4("lightSpaceMatrix", lightSpaceMatrix);

    // 1. Draw Terrain
    depthShader.SetMat4("model", glm::mat4(1.0f));
    terrain->Draw(depthShader.ID);

    // 2. Draw Trees (Instanced)
    if (treeModel && !treeTransforms.empty()) {
        treeModel->DrawInstanced(depthShader, treeTransforms);
    }

    // 3. Draw Rocks (Instanced)
    if (rockModel && !rockTransforms.empty()) {
        rockModel->DrawInstanced(depthShader, rockTransforms);
    }
}
