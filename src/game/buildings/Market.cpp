#include "Market.h"

Market::Market(glm::vec3 pos, Model* foundation, Model* finalModel, int ownerID, Resources* resources)
    : Building(pos, EntityType::Market, foundation, finalModel, ownerID),
      ownerResources_(resources)
{
    SetMaxHealth(700.0f);
}

void Market::Update(float dt)
{
    Building::Update(dt);
    if (isUnderConstruction || !ownerResources_)
        return;

    goldTimer_ += dt;
    if (goldTimer_ >= goldInterval_)
    {
        goldTimer_ = 0.0f;
        ownerResources_->AddGold(goldTickAmount_);
    }
}

void Market::SpawnUnit(std::vector<GameEntity*>& /*entities*/)
{
    // Market does not create units.
}
