#pragma once
#include <algorithm>

struct UnitCost
{
    int food = 0;
    int wood = 0;
    int ore  = 0;
    int gold = 0;
};

struct Resources {
    int food  = 500;
    int wood  = 500;
    int ore   = 250;
    int gold  = 300;

    int foodCapacity = 800;
    int woodCapacity = 800;
    int oreCapacity  = 500;
    int goldCapacity = 600;

    int population     = 0;
    int populationCap  = 4;
    int villagers      = 0;

    bool Spend(const UnitCost& cost)
    {
        if (food < cost.food || wood < cost.wood || ore < cost.ore || gold < cost.gold)
            return false;

        food -= cost.food;
        wood -= cost.wood;
        ore  -= cost.ore;
        gold -= cost.gold;
        return true;
    }
    void AddFood(int amount)
    {
        food = std::clamp(food + amount, 0, foodCapacity);
    }

    void AddWood(int amount)
    {
        wood = std::clamp(wood + amount, 0, woodCapacity);
    }

    void AddOre(int amount)
    {
        ore = std::clamp(ore + amount, 0, oreCapacity);
    }

    void AddGold(int amount)
    {
        gold = std::clamp(gold + amount, 0, goldCapacity);
    }

    void IncreaseStorageCapacity(int foodDelta, int woodDelta, int oreDelta, int goldDelta)
    {
        foodCapacity = std::max(0, foodCapacity + foodDelta);
        woodCapacity = std::max(0, woodCapacity + woodDelta);
        oreCapacity  = std::max(0, oreCapacity + oreDelta);
        goldCapacity = std::max(0, goldCapacity + goldDelta);

        food = std::min(food, foodCapacity);
        wood = std::min(wood, woodCapacity);
        ore  = std::min(ore,  oreCapacity);
        gold = std::min(gold, goldCapacity);
    }

    bool HasPopulationRoom(int required) const
    {
        return (population + required) <= populationCap;
    }

    void AddPopulation(int amount)
    {
        population += amount;
        population = std::max(population, 0);
    }

    void AddVillager(int amount)
    {
        villagers += amount;
        villagers = std::max(villagers, 0);
    }

    void AddPopulationCap(int amount)
    {
        populationCap += amount;
        populationCap = std::max(populationCap, 0);
    }
};
