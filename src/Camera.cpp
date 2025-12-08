#include "Camera.h"
#include <iostream> 

// Constructor
Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch) 
    : Front(glm::vec3(0.0f, 0.0f, -1.0f)), 
      BaseSpeed(40.0f),      // Start slow
      MaxSpeed(120.0f),      // Go fast if held
      Acceleration(50.0f),   // Rate of gain
      CurrentSpeed(40.0f),   // Init
      MouseSensitivity(0.1f), 
      Zoom(45.0f) 
{
    Position = position;
    WorldUp = up;
    Yaw = yaw;
    Pitch = pitch;
    updateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix() {
    return glm::lookAt(Position, Position + Front, Up);
}

void Camera::ResetSpeed() {
    CurrentSpeed = BaseSpeed;
}

void Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime) {
    // 1. Accelerate
    CurrentSpeed += Acceleration * deltaTime;
    if (CurrentSpeed > MaxSpeed) CurrentSpeed = MaxSpeed;

    float velocity = CurrentSpeed * deltaTime;

    // 2. RTS Movement (XZ Plane)
    glm::vec3 groundFront = glm::normalize(glm::vec3(Front.x, 0.0f, Front.z));
    glm::vec3 groundRight = glm::normalize(glm::vec3(Right.x, 0.0f, Right.z));

    if (direction == FORWARD)
        Position += groundFront * velocity;
    if (direction == BACKWARD)
        Position -= groundFront * velocity;
    if (direction == LEFT)
        Position -= groundRight * velocity;
    if (direction == RIGHT)
        Position += groundRight * velocity;
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch) {
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw   += xoffset;
    Pitch += yoffset;

    if (constrainPitch) {
        if (Pitch > 89.0f) Pitch = 89.0f;
        if (Pitch < -89.0f) Pitch = -89.0f;
    }

    updateCameraVectors();
}

// Zoom Logic
void Camera::ProcessMouseScroll(float yoffset) {
    // Zoom speed
    float zoomSpeed = 10.0f; 
    
    // Subtracting yoffset because scrolling UP usually means "Zoom In" (Go Down)
    Position.y -= yoffset * zoomSpeed;

    // Hard Limits
    if (Position.y < 20.0f)  Position.y = 20.0f;  
    if (Position.y > 400.0f) Position.y = 400.0f; 
}

void Camera::updateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);
    Right = glm::normalize(glm::cross(Front, WorldUp));  
    Up    = glm::normalize(glm::cross(Right, Front));
}