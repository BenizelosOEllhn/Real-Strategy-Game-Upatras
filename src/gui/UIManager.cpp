#include "UIManager.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>


void UIManager::init(Shader* shader, int screenW, int screenH)
{
    shader_   = shader;
    screenW_  = screenW;
    screenH_  = screenH;

    proj_ = glm::ortho(0.0f, (float)screenW_,
                       0.0f, (float)screenH_);

    // Simple quad VBO (we overwrite positions per element)
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0); // aPos
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1); // aUV
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glBindVertexArray(0);
}

void UIManager::setFontTexture(GLuint tex, int cols, int rows, float charW, float charH)
{
    fontTex_    = tex;
    fontCols_   = cols;
    fontRows_   = rows;
    fontCharW_  = charW;
    fontCharH_  = charH;
}

void UIManager::addButton(const UIButton& btn)
{
    buttons_.push_back(btn);
}

void UIManager::addLabel(const std::string& text, const glm::vec2& pos, float scale)
{
    labels_.push_back({ pos, text, scale });
}

void UIManager::update(float mouseX, float mouseY)
{
    for (auto& b : buttons_) {
        bool inside =
            mouseX >= b.pos.x &&
            mouseX <= b.pos.x + b.size.x &&
            mouseY >= b.pos.y &&
            mouseY <= b.pos.y + b.size.y;
        b.hovered = inside;
    }

}

bool UIManager::handleClick(float mx, float my)
{

    for (UIButton& b : buttons_)
    {

        if (b.contains(mx, my))
        {
        if (b.contains(mx, my))
        {
            if (b.onClick) {
                b.onClick();
                return true; // consumed ONLY if clickable
            }
            // not clickable -> ignore, keep searching
        }

        }
    }
    return false;
}


void UIManager::render()
{
    if (!shader_) return;

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader_->Use();
    shader_->SetMat4("uProj", proj_);
    shader_->SetInt("uTex", 0);

    glBindVertexArray(vao_);

    // --- 1) Draw buttons (bar background + icons + hover frame) ---
    for (auto& b : buttons_)
    {
        float x = b.pos.x;
        float y = b.pos.y;
        float w = b.size.x;
        float h = b.size.y;

        // If this is the special "bar" (texture == 0 and no click)
        if (b.texture == 0 && !b.onClick) {
            // Solid brown/beige bar
            shader_->SetInt("uHasTexture", 0);
            shader_->SetVec4("uTint", glm::vec4(0.62f, 0.52f, 0.38f, 0.95f));

            float vertices[6 * 4] = {
                x,     y,     0.0f, 0.0f,
                x + w, y,     1.0f, 0.0f,
                x + w, y + h, 1.0f, 1.0f,

                x,     y,     0.0f, 0.0f,
                x + w, y + h, 1.0f, 1.0f,
                x,     y + h, 0.0f, 1.0f
            };

            glBindBuffer(GL_ARRAY_BUFFER, vbo_);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            continue;
        }

        // Otherwise: normal clickable icon button

        // Optional hover frame (slightly bigger solid rect)
        if (b.hovered) {
            float fx = x - 4.0f;
            float fy = y - 4.0f;
            float fw = w + 8.0f;
            float fh = h + 8.0f;

            shader_->SetInt("uHasTexture", 0);
            shader_->SetVec4("uTint", glm::vec4(0.95f, 0.9f, 0.6f, 0.9f));

            float frameVerts[6 * 4] = {
                fx,     fy,     0.0f, 0.0f,
                fx+fw,  fy,     1.0f, 0.0f,
                fx+fw,  fy+fh,  1.0f, 1.0f,

                fx,     fy,     0.0f, 0.0f,
                fx+fw,  fy+fh,  1.0f, 1.0f,
                fx,     fy+fh,  0.0f, 1.0f
            };

            glBindBuffer(GL_ARRAY_BUFFER, vbo_);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(frameVerts), frameVerts);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // Icon itself
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, b.texture);

        shader_->SetInt("uHasTexture", 1);
        glm::vec4 tint = b.hovered
            ? glm::vec4(1.0f, 1.0f, 0.85f, 1.0f)
            : glm::vec4(1.0f);
        shader_->SetVec4("uTint", tint);

        float vertices[6 * 4] = {
            x,     y,     0.0f, 0.0f,
            x + w, y,     1.0f, 0.0f,
            x + w, y + h, 1.0f, 1.0f,

            x,     y,     0.0f, 0.0f,
            x + w, y + h, 1.0f, 1.0f,
            x,     y + h, 0.0f, 1.0f
        };

        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    // --- 2) Draw labels using bitmap font ---
    for (const auto& lbl : labels_) {
        drawText(lbl.text, lbl.pos.x, lbl.pos.y, lbl.scale);
    }

    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

// Basic monospace bitmap font text
void UIManager::drawText(const std::string& text, float x, float y, float scale)
{
    if (!shader_ || fontTex_ == 0) return;

    shader_->Use();
    shader_->SetMat4("uProj", proj_);
    shader_->SetInt("uTex", 0);
    shader_->SetInt("uHasTexture", 1);
    shader_->SetVec4("uTint", glm::vec4(1.0f)); // white text

    glBindVertexArray(vao_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fontTex_);

    float cursorX = x;
    float cursorY = y;
    float cw = fontCharW_ * scale;
    float ch = fontCharH_ * scale;

    for (char c : text) {
        if (c == ' ') {
            cursorX += cw;
            continue;
        }
        if (c == '\n') {
            cursorX = x;
            cursorY -= ch; // move down line
            continue;
        }

        int idx = static_cast<unsigned char>(c);
        // assuming ASCII starts from row 2 in the atlas (often first rows are control/junk)
        int col = idx % fontCols_;
        int row = idx / fontCols_;

        float u0 = (float)col / (float)fontCols_;
        float v0 = (float)row / (float)fontRows_;
        float u1 = u0 + 1.0f / (float)fontCols_;
        float v1 = v0 + 1.0f / (float)fontRows_;

        // V flips depending on how your atlas is stored. If upside-down, swap v0/v1.
        // Here we assume (0,0) is top-left; adjust if needed:
        v0 = 1.0f - v0;
        v1 = 1.0f - v1;

        float x0 = cursorX;
        float y0 = cursorY;
        float x1 = cursorX + cw;
        float y1 = cursorY + ch;

        float verts[6 * 4] = {
            x0, y0, u0, v1,
            x1, y0, u1, v1,
            x1, y1, u1, v0,

            x0, y0, u0, v1,
            x1, y1, u1, v0,
            x0, y1, u0, v0
        };

        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        cursorX += cw;
    }
}

