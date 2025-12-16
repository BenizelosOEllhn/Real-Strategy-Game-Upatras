#include "Farm.h"

Farm::Farm(glm::vec3 pos, Model* foundation, Model* finalModel, int ownerID, Resources* resources)
    : Building(pos, EntityType::Farm, foundation, finalModel, ownerID),
      ownerResources_(resources)
{
    SetMaxHealth(400.0f);
}

void Farm::Update(float dt) {
    Building::Update(dt);
    if (isUnderConstruction || !ownerResources_) return;

    productionTimer_ += dt;
    if (productionTimer_ >= productionInterval_) {
        productionTimer_ = 0.0f;
        ownerResources_->AddFood(productionAmount_);
    }
}

void Farm::SpawnUnit(std::vector<GameEntity*>& /*entities*/)
{
    // Farms don't train units.
}
