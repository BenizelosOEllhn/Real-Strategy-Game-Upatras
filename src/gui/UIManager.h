// UIManager.h
#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <functional>
#include <string>

#include "Shader.h"

struct UIButton
{
    glm::vec2 pos;
    glm::vec2 size;
    GLuint    texture = 0;

    bool hovered = false;

    std::function<void()> onClick;
};

struct UILabel
{
    glm::vec2 pos;
    std::string text;
    float scale = 1.0f;
};

class UIManager
{
public:
    UIManager() = default;

    void init(Shader* shader, int screenW, int screenH);
    void setFontTexture(GLuint tex, int cols = 16, int rows = 16,
                        float charW = 8.0f, float charH = 12.0f);

    void addButton(const UIButton& btn);
    void addLabel(const std::string& text, const glm::vec2& pos,
                  float scale = 1.0f);

    void update(float mouseX, float mouseY);
    void handleClick(float mouseX, float mouseY);
    void render();

    // On-the-fly text rendering (used by labels internally, but you can call it too)
    void drawText(const std::string& text, float x, float y, float scale = 1.0f);

private:
    Shader* shader_ = nullptr;
    GLuint vao_ = 0;
    GLuint vbo_ = 0;

    int screenW_ = 0;
    int screenH_ = 0;
    glm::mat4 proj_{1.0f};

    std::vector<UIButton> buttons_;
    std::vector<UILabel>  labels_;

    // Bitmap font
    GLuint fontTex_ = 0;
    int fontCols_ = 16;
    int fontRows_ = 16;
    float fontCharW_ = 8.0f;
    float fontCharH_ = 12.0f;
};
