#include "Market.h"

Market::Market(glm::vec3 pos, Model* model, int ownerID, Resources* resources)
    : GameEntity(pos, EntityType::Market, model, ownerID),
      ownerResources_(resources)
{
}
