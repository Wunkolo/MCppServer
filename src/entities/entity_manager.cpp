#include "entity_manager.h"

#include <functional>

#include "entity.h"
#include "networking/clientbound_packets.h"

int32_t EntityManager::generateUniqueEntityID() {
    return nextEntityID.fetch_add(1);
}

void EntityManager::addEntity(const std::shared_ptr<Entity>& entity) {
    std::lock_guard lock(mutex);
    entitiesByID[entity->entityID] = entity;
    entitiesByID[entity->entityID]->uuidString = bytesToUUIDString(entity->uuid);
    std::erase(entitiesByID[entity->entityID]->uuidString, '-');
    uuidToEntityID[entitiesByID[entity->entityID]->uuidString] = entity->entityID;
}

void EntityManager::removeEntity(const std::string& uuidString) {
    std::lock_guard lock(mutex);
    auto it = uuidToEntityID.find(uuidString);
    if (it != uuidToEntityID.end()) {
        int32_t entityID = it->second;
        sendRemoveEntityPacket(entityID);
        entitiesByID.erase(entityID);
        uuidToEntityID.erase(it);
    }
}

std::shared_ptr<Entity> EntityManager::getEntity(const std::string& uuidString) {
    std::lock_guard lock(mutex);
    auto it = uuidToEntityID.find(uuidString);
    if (it != uuidToEntityID.end()) {
        int32_t entityID = it->second;
        return entitiesByID[entityID];
    }
    return nullptr;
}

std::unordered_map<int32_t, std::shared_ptr<Entity>>& EntityManager::getAllEntities() {
    std::lock_guard lock(mutex);
    return entitiesByID;
}