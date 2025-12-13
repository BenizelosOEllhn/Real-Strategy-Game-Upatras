#include "House.h"

House::House(glm::vec3 pos, Model* model, int ownerID, Resources* resources)
    : GameEntity(pos, EntityType::House, model, ownerID),
      ownerResources_(resources)
{
}
