#include "Storage.h"

Storage::Storage(glm::vec3 pos, Model* foundation, Model* finalModel, int ownerID, Resources* resources)
    : Building(pos, EntityType::Storage, foundation, finalModel, ownerID),
      ownerResources_(resources)
{
    SetMaxHealth(800.0f);
}

void Storage::Update(float dt)
{
    Building::Update(dt);
    if (isUnderConstruction || !ownerResources_ || capacityGranted_)
        return;

    ownerResources_->IncreaseStorageCapacity(150, 150, 80, 120);
    capacityGranted_ = true;
}

void Storage::SpawnUnit(std::vector<GameEntity*>& /*entities*/)
{
    // Storage does not spawn units.
}
