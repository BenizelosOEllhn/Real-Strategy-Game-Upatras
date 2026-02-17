#include "Scene.h"
#include <fstream>
#include <functional>
#include <sstream>
#include <utility>

void Scene::setupBuildingBar()
{
    const float buttonSize = 128.0f;
    const float barHeight = 150.0f;
    const float padding    = 32.0f;
    const float spacing    = 96.0f;
    const float baseY      = 10.0f;

    buildingBarPos_ = glm::vec2(0.0f, baseY - 8.0f);
    buildingBarSize_ = glm::vec2(fbWidth, barHeight);

    UIButton bar;
    bar.pos  = buildingBarPos_;
    bar.size = buildingBarSize_;
    bar.texture = 0;
    bar.onClick = nullptr;
    bar.clickable = false;

    buildingBarBackgroundIndex_ = uiManager_.addButton(bar);

    buildingButtonIndices_.clear();
    buildingLabelIndices_.clear();
    buildingButtonTypes_.clear();
    buildingButtonIcons_.clear();
    buildingBarTextures_.clear();

    int i = 0;
    const float labelScale   = 1.5f;
    const float charW        = 8.0f * labelScale;

    auto loadGuiTexture = [&](const std::string& fileName) -> GLuint
    {
        if (fileName.empty())
            return 0;
        auto tex = std::make_unique<Texture>(AssetPath("gui/" + fileName).c_str());
        GLuint id = tex->ID;
        buildingBarTextures_.push_back(std::move(tex));
        return id;
    };

    auto addButton = [&](const std::string& friendlyFile,
                         const std::string& evilFile,
                         BuildType type)
    {
        GLuint friendlyTex = loadGuiTexture(friendlyFile);
        GLuint evilTex = evilFile.empty() ? friendlyTex : loadGuiTexture(evilFile);

        UIButton btn;
        btn.pos  = glm::vec2(padding + i * (buttonSize + spacing),
                             baseY + (barHeight - buttonSize) * 0.5f - 8.0f);
        btn.size = glm::vec2(buttonSize, buttonSize);

        btn.texture = friendlyTex;

        btn.onClick = [this, type]() {
            updateBuildingInfoPanel(type);
            buildingManager_.startPlacing(type);
        };

        size_t idx = uiManager_.addButton(btn);
        buildingButtonIndices_.push_back(idx);

        std::string text = "";
        float labelX    = btn.pos.x + 10.0f;
        float labelY    = bar.pos.y + 8.0f;

        size_t labelIdx = uiManager_.addLabel(text, glm::vec2(labelX, labelY), labelScale);
        buildingLabelIndices_.push_back(labelIdx);
        buildingButtonTypes_.push_back(type);
        buildingButtonIcons_.push_back({ friendlyTex, evilTex });

        ++i;
    };

    addButton("TownCenter_FirstAge_Level1.png", "altar.png", BuildType::TownCenter);
    addButton("Barracks_FirstAge_Level1.png",   "graveyard.png", BuildType::Barracks);
    addButton("Farm_FirstAge_Level1_Wheat.png", "hangman.png", BuildType::Farm);
    addButton("Houses_FirstAge_1_Level1.png",   "hut.png", BuildType::House);
    addButton("Market_FirstAge_Level1.png",     "smithy.png", BuildType::Market);
    addButton("Storage_FirstAge_Level1.png",    "temple.png", BuildType::Storage);
    addButton("Bridge.png",                     "", BuildType::Bridge);

    updateBuildingBarLabels();
    updateBuildingButtonTexturesForOwner(activePlayerIndex_ + 1);
}

void Scene::updateBuildingBarLabels()
{
    int owner = activePlayerIndex_ + 1;
    for (size_t idx = 0; idx < buildingLabelIndices_.size(); ++idx)
    {
        if (idx >= buildingButtonTypes_.size())
            break;
        size_t labelIdx = buildingLabelIndices_[idx];
        if (labelIdx == SIZE_MAX)
            continue;
        std::string text = buildingNameForOwner(buildingButtonTypes_[idx], owner);
        uiManager_.setLabelText(labelIdx, text);
    }
}

void Scene::updateBuildingButtonTexturesForOwner(int ownerId)
{
    bool evil = (ownerId == 2);
    for (size_t i = 0; i < buildingButtonIndices_.size(); ++i)
    {
        if (i >= buildingButtonIcons_.size())
            break;
        size_t buttonIdx = buildingButtonIndices_[i];
        if (buttonIdx == SIZE_MAX)
            continue;
        GLuint tex = buildingButtonIcons_[i].friendlyTex;
        if (evil && buildingButtonIcons_[i].evilTex != 0)
            tex = buildingButtonIcons_[i].evilTex;
        uiManager_.setButtonTexture(buttonIdx, tex);
    }
}

void Scene::setupResourceBar()
{
    const float barHeight = 80.0f;
    const float paddingX = 30.0f;
    const float iconSize = 48.0f;
    const float labelScale = 1.2f;
    const float topEdge = static_cast<float>(fbHeight);
    float baseY = topEdge - barHeight;

    UIButton bar;
    bar.pos  = glm::vec2(0.0f, baseY);
    bar.size = glm::vec2(static_cast<float>(fbWidth), barHeight);
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
            texPtr = new Texture(AssetPath("resources/" + fileName).c_str());
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

        cursorX = textX + 210.0f;
    };

    addEntry(cornIconTex, "corn.png", foodLabelIndex_);
    addEntry(woodIconTex, "log.png", woodLabelIndex_);
    addEntry(goldIconTex, "gold-bar.png", goldLabelIndex_);
    addEntry(oreIconTex,  "ore.png", oreLabelIndex_);
    addEntry(populationIconTex, "village.png", populationLabelIndex_);

    float timerLabelX = cursorX + 100.0f;
    float timerLabelY = baseY + barHeight * 0.5f - 10.0f;
    timerLabelIndex_ = uiManager_.addLabel("0:00", glm::vec2(timerLabelX, timerLabelY), 1.2f);

    const float flagSize = 60.0f;
    const float ringSize = 84.0f;
    const float ringThickness = 10.0f;
    const float centerX = static_cast<float>(fbWidth) * 0.5f;
    const float flagY = baseY - 58.0f;

    if (greyRingTex)
    {
        UIButton ring;
        ring.pos = glm::vec2(centerX - ringSize * 0.5f, flagY - (ringSize - flagSize) * 0.5f);
        ring.size = glm::vec2(ringSize, ringSize);
        ring.texture = greyRingTex->ID;
        ring.tint = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        ring.clickable = false;
        ring.onClick = nullptr;
        neutralFlagRingIndex_ = uiManager_.addButton(ring);

        UIButton innerRing = ring;
        innerRing.pos += glm::vec2(ringThickness * 0.5f, ringThickness * 0.5f);
        innerRing.size -= glm::vec2(ringThickness, ringThickness);
        innerRing.tint = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        neutralFlagRingInnerIndex_ = uiManager_.addButton(innerRing);

        UIButton progressRing = ring;
        progressRing.radialFill = true;
        progressRing.radialProgress = 0.0f;
        neutralFlagProgressRingIndex_ = uiManager_.addButton(progressRing);
    }

    if (neutralFlagTex)
    {
        UIButton flag;
        flag.pos = glm::vec2(centerX - flagSize * 0.5f, flagY);
        flag.size = glm::vec2(flagSize, flagSize);
        flag.texture = neutralFlagTex->ID;
        flag.clickable = false;
        flag.onClick = nullptr;
        neutralFlagIconIndex_ = uiManager_.addButton(flag);
    }

    float playerLabelX = static_cast<float>(fbWidth) - 180.0f;
    float playerLabelY = baseY + barHeight * 0.5f - 10.0f;
    playerLabelIndex_ = uiManager_.addLabel("Player 1", glm::vec2(playerLabelX, playerLabelY), 1.2f);

    glm::vec2 victoryPos(static_cast<float>(fbWidth) * 0.5f - 140.0f,
                         static_cast<float>(fbHeight) * 0.5f - 20.0f);
    victoryLabelIndex_ = uiManager_.addLabel("", victoryPos, 2.6f);
    uiManager_.setLabelVisibility(victoryLabelIndex_, false);

    updateResourceTexts();
}

void Scene::updateResourceTexts()
{
    auto setVal = [&](size_t idx, const std::string& value)
    {
        if (idx == SIZE_MAX) return;
        uiManager_.setLabelText(idx, value);
    };

    auto formatResource = [](int value, int capacity) -> std::string
    {
        return std::to_string(value) + "/" + std::to_string(capacity);
    };

    const Resources& res = activePlayer();

    setVal(foodLabelIndex_, formatResource(res.food, res.foodCapacity));
    setVal(woodLabelIndex_, formatResource(res.wood, res.woodCapacity));
    setVal(goldLabelIndex_, formatResource(res.gold, res.goldCapacity));
    setVal(oreLabelIndex_,  formatResource(res.ore,  res.oreCapacity));

    std::string populationText = std::to_string(res.population) + "/" + std::to_string(res.populationCap);
    setVal(populationLabelIndex_, populationText);
    if (playerLabelIndex_ != SIZE_MAX)
    {
        std::string playerText = "Player " + std::to_string(activePlayerIndex_ + 1);
        uiManager_.setLabelText(playerLabelIndex_, playerText);
    }
}

void Scene::setupTabButtons()
{
    const glm::vec2 tabSize(220.0f, 56.0f);
    glm::vec2 start = buildingBarPos_ + glm::vec2(20.0f, buildingBarSize_.y + 10.0f);

    auto addTab = [&](const char* text, UITab tab, size_t& buttonIndex, size_t& labelIndex)
    {
        UIButton btn;
        btn.pos = start;
        btn.size = tabSize;
        btn.texture = 0;
        btn.clickable = true;
        btn.onClick = [this, tab]() { setActiveTab(tab); };
        buttonIndex = uiManager_.addButton(btn);

        glm::vec2 labelPos = glm::vec2(btn.pos.x + 12.0f, btn.pos.y + btn.size.y * 0.5f - 7.0f);
        labelIndex = uiManager_.addLabel(text, labelPos, 1.2f);

        start.x += tabSize.x + 60.0f;
    };

    addTab("Buildings", UITab::Buildings, buildingTabButtonIndex_, buildingTabLabelIndex_);
    addTab("Units", UITab::Units, unitTabButtonIndex_, unitTabLabelIndex_);
}

void Scene::setupUnitPanel()
{
    UIButton panel;
    panel.pos = buildingBarPos_;
    panel.size = buildingBarSize_;
    panel.texture = 0;
    panel.clickable = false;

    unitPanelBackgroundIndex_ = uiManager_.addButton(panel);
    uiManager_.setButtonVisibility(unitPanelBackgroundIndex_, false);

    glm::vec2 titlePos(panel.pos.x + 20.0f, panel.pos.y + panel.size.y - 40.0f);
    unitPanelTitleLabelIndex_ = uiManager_.addLabel("Units", titlePos, 1.4f);
    uiManager_.setLabelVisibility(unitPanelTitleLabelIndex_, false);

    unitEntryIconIndices_.clear();
    unitEntryLabelIndices_.clear();
    unitEntryTargets_.clear();
    const size_t kMaxEntries = 12;
    const int columns = 2;
    const float cellPadding = 20.0f;
    const float cellWidth = (panel.size.x - cellPadding * 2.0f) / columns;
    const float cellHeight = 90.0f;
    const float iconSize = 64.0f;

    for (size_t i = 0; i < kMaxEntries; ++i)
    {
        int col = static_cast<int>(i % columns);
        int row = static_cast<int>(i / columns);
        float iconX = panel.pos.x + cellPadding + col * cellWidth;
        float iconY = panel.pos.y + panel.size.y - cellPadding - iconSize - row * cellHeight;

        UIButton icon;
        icon.pos  = glm::vec2(iconX, iconY);
        icon.size = glm::vec2(iconSize, iconSize);
        icon.texture = villagerIconTex ? villagerIconTex->ID : 0;
        icon.clickable = true;
        size_t entryIndex = i;
        icon.onClick = [this, entryIndex]() { selectUnitFromList(entryIndex); };
        size_t iconIndex = uiManager_.addButton(icon);
        uiManager_.setButtonVisibility(iconIndex, false);
        unitEntryIconIndices_.push_back(iconIndex);
        unitEntryTargets_.push_back(nullptr);

        glm::vec2 labelPos = glm::vec2(icon.pos.x + icon.size.x + 8.0f, icon.pos.y + 20.0f);
        size_t labelIndex = uiManager_.addLabel("", labelPos, 1.2f);
        uiManager_.setLabelVisibility(labelIndex, false);
        unitEntryLabelIndices_.push_back(labelIndex);
    }
}

void Scene::setupProductionPanel()
{
    UIButton panel;
    const float panelWidth = 300.0f;
    const float panelHeight = buildingBarSize_.y + 210.0f;
    panel.pos = glm::vec2(fbWidth - panelWidth - 20.0f, buildingBarPos_.y);
    panel.size = glm::vec2(panelWidth, panelHeight);
    panel.texture = 0;
    panel.clickable = false;
    productionPanelBackgroundIndex_ = uiManager_.addButton(panel);
    uiManager_.setButtonVisibility(productionPanelBackgroundIndex_, false);

    productionButtonIndices_.clear();
    productionLabelIndices_.clear();
    productionButtonTypes_.clear();

    struct ButtonDef {
        const char* label;
        EntityType type;
        Texture* icon;
    } defs[] = {
        { "Villager", EntityType::Worker, villagerIconTex },
        { "Archer",   EntityType::Archer, archerIconTex },
        { "Knight",   EntityType::Knight, knightIconTex }
    };

    float cursorY = panel.pos.y + panel.size.y - 90.0f;
    for (const auto& def : defs)
    {
        UIButton btn;
        btn.pos  = glm::vec2(panel.pos.x + 18.0f, cursorY);
        btn.size = glm::vec2(78.0f, 78.0f);
        btn.texture = def.icon ? def.icon->ID : 0;
        btn.onClick = [this, def]() { handleProductionRequest(def.type); };
        size_t idx = uiManager_.addButton(btn);
        uiManager_.setButtonVisibility(idx, false);
        productionButtonIndices_.push_back(idx);

        glm::vec2 labelPos = glm::vec2(btn.pos.x + btn.size.x + 12.0f, btn.pos.y + 26.0f);
        size_t labelIndex = uiManager_.addLabel(def.label, labelPos, 1.25f);
        uiManager_.setLabelVisibility(labelIndex, false);
        productionLabelIndices_.push_back(labelIndex);
        productionButtonTypes_.push_back(def.type);

        cursorY -= 96.0f;
    }

    UIButton upgradeBtn;
    upgradeBtn.pos = panel.pos + glm::vec2(18.0f, 10.0f);
    upgradeBtn.size = glm::vec2(panel.size.x - 36.0f, 42.0f);
    upgradeBtn.texture = 0;
    upgradeBtn.clickable = true;
    upgradeBtn.onClick = [this]() { handleUpgradeRequest(); };
    productionUpgradeButtonIndex_ = uiManager_.addButton(upgradeBtn);
    uiManager_.setButtonVisibility(productionUpgradeButtonIndex_, false);

    glm::vec2 upgradeLabelPos = upgradeBtn.pos + glm::vec2(12.0f, 12.0f);
    productionUpgradeLabelIndex_ = uiManager_.addLabel("Upgrade", upgradeLabelPos, 1.15f);
    uiManager_.setLabelVisibility(productionUpgradeLabelIndex_, false);

    glm::vec2 upgradeCostPos = upgradeBtn.pos + glm::vec2(0.0f, upgradeBtn.size.y + 8.0f);
    productionUpgradeCostLabelIndex_ = uiManager_.addLabel("", upgradeCostPos, 1.0f);
    uiManager_.setLabelVisibility(productionUpgradeCostLabelIndex_, false);
}

void Scene::setupUnitInfoPanel()
{
    UIButton panel;
    panel.pos = glm::vec2(fbWidth - 260.0f, fbHeight - 260.0f);
    panel.size = glm::vec2(240.0f, 140.0f);
    panel.texture = 0;
    panel.clickable = false;
    unitInfoPanelBackgroundIndex_ = uiManager_.addButton(panel);
    uiManager_.setButtonVisibility(unitInfoPanelBackgroundIndex_, false);

    glm::vec2 namePos = glm::vec2(panel.pos.x + 16.0f, panel.pos.y + panel.size.y - 40.0f);
    unitInfoNameLabelIndex_ = uiManager_.addLabel("", namePos, 1.3f);
    uiManager_.setLabelVisibility(unitInfoNameLabelIndex_, false);

    glm::vec2 hpPos = glm::vec2(panel.pos.x + 16.0f, panel.pos.y + panel.size.y - 80.0f);
    unitInfoHealthLabelIndex_ = uiManager_.addLabel("", hpPos, 1.2f);
    uiManager_.setLabelVisibility(unitInfoHealthLabelIndex_, false);

    UIButton deleteBtn;
    deleteBtn.pos = glm::vec2(panel.pos.x + panel.size.x - 120.0f, panel.pos.y + 20.0f);
    deleteBtn.size = glm::vec2(100.0f, 36.0f);
    deleteBtn.texture = 0;
    deleteBtn.onClick = [this]() { handleDeleteCurrentUnit(); };
    unitDeleteButtonIndex_ = uiManager_.addButton(deleteBtn);
    uiManager_.setButtonVisibility(unitDeleteButtonIndex_, false);

    glm::vec2 deleteLabelPos = glm::vec2(deleteBtn.pos.x + 12.0f, deleteBtn.pos.y + 12.0f);
    unitDeleteLabelIndex_ = uiManager_.addLabel("Delete", deleteLabelPos, 1.0f);
    uiManager_.setLabelVisibility(unitDeleteLabelIndex_, false);
}

void Scene::setupBuildingInfoPanel()
{
    UIButton panel;
    panel.pos = buildingBarPos_ + glm::vec2(20.0f, buildingBarSize_.y + 20.0f);
    panel.size = glm::vec2(840.0f, 190.0f);
    panel.texture = 0;
    panel.clickable = false;
    buildingInfoPanelIndex_ = uiManager_.addButton(panel);

    glm::vec2 titlePos = panel.pos + glm::vec2(16.0f, panel.size.y - 32.0f);
    buildingInfoTitleLabelIndex_ = uiManager_.addLabel("Building Info", titlePos, 1.6f);

    glm::vec2 textPos = panel.pos + glm::vec2(16.0f, panel.size.y - 86.0f);
    buildingInfoTextLabelIndex_ = uiManager_.addLabel("Select a building to see its role.", textPos, 1.3f);



    buildingInfoText_.clear();
    buildingInfoText_[BuildType::TownCenter] = "Town Hall\nTrains villagers, stores goods.";
    buildingInfoText_[BuildType::Barracks]   = "Barracks\nProduces rangers and knights.";
    buildingInfoText_[BuildType::Farm]       = "Farm\nGenerates a steady food trickle.";
    buildingInfoText_[BuildType::House]      = "House\nAdds +5 population cap.";
    buildingInfoText_[BuildType::Market]     = "Market\nPassive gold income & trades.";
    buildingInfoText_[BuildType::Storage]    = "Storage\nExpands resource storage caps.";
    buildingInfoText_[BuildType::Bridge]     = "Bridge\nAllows units to cross rivers safely.";

    evilBuildingInfoText_.clear();
    evilBuildingInfoText_[BuildType::TownCenter] = "Altar\nSummons orc peasants and stores goods.";
    evilBuildingInfoText_[BuildType::Barracks]   = "Graveyard\nRaises skeleton warriors and wizards.";
    evilBuildingInfoText_[BuildType::Farm]       = "Hangman\nHarvests food from grim tributes.";
    evilBuildingInfoText_[BuildType::House]      = "Hut\nExpands the horde population by +5.";
    evilBuildingInfoText_[BuildType::Market]     = "Smithy\nPassive ore and gold income.";
    evilBuildingInfoText_[BuildType::Storage]    = "Temple\nExpands dark resource storage.";
    evilBuildingInfoText_[BuildType::Bridge]     = "Bridge\nAllows units to cross rivers safely.";

    updateBuildingInfoPanel(BuildType::None);
}

void Scene::setupMainMenu()
{
    const glm::vec2 menuSize(630.0f, 480.0f);
    glm::vec2 menuPos(
        (static_cast<float>(fbWidth)  - menuSize.x) * 0.5f,
        (static_cast<float>(fbHeight) - menuSize.y) * 0.5f);

    UIButton panel;
    panel.pos = menuPos;
    panel.size = menuSize;
    panel.texture = 0;
    panel.clickable = false;
    mainMenuBackgroundIndex_ = uiManager_.addButton(panel);

    glm::vec2 titlePos(menuPos.x + 20.0f, menuPos.y + menuSize.y - 60.0f);
    mainMenuTitleLabelIndex_ = uiManager_.addLabel("Chronicles\nIn\nNature", titlePos, 2.2f);

    auto addMenuButton = [&](const std::string& text, float relativeY, std::function<void()> handler,
                             size_t& labelIndex) -> size_t
    {
        UIButton btn;
        btn.pos = glm::vec2(menuPos.x + 40.0f, menuPos.y + relativeY);
        btn.size = glm::vec2(menuSize.x - 80.0f, 50.0f);
        btn.texture = 0;
        btn.clickable = true;
        btn.onClick = [handler]() { handler(); };
        size_t index = uiManager_.addButton(btn);
        glm::vec2 labelPos(btn.pos.x + 20.0f, btn.pos.y + 18.0f);
        labelIndex = uiManager_.addLabel(text, labelPos, 1.3f);
        return index;
    };

    mainMenuSingleBtnIndex_ = addMenuButton("Single Player", 170.0f,
        [this]() { startSinglePlayerGame(); }, mainMenuSingleLabelIndex_);
    mainMenuHostBtnIndex_ = addMenuButton("Host LAN Game", 110.0f,
        [this]() { startLanHostGame(); }, mainMenuHostLabelIndex_);
    mainMenuJoinBtnIndex_ = addMenuButton("Join LAN Game", 50.0f,
        [this]() { startLanJoinGame(); }, mainMenuJoinLabelIndex_);

    glm::vec2 statusPos(menuPos.x + 20.0f, menuPos.y + 20.0f);
    lanStatusText_ = "Select a mode to begin.";
    mainMenuStatusLabelIndex_ = uiManager_.addLabel(lanStatusText_, statusPos, 1.2f);
}

void Scene::setMainMenuVisible(bool visible)
{
    auto setBtn = [&](size_t idx, bool v)
    {
        if (idx != SIZE_MAX)
            uiManager_.setButtonVisibility(idx, v);
    };
    auto setLbl = [&](size_t idx, bool v)
    {
        if (idx != SIZE_MAX)
            uiManager_.setLabelVisibility(idx, v);
    };

    setBtn(mainMenuBackgroundIndex_, visible);
    setBtn(mainMenuSingleBtnIndex_, visible);
    setBtn(mainMenuHostBtnIndex_, visible);
    setBtn(mainMenuJoinBtnIndex_, visible);
    setLbl(mainMenuTitleLabelIndex_, visible);
    setLbl(mainMenuStatusLabelIndex_, visible);
    setLbl(mainMenuSingleLabelIndex_, visible);
    setLbl(mainMenuHostLabelIndex_, visible);
    setLbl(mainMenuJoinLabelIndex_, visible);
}

void Scene::setActiveTab(UITab tab)
{
    currentTab_ = tab;

    bool showBuildings = (tab == UITab::Buildings);
    if (buildingBarBackgroundIndex_ != SIZE_MAX)
        uiManager_.setButtonVisibility(buildingBarBackgroundIndex_, showBuildings);
    for (size_t idx : buildingButtonIndices_)
        uiManager_.setButtonVisibility(idx, showBuildings);
    for (size_t idx : buildingLabelIndices_)
        uiManager_.setLabelVisibility(idx, showBuildings);
    if (buildingInfoPanelIndex_ != SIZE_MAX)
        uiManager_.setButtonVisibility(buildingInfoPanelIndex_, showBuildings);
    if (buildingInfoTitleLabelIndex_ != SIZE_MAX)
        uiManager_.setLabelVisibility(buildingInfoTitleLabelIndex_, showBuildings);
    if (buildingInfoTextLabelIndex_ != SIZE_MAX)
        uiManager_.setLabelVisibility(buildingInfoTextLabelIndex_, showBuildings);

    bool showUnits = (tab == UITab::Units);
    if (unitPanelBackgroundIndex_ != SIZE_MAX)
        uiManager_.setButtonVisibility(unitPanelBackgroundIndex_, showUnits);
    if (unitPanelTitleLabelIndex_ != SIZE_MAX)
        uiManager_.setLabelVisibility(unitPanelTitleLabelIndex_, showUnits);
    refreshUnitListUI();

    if (buildingTabLabelIndex_ != SIZE_MAX)
    {
        uiManager_.setLabelText(buildingTabLabelIndex_,
            tab == UITab::Buildings ? "[Buildings]" : "Buildings");
    }
    if (unitTabLabelIndex_ != SIZE_MAX)
    {
        uiManager_.setLabelText(unitTabLabelIndex_,
            tab == UITab::Units ? "[Units]" : "Units");
    }
}

void Scene::refreshUnitListUI()
{
    bool show = (currentTab_ == UITab::Units);
    if (!show)
    {
        for (size_t idx : unitEntryIconIndices_)
            uiManager_.setButtonVisibility(idx, false);
        for (size_t idx : unitEntryLabelIndices_)
        {
            uiManager_.setLabelVisibility(idx, false);
            uiManager_.setLabelText(idx, "");
        }
        for (Unit*& target : unitEntryTargets_)
            target = nullptr;
        return;
    }

    std::vector<Unit*> units;
    units.reserve(entities_.size());
    for (GameEntity* e : entities_)
    {
        if (Unit* unit = dynamic_cast<Unit*>(e))
        {
            if (unit->ownerID != activePlayerIndex_ + 1)
                continue;
            units.push_back(unit);
        }
    }

    std::unordered_map<EntityType, int> counts;
    size_t displayCount = std::min(units.size(), unitEntryIconIndices_.size());

    for (size_t i = 0; i < unitEntryIconIndices_.size(); ++i)
    {
        bool active = (i < displayCount);
        uiManager_.setButtonVisibility(unitEntryIconIndices_[i], active);
        uiManager_.setLabelVisibility(unitEntryLabelIndices_[i], active);
        if (!active)
        {
            uiManager_.setLabelText(unitEntryLabelIndices_[i], "");
            if (i < unitEntryTargets_.size())
                unitEntryTargets_[i] = nullptr;
        }
    }

    auto iconForUnit = [&](EntityType type, bool evilOwner) -> Texture*
    {
        switch (type)
        {
        case EntityType::Worker: return evilOwner ? evilVillagerIconTex : villagerIconTex;
        case EntityType::Archer: return evilOwner ? evilArcherIconTex : archerIconTex;
        case EntityType::Knight: return evilOwner ? evilKnightIconTex : knightIconTex;
        default: return nullptr;
        }
    };

    for (size_t i = 0; i < displayCount; ++i)
    {
        Unit* unit = units[i];
        EntityType type = unit->type;
        counts[type]++;

        Texture* icon = nullptr;
        std::string prefix = "Unit";
        if (type == EntityType::Worker)
        {
            icon = iconForUnit(type, unit->ownerID == 2);
            prefix = "Villager";
        }
        else if (type == EntityType::Archer)
        {
            icon = iconForUnit(type, unit->ownerID == 2);
            prefix = "Archer";
        }
        else if (type == EntityType::Knight)
        {
            icon = iconForUnit(type, unit->ownerID == 2);
            prefix = "Knight";
        }

        uiManager_.setButtonTexture(unitEntryIconIndices_[i], icon ? icon->ID : 0);
        std::string label = prefix + " " + std::to_string(counts[type]);
        uiManager_.setLabelText(unitEntryLabelIndices_[i], label);
        if (i < unitEntryTargets_.size())
            unitEntryTargets_[i] = unit;
    }
}

void Scene::selectUnitFromList(size_t entryIndex)
{
    if (entryIndex >= unitEntryTargets_.size())
        return;

    Unit* unit = unitEntryTargets_[entryIndex];
    if (!unit)
        return;

    clearUnitSelection();
    unit->SetSelected(true);
    selectedUnits_.push_back(unit);
    selectedBuilding_ = nullptr;
    updateProductionPanel();
    updateUnitInfoPanel();
}

void Scene::updateProductionPanel()
{
    bool hasBuilding = (selectedBuilding_ != nullptr);
    uiManager_.setButtonVisibility(productionPanelBackgroundIndex_, hasBuilding);
    if (!hasBuilding)
    {
        for (size_t idx : productionButtonIndices_)
            uiManager_.setButtonVisibility(idx, false);
        for (size_t idx : productionLabelIndices_)
            uiManager_.setLabelVisibility(idx, false);
        if (productionUpgradeButtonIndex_ != SIZE_MAX)
            uiManager_.setButtonVisibility(productionUpgradeButtonIndex_, false);
        if (productionUpgradeLabelIndex_ != SIZE_MAX)
            uiManager_.setLabelVisibility(productionUpgradeLabelIndex_, false);
        if (productionUpgradeCostLabelIndex_ != SIZE_MAX)
            uiManager_.setLabelVisibility(productionUpgradeCostLabelIndex_, false);
        return;
    }

    if (selectedBuilding_->isUnderConstruction)
    {
        for (size_t idx : productionButtonIndices_)
            uiManager_.setButtonVisibility(idx, false);
        for (size_t idx : productionLabelIndices_)
            uiManager_.setLabelVisibility(idx, false);
        if (productionUpgradeButtonIndex_ != SIZE_MAX)
            uiManager_.setButtonVisibility(productionUpgradeButtonIndex_, false);
        if (productionUpgradeLabelIndex_ != SIZE_MAX)
            uiManager_.setLabelVisibility(productionUpgradeLabelIndex_, false);
        if (productionUpgradeCostLabelIndex_ != SIZE_MAX)
            uiManager_.setLabelVisibility(productionUpgradeCostLabelIndex_, false);
        return;
    }

    for (size_t i = 0; i < productionButtonIndices_.size(); ++i)
    {
        EntityType type = productionButtonTypes_[i];
        bool visible = false;
        if (selectedBuilding_->type == EntityType::TownCenter && type == EntityType::Worker)
            visible = true;
        else if (selectedBuilding_->type == EntityType::Barracks &&
                (type == EntityType::Archer || type == EntityType::Knight))
            visible = true;

        bool ownerIsEvil = (selectedBuilding_->ownerID == 2);
        Texture* iconTex = nullptr;
        if (type == EntityType::Worker)
            iconTex = ownerIsEvil ? evilVillagerIconTex : villagerIconTex;
        else if (type == EntityType::Archer)
            iconTex = ownerIsEvil ? evilArcherIconTex : archerIconTex;
        else if (type == EntityType::Knight)
            iconTex = ownerIsEvil ? evilKnightIconTex : knightIconTex;

        std::string label = "Unit";
        if (type == EntityType::Worker) label = "Villager";
        else if (type == EntityType::Archer) label = "Archer";
        else if (type == EntityType::Knight) label = "Knight";
        if (selectedBuilding_->type == EntityType::Barracks)
        {
            const Barracks* barracks = dynamic_cast<const Barracks*>(selectedBuilding_);
            if (barracks && barracks->HasEmpoweredTraining() &&
                (type == EntityType::Archer || type == EntityType::Knight))
            {
                label += " x1.5 DMG";
            }
        }

        uiManager_.setButtonTexture(productionButtonIndices_[i], iconTex ? iconTex->ID : 0);
        uiManager_.setButtonVisibility(productionButtonIndices_[i], visible);
        uiManager_.setLabelVisibility(productionLabelIndices_[i], visible);
        uiManager_.setLabelText(productionLabelIndices_[i], label);
    }

    bool showUpgrade = false;
    std::string upgradeLabel = "Upgrade";
    std::string upgradeCostText;
    BuildType type = buildTypeFromEntityType(selectedBuilding_->type);
    if (selectedBuilding_->ownerID == activePlayerIndex_ + 1 &&
        type != BuildType::None && type != BuildType::Bridge)
    {
        if (selectedBuilding_->CanUpgrade())
        {
            UnitCost cost = getUpgradeCost(type, selectedBuilding_->GetLevel());
            std::ostringstream oss;
            oss << "Cost: F" << cost.food
                << " W" << cost.wood
                << " O" << cost.ore
                << " G" << cost.gold;
            upgradeCostText = oss.str();
            if (!canAffordUpgrade(selectedBuilding_))
                upgradeLabel = "Upgrade (Need Resources)";
            showUpgrade = true;
        }
        else
        {
            upgradeLabel = "Max Level";
            upgradeCostText = "No further upgrades.";
            showUpgrade = true;
        }
    }

    if (productionUpgradeLabelIndex_ != SIZE_MAX)
        uiManager_.setLabelText(productionUpgradeLabelIndex_, upgradeLabel);
    if (productionUpgradeCostLabelIndex_ != SIZE_MAX)
        uiManager_.setLabelText(productionUpgradeCostLabelIndex_, upgradeCostText);

    if (productionUpgradeButtonIndex_ != SIZE_MAX)
        uiManager_.setButtonVisibility(productionUpgradeButtonIndex_, showUpgrade);
    if (productionUpgradeLabelIndex_ != SIZE_MAX)
        uiManager_.setLabelVisibility(productionUpgradeLabelIndex_, showUpgrade);
    if (productionUpgradeCostLabelIndex_ != SIZE_MAX)
        uiManager_.setLabelVisibility(productionUpgradeCostLabelIndex_, showUpgrade);
}

bool Scene::handleProductionRequest(EntityType unitType)
{
    if (!selectedBuilding_ || selectedBuilding_->isUnderConstruction)
        return false;
    if (selectedBuilding_->ownerID != activePlayerIndex_ + 1)
        return false;

    if (unitManager_.TrainUnit(unitType, selectedBuilding_))
    {
        updateResourceTexts();
        refreshUnitListUI();
        updateProductionPanel();

        int trainedNetId = -1;
        if (GameEntity* spawned = unitManager_.GetLastSpawnedEntity())
        {
            if (Unit* spawnedUnit = dynamic_cast<Unit*>(spawned))
                trainedNetId = registerEntity(spawnedUnit);
        }

        if (lanModeActive_ && networkSession_.IsConnected() && !suppressNetworkSend_ &&
            unitManager_.HasPendingSpawn() && trainedNetId > 0)
        {
            sendTrainCommand(unitManager_.GetLastTrainedType(),
                             selectedBuilding_->ownerID,
                             unitManager_.GetLastSpawnPosition(),
                             trainedNetId);
        }
        return true;
    }
    return false;
}

void Scene::updateUnitInfoPanel()
{
    Unit* unit = selectedUnits_.empty() ? nullptr : selectedUnits_.front();
    unitInfoTarget_ = unit;
    bool show = (unit != nullptr);

    auto setButtonVisible = [&](size_t idx, bool visible)
    {
        if (idx != SIZE_MAX)
            uiManager_.setButtonVisibility(idx, visible);
    };
    auto setLabelVisible = [&](size_t idx, bool visible)
    {
        if (idx != SIZE_MAX)
            uiManager_.setLabelVisibility(idx, visible);
    };

    setButtonVisible(unitInfoPanelBackgroundIndex_, show);
    setLabelVisible(unitInfoNameLabelIndex_, show);
    setLabelVisible(unitInfoHealthLabelIndex_, show);
    setButtonVisible(unitDeleteButtonIndex_, show);
    setLabelVisible(unitDeleteLabelIndex_, show);

    if (!show)
        return;

    auto getUnitName = [](EntityType type) -> std::string
    {
        switch (type)
        {
        case EntityType::Worker: return "Villager";
        case EntityType::Archer: return "Archer";
        case EntityType::Knight: return "Knight";
        default: return "Unit";
        }
    };

    if (unitInfoNameLabelIndex_ != SIZE_MAX)
    {
        uiManager_.setLabelText(unitInfoNameLabelIndex_, getUnitName(unit->type));
    }

    if (unitInfoHealthLabelIndex_ != SIZE_MAX)
    {
        int hp = static_cast<int>(unit->GetHealth());
        int maxHp = static_cast<int>(unit->GetMaxHealth());
        uiManager_.setLabelText(unitInfoHealthLabelIndex_, "HP: " + std::to_string(hp) + "/" + std::to_string(maxHp));
    }
}

void Scene::handleDeleteCurrentUnit()
{
    if (!unitInfoTarget_)
        return;
    deleteUnit(unitInfoTarget_);
}

void Scene::updateBuildingInfoPanel(BuildType type)
{
    if (buildingInfoTitleLabelIndex_ == SIZE_MAX || buildingInfoTextLabelIndex_ == SIZE_MAX)
        return;

    Building* selectedOwnedBuilding = nullptr;
    if (selectedBuilding_ && selectedBuilding_->ownerID == activePlayerIndex_ + 1)
        selectedOwnedBuilding = selectedBuilding_;

    if (selectedOwnedBuilding)
    {
        BuildType selectedType = buildTypeFromEntityType(selectedOwnedBuilding->type);
        if (selectedType != BuildType::None)
            type = selectedType;
    }

    std::string title = "Building Info";
    std::string text = "Select a building to see its description.";

    const auto& infoMap = buildingInfoMapForOwner(activePlayerIndex_ + 1);
    auto infoForType = [&](BuildType t) -> std::string
    {
        auto it = infoMap.find(t);
        if (it != infoMap.end())
            return it->second;
        if (&infoMap != &buildingInfoText_)
        {
            auto fallbackIt = buildingInfoText_.find(t);
            if (fallbackIt != buildingInfoText_.end())
                return fallbackIt->second;
        }
        return "No description available.";
    };

    if (type != BuildType::None)
    {
        title = getBuildingName(type);
        if (selectedOwnedBuilding)
        {
            title += " - Level " + std::to_string(selectedOwnedBuilding->GetLevel());
        }

        text = infoForType(type);
        if (selectedOwnedBuilding)
        {
            text += "\n";
            switch (type)
            {
            case BuildType::House:
                text += "Population bonus: +" + std::to_string(5 * selectedOwnedBuilding->GetLevel());
                break;
            case BuildType::Farm:
                text += "Food income: " + std::to_string(5 * selectedOwnedBuilding->GetLevel()) + " every 3s";
                break;
            case BuildType::Market:
                text += "Gold income: " + std::to_string(5 * selectedOwnedBuilding->GetLevel()) + " every 5s";
                break;
            case BuildType::Storage:
                text += "Storage bonus: +" + std::to_string(150 * selectedOwnedBuilding->GetLevel()) +
                        "F +" + std::to_string(150 * selectedOwnedBuilding->GetLevel()) +
                        "W +" + std::to_string(80 * selectedOwnedBuilding->GetLevel()) +
                        "O +" + std::to_string(120 * selectedOwnedBuilding->GetLevel()) + "G";
                break;
            case BuildType::Barracks:
                text += (selectedOwnedBuilding->GetLevel() >= 2)
                    ? "Trains empowered units (x1.5 damage)."
                    : "Can be upgraded to train empowered units.";
                break;
            default:
                text += "Upgrade improves durability.";
                break;
            }
        }
    }

    uiManager_.setLabelText(buildingInfoTitleLabelIndex_, title);
    uiManager_.setLabelText(buildingInfoTextLabelIndex_, text);


}

std::string Scene::getBuildingName(BuildType type) const
{
    return buildingNameForOwner(type, activePlayerIndex_ + 1);
}

std::string Scene::buildingNameForOwner(BuildType type, int ownerId) const
{
    const bool evil = (ownerId == 2);
    switch (type)
    {
    case BuildType::TownCenter: return evil ? "Altar" : "Town Hall";
    case BuildType::Barracks:   return evil ? "Graveyard" : "Barracks";
    case BuildType::Farm:       return evil ? "Hangman" : "Farm";
    case BuildType::House:      return evil ? "Hut" : "House";
    case BuildType::Market:     return evil ? "Smithy" : "Market";
    case BuildType::Storage:    return evil ? "Temple" : "Storage";
    case BuildType::Bridge:     return "Bridge";
    default:                    return evil ? "Structure" : "Building";
    }
}

const std::unordered_map<BuildType, std::string>& Scene::buildingInfoMapForOwner(int ownerId) const
{
    if (ownerId == 2 && !evilBuildingInfoText_.empty())
        return evilBuildingInfoText_;
    return buildingInfoText_;
}

BuildType Scene::buildTypeFromEntityType(EntityType type) const
{
    switch (type)
    {
    case EntityType::TownCenter: return BuildType::TownCenter;
    case EntityType::Barracks:   return BuildType::Barracks;
    case EntityType::Farm:       return BuildType::Farm;
    case EntityType::House:      return BuildType::House;
    case EntityType::Market:     return BuildType::Market;
    case EntityType::Storage:    return BuildType::Storage;
    case EntityType::Bridge:     return BuildType::Bridge;
    default:                     return BuildType::None;
    }
}

UnitCost Scene::getUpgradeCost(BuildType type, int currentLevel) const
{
    UnitCost cost;
    if (currentLevel >= 2)
        return cost;

    switch (type)
    {
    case BuildType::TownCenter:
        cost.wood = 450; cost.ore = 220; cost.gold = 160;
        break;
    case BuildType::Barracks:
        cost.wood = 280; cost.ore = 120; cost.gold = 120;
        break;
    case BuildType::Farm:
        cost.wood = 120; cost.ore = 20; cost.gold = 40;
        break;
    case BuildType::House:
        cost.wood = 90; cost.gold = 30;
        break;
    case BuildType::Market:
        cost.wood = 180; cost.ore = 70; cost.gold = 120;
        break;
    case BuildType::Storage:
        cost.wood = 150; cost.ore = 70; cost.gold = 70;
        break;
    default:
        break;
    }
    return cost;
}

bool Scene::canAffordUpgrade(const Building* building) const
{
    if (!building)
        return false;
    BuildType type = buildTypeFromEntityType(building->type);
    if (type == BuildType::None || type == BuildType::Bridge)
        return false;
    if (building->ownerID != activePlayerIndex_ + 1 || !building->CanUpgrade())
        return false;

    UnitCost cost = getUpgradeCost(type, building->GetLevel());
    const Resources* res = resourcesForOwner(building->ownerID);
    if (!res)
        return false;
    return res->food >= cost.food &&
           res->wood >= cost.wood &&
           res->ore  >= cost.ore &&
           res->gold >= cost.gold;
}

bool Scene::handleUpgradeRequest()
{
    if (!selectedBuilding_)
        return false;
    if (selectedBuilding_->ownerID != activePlayerIndex_ + 1)
        return false;

    BuildType type = buildTypeFromEntityType(selectedBuilding_->type);
    if (type == BuildType::None || type == BuildType::Bridge)
        return false;
    if (!selectedBuilding_->CanUpgrade())
        return false;

    Resources* ownerRes = resourcesForOwner(selectedBuilding_->ownerID);
    if (!ownerRes)
        return false;

    UnitCost cost = getUpgradeCost(type, selectedBuilding_->GetLevel());
    if (!ownerRes->Spend(cost))
    {
        std::cout << "Insufficient resources for upgrade." << std::endl;
        updateBuildingInfoPanel(type);
        return false;
    }

    if (!selectedBuilding_->UpgradeLevel())
        return false;

    selectedBuilding_->SetMaxHealth(selectedBuilding_->GetMaxHealth() * 1.5f);
    selectedBuilding_->RestoreFullHealth();

    if (selectedBuilding_->ownerID != 2)
    {
        Model* upgradedModel = modelForBuildType(type,
                                                 selectedBuilding_->ownerID,
                                                 selectedBuilding_->GetLevel());
        if (upgradedModel)
            selectedBuilding_->StartUpgradeTransition(upgradedModel, 1.0f);
    }

    applyBuildingVisualTweaks(selectedBuilding_, type, selectedBuilding_->ownerID);

    if (lanModeActive_ && networkSession_.IsConnected() && !suppressNetworkSend_)
    {
        int buildingNetId = selectedBuilding_->GetNetworkId();
        if (buildingNetId > 0)
            sendUpgradeCommand(selectedBuilding_->ownerID, buildingNetId, selectedBuilding_->GetLevel());
    }

    updateResourceTexts();
    updateProductionPanel();
    updateBuildingInfoPanel(type);
    std::cout << "Upgraded " << getBuildingName(type)
              << " to level " << selectedBuilding_->GetLevel() << std::endl;
    return true;
}

UnitCost Scene::getBuildingCost(BuildType type) const
{
    UnitCost cost;
    switch (type)
    {
    case BuildType::TownCenter:
        cost.food = 0;
        cost.wood = 300;
        cost.ore  = 150;
        cost.gold = 100;
        break;
    case BuildType::Barracks:
        cost.food = 0;
        cost.wood = 200;
        cost.ore  = 80;
        cost.gold = 50;
        break;
    case BuildType::Farm:
        cost.food = 0;
        cost.wood = 75;
        cost.ore  = 0;
        cost.gold = 0;
        break;
    case BuildType::House:
        cost.food = 0;
        cost.wood = 60;
        cost.ore  = 0;
        cost.gold = 0;
        break;
    case BuildType::Market:
        cost.food = 0;
        cost.wood = 120;
        cost.ore  = 40;
        cost.gold = 60;
        break;
    case BuildType::Storage:
        cost.food = 0;
        cost.wood = 90;
        cost.ore  = 30;
        cost.gold = 0;
        break;
    case BuildType::Bridge:
        cost.food = 0;
        cost.wood = 180;
        cost.ore  = 60;
        cost.gold = 0;
        break;
    default:
        break;
    }
    return cost;
}

UnitCost Scene::getUnitCost(EntityType type) const
{
    UnitCost cost;
    switch (type)
    {
    case EntityType::Worker:
        cost.food = 50;
        break;
    case EntityType::Archer:
        cost.food = 40;
        cost.ore  = 20;
        cost.gold = 45;
        break;
    case EntityType::Knight:
        cost.food = 60;
        cost.ore  = 35;
        cost.gold = 60;
        break;
    default:
        break;
    }
    return cost;
}

bool Scene::canAffordBuilding(BuildType type) const
{
    UnitCost cost = getBuildingCost(type);
    const Resources& res = activePlayer();
    return res.food >= cost.food &&
           res.wood >= cost.wood &&
           res.ore  >= cost.ore &&
           res.gold >= cost.gold;
}

void Scene::startSinglePlayerGame()
{
    lanModeActive_ = false;
    lanSessionPending_ = false;
    networkSession_.Shutdown();
    beginGameplay(false);
}

void Scene::startLanHostGame()
{
    const uint16_t port = 47017;
    lanModeActive_ = true;
    lanIsHost_ = true;
    bool ok = networkSession_.StartHosting(port);
    lanSessionPending_ = ok;
    lanStatusText_ = ok
        ? "Hosting on port " + std::to_string(port) + ", waiting for player..."
        : "Failed to start LAN host.";
    if (mainMenuStatusLabelIndex_ != SIZE_MAX)
        uiManager_.setLabelText(mainMenuStatusLabelIndex_, lanStatusText_);
    if (!ok)
        lanModeActive_ = false;
}

void Scene::startLanJoinGame()
{
    const uint16_t port = 47017;
    std::string host = readLanAddress();
    lanModeActive_ = true;
    lanIsHost_ = false;
    bool ok = networkSession_.ConnectToHost(host, port);
    lanSessionPending_ = ok;
    lanStatusText_ = ok
        ? "Connecting to " + host + ":" + std::to_string(port) + " ..."
        : "Failed to start LAN client.";
    if (mainMenuStatusLabelIndex_ != SIZE_MAX)
        uiManager_.setLabelText(mainMenuStatusLabelIndex_, lanStatusText_);
    if (!ok)
        lanModeActive_ = false;
}

void Scene::beginGameplay(bool enableLanMode)
{
    mainMenuActive_ = false;
    lanModeActive_ = enableLanMode;
    lanSessionPending_ = false;
    setMainMenuVisible(false);
    victoryShown_ = false;
    if (victoryLabelIndex_ != SIZE_MAX)
        uiManager_.setLabelVisibility(victoryLabelIndex_, false);
    resetFogOfWar();
    spawnStartingTownCenters();
    spawnObjectiveTemple();

    // Partition the network-ID space so host and join never collide.
    // Both sides already share IDs 1–N for the starting entities.
    // After that, host keeps allocating from N+1, join jumps to 10000+.
    if (enableLanMode && networkSession_.GetMode() == NetworkSession::Mode::Client)
    {
        if (nextNetworkId_ < 10000)
            nextNetworkId_ = 10000;
    }

    timerElapsed_ = 0.0f;
    captureState_ = CaptureState::Neutral;
    captureProgress_ = 0.0f;
    updateCaptureUI();
    updateResourceTexts();
}

void Scene::updateMainMenu(float /*dt*/)
{
    if (mainMenuStatusLabelIndex_ != SIZE_MAX)
    {
        if (lanSessionPending_)
            lanStatusText_ = networkSession_.GetStatus();
        uiManager_.setLabelText(mainMenuStatusLabelIndex_, lanStatusText_.empty()
                                ? "Select a mode to begin."
                                : lanStatusText_);
    }

    if (lanSessionPending_ && networkSession_.IsConnected())
    {
        beginGameplay(true);
    }
}

std::string Scene::readLanAddress() const
{
    std::string defaultIp = "127.0.0.1";
    std::string path = AssetPath("config/lan_peer.txt");
    std::ifstream file(path);
    if (!file)
        return defaultIp;

    std::string value;
    std::getline(file, value);
    if (value.empty())
        return defaultIp;
    return value;
}
