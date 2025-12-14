#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../../common/Shader.h"
#include "../../common/Model.h"
#include "EntityType.h"

class GameEntity {
public:
    glm::vec3 position;
    int ownerID;
    EntityType type;
    Model* model;

    glm::mat4 transform;

    GameEntity(glm::vec3 pos, EntityType t, Model* m, int owner)
        : position(pos), type(t), model(m), ownerID(owner),
          transform(glm::mat4(1.0f))
    {
        transform = glm::translate(transform, position);
        transform = glm::scale(transform, glm::vec3(20.0f)); // your scale
    }

    virtual ~GameEntity() = default;

    virtual void Update(float dt) = 0;

    virtual void Draw(Shader& shader)
    {
        if (!model) return;
        shader.Use();
        shader.SetMat4("model", transform);
        model->Draw(shader);
    }
};
