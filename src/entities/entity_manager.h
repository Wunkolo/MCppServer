#ifndef ENTITY_MANAGER_H
#define ENTITY_MANAGER_H
#include <atomic>
#include <memory>
#include <unordered_map>

#include "entity.h"
#include "core/utils.h"

class EntityManager {
public:
    EntityManager() : nextEntityID(1000) {} // Starting ID

    int32_t generateUniqueEntityID() {
        return nextEntityID.fetch_add(1);
    }

    void addEntity(const std::shared_ptr<Entity>& entity) {
        std::lock_guard<std::mutex> lock(mutex);
        entitiesByID[entity->entityID] = entity;
        entitiesByID[entity->entityID]->uuidString = bytesToUUIDString(entity->uuid);
        std::erase(entitiesByID[entity->entityID]->uuidString, '-');
        uuidToEntityID[entitiesByID[entity->entityID]->uuidString] = entity->entityID;
    }

    void removeEntity(const std::string& uuidString) {
        std::lock_guard lock(mutex);
        auto it = uuidToEntityID.find(uuidString);
        if (it != uuidToEntityID.end()) {
            int32_t entityID = it->second;
            entitiesByID.erase(entityID);
            uuidToEntityID.erase(it);
        }
    }

    std::shared_ptr<Entity> getEntity(const std::string& uuidString) {
        std::lock_guard lock(mutex);
        auto it = uuidToEntityID.find(uuidString);
        if (it != uuidToEntityID.end()) {
            int32_t entityID = it->second;
            return entitiesByID[entityID];
        }
        return nullptr;
    }

    std::vector<std::shared_ptr<Entity>> getAllEntities() {
        std::lock_guard lock(mutex);
        std::vector<std::shared_ptr<Entity>> allEntities;
        for (const auto& [id, entity] : entitiesByID) {
            allEntities.push_back(entity);
        }
        return allEntities;
    }

private:
    std::atomic<int32_t> nextEntityID;
    std::unordered_map<int32_t, std::shared_ptr<Entity>> entitiesByID;
    std::unordered_map<std::string, int32_t> uuidToEntityID;
    std::mutex mutex;
};

#endif //ENTITY_MANAGER_H
