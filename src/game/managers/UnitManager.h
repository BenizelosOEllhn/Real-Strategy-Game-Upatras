#pragma once

#include <vector>
#include <glm/glm.hpp>

class Resources;
class GameEntity;
class TownCenter;
class Barracks;
class Building;
class Model;
enum class EntityType;

class UnitManager
{
public:
    struct UnitAssets {
        Model* farmer = nullptr;
        Model* archer = nullptr;
        Model* knight = nullptr;
    };

    void init(Resources* resources,
              std::vector<GameEntity*>* entities,
              const UnitAssets& assets);
    void setActiveResources(Resources* resources);

    void registerTownCenter(TownCenter* tc);
    void registerBarracks(Barracks* barracks);

    bool TrainUnit(EntityType type, Building* building);
    bool HasPendingSpawn() const { return lastSpawnValid_; }
    const glm::vec3& GetLastSpawnPosition() const { return lastSpawnPos_; }
    EntityType GetLastTrainedType() const { return lastTrainedType_; }
    GameEntity* GetLastSpawnedEntity() const { return lastSpawnedEntity_; }

private:
    Resources* resources_ = nullptr;
    std::vector<GameEntity*>* entities_ = nullptr;
    UnitAssets assets_;

    std::vector<TownCenter*> townCenters_;
    std::vector<Barracks*> barracks_;
    glm::vec3 lastSpawnPos_{0.0f};
    EntityType lastTrainedType_;
    bool lastSpawnValid_ = false;
    GameEntity* lastSpawnedEntity_ = nullptr;

    bool trainVillager(TownCenter* tc);
    bool trainRanger(Barracks* barracks);
    bool trainKnight(Barracks* barracks);
    glm::vec3 computeSpawnPosition(const Building* building, const glm::vec3& offset) const;
};
