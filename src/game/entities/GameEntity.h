#pragma once
#include <glm/glm.hpp>
#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../../common/Shader.h"
#include "../../common/Model.h"
#include "EntityType.h"
#include <glm/gtx/euler_angles.hpp>

class GameEntity {
public:
    glm::vec3 position;
    int ownerID;
    EntityType type;
    Model* model;

    glm::mat4 transform;

    GameEntity(glm::vec3 pos, EntityType t, Model* m, int owner, float scale = 20.0f)
        : position(pos), type(t), model(m), ownerID(owner),
          transform(glm::mat4(1.0f)),
          uniformScale_(scale)
    {
        RebuildTransform();
    }

    virtual ~GameEntity() = default;

    virtual void Update(float dt) = 0;

    virtual void Draw(Shader& shader)
    {
        if (!model) return;
        shader.Use();
        shader.SetMat4("model", transform);
        shader.SetFloat("uAlpha", 1.0f);
        shader.SetBool("uUseSkinning", false);
        model->Draw(shader);
    }

    void SetSelected(bool selected) { isSelected_ = selected; }
    bool IsSelected() const { return isSelected_; }
    float GetYaw() const { return rotationEuler_.y; }
    int  GetNetworkId() const { return networkId_; }
    void SetNetworkId(int id) { networkId_ = id; }
    void SetVisualOffset(const glm::vec3& offset)
    {
        visualOffset_ = offset;
        RebuildTransform();
    }
    void SetYaw(float yaw)
    {
        rotationEuler_.y = yaw;
        RebuildTransform();
    }
    void SetRotationEuler(const glm::vec3& euler)
    {
        rotationEuler_ = euler;
        RebuildTransform();
    }
    void SetUniformScale(float scale)
    {
        uniformScale_ = scale;
        RebuildTransform();
    }

protected:
    const glm::vec3& GetVisualOffset() const { return visualOffset_; }

    void RebuildTransform()
    {
        glm::vec3 translated = position + visualOffset_;
        glm::mat4 rotation = glm::yawPitchRoll(rotationEuler_.y, rotationEuler_.x, rotationEuler_.z);
        glm::mat4 translationMat = glm::translate(glm::mat4(1.0f), translated);
        glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(uniformScale_));
        transform = translationMat * rotation * scaleMat;
    }

private:
    float uniformScale_;
    glm::vec3 visualOffset_{0.0f};
    glm::vec3 rotationEuler_{0.0f};
    bool isSelected_ = false;
    int  networkId_ = -1;
};
