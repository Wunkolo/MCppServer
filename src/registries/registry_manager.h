#ifndef REGISTRY_MANAGER_H
#define REGISTRY_MANAGER_H
#include <cstdint>
#include <string>
#include <unordered_map>

#include "encryption/rsa_key.h"


// Registry Manager to handle registry entries and their IDs (and some other stuff like RSA keys, because why not)
class RegistryManager {
public:
    // Maps registry name to a map of entry identifier to ID
    std::unordered_map<std::string, std::unordered_map<std::string, int32_t>> registries;

    // Adds a registry entry and returns its assigned ID
    int32_t addRegistryEntry(const std::string& registryName, const std::string& entryIdentifier);

    // Retrieves the ID of a registry entry
    int32_t getRegistryID(const std::string& registryName, const std::string& entryIdentifier) const;

    RSAKeyPair& getRSAKeyPair() { return rsaKeyPair; }

private:
    RSAKeyPair rsaKeyPair;
};


#endif //REGISTRY_MANAGER_H
