#include "TownCenter.h"

void TownCenter::SpawnUnit(std::vector<GameEntity*>& list)
{
    if (isUnderConstruction) return;

    glm::vec3 spawnPos = position + glm::vec3(5.0f, 0.0f, 5.0f);
    list.push_back(new Worker(spawnPos, model, ownerID));
}
