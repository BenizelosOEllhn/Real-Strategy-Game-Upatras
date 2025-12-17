#include "Scene.h"
#include <fstream>
#include <functional>

void Scene::setupBuildingBar()
{
    const float buttonSize = 128.0f;
    const float barHeight = 150.0f;
    const float padding    = 32.0f;
    const float spacing    = 64.0f;
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

    int i = 0;
    const float labelScale   = 1.6f;
    const float charW        = 8.0f * labelScale;

    auto addButton = [&](const char* fileName,
                         const char* label,
                         BuildType type)
    {
        UIButton btn;
        btn.pos  = glm::vec2(padding + i * (buttonSize + spacing),
                             baseY + (barHeight - buttonSize) * 0.5f - 8.0f);
        btn.size = glm::vec2(buttonSize, buttonSize);

        std::string fullPath = std::string(ASSET_PATH) + "gui/" + fileName;
        Texture* btnTex = new Texture(fullPath.c_str());
        btn.texture = btnTex->ID;

        btn.onClick = [this, type]() {
            updateBuildingInfoPanel(type);
            buildingManager_.startPlacing(type);
        };

        size_t idx = uiManager_.addButton(btn);
        buildingButtonIndices_.push_back(idx);

        std::string text(label);
        float textWidth = text.size() * charW;
        float centerX   = btn.pos.x + btn.size.x * 0.5f;
        float labelX    = centerX - textWidth * 0.5f;
        float labelY    = bar.pos.y + 8.0f;

        size_t labelIdx = uiManager_.addLabel(text, glm::vec2(labelX, labelY), labelScale);
        buildingLabelIndices_.push_back(labelIdx);

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
    const float paddingX = 30.0f;
    const float iconSize = 48.0f;
    const float labelScale = 1.6f;
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

        cursorX = textX + 190.0f;
    };

    addEntry(cornIconTex, "corn.png", foodLabelIndex_);
    addEntry(woodIconTex, "log.png", woodLabelIndex_);
    addEntry(goldIconTex, "gold-bar.png", goldLabelIndex_);
    addEntry(oreIconTex,  "ore.png", oreLabelIndex_);
    addEntry(populationIconTex, "village.png", populationLabelIndex_);

    float playerLabelX = static_cast<float>(fbWidth) - 180.0f;
    float playerLabelY = baseY + barHeight * 0.5f - 10.0f;
    playerLabelIndex_ = uiManager_.addLabel("Player 1", glm::vec2(playerLabelX, playerLabelY), 1.4f);

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
    const glm::vec2 tabSize(150.0f, 40.0f);
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

        glm::vec2 labelPos = glm::vec2(btn.pos.x + 12.0f, btn.pos.y + btn.size.y * 0.5f - 6.0f);
        labelIndex = uiManager_.addLabel(text, labelPos, 1.2f);

        start.x += tabSize.x + 20.0f;
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
    const float panelHeight = buildingBarSize_.y + 160.0f;
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

    float cursorY = panel.pos.y + panel.size.y - 120.0f;
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
    panel.size = glm::vec2(420.0f, 140.0f);
    panel.texture = 0;
    panel.clickable = false;
    buildingInfoPanelIndex_ = uiManager_.addButton(panel);

    glm::vec2 titlePos = panel.pos + glm::vec2(16.0f, panel.size.y - 32.0f);
    buildingInfoTitleLabelIndex_ = uiManager_.addLabel("Building Info", titlePos, 1.6f);

    glm::vec2 textPos = panel.pos + glm::vec2(16.0f, panel.size.y - 86.0f);
    buildingInfoTextLabelIndex_ = uiManager_.addLabel("Select a building to see its role.", textPos, 1.3f);

    buildingInfoText_.clear();
    buildingInfoText_[BuildType::TownCenter] = "Town Center\nTrains villagers, stores goods.";
    buildingInfoText_[BuildType::Barracks]   = "Barracks\nProduces rangers and knights.";
    buildingInfoText_[BuildType::Farm]       = "Farm\nGenerates a steady food trickle.";
    buildingInfoText_[BuildType::House]      = "House\nAdds +5 population cap.";
    buildingInfoText_[BuildType::Market]     = "Market\nPassive gold income & trades.";
    buildingInfoText_[BuildType::Storage]    = "Storage\nExpands resource storage caps.";

    updateBuildingInfoPanel(BuildType::None);
}

void Scene::setupMainMenu()
{
    const glm::vec2 menuSize(420.0f, 320.0f);
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
    mainMenuTitleLabelIndex_ = uiManager_.addLabel("Chronicles in Nature", titlePos, 2.2f);

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
        labelIndex = uiManager_.addLabel(text, labelPos, 1.5f);
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

    for (size_t i = 0; i < displayCount; ++i)
    {
        Unit* unit = units[i];
        EntityType type = unit->type;
        counts[type]++;

        Texture* icon = nullptr;
        std::string prefix = "Unit";
        if (type == EntityType::Worker)
        {
            icon = villagerIconTex;
            prefix = "Villager";
        }
        else if (type == EntityType::Archer)
        {
            icon = archerIconTex;
            prefix = "Archer";
        }
        else if (type == EntityType::Knight)
        {
            icon = knightIconTex;
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
        return;
    }

    if (selectedBuilding_->isUnderConstruction)
    {
        for (size_t idx : productionButtonIndices_)
            uiManager_.setButtonVisibility(idx, false);
        for (size_t idx : productionLabelIndices_)
            uiManager_.setLabelVisibility(idx, false);
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

        uiManager_.setButtonVisibility(productionButtonIndices_[i], visible);
        uiManager_.setLabelVisibility(productionLabelIndices_[i], visible);
    }
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

    std::string title = (type == BuildType::None) ? "Building Info" : getBuildingName(type);
    auto it = buildingInfoText_.find(type);
    std::string text = (it != buildingInfoText_.end())
        ? it->second
        : "Select a building to see its description.";

    uiManager_.setLabelText(buildingInfoTitleLabelIndex_, title);
    uiManager_.setLabelText(buildingInfoTextLabelIndex_, text);
}

std::string Scene::getBuildingName(BuildType type) const
{
    switch (type)
    {
    case BuildType::TownCenter: return "Town Center";
    case BuildType::Barracks:   return "Barracks";
    case BuildType::Farm:       return "Farm";
    case BuildType::House:      return "House";
    case BuildType::Market:     return "Market";
    case BuildType::Storage:    return "Storage";
    default:                    return "Building Info";
    }
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
    std::string path = std::string(ASSET_PATH) + "config/lan_peer.txt";
    std::ifstream file(path);
    if (!file)
        return defaultIp;

    std::string value;
    std::getline(file, value);
    if (value.empty())
        return defaultIp;
    return value;
}
