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

    // 1. Initialize immediately in the definition
    glm::mat4 transform = glm::mat4(1.0f); 

    GameEntity(glm::vec3 pos, EntityType t, Model* m, int owner)
        : position(pos), type(t), model(m), ownerID(owner)
    {
        // 2. Reset to Identity to be safe
        transform = glm::mat4(1.0f); 

        // 3. Translate to the click position
        transform = glm::translate(transform, position);

        // 4. SCALE IS CRITICAL
        // If this is missing, the building is too small to see (Invisible Barracks)
        transform = glm::scale(transform, glm::vec3(5.0f)); 
    }

    virtual ~GameEntity() {}
    virtual void Update(float dt) = 0;

    virtual void Draw(Shader& shader) {
        if (!model) return;
        shader.Use();
        shader.SetMat4("model", transform);
        model->Draw(shader);
    }
};