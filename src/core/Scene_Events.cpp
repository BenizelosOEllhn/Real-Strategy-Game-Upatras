#include "Scene.h"

void Scene::Update(float dt)
{
    uiManager_.update(mouseX_, mouseY_);
    buildingManager_.update(mouseX_, mouseY_);
}


void Scene::setupBuildingBar()
{
    float buttonSize = 128.0f;   // double original size
    float barHeight = 150.0f;    // scale bar appropriately
    float padding    = 20.0f;
    float baseY      = 10.0f; // 10 px from bottom

    // 1) Background bar (full width)
    UIButton bar;
    bar.pos  = glm::vec2(0.0f, baseY - 8.0f);          // little margin
    bar.size = glm::vec2(fbWidth, barHeight);
    bar.texture = 0;        // special = solid color
    bar.onClick = nullptr;  // not clickable

    uiManager_.addButton(bar);

    // 2) Building buttons + labels
    int i = 0;
    const float labelOffsetY = 6.0f;
    const float labelScale   = 1.0f;
    const float charW        = 8.0f * labelScale;  // matches setFontTexture

    auto addButton = [&](const char* fileName,
                         const char* label,
                         BuildType type)
    {
        UIButton btn;
        btn.pos  = glm::vec2(padding + i * (buttonSize + padding),
                             baseY + (barHeight - buttonSize) * 0.5f - 8.0f);
        btn.size = glm::vec2(buttonSize, buttonSize);

        std::string fullPath = std::string(ASSET_PATH) + "gui/" + fileName;
        Texture* btnTex = new Texture(fullPath.c_str());
        btn.texture = btnTex->ID;

        btn.onClick = [this, type]() {
            buildingManager_.startPlacing(type);
        };

        uiManager_.addButton(btn);

        // Centered label under the icon
        std::string text(label);
        float textWidth = text.size() * charW;
        float centerX   = btn.pos.x + btn.size.x * 0.5f;
        float labelX    = centerX - textWidth * 0.5f;
        float labelY    = bar.pos.y + 4.0f;  // near bottom of bar

        uiManager_.addLabel(text, glm::vec2(labelX, labelY), labelScale);

        ++i;
    };

    addButton("TownCenter_FirstAge_Level1.png", "Town Center", BuildType::TownCenter);
    addButton("Barracks_FirstAge_Level1.png",   "Barracks",    BuildType::Barracks);
    addButton("Farm_FirstAge_Level1_Wheat.png", "Farm",        BuildType::Farm);
    addButton("Houses_FirstAge_1_Level1.png",   "Houses",      BuildType::House);
    addButton("Market_FirstAge_Level1.png",     "Market",      BuildType::Market);
    addButton("Storage_FirstAge_Level1.png",    "Storage",     BuildType::Storage);
}


void Scene::onMouseMove(double x, double y)
{
    int fbW, fbH;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &fbW, &fbH);

    // Convert from window (GLFW) coords → framebuffer (pixel) coords correctly
    int winW, winH;
    glfwGetWindowSize(glfwGetCurrentContext(), &winW, &winH);

    float scaleX = (float)fbW / (float)winW;
    float scaleY = (float)fbH / (float)winH;

    mouseX_ = x * scaleX;
    mouseY_ = fbH - (y * scaleY);   // ← CORRECT: flip AFTER scaling
}

void Scene::onMouseButton(int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        // first let UI consume it
        uiManager_.handleClick(mouseX_, mouseY_);

        // then building placement
        buildingManager_.confirmPlacement(mouseX_, mouseY_);
    }
}
