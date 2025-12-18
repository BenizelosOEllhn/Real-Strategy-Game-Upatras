#include "Scene.h"
#include <algorithm>
#include <limits>
#include <sstream>
#include <glm/gtc/constants.hpp>

void Scene::Update(float dt, const Camera& cam)
{   
    int fbW, fbH;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &fbW, &fbH);
    fbWidth = fbW;
    fbHeight = fbH;

    if (!camera)
        camera = const_cast<Camera*>(&cam);
    
    uiManager_.update(mouseX_, mouseY_);
    if (draggingSelection_)
        uiManager_.setSelectionRect(dragStart_, dragCurrent_, true);
    else
        uiManager_.setSelectionRect(dragStart_, dragStart_, false);

    if (mainMenuActive_)
    {
        updateMainMenu(dt);
        return;
    }
    
    // Pass the camera AND dimensions
    buildingManager_.update(mouseX_, mouseY_, fbW, fbH, cam); 

    updateResourceTexts();

    for (GameEntity* e : entities_)
    {
        if (e)
            e->Update(dt);
    }

    refreshUnitListUI();
    updateProductionPanel();
    updateUnitInfoPanel();
    updateGatherTasks(dt);
    updateCombat(dt);
    updateFogOfWar();
    processNetworkMessages();
    updateUnitCameraView();
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

    if (draggingSelection_)
    {
        dragCurrent_ = glm::vec2(mouseX_, mouseY_);
    }
}
    

void Scene::onMouseButton(int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            bool uiConsumed = uiManager_.handleClick(mouseX_, mouseY_);
            if (uiConsumed)
                return;
            if (mainMenuActive_)
                return;

            if (buildingManager_.isPlacing())
            {
                buildingManager_.confirmPlacement(mouseX_, mouseY_);
                return;
            }

            draggingSelection_ = true;
            additiveSelection_ = (mods & GLFW_MOD_SHIFT);
            dragStart_ = glm::vec2(mouseX_, mouseY_);
            dragCurrent_ = dragStart_;
        }
        else if (action == GLFW_RELEASE)
        {
            if (mainMenuActive_)
                return;
            if (!draggingSelection_)
                return;

            draggingSelection_ = false;
            glm::vec2 end(mouseX_, mouseY_);
            float dragDist = glm::length(end - dragStart_);
            if (dragDist < 5.0f)
            {
                selectSingleUnit(end, additiveSelection_);
            }
            else
            {
                selectUnitsInRect(dragStart_, end, additiveSelection_);
            }
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        if (mainMenuActive_)
            return;
        if (buildingManager_.isPlacing())
            return;
        issueMoveCommand();
    }
}

void Scene::cancelCurrentAction()
{
    if (mainMenuActive_)
        return;
    if (buildingManager_.isPlacing())
    {
        buildingManager_.startPlacing(BuildType::None);
        updateBuildingInfoPanel(BuildType::None);
        return;
    }

    for (Unit* unit : selectedUnits_)
    {
        if (!unit) continue;
        clearGatherTasksFor(unit);
        unit->ClearMoveTarget();
    }
}

void Scene::rotatePlacementPreview(float radians)
{
    if (mainMenuActive_)
        return;
    if (!buildingManager_.isPlacing())
        return;
    buildingManager_.rotatePreviewYaw(radians);
}

void Scene::toggleUnitCamera()
{
    if (!camera)
        return;

    if (!unitCameraActive_)
    {
        if (selectedUnits_.empty() || !selectedUnits_.front())
            return;
        unitCameraTarget_ = selectedUnits_.front();
        unitCameraActive_ = true;
        savedCameraPos_ = camera->Position;
        savedCameraYaw_ = camera->Yaw;
        savedCameraPitch_ = camera->Pitch;
        unitCameraYawOffset_ = 0.0f;
        unitCameraPitchOffset_ = 0.0f;
    }
    else
    {
        unitCameraActive_ = false;
        unitCameraTarget_ = nullptr;
        unitCameraYawOffset_ = 0.0f;
        unitCameraPitchOffset_ = 0.0f;
        camera->SetPose(savedCameraPos_, savedCameraYaw_, savedCameraPitch_);
    }
}

void Scene::RotateUnitCamera(float yawDeltaDeg, float pitchDeltaDeg)
{
    if (!unitCameraActive_)
        return;
    unitCameraYawOffset_ += yawDeltaDeg;
    unitCameraPitchOffset_ = glm::clamp(unitCameraPitchOffset_ + pitchDeltaDeg, -35.0f, 25.0f);
}

void Scene::updateUnitCameraView()
{
    if (!unitCameraActive_ || !camera)
        return;

    if (!unitCameraTarget_)
    {
        unitCameraActive_ = false;
        camera->SetPose(savedCameraPos_, savedCameraYaw_, savedCameraPitch_);
        return;
    }

    glm::vec3 pos = unitCameraTarget_->position;
    pos.y += 9.0f;
    float yawDeg = glm::degrees(unitCameraTarget_->GetYaw()) + unitCameraYawOffset_;
    float pitchDeg = -5.0f + unitCameraPitchOffset_;
    float yawRad = glm::radians(yawDeg);
    glm::vec3 forward(std::cos(yawRad), 0.0f, std::sin(yawRad));
    pos += forward * 2.5f;
    camera->SetPose(pos, yawDeg, pitchDeg);
}

void Scene::focusCameraOnTownCenter()
{
    if (!camera)
        return;

    const int owner = activePlayerIndex_ + 1;
    TownCenter* target = nullptr;
    for (TownCenter* tc : townCenters_)
    {
        if (tc && tc->ownerID == owner)
        {
            target = tc;
            break;
        }
    }

    if (!target)
        return;

    glm::vec3 focus = target->position;
    const float height = 160.0f;
    const float backOffset = 180.0f;
    glm::vec3 camPos = focus + glm::vec3(0.0f, height, backOffset);

    glm::vec3 dir = glm::normalize(focus - camPos);
    float yawDeg = glm::degrees(std::atan2(dir.z, dir.x));
    float pitchDeg = glm::degrees(std::asin(dir.y));

    camera->SetPose(camPos, yawDeg, pitchDeg);
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

void Scene::clearUnitSelection()
{
    for (Unit* unit : selectedUnits_)
    {
        if (unit)
            unit->SetSelected(false);
    }
    selectedUnits_.clear();
    updateUnitInfoPanel();
}

glm::vec2 Scene::worldToScreen(const glm::vec3& worldPos) const
{
    glm::vec4 clip = lastProjMatrix_ * lastViewMatrix_ * glm::vec4(worldPos, 1.0f);
    if (clip.w == 0.0f)
        return glm::vec2(-1000.0f);

    glm::vec3 ndc = glm::vec3(clip) / clip.w;
    glm::vec2 screen;
    screen.x = (ndc.x * 0.5f + 0.5f) * static_cast<float>(fbWidth);
    screen.y = (ndc.y * 0.5f + 0.5f) * static_cast<float>(fbHeight);
    return screen;
}

void Scene::selectSingleUnit(const glm::vec2& screenPos, bool additive)
{
    Unit* bestUnit = nullptr;
    float bestDist = std::numeric_limits<float>::max();

    for (GameEntity* entity : entities_)
    {
        Unit* unit = dynamic_cast<Unit*>(entity);
        if (!unit) continue;
        if (unit->ownerID != activePlayerIndex_ + 1) continue;

        glm::vec2 projected = worldToScreen(unit->position);
        float dist = glm::length(projected - screenPos);
        if (dist < bestDist && dist < 30.0f)
        {
            bestDist = dist;
            bestUnit = unit;
        }
    }

    if (bestUnit)
    {
        if (!additive)
            clearUnitSelection();
        if (std::find(selectedUnits_.begin(), selectedUnits_.end(), bestUnit) == selectedUnits_.end())
        {
            bestUnit->SetSelected(true);
            selectedUnits_.push_back(bestUnit);
        }
        selectedBuilding_ = nullptr;
        updateProductionPanel();
        return;
    }

    if (!additive)
    {
        clearUnitSelection();
        selectBuildingAtScreen(screenPos);
    }
}

void Scene::selectUnitsInRect(const glm::vec2& a, const glm::vec2& b, bool additive)
{
    glm::vec2 minPt(std::min(a.x, b.x), std::min(a.y, b.y));
    glm::vec2 maxPt(std::max(a.x, b.x), std::max(a.y, b.y));

    if (!additive)
        clearUnitSelection();

    for (GameEntity* entity : entities_)
    {
        Unit* unit = dynamic_cast<Unit*>(entity);
        if (!unit) continue;
        if (unit->ownerID != activePlayerIndex_ + 1) continue;

        glm::vec2 projected = worldToScreen(unit->position);
        if (projected.x >= minPt.x && projected.x <= maxPt.x &&
            projected.y >= minPt.y && projected.y <= maxPt.y)
        {
            if (std::find(selectedUnits_.begin(), selectedUnits_.end(), unit) == selectedUnits_.end())
            {
                unit->SetSelected(true);
                selectedUnits_.push_back(unit);
            }
        }
    }

    selectedBuilding_ = nullptr;
    updateProductionPanel();
    updateUnitInfoPanel();
}

int Scene::findEntityIndex(const GameEntity* entity) const
{
    if (!entity)
        return -1;
    for (size_t i = 0; i < entities_.size(); ++i)
    {
        if (entities_[i] == entity)
            return static_cast<int>(i);
    }
    return -1;
}

int Scene::allocateNetworkId()
{
    if (nextNetworkId_ < 1)
        nextNetworkId_ = 1;
    return nextNetworkId_++;
}

int Scene::registerEntity(GameEntity* entity, int requestedId)
{
    if (!entity)
        return -1;

    int id = requestedId > 0 ? requestedId : allocateNetworkId();
    if (id <= 0)
        return -1;

    networkEntities_.erase(id);
    entity->SetNetworkId(id);
    networkEntities_[id] = entity;
    return id;
}

void Scene::unregisterEntity(GameEntity* entity)
{
    if (!entity)
        return;
    const int id = entity->GetNetworkId();
    if (id > 0)
        networkEntities_.erase(id);
    entity->SetNetworkId(-1);
}

GameEntity* Scene::findEntityByNetworkId(int networkId) const
{
    auto it = networkEntities_.find(networkId);
    if (it == networkEntities_.end())
        return nullptr;
    return it->second;
}

void Scene::selectBuildingAtScreen(const glm::vec2& screenPos)
{
    Building* bestBuilding = nullptr;
    float bestDist = std::numeric_limits<float>::max();

    for (GameEntity* entity : entities_)
    {
        Building* building = dynamic_cast<Building*>(entity);
        if (!building) continue;
        if (building->ownerID != activePlayerIndex_ + 1) continue;

        glm::vec2 projected = worldToScreen(building->position);
        float dist = glm::length(projected - screenPos);
        if (dist < bestDist && dist < 45.0f)
        {
            bestDist = dist;
            bestBuilding = building;
        }
    }

    if (bestBuilding)
    {
        if (selectedBuilding_ == bestBuilding)
        {
            selectedBuilding_ = nullptr;
        }
        else
        {
            selectedBuilding_ = bestBuilding;
        }
    }
    else
    {
        selectedBuilding_ = nullptr;
    }

    updateProductionPanel();
}

void Scene::issueMoveCommand()
{
    if (selectedUnits_.empty() || !terrain || !camera)
        return;

    Ray ray = Raycaster::screenPointToRay(
        static_cast<float>(mouseX_),
        static_cast<float>(mouseY_),
        fbWidth,
        fbHeight,
        *camera
    );

    glm::vec3 hit;
    bool hitFound = Raycaster::raycastTerrain(ray, *terrain, hit);
    if (!hitFound)
    {
        glm::mat4 view = camera->GetViewMatrix();
        float aspect = fbHeight > 0 ? static_cast<float>(fbWidth) / static_cast<float>(fbHeight) : 1.0f;
        glm::mat4 projection = glm::perspective(glm::radians(camera->Zoom), aspect, 0.1f, 3000.0f);
        glm::vec3 fallback = GetMouseWorldPos(mouseX_, mouseY_, fbWidth, fbHeight, view, projection, 0.0f);
        fallback.y = Terrain::getHeight(fallback.x, fallback.z);
        if (std::isfinite(fallback.x) && std::isfinite(fallback.y) && std::isfinite(fallback.z))
        {
            hit = fallback;
            hitFound = true;
        }
    }
    if (!hitFound)
        return;

    if (handleResourceGather(hit))
        return;

    const size_t unitCount = selectedUnits_.size();
    if (unitCount == 0) return;

    int formationCols = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(unitCount))));
    int formationRows = static_cast<int>(std::ceil(unitCount / static_cast<float>(formationCols)));

    const float spacing = 4.0f;

    for (size_t i = 0; i < unitCount; ++i)
    {
        Unit* unit = selectedUnits_[i];
        if (!unit) continue;

        int row = static_cast<int>(i) / formationCols;
        int col = static_cast<int>(i) % formationCols;

        float offsetX = (col - formationCols / 2) * spacing;
        float offsetZ = (row - formationRows / 2) * spacing;

        glm::vec3 target = hit + glm::vec3(offsetX, 0.0f, offsetZ);
        target.y = Terrain::getHeight(target.x, target.z);
        glm::vec3 adjusted;
        if (findClosestLandPoint(target, adjusted))
        {
            if (unit->type == EntityType::Worker)
                clearGatherTasksFor(unit);
            commandUnitTo(unit, adjusted);
            unit->SetTaskState(Unit::TaskState::Moving);
            if (lanModeActive_ && networkSession_.IsConnected() && !suppressNetworkSend_)
            {
                int netId = unit->GetNetworkId();
                if (netId > 0)
                    sendMoveCommand(netId, unit->ownerID, adjusted);
            }
        }
    }
}

void Scene::deleteUnit(Unit* unit)
{
    if (!unit)
        return;

    unregisterEntity(unit);

    if (unitCameraActive_ && unitCameraTarget_ == unit)
    {
        unitCameraActive_ = false;
        unitCameraTarget_ = nullptr;
        if (camera)
            camera->SetPose(savedCameraPos_, savedCameraYaw_, savedCameraPitch_);
    }

    selectedUnits_.erase(
        std::remove(selectedUnits_.begin(), selectedUnits_.end(), unit),
        selectedUnits_.end());

    auto entityIt = std::find(entities_.begin(), entities_.end(), unit);
    if (entityIt != entities_.end())
        entities_.erase(entityIt);

    clearGatherTasksFor(unit);

    Resources* ownerRes = resourcesForOwner(unit->ownerID);
    if (ownerRes)
    {
        ownerRes->AddPopulation(-1);
        if (unit->type == EntityType::Worker)
            ownerRes->AddVillager(-1);
    }

    delete unit;
    unitInfoTarget_ = nullptr;
    refreshUnitListUI();
    updateResourceTexts();
    updateUnitInfoPanel();
}

Unit* Scene::spawnInitialVillager(TownCenter* tc, int forcedNetworkId)
{
    if (!tc)
        return nullptr;

    Resources* ownerRes = resourcesForOwner(tc->ownerID);
    if (!ownerRes || !ownerRes->HasPopulationRoom(1))
        return nullptr;

    glm::vec3 spawnPos = tc->position + glm::vec3(6.0f, 0.0f, 6.0f);
    return spawnUnitForOwner(EntityType::Worker, spawnPos, tc->ownerID, true, forcedNetworkId);
}

void Scene::spawnStartingTownCenters()
{
    if (startingBasesSpawned_)
        return;

    struct StartInfo
    {
        glm::vec3 pos;
        int owner;
    };

    const StartInfo starts[] = {
        { glm::vec3(-188.863f, 0.0f, -51.4825f), 1 },
        { glm::vec3( 181.932f, 0.0f, -81.6087f), 2 }
    };

    for (const StartInfo& info : starts)
    {
        Resources* res = resourcesForOwner(info.owner);
        if (!res)
            continue;

        glm::vec3 pos = info.pos;
        pos.y = Terrain::getHeight(pos.x, pos.z);

        const bool previous = suppressNetworkSend_;
        suppressNetworkSend_ = true;
        Building* tcBuilding = placeBuildingForOwner(BuildType::TownCenter, pos, info.owner, res, false);
        suppressNetworkSend_ = previous;
        if (tcBuilding)
        {
            if (TownCenter* tc = dynamic_cast<TownCenter*>(tcBuilding))
                spawnInitialVillager(tc);
        }
    }

    startingBasesSpawned_ = true;
    fogDirty_ = true;
}

bool Scene::findClosestLandPoint(const glm::vec3& desired, glm::vec3& out) const
{
    if (!isWaterArea(desired.x, desired.z))
    {
        out = desired;
        return true;
    }

    const float maxRadius = 60.0f;
    const float step = 3.0f;
    const int samples = 18;

    for (float radius = step; radius <= maxRadius; radius += step)
    {
        for (int i = 0; i < samples; ++i)
        {
            float angle = (glm::two_pi<float>() / samples) * i;
            glm::vec3 candidate = desired;
            candidate.x += std::cos(angle) * radius;
            candidate.z += std::sin(angle) * radius;
            candidate.y = Terrain::getHeight(candidate.x, candidate.z);
            if (!isWaterArea(candidate.x, candidate.z))
            {
                out = candidate;
                return true;
            }
        }
    }
    return false;
}

bool Scene::segmentCrossesWater(const glm::vec3& start, const glm::vec3& end) const
{
    const int samples = 64;
    for (int i = 1; i < samples; ++i)
    {
        float t = static_cast<float>(i) / static_cast<float>(samples);
        glm::vec3 point = glm::mix(start, end, t);
        if (isWaterArea(point.x, point.z))
            return true;
    }
    return false;
}

bool Scene::findNearestTree(const glm::vec3& point, float radius, size_t& outIndex, glm::vec3& outPos) const
{
    float bestDist = radius;
    bool found = false;
    for (size_t i = 0; i < treePositions_.size(); ++i)
    {
        float dist = glm::distance(glm::vec2(point.x, point.z),
                                   glm::vec2(treePositions_[i].x, treePositions_[i].z));
        if (dist < bestDist)
        {
            bestDist = dist;
            outIndex = i;
            outPos = treePositions_[i];
            found = true;
        }
    }
    return found;
}

bool Scene::findNearestRock(const glm::vec3& point, float radius, size_t& outIndex, glm::vec3& outPos) const
{
    float bestDist = radius;
    bool found = false;
    for (size_t i = 0; i < rockPositions_.size(); ++i)
    {
        float dist = glm::distance(glm::vec2(point.x, point.z),
                                   glm::vec2(rockPositions_[i].x, rockPositions_[i].z));
        if (dist < bestDist)
        {
            bestDist = dist;
            outIndex = i;
            outPos = rockPositions_[i];
            found = true;
        }
    }
    return found;
}

void Scene::removeTree(size_t index)
{
    if (index >= treeTransforms.size() || index >= treePositions_.size())
        return;

    clearGatherTasksFor(ResourceNodeType::Tree, index);

    size_t last = treeTransforms.size() - 1;
    if (index != last)
    {
        treeTransforms[index] = treeTransforms[last];
        treePositions_[index] = treePositions_[last];
        for (auto& task : gatherTasks_)
        {
            if (task.type == ResourceNodeType::Tree && task.resourceIndex == last)
                task.resourceIndex = index;
        }
    }

    treeTransforms.pop_back();
    treePositions_.pop_back();
}

void Scene::removeRock(size_t index)
{
    if (index >= rockTransforms.size() || index >= rockPositions_.size())
        return;

    clearGatherTasksFor(ResourceNodeType::Rock, index);

    size_t last = rockTransforms.size() - 1;
    if (index != last)
    {
        rockTransforms[index] = rockTransforms[last];
        rockPositions_[index] = rockPositions_[last];
        for (auto& task : gatherTasks_)
        {
            if (task.type == ResourceNodeType::Rock && task.resourceIndex == last)
                task.resourceIndex = index;
        }
    }

    rockTransforms.pop_back();
    rockPositions_.pop_back();
}

bool Scene::handleResourceGather(const glm::vec3& point)
{
    Unit* worker = nullptr;
    for (Unit* unit : selectedUnits_)
    {
        if (unit && unit->type == EntityType::Worker)
        {
            worker = unit;
            break;
        }
    }
    if (!worker)
        return false;

    size_t resourceIndex = 0;
    glm::vec3 resourcePos(0.0f);
    const float gatherRadius = 12.0f;

    if (findNearestTree(point, gatherRadius, resourceIndex, resourcePos))
    {
        clearGatherTasksFor(worker);
        GatherTask newTask;
        newTask.worker = worker;
        newTask.type = ResourceNodeType::Tree;
        newTask.resourceIndex = resourceIndex;
        gatherTasks_.push_back(newTask);
        float groundY = Terrain::getHeight(resourcePos.x, resourcePos.z);
        glm::vec3 dest(resourcePos.x, groundY, resourcePos.z);
        commandUnitTo(worker, dest);
        return true;
    }

    if (findNearestRock(point, gatherRadius, resourceIndex, resourcePos))
    {
        clearGatherTasksFor(worker);
        GatherTask newTask;
        newTask.worker = worker;
        newTask.type = ResourceNodeType::Rock;
        newTask.resourceIndex = resourceIndex;
        gatherTasks_.push_back(newTask);
        float groundY = Terrain::getHeight(resourcePos.x, resourcePos.z);
        glm::vec3 dest(resourcePos.x, groundY, resourcePos.z);
        commandUnitTo(worker, dest);
        return true;
    }

    return false;
}

void Scene::clearGatherTasksFor(Unit* worker)
{
    if (!worker || gatherTasks_.empty())
        return;

    gatherTasks_.erase(
        std::remove_if(
            gatherTasks_.begin(),
            gatherTasks_.end(),
            [worker](const GatherTask& task)
            {
                return task.worker == worker;
            }),
        gatherTasks_.end());

    if (worker->GetTaskState() == Unit::TaskState::Gathering)
        worker->SetTaskState(Unit::TaskState::Idle);
    worker->ClearActionAnimation();
}

void Scene::clearGatherTasksFor(ResourceNodeType type, size_t resourceIndex)
{
    if (gatherTasks_.empty())
        return;

    gatherTasks_.erase(
        std::remove_if(
            gatherTasks_.begin(),
            gatherTasks_.end(),
            [type, resourceIndex](const GatherTask& task)
            {
                if (task.type == type && task.resourceIndex == resourceIndex)
                {
                    if (task.worker && task.worker->GetTaskState() == Unit::TaskState::Gathering)
                        task.worker->SetTaskState(Unit::TaskState::Idle);
                    if (task.worker)
                        task.worker->ClearActionAnimation();
                    return true;
                }
                return false;
            }),
        gatherTasks_.end());
}

void Scene::updateGatherTasks(float dt)
{
    size_t i = 0;
    while (i < gatherTasks_.size())
    {
        GatherTask& task = gatherTasks_[i];
        bool removeTask = false;

        if (!task.worker || task.worker->type != EntityType::Worker)
        {
            removeTask = true;
        }
        else
        {
            const std::vector<glm::vec3>& positions =
                (task.type == ResourceNodeType::Tree) ? treePositions_ : rockPositions_;

            if (task.resourceIndex >= positions.size())
            {
                removeTask = true;
            }
            else
            {
                glm::vec3 resPos = positions[task.resourceIndex];
                glm::vec2 workerXZ(task.worker->position.x, task.worker->position.z);
                glm::vec2 resXZ(resPos.x, resPos.z);
                float dist = glm::distance(workerXZ, resXZ);

                if (dist < 3.0f)
                {
                    if (task.worker && task.worker->GetTaskState() != Unit::TaskState::Gathering)
                        task.worker->SetTaskState(Unit::TaskState::Gathering);
                    if (!task.animationActive && task.worker)
                    {
                        if (task.type == ResourceNodeType::Tree)
                            task.worker->SetActionAnimation("CharacterArmature|Punch_Right");
                        else
                            task.worker->SetActionAnimation("CharacterArmature|Punch_Left");
                        task.animationActive = true;
                    }
                    task.progress += dt;
                    if (!task.soundActive)
                    {
                        if (task.type == ResourceNodeType::Tree)
                            soundManager_.PlayWoodChop();
                        else
                            soundManager_.PlayStoneMine();
                        task.soundActive = true;
                    }

                    if (task.progress >= 2.0f)
                    {
                        task.soundActive = false;
                        ResourceNodeType type = task.type;
                        size_t resourceIdx = task.resourceIndex;
                        Unit* workerPtr = task.worker;

                        if (workerPtr && workerPtr->GetTaskState() == Unit::TaskState::Gathering)
                            workerPtr->SetTaskState(Unit::TaskState::Idle);
                        if (workerPtr)
                            workerPtr->ClearActionAnimation();

                        gatherTasks_[i] = gatherTasks_.back();
                        gatherTasks_.pop_back();

        Resources* awardRes = workerPtr ? resourcesForOwner(workerPtr->ownerID) : nullptr;
        if (type == ResourceNodeType::Tree)
        {
            if (awardRes)
                awardRes->AddWood(50);
            removeTree(resourceIdx);
        }
        else
        {
            if (awardRes)
                awardRes->AddOre(30);
            removeRock(resourceIdx);
        }
                        updateResourceTexts();
                        continue;
                    }
                }
                else
                {
                    task.progress = std::max(0.0f, task.progress - dt);
                    task.soundActive = false;
                    if (task.animationActive && task.worker)
                    {
                        task.worker->ClearActionAnimation();
                        task.animationActive = false;
                    }
                }
            }
        }

        if (removeTask)
        {
            Unit* workerPtr = task.worker;
            gatherTasks_[i] = gatherTasks_.back();
            gatherTasks_.pop_back();
            if (workerPtr && workerPtr->GetTaskState() == Unit::TaskState::Gathering)
                workerPtr->SetTaskState(Unit::TaskState::Idle);
            task.soundActive = false;
            if (task.animationActive && workerPtr)
                workerPtr->ClearActionAnimation();
            if (workerPtr)
                workerPtr->ClearActionAnimation();
        }
        else
        {
            ++i;
        }
    }
}

void Scene::updateCombat(float dt)
{
    if (victoryShown_)
        return;

    std::vector<Knight*> knights;
    std::vector<Unit*> allUnits;
    std::vector<Building*> allBuildings;
    knights.reserve(16);
    allUnits.reserve(32);
    allBuildings.reserve(16);

    for (GameEntity* entity : entities_)
    {
        if (Knight* knight = dynamic_cast<Knight*>(entity))
            knights.push_back(knight);
        if (Unit* unit = dynamic_cast<Unit*>(entity))
            allUnits.push_back(unit);
        if (Building* building = dynamic_cast<Building*>(entity))
            allBuildings.push_back(building);
    }

    auto entityExists = [&](GameEntity* ptr) -> bool
    {
        return std::find(entities_.begin(), entities_.end(), ptr) != entities_.end();
    };

    for (Knight* knight : knights)
    {
        if (!knight || !entityExists(knight))
            continue;

        Unit* unitTarget = nullptr;
        float bestRange = knight->AttackRange();
        for (Unit* candidate : allUnits)
        {
            if (!candidate || candidate == knight)
                continue;
            if (!entityExists(candidate))
                continue;
            if (candidate->ownerID == knight->ownerID)
                continue;
            float dist = glm::distance(knight->position, candidate->position);
            if (dist < bestRange)
            {
                bestRange = dist;
                unitTarget = candidate;
            }
        }

        Building* buildingTarget = nullptr;
        if (!unitTarget)
        {
            for (Building* candidate : allBuildings)
            {
                if (!candidate)
                    continue;
                if (!entityExists(candidate))
                    continue;
                if (candidate->ownerID == knight->ownerID)
                    continue;
                float dist = glm::distance(knight->position, candidate->position);
                if (dist <= knight->AttackRange())
                {
                    buildingTarget = candidate;
                    break;
                }
            }
        }

        if (unitTarget)
        {
            knight->SetTaskState(Unit::TaskState::Combat);
            knight->SetActionAnimation("Attack");
            if (knight->ReadyToStrike())
            {
                unitTarget->SetHealth(unitTarget->GetHealth() - knight->AttackDamage());
                knight->ResetAttackTimer();
                if (unitTarget->GetHealth() <= 0.0f)
                    deleteUnit(unitTarget);
            }
        }
        else if (buildingTarget)
        {
            knight->SetTaskState(Unit::TaskState::Combat);
            knight->SetActionAnimation("Attack");
            if (knight->ReadyToStrike())
            {
                buildingTarget->ApplyDamage(knight->AttackDamage());
                knight->ResetAttackTimer();
                if (buildingTarget->IsDestroyed())
                {
                    destroyBuilding(buildingTarget);
                }
            }
        }
        else
        {
            if (knight->GetTaskState() == Unit::TaskState::Combat)
                knight->SetTaskState(Unit::TaskState::Idle);
            knight->ClearActionAnimation();
        }
    }
}

Resources& Scene::activePlayer()
{
    if (!activeResources_)
        activeResources_ = &player1;
    return *activeResources_;
}

const Resources& Scene::activePlayer() const
{
    return (activePlayerIndex_ == 0) ? player1 : player2;
}

Resources* Scene::activePlayerPtr()
{
    if (!activeResources_)
        activeResources_ = resourcesForOwner(activePlayerIndex_ + 1);
    return activeResources_;
}

Resources* Scene::resourcesForOwner(int ownerId)
{
    if (ownerId == 2)
        return &player2;
    return &player1;
}

const Resources* Scene::resourcesForOwner(int ownerId) const
{
    return const_cast<Scene*>(this)->resourcesForOwner(ownerId);
}

void Scene::switchActivePlayer()
{
    activePlayerIndex_ = 1 - activePlayerIndex_;
    activeResources_ = resourcesForOwner(activePlayerIndex_ + 1);
    unitManager_.setActiveResources(activeResources_);
    clearUnitSelection();
    selectedBuilding_ = nullptr;
    configureBuildingPreviewsForOwner(activePlayerIndex_ + 1);
    updateBuildingBarLabels();
    updateBuildingButtonTexturesForOwner(activePlayerIndex_ + 1);
    updateBuildingInfoPanel(BuildType::None);
    if (unitCameraActive_)
    {
        unitCameraActive_ = false;
        unitCameraTarget_ = nullptr;
        if (camera)
            camera->SetPose(savedCameraPos_, savedCameraYaw_, savedCameraPitch_);
    }
    fogDirty_ = true;
    updateResourceTexts();
    refreshUnitListUI();
    updateProductionPanel();
    updateUnitInfoPanel();
}

void Scene::destroyBuilding(Building* building)
{
    if (!building)
        return;

    unregisterEntity(building);

    bool wasTownCenter = (building->type == EntityType::TownCenter);

    if (selectedBuilding_ == building)
    {
        selectedBuilding_ = nullptr;
        updateProductionPanel();
    }

    if (building->type == EntityType::TownCenter)
    {
        TownCenter* tcPtr = dynamic_cast<TownCenter*>(building);
        townCenters_.erase(
            std::remove(townCenters_.begin(), townCenters_.end(), tcPtr),
            townCenters_.end());
    }
    else if (building->type == EntityType::Barracks)
    {
        Barracks* barracksPtr = dynamic_cast<Barracks*>(building);
        barracks_.erase(
            std::remove(barracks_.begin(), barracks_.end(), barracksPtr),
            barracks_.end());
    }

    auto it = std::find(entities_.begin(), entities_.end(), building);
    if (it != entities_.end())
        entities_.erase(it);

    delete building;
    refreshNavObstacles();
    updateResourceTexts();
    refreshUnitListUI();
    updateUnitInfoPanel();

    if (wasTownCenter)
        checkVictoryState();
}

void Scene::showVictoryMessage(int winningPlayer)
{
    if (victoryShown_)
        return;

    victoryShown_ = true;
    if (victoryLabelIndex_ != SIZE_MAX)
    {
        std::string text = "Player " + std::to_string(winningPlayer) + " Wins!";
        uiManager_.setLabelText(victoryLabelIndex_, text);
        uiManager_.setLabelVisibility(victoryLabelIndex_, true);
    }
}

void Scene::checkVictoryState()
{
    if (victoryShown_)
        return;

    bool player1Alive = false;
    bool player2Alive = false;
    for (TownCenter* tc : townCenters_)
    {
        if (!tc)
            continue;
        if (tc->ownerID == 1)
            player1Alive = true;
        else if (tc->ownerID == 2)
            player2Alive = true;
    }

    if (!player1Alive && player2Alive)
    {
        showVictoryMessage(2);
    }
    else if (!player2Alive && player1Alive)
    {
        showVictoryMessage(1);
    }
}

void Scene::processNetworkMessages()
{
    if (!lanModeActive_ || !networkSession_.IsConnected())
        return;

    std::string message;
    while (networkSession_.PollMessage(message))
    {
        handleNetworkMessage(message);
    }
}

void Scene::handleNetworkMessage(const std::string& message)
{
    std::istringstream iss(message);
    std::string cmd;
    iss >> cmd;
    if (cmd == "BUILD")
    {
        int ownerId = 0;
        int typeInt = 0;
        float x = 0.0f, y = 0.0f, z = 0.0f;
        int buildingId = -1;
        int workerId = -1;
        float rotX = 0.0f, rotY = 0.0f, rotZ = 0.0f;
        if (iss >> ownerId >> typeInt >> x >> y >> z >> buildingId >> workerId >> rotX >> rotY >> rotZ)
        {
            applyBuildCommand(ownerId,
                              static_cast<BuildType>(typeInt),
                              glm::vec3(x, y, z),
                              buildingId,
                              workerId,
                              glm::vec3(rotX, rotY, rotZ));
        }
    }
    else if (cmd == "TRAIN")
    {
        int ownerId = 0;
        int typeInt = 0;
        float x = 0.0f, y = 0.0f, z = 0.0f;
        int unitId = -1;
        if (iss >> ownerId >> typeInt >> x >> y >> z >> unitId)
            applyTrainCommand(ownerId,
                              static_cast<EntityType>(typeInt),
                              glm::vec3(x, y, z),
                              unitId);
    }
    else if (cmd == "MOVE")
    {
        int ownerId = 0;
        int networkId = -1;
        float x = 0.0f, y = 0.0f, z = 0.0f;
        if (iss >> ownerId >> networkId >> x >> y >> z)
            applyMoveCommand(ownerId, networkId, glm::vec3(x, y, z));
    }
}

void Scene::sendBuildCommand(BuildType type,
                             int ownerId,
                             const glm::vec3& pos,
                             int buildingNetId,
                             int initialWorkerNetId,
                             const glm::vec3& rotation)
{
    if (!lanModeActive_ || !networkSession_.IsConnected())
        return;

    std::ostringstream oss;
    oss << "BUILD " << ownerId << " " << static_cast<int>(type) << " "
        << pos.x << " " << pos.y << " " << pos.z << " "
        << buildingNetId << " " << initialWorkerNetId << " "
        << rotation.x << " " << rotation.y << " " << rotation.z;
    networkSession_.SendMessage(oss.str());
}

bool Scene::applyBuildCommand(int ownerId,
                              BuildType type,
                              const glm::vec3& pos,
                              int buildingNetId,
                              int initialWorkerNetId,
                              const glm::vec3& rotation)
{
    Resources* ownerRes = resourcesForOwner(ownerId);
    if (!ownerRes)
        return false;

    const bool previous = suppressNetworkSend_;
    suppressNetworkSend_ = true;
    Building* building = placeBuildingForOwner(type, pos, ownerId, ownerRes, true, buildingNetId, &rotation);
    bool result = (building != nullptr);
    if (result && type == BuildType::TownCenter && building && initialWorkerNetId > 0)
    {
        if (TownCenter* tc = dynamic_cast<TownCenter*>(building))
            spawnInitialVillager(tc, initialWorkerNetId);
    }
    suppressNetworkSend_ = previous;
    return result;
}

void Scene::sendTrainCommand(EntityType type,
                             int ownerId,
                             const glm::vec3& pos,
                             int unitNetId)
{
    if (!lanModeActive_ || !networkSession_.IsConnected())
        return;

    std::ostringstream oss;
    oss << "TRAIN " << ownerId << " " << static_cast<int>(type) << " "
        << pos.x << " " << pos.y << " " << pos.z << " "
        << unitNetId;
    networkSession_.SendMessage(oss.str());
}

bool Scene::applyTrainCommand(int ownerId,
                              EntityType type,
                              const glm::vec3& pos,
                              int unitNetId)
{
    Resources* res = resourcesForOwner(ownerId);
    if (!res)
        return false;

    UnitCost cost = getUnitCost(type);
    res->food = std::max(0, res->food - cost.food);
    res->wood = std::max(0, res->wood - cost.wood);
    res->ore  = std::max(0, res->ore  - cost.ore);
    res->gold = std::max(0, res->gold - cost.gold);

    const bool previous = suppressNetworkSend_;
    suppressNetworkSend_ = true;
    Unit* unit = spawnUnitForOwner(type, pos, ownerId, true, unitNetId);
    bool result = (unit != nullptr);
    suppressNetworkSend_ = previous;
    return result;
}

void Scene::sendMoveCommand(int networkId, int ownerId, const glm::vec3& pos)
{
    if (!lanModeActive_ || !networkSession_.IsConnected())
        return;

    std::ostringstream oss;
    oss << "MOVE " << ownerId << " " << networkId << " "
        << pos.x << " " << pos.y << " " << pos.z;
    networkSession_.SendMessage(oss.str());
}

bool Scene::applyMoveCommand(int ownerId, int networkId, const glm::vec3& pos)
{
    if (networkId <= 0)
        return false;

    Unit* unit = dynamic_cast<Unit*>(findEntityByNetworkId(networkId));
    if (!unit || unit->ownerID != ownerId)
        return false;

    glm::vec3 adjusted = pos;
    adjusted.y = Terrain::getHeight(adjusted.x, adjusted.z);
    glm::vec3 finalPos;
    if (!findClosestLandPoint(adjusted, finalPos))
        finalPos = adjusted;

    const bool previous = suppressNetworkSend_;
    suppressNetworkSend_ = true;
    if (unit->type == EntityType::Worker)
        clearGatherTasksFor(unit);
    commandUnitTo(unit, finalPos);
    unit->SetTaskState(Unit::TaskState::Moving);
    suppressNetworkSend_ = previous;
    return true;
}

void Scene::initFogOfWar()
{
    if (navGridCols_ <= 0 || navGridRows_ <= 0)
        return;

    const size_t cellCount = static_cast<size_t>(navGridCols_) * static_cast<size_t>(navGridRows_);
    for (auto& fog : fogStates_)
        fog.assign(cellCount, 0);
    for (auto& vis : fogVisibility_)
        vis.assign(cellCount, 0);
    fogDirty_ = true;

    if (fogVAO_ == 0)
    {
        glGenVertexArrays(1, &fogVAO_);
        glGenBuffers(1, &fogVBO_);
        glBindVertexArray(fogVAO_);
        glBindBuffer(GL_ARRAY_BUFFER, fogVBO_);
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
        glBindVertexArray(0);
    }
}

void Scene::resetFogOfWar()
{
    for (size_t i = 0; i < 2; ++i)
    {
        std::fill(fogStates_[i].begin(), fogStates_[i].end(), 0);
        fogVisibility_[i].assign(fogStates_[i].size(), 0);
    }
    fogDirty_ = true;
}

void Scene::updateFogOfWar()
{
    if (fogRevealOverride_)
        return;
    if (navGridCols_ <= 0 || navGridRows_ <= 0)
        return;
    if (fogStates_[0].empty())
        initFogOfWar();
    if (fogStates_[0].empty())
        return;

    bool changed = false;
    changed |= updatePlayerFog(1);
    changed |= updatePlayerFog(2);
    if (changed)
        fogDirty_ = true;
}

bool Scene::updatePlayerFog(int playerId)
{
    if (playerId < 1 || playerId > 2)
        return false;
    auto& fog = fogStates_[playerId - 1];
    if (fog.empty())
        return false;

    auto& visibility = fogVisibility_[playerId - 1];
    if (visibility.size() != fog.size())
        visibility.assign(fog.size(), 0);
    std::fill(visibility.begin(), visibility.end(), 0);

    bool changed = false;

    auto revealAround = [&](const glm::vec3& center, float radius)
    {
        if (radius <= 0.0f)
            return;
        int minCol = static_cast<int>((center.x - radius - navOrigin_.x) / navCellSize_);
        int maxCol = static_cast<int>((center.x + radius - navOrigin_.x) / navCellSize_);
        int minRow = static_cast<int>((center.z - radius - navOrigin_.y) / navCellSize_);
        int maxRow = static_cast<int>((center.z + radius - navOrigin_.y) / navCellSize_);
        minCol = std::max(0, minCol);
        minRow = std::max(0, minRow);
        maxCol = std::min(navGridCols_ - 1, maxCol);
        maxRow = std::min(navGridRows_ - 1, maxRow);

        const float radiusSq = radius * radius;
        for (int row = minRow; row <= maxRow; ++row)
        {
            for (int col = minCol; col <= maxCol; ++col)
            {
                glm::vec3 world = navToWorld(col, row);
                glm::vec2 delta(world.x - center.x, world.z - center.z);
                if (glm::dot(delta, delta) > radiusSq)
                    continue;
                size_t idx = static_cast<size_t>(row) * static_cast<size_t>(navGridCols_) + static_cast<size_t>(col);
                if (idx < fog.size())
                {
                    visibility[idx] = 1;
                }
            }
        }
    };

    for (GameEntity* entity : entities_)
    {
        if (!entity || entity->ownerID != playerId)
            continue;
        float radius = visibilityRadiusForEntity(entity);
        revealAround(entity->position, radius);
    }

    for (size_t i = 0; i < fog.size(); ++i)
    {
        uint8_t prev = fog[i];
        uint8_t desired = prev;
        if (visibility[i])
            desired = 2;
        else if (prev > 0)
            desired = 1;
        else
            desired = 0;

        if (desired != prev)
        {
            fog[i] = desired;
            changed = true;
        }
    }

    return changed;
}

float Scene::visibilityRadiusForEntity(const GameEntity* entity) const
{
    if (!entity)
        return 0.0f;
    if (dynamic_cast<const Building*>(entity))
        return buildingNavRadius(entity->type) + 20.0f;
    if (dynamic_cast<const Unit*>(entity))
        return 32.0f;
    return 24.0f;
}

bool Scene::isPositionVisibleToPlayer(const glm::vec3& pos, int playerId) const
{
    if (fogRevealOverride_)
        return true;
    if (playerId < 1 || playerId > 2)
        return true;
    if (navGridCols_ <= 0 || navGridRows_ <= 0)
        return true;
    const auto& fog = fogStates_[playerId - 1];
    if (fog.empty())
        return true;
    int col = 0, row = 0;
    if (!worldToNav(pos, col, row))
        return true;
    size_t idx = static_cast<size_t>(row) * static_cast<size_t>(navGridCols_) + static_cast<size_t>(col);
    if (idx >= fog.size())
        return true;
    return fog[idx] == 2;
}

bool Scene::isPositionExploredByPlayer(const glm::vec3& pos, int playerId) const
{
    if (fogRevealOverride_)
        return true;
    if (playerId < 1 || playerId > 2)
        return true;
    if (navGridCols_ <= 0 || navGridRows_ <= 0)
        return true;
    const auto& fog = fogStates_[playerId - 1];
    if (fog.empty())
        return true;
    int col = 0, row = 0;
    if (!worldToNav(pos, col, row))
        return true;
    size_t idx = static_cast<size_t>(row) * static_cast<size_t>(navGridCols_) + static_cast<size_t>(col);
    if (idx >= fog.size())
        return true;
    return fog[idx] >= 1;
}

void Scene::toggleFogReveal()
{
    fogRevealOverride_ = !fogRevealOverride_;
    fogDirty_ = true;
}
