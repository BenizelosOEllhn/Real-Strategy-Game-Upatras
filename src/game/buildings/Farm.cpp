#include "Farm.h"

Farm::Farm(glm::vec3 pos, Model* model, int ownerID, Resources* resources)
    : GameEntity(pos, EntityType::Farm, model, ownerID),
      ownerResources_(resources)
{
}

void Farm::Update(float dt) {
    if (!ownerResources_) return;

    productionTimer_ += dt;
    if (productionTimer_ >= productionInterval_) {
        productionTimer_ = 0.0f;
        ownerResources_->food += productionAmount_;
    }
}
