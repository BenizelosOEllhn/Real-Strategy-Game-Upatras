#pragma once
#include <glm/vec2.hpp>
#include <functional>
#include <GL/glew.h>

struct UIButton {
    glm::vec2 pos;   // bottom-left in screen space (pixels)
    glm::vec2 size;  // width/height in pixels
    GLuint     texture = 0;

    bool hovered = false;

    std::function<void()> onClick;
};
