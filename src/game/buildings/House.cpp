#include "House.h"

House::House(glm::vec3 pos, Model* foundation, Model* finalModel, int ownerID, Resources* resources)
    : Building(pos, EntityType::House, foundation, finalModel, ownerID),
      ownerResources_(resources)
{
    SetMaxHealth(500.0f);
}

void House::Update(float dt)
{
    Building::Update(dt);
    if (isUnderConstruction || !ownerResources_ || grantedPopulation_)
        return;

    ownerResources_->AddPopulationCap(5);
    grantedPopulation_ = true;
}

void House::SpawnUnit(std::vector<GameEntity*>& /*entities*/)
{
    // Houses don't spawn units.
}
