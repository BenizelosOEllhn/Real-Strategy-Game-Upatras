#include "TownCenter.h"

void TownCenter::SpawnUnit(std::vector<GameEntity*>& list)
{
    glm::vec3 spawnPos = position + glm::vec3(5, 0, 5);
    list.push_back(new Worker(spawnPos, model, ownerID));
}
