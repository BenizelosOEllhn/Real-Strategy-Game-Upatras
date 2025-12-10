#include "Barracks.h"

void Barracks::SpawnUnit(std::vector<GameEntity*>& list)
{
    glm::vec3 spawnPos = position + glm::vec3(-5, 0, -5);

    // Example: alternate spawn types
    if (rand() % 2 == 0)
        list.push_back(new Knight(spawnPos, model, ownerID));
    else
        list.push_back(new Archer(spawnPos, model, ownerID));
}
