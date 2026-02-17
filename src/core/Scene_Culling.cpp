#include "Scene.h"

float Scene::getEntityCullRadius(const GameEntity* entity) const
{
    if (!entity)
        return 0.0f;
    
    // Use entity type to determine culling radius
    switch (entity->type)
    {
    case EntityType::TownCenter:
        return 40.0f;
    case EntityType::Barracks:
        return 35.0f;
    case EntityType::Storage:
    case EntityType::Market:
        return 30.0f;
    case EntityType::Farm:
    case EntityType::House:
        return 25.0f;
    case EntityType::Bridge:
        return 80.0f; // Bridges are long
    case EntityType::Worker:
    case EntityType::Archer:
    case EntityType::Knight:
        return 5.0f; // Units are small
    default:
        return 20.0f;
    }
}
