#include "Barracks.h"

void Barracks::SpawnUnit(std::vector<GameEntity*>& list)
{
    if (isUnderConstruction) return;

    glm::vec3 spawnPos = position + glm::vec3(-5.0f, 0.0f, -5.0f);

    if (rand() % 2 == 0)
        list.push_back(new Knight(spawnPos, model, ownerID));
    else
        list.push_back(new Archer(spawnPos, model, ownerID));
}
