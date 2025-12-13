#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../../common/Shader.h"
#include "../../common/Model.h"
#include "EntityType.h"

class GameEntity {
public:
    glm::vec3 position;
    float health = 100.0f;
    int ownerID; // player 1 or 2
    EntityType type;

    Model* model; // pointer to shared mesh 
    glm::mat4 transform;

    GameEntity(glm::vec3 pos, EntityType t, Model* m, int owner)
        : position(pos), type(t), model(m), ownerID(owner)
    {
        transform = glm::translate(glm::mat4(1.0f), position);
    }

    virtual ~GameEntity() {}

    virtual void Update(float dt) = 0;
    virtual void Draw(Shader& shader);
};
