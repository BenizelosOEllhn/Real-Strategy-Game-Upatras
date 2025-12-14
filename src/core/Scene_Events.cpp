#include "Scene.h"

void Scene::Update(float dt, const Camera& cam)
{   
    int fbW, fbH;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &fbW, &fbH);
    
    uiManager_.update(mouseX_, mouseY_);
    
    // Pass the camera AND dimensions
    buildingManager_.update(mouseX_, mouseY_, fbW, fbH, cam); 

    updateResourceTexts();

    for (GameEntity* e : entities_)
    {
        if (e)
            e->Update(dt);
    }
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
    bar.clickable = false;

    uiManager_.addButton(bar);

    // 2) Building buttons + labels
    int i = 0;
    const float labelOffsetY = 6.0f;
    const float labelScale   = 1.6f;
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
        float labelY    = bar.pos.y + 8.0f;  // near bottom of bar

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

void Scene::setupResourceBar()
{
    const float barHeight = 80.0f;
    const float marginTop = 10.0f;
    const float paddingX = 20.0f;
    const float iconSize = 48.0f;
    const float labelScale = 1.6f;
    float baseY = fbHeight - barHeight - marginTop;

    UIButton bar;
    bar.pos  = glm::vec2(0.0f, baseY);
    bar.size = glm::vec2(fbWidth, barHeight);
    bar.texture = 0;
    bar.onClick = nullptr;
    bar.clickable = false;
    uiManager_.addButton(bar);

    float cursorX = paddingX;
    float iconY = baseY + (barHeight - iconSize) * 0.5f;

    auto addEntry = [&](Texture*& texPtr, const std::string& fileName, size_t& labelIndex)
    {
        if (!texPtr)
        {
            texPtr = new Texture((std::string(ASSET_PATH) + "resources/" + fileName).c_str());
        }

        UIButton icon;
        icon.pos = glm::vec2(cursorX, iconY);
        icon.size = glm::vec2(iconSize, iconSize);
        icon.texture = texPtr->ID;
        icon.clickable = false;
        icon.onClick = nullptr;
        uiManager_.addButton(icon);

        float textX = icon.pos.x + icon.size.x + 10.0f;
        float textY = icon.pos.y + icon.size.y * 0.5f - 8.0f;
        labelIndex = uiManager_.addLabel("0", glm::vec2(textX, textY), labelScale);

        cursorX = textX + 90.0f;
    };

    addEntry(cornIconTex, "corn.png", foodLabelIndex_);
    addEntry(woodIconTex, "log.png", woodLabelIndex_);
    addEntry(goldIconTex, "gold-bar.png", goldLabelIndex_);
    addEntry(oreIconTex,  "ore.png", oreLabelIndex_);
    addEntry(populationIconTex, "village.png", populationLabelIndex_);

    updateResourceTexts();
}

void Scene::updateResourceTexts()
{
    auto setVal = [&](size_t idx, int value)
    {
        if (idx == SIZE_MAX) return;
        uiManager_.setLabelText(idx, std::to_string(value));
    };

    setVal(foodLabelIndex_, player1.food);
    setVal(woodLabelIndex_, player1.wood);
    setVal(goldLabelIndex_, player1.gold);
    setVal(oreLabelIndex_, player1.stone);
    setVal(populationLabelIndex_, player1.maxPopulation);
}


void Scene::onMouseMove(double x, double y)
{
    int fbW, fbH;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &fbW, &fbH);

    int winW, winH;
    glfwGetWindowSize(glfwGetCurrentContext(), &winW, &winH);

    float scaleX = (float)fbW / (float)winW;
    float scaleY = (float)fbH / (float)winH;

    // Convert to framebuffer pixels
    double px = x * scaleX;
    double py = y * scaleY;

    // IMPORTANT: store bottom-left origin
    mouseX_ = px;
    mouseY_ = (double)fbH - py;
}
    

void Scene::onMouseButton(int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        bool uiConsumed = uiManager_.handleClick(mouseX_, mouseY_);
        if (!uiConsumed)
        {
            buildingManager_.confirmPlacement(mouseX_, mouseY_);
        }
    }
}


glm::vec3 Scene::GetMouseWorldPos(double mouseX, double mouseY,
                                  int screenW, int screenH,
                                  const glm::mat4& view,
                                  const glm::mat4& projection,
                                  float groundY)
{
    // 1. NDC
    float x = (2.0f * mouseX) / screenW - 1.0f;
    float y = 1.0f - (2.0f * mouseY) / screenH;

    glm::vec4 rayClip(x, y, -1.0f, 1.0f);

    // 2. Eye space
    glm::vec4 rayEye = glm::inverse(projection) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

    // 3. World space
    glm::vec3 rayWorld = glm::normalize(
        glm::vec3(glm::inverse(view) * rayEye)
    );

    // 4. Ray-plane intersection
    glm::vec3 camPos = camera->GetPosition();

    if (fabs(rayWorld.y) < 0.0001f)
        return camPos; // fallback, avoids NaN

    float t = (groundY - camPos.y) / rayWorld.y;
    return camPos + rayWorld * t;

}
