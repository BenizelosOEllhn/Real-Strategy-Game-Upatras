#include "UnitManager.h"

#include "../data/Resources.h"
#include "../entities/GameEntity.h"
#include "../entities/Building.h"
#include "../buildings/TownCenter.h"
#include "../buildings/Barracks.h"
#include "../units/Worker.h"
#include "../units/Archer.h"
#include "../units/Knight.h"

namespace
{
    UnitCost makeCost(int food, int wood, int ore, int gold)
    {
        UnitCost cost;
        cost.food = food;
        cost.wood = wood;
        cost.ore  = ore;
        cost.gold = gold;
        return cost;
    }
}

void UnitManager::init(Resources* resources,
                       std::vector<GameEntity*>* entities,
                       const UnitAssets& assets)
{
    resources_ = resources;
    entities_  = entities;
    assets_    = assets;
    townCenters_.clear();
    barracks_.clear();
}

void UnitManager::setActiveResources(Resources* resources)
{
    resources_ = resources;
}

void UnitManager::registerTownCenter(TownCenter* tc)
{
    if (!tc) return;
    townCenters_.push_back(tc);
}

void UnitManager::registerBarracks(Barracks* barracks)
{
    if (!barracks) return;
    barracks_.push_back(barracks);
}

bool UnitManager::TrainUnit(EntityType type, Building* building)
{
    if (!resources_ || !entities_ || !building)
        return false;

    switch (type)
    {
    case EntityType::Worker:
        return trainVillager(dynamic_cast<TownCenter*>(building));
    case EntityType::Archer:
        return trainRanger(dynamic_cast<Barracks*>(building));
    case EntityType::Knight:
        return trainKnight(dynamic_cast<Barracks*>(building));
    default:
        break;
    }
    return false;
}

bool UnitManager::trainVillager(TownCenter* tc)
{
    if (!tc || !assets_.farmer) return false;
    if (!resources_->HasPopulationRoom(1))
        return false;

    UnitCost cost = makeCost(50, 0, 0, 0);
    if (!resources_->Spend(cost))
        return false;

    resources_->AddPopulation(1);
    resources_->AddVillager(1);

    glm::vec3 spawnPos = computeSpawnPosition(tc, glm::vec3(5.0f, 0.0f, 5.0f));
    entities_->push_back(new Worker(spawnPos, assets_.farmer, tc->ownerID));
    return true;
}

bool UnitManager::trainRanger(Barracks* barracks)
{
    if (!barracks || !assets_.archer)
        return false;
    if (!resources_->HasPopulationRoom(1))
        return false;

    UnitCost cost = makeCost(40, 0, 20, 45);
    if (!resources_->Spend(cost))
        return false;

    resources_->AddPopulation(1);
    glm::vec3 spawnPos = computeSpawnPosition(barracks, glm::vec3(-5.0f, 0.0f, -5.0f));
    entities_->push_back(new Archer(spawnPos, assets_.archer, barracks->ownerID));
    return true;
}

bool UnitManager::trainKnight(Barracks* barracks)
{
    if (!barracks || !assets_.knight)
        return false;
    if (!resources_->HasPopulationRoom(1))
        return false;

    UnitCost cost = makeCost(60, 0, 35, 60);
    if (!resources_->Spend(cost))
        return false;

    resources_->AddPopulation(1);
    glm::vec3 spawnPos = computeSpawnPosition(barracks, glm::vec3(-5.0f, 0.0f, 5.0f));
    entities_->push_back(new Knight(spawnPos, assets_.knight, barracks->ownerID));
    return true;
}

glm::vec3 UnitManager::computeSpawnPosition(const Building* building, const glm::vec3& offset) const
{
    if (!building) return offset;
    return building->position + offset;
}
