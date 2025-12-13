#pragma once

#include <glm/vec2.hpp>
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

    bool hovered = false;
    bool clickable = true; 
    
    // Callback when clicked
    std::function<void()> onClick;

    bool contains(float mx, float my) const;
};
