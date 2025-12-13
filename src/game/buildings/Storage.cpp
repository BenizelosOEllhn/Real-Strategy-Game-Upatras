#include "Storage.h"

Storage::Storage(glm::vec3 pos, Model* model, int ownerID, Resources* resources)
    : GameEntity(pos, EntityType::Storage, model, ownerID),
      ownerResources_(resources)
{
}
