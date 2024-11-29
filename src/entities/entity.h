#ifndef ENTITY_H
#define ENTITY_H
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "equipment.h"

// TODO: Replace with actual entity types
enum class EntityType : int32_t {
    Player = 128
};

struct Position {
    double x;
    double y;
    double z;
};

struct Rotation {
    float yaw;
    float pitch;
    float headYaw;
};

class Entity {
public:
    // Unique server-side entity ID
    int32_t entityID;
    bool newSpawn = true;

    // Unique UUID for the entity
    std::array<uint8_t, 16> uuid;
    std::string uuidString;

    // Position and rotation
    Position position;
    Rotation rotation;
    bool onGround;

    bool hasHeadRotation;

    Equipment equipment;

    // Entity type
    EntityType type;
    
    Entity(int32_t id, const std::array<uint8_t, 16>& uuidBytes, EntityType entityType)
        : entityID(id), uuid(uuidBytes), onGround(false), hasHeadRotation(false), equipment(), type(entityType) {
        // Initialize default position and rotation
        position = {0.0, 0.0, 0.0};
        rotation = {0.0f, 0.0f, 0.0f};
    }

    // Virtual destructor for proper cleanup
    virtual ~Entity() = default;

    // Serialize entity-specific data (to be overridden by derived classes)
    virtual void serializeAdditionalData(std::vector<uint8_t>& packetData) const {
        // Base Entity has no additional data
    }
};

#endif //ENTITY_H
