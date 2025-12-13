#include "GameEntity.h"
#include <glm/gtc/matrix_transform.hpp>

void GameEntity::Draw(Shader& shader)
{
    shader.Use();
    shader.SetMat4("model", transform);
    model->Draw(shader);
}
