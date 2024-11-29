#include "registry_manager.h"

int32_t RegistryManager::addRegistryEntry(const std::string& registryName, const std::string& entryIdentifier) {
    auto& registry = registries[registryName];
    if (registry.contains(entryIdentifier)) {
        return registry[entryIdentifier]; // Entry already exists
    }
    const int32_t id = registry.size();
    registry[entryIdentifier] = id;
    return id;
}

int32_t RegistryManager::getRegistryID(const std::string& registryName, const std::string& entryIdentifier) const {
    auto it = registries.find(registryName);
    if (it != registries.end()) {
        auto entryIt = it->second.find(entryIdentifier);
        if (entryIt != it->second.end()) {
            return entryIt->second;
        }
    }
    return -1; // Indicates not found
}