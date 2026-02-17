#pragma once

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <functional>
#include <GL/glew.h>

struct UIButton
{
    // Bottom-left corner in screen space (pixels)
    glm::vec2 pos;

    // Width / height in pixels
    glm::vec2 size;

    // Optional texture (0 = none)
    GLuint texture = 0;

    // Optional tint for textured buttons
    glm::vec4 tint = glm::vec4(1.0f);

    // Radial fill mode (for progress rings)
    bool radialFill = false;
    float radialProgress = 0.0f; // 0.0 to 1.0

    bool hovered = false;
    bool clickable = true;
    bool visible = true;
    
    // Callback when clicked
    std::function<void()> onClick;

    bool contains(float mx, float my) const;
};
