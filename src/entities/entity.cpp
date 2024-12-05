#include "entity.h"

#include "core/server.h"

Entity::Entity(EntityType entityType, double dragX, double dragY, BoundingBox hitBox)
    : onGround(false), hasHeadRotation(false), equipment(), type(entityType), hitBox(hitBox), dragX(dragX), dragY(dragY) {
    entityID = entityManager.generateUniqueEntityID();
    uuid = generateUUID();
    position = {0.0, 0.0, 0.0};
    rotation = {0.0f, 0.0f, 0.0f};
}

Entity::Entity(const std::array<uint8_t, 16>& uuidBytes, EntityType entityType, double dragX, double dragY, BoundingBox hitBox)
    : uuid(uuidBytes), onGround(false), hasHeadRotation(false), equipment(), type(entityType), hitBox(hitBox), dragX(dragX),
      dragY(dragY) {
    entityID = entityManager.generateUniqueEntityID();
    position = {0.0, 0.0, 0.0};
    rotation = {0.0f, 0.0f, 0.0f};
}

void Entity::setMotion(double x, double y, double z) {
    motionX = x;
    motionY = y;
    motionZ = z;
}

BoundingBox Entity::getHitBox() const {
    // Add position to hit box
    BoundingBox adjustedHitBox;
    adjustedHitBox.minX = position.x + hitBox.minX;
    adjustedHitBox.minY = position.y + hitBox.minY;
    adjustedHitBox.minZ = position.z + hitBox.minZ;
    adjustedHitBox.maxX = position.x + hitBox.maxX;
    adjustedHitBox.maxY = position.y + hitBox.maxY;
    adjustedHitBox.maxZ = position.z + hitBox.maxZ;
    return adjustedHitBox;
}
