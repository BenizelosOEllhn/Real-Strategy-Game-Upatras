#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Defines several possible options for camera movement
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

class Camera {
public:
    // Attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;

    // Euler Angles
    float Yaw;
    float Pitch;

    // Camera Options 
    float BaseSpeed;    
    float MaxSpeed;     
    float Acceleration; 
    float CurrentSpeed; 
    
    float MouseSensitivity;
    float Zoom;

    // Constructor
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), 
           glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), 
           float yaw = -90.0f, float pitch = 0.0f);

    glm::mat4 GetViewMatrix();
    void ProcessKeyboard(Camera_Movement direction, float deltaTime);
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true);
    
    // Zoom Handler
    void ProcessMouseScroll(float yoffset);
    
    // Reset Speed (Call this when no keys are pressed)
    void ResetSpeed();

    glm::mat4 GetViewMatrix() const;

    // 2. Add GetPosition helper (or access camera.Position directly)
    glm::vec3 GetPosition() const { return Position; }

private:
    void updateCameraVectors();
};