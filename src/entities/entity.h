#ifndef ENTITY_H
#define ENTITY_H
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "equipment.h"
#include "data/data.h"

// TODO: Replace with actual entity types
enum class EntityType : int32_t {
    Player = 128,
    Item = 58
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
    int32_t entityID{};
    bool newSpawn = true;

    // Unique UUID for the entity
    std::array<uint8_t, 16> uuid{};
    std::string uuidString;

    // Position and rotation
    Position position{};
    Rotation rotation{};
    bool onGround;

    double motionX{};
    double motionY{};
    double motionZ{};

    // Reference: https://minecraft.wiki/w/Entity#Motion_of_entities
    double dragX, dragY;

    bool hasHeadRotation;

    Equipment equipment;

    // Entity type
    EntityType type;

    BoundingBox hitBox;
    
    Entity(const std::array<uint8_t, 16>& uuidBytes, EntityType entityType, double dragX, double dragY, BoundingBox hitBox);
    explicit Entity(EntityType entityType, double dragX, double dragY, BoundingBox hitBox);

    // Virtual destructor for proper cleanup
    virtual ~Entity() = default;

    // Serialize entity-specific data (to be overridden by derived classes)
    virtual void serializeAdditionalData(std::vector<uint8_t>& packetData) const {
        // Base Entity has no additional data
    }

    void setOnGround(bool isOnGround) {
        onGround = isOnGround;
    }

    [[nodiscard]] bool isOnGround() const {
        return onGround;
    }

    void setPosition(double x, double y, double z) {
        position = {x, y, z};
    }

    [[nodiscard]] double getPositionX() const {
        return position.x;
    }

    [[nodiscard]] double getPositionY() const {
        return position.y;
    }

    [[nodiscard]] double getPositionZ() const {
        return position.z;
    }

    void setMotion(double x, double y, double z);

    [[nodiscard]] double getMotionX() const {
        return motionX;
    }

    [[nodiscard]] double getMotionY() const {
        return motionY;
    }

    [[nodiscard]] double getMotionZ() const {
        return motionZ;
    }

    void setDrag(double dx, double dy) {
        dragX = dx;
        dragY = dy;
    }

    double getDragX() const { return dragX; }
    double getDragY() const { return dragY; }

    BoundingBox getHitBox() const;

};

#endif //ENTITY_H
