#include "item_entity.h"

#include "core/server.h"
#include "networking/clientbound_packets.h"
#include "networking/network.h"

Item::Item() : Entity(EntityType::Item, 0.02, 0.02, {-0.125, 0, -0.125, 0.125, 0.25, 0.125}) {}

void Item::serializeAdditionalData(std::vector<uint8_t> &packetData) const {

}

int32_t Item::id() const {
    if (slotData.itemId.has_value()) {
        return slotData.itemId.value();
    }
    return 0;
}

void Item::setItemId(int16_t id) {
    slotData.itemId = id;
}

void Item::setItemCount(int8_t count) {
    slotData.itemCount = count;
}

std::vector<MetadataEntry> Item::getMetadata() const {
    std::vector<MetadataEntry> entries;
    MetadataEntry entry;
    std::vector<uint8_t> value;
    entry.index = 8; // Entity Metadata index for item
    entry.type = 7; // Slot
    writeSlotSimple(value, slotData);
    entry.value = value;
    entries.push_back(entry);
    return entries;
}

void Item::tryMerge() {
    for (auto &entity : entityManager.getAllEntities() | std::views::values) {
        if (entity->type != EntityType::Item) {
            continue;
        }

        auto item = std::static_pointer_cast<Item>(entity);
        if (item->uuidString == uuidString) {
            continue;
        }

        if (item->id() != id()) {
            continue;
        }

        // Test if it is in 0.5 x 0.25 x 0.5 radius
        if (std::abs(item->position.x - position.x) > 0.5 || std::abs(item->position.y - position.y) > 0.25 || std::abs(item->position.z - position.z) > 0.5) {
            continue;
        }

        // Merge the items
        if (slotData.itemCount + item->slotData.itemCount > 64) {
            continue;
        }
        if (slotData.itemCount > item->slotData.itemCount) {
            slotData.itemCount += item->slotData.itemCount;
            item->slotData.itemCount = 0;
            newSpawn = false;
            entityManager.removeEntity(item->uuidString);
            sendEntityMetadataPacket(getMetadata(), entityID);
            return;
        }
        item->slotData.itemCount += slotData.itemCount;
        slotData.itemCount = 0;
        newSpawn = false;
        entityManager.removeEntity(uuidString);
        sendEntityMetadataPacket(item->getMetadata(), item->entityID);
        return;
    }
}
