#ifndef ENTITY_MANAGER_H
#define ENTITY_MANAGER_H
#include <atomic>
#include <functional>
#include <memory>
#include <unordered_map>

#include "core/utils.h"

class Entity;

class EntityManager {
public:
    EntityManager() : nextEntityID(1000) {} // Starting ID

    int32_t generateUniqueEntityID();
    void addEntity(const std::shared_ptr<Entity>& entity);
    void removeEntity(const std::string& uuidString);
    std::shared_ptr<Entity> getEntity(const std::string& uuidString);
    std::unordered_map<int32_t, std::shared_ptr<Entity>>& getAllEntities();

private:
    std::atomic<int32_t> nextEntityID;
    std::unordered_map<int32_t, std::shared_ptr<Entity>> entitiesByID;
    std::unordered_map<std::string, int32_t> uuidToEntityID;
    std::mutex mutex;
};

#endif //ENTITY_MANAGER_H
