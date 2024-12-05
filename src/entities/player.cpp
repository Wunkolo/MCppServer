#include "player.h"

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "networking/clientbound_packets.h"

bool Player::operator==(const std::shared_ptr<Player> &shared) const {
    return uuid == shared->uuid;
}

Player::Player(const std::array<uint8_t, 16>& uuidBytes, const std::string& playerName, EntityType entityType) : Entity(uuidBytes, entityType, 0.09, 0.02, {-0.3, 0, -0.3, 0.3, 1.8, 0.3}), uuid(uuidBytes), name(playerName), gameMode(), listed(false), ping(0),
                                                                                                                 currentChunkX(0), currentChunkZ(0), flags(0), client(nullptr), viewDistance(0), sessionKey() {
    hasHeadRotation = true;
}

void Player::serializeAdditionalData(std::vector<uint8_t>& packetData) const {
    nlohmann::json metadataJson;
    metadataJson["Flags"] = 0; // No flags set
}

BoundingBox Player::getPickUpBox() const {
    auto pickUpBox = getHitBox();
    pickUpBox.minX -= 0.5;
    pickUpBox.maxX += 0.5;
    pickUpBox.minY -= 0.25;
    pickUpBox.maxY += 0.25;
    pickUpBox.minZ -= 0.5;
    pickUpBox.maxZ += 0.5;
    return pickUpBox;
}

// Returns how many items can be added to the inventory
int8_t Player::canItemBeAddedToInventory(uint16_t id, uint8_t count) const {
    if (id == 0 || count == 0) return 0; // Invalid item or count

    uint8_t maxStack = itemIDs[id].stackSize;
    uint8_t remaining = count;

    // Iterate through hotbar slots (36-44)
    for(int slot = 36; slot <= 44; ++slot) {
        if (inventory.slots.contains(slot)) {
            if (inventory.slots.at(slot).itemId == id && inventory.slots.at(slot).itemCount < maxStack) {
                uint8_t availableSpace = maxStack - inventory.slots.at(slot).itemCount;
                remaining = (remaining > availableSpace) ? (remaining - availableSpace) : 0;
                if (remaining == 0) return count;
            }
        }
    }
    // Iterate through main inventory slots (9-35)
    for(int slot = 9; slot <= 35; ++slot) {
        if (inventory.slots.contains(slot)) {
            if (inventory.slots.at(slot).itemId == id && inventory.slots.at(slot).itemCount < maxStack) {
                uint8_t availableSpace = maxStack - inventory.slots.at(slot).itemCount;
                remaining = (remaining > availableSpace) ? (remaining - availableSpace) : 0;
                if (remaining == 0) return count;
            }
        }
    }

    // After trying to add to existing stacks, check for empty slots in hotbar
    for(int slot = 36; slot <= 44; ++slot) {
        if (!inventory.slots.contains(slot)) {
            uint8_t availableSpace = maxStack;
            remaining = (remaining > availableSpace) ? (remaining - availableSpace) : 0;
            if (remaining == 0) return count;
        } else {
            if (inventory.slots.at(slot).itemId == 0) {
                uint8_t availableSpace = maxStack - inventory.slots.at(slot).itemCount;
                remaining = (remaining > availableSpace) ? (remaining - availableSpace) : 0;
                if (remaining == 0) return count;
            }
        }
    }

    // After trying to add to existing stacks, check for empty slots in main inventory
    for(int slot = 9; slot <= 35; ++slot) {
        if (!inventory.slots.contains(slot)) {
            uint8_t availableSpace = maxStack;
            remaining = (remaining > availableSpace) ? (remaining - availableSpace) : 0;
            if (remaining == 0) return count;
        } else {
            if (inventory.slots.at(slot).itemId == 0) {
                uint8_t availableSpace = maxStack - inventory.slots.at(slot).itemCount;
                remaining = (remaining > availableSpace) ? (remaining - availableSpace) : 0;
                if (remaining == 0) return count;
            }
        }
    }

    // If there's still remaining items, cannot add all
    return count - remaining;
}

void Player::addItemToInventory(uint16_t id, uint8_t count) {
    if (id == 0 || count == 0) return; // Invalid item or count

    uint8_t maxStack = itemIDs[id].stackSize;
    uint8_t remaining = count;

    // First, try to add to existing stacks in hotbar
    for(int slot = 36; slot <= 44; ++slot) {
        if (inventory.slots.contains(slot)) {
            SlotData& slotData = inventory.slots.at(slot);

            if(slotData.itemId == id && slotData.itemCount < maxStack) {
                uint8_t availableSpace = maxStack - slotData.itemCount;
                uint8_t toAdd = std::min(availableSpace, remaining);
                slotData.itemCount += toAdd;
                remaining -= toAdd;
                SendSetContainerSlot(*client, 0, 0, slot, slotData);

                if(remaining == 0) return;
            }
        }
    }

    // Next, add to existing stacks in main inventory
    for(int slot = 9; slot <= 35; ++slot) {
        if (inventory.slots.contains(slot)) {
            SlotData& slotData = inventory.slots.at(slot);

            if(slotData.itemId == id && slotData.itemCount < maxStack) {
                uint8_t availableSpace = maxStack - slotData.itemCount;
                uint8_t toAdd = std::min(availableSpace, remaining);
                slotData.itemCount += toAdd;
                remaining -= toAdd;
                SendSetContainerSlot(*client, 0, 0, slot, slotData);

                if(remaining == 0) return;
            }
        }
    }

    // Next, add to empty slots in hotbar
    for(int slot = 36; slot <= 44; ++slot) {
        if (!inventory.slots.contains(slot)) {
            SlotData slotData;
            uint8_t toAdd = std::min(maxStack, remaining);
            slotData.itemId = id;
            slotData.itemCount = toAdd;
            remaining -= toAdd;
            inventory.slots[slot] = slotData;
            SendSetContainerSlot(*client, 0, 0, slot, slotData);

            if(remaining == 0) return;
        } else {
            SlotData& slotData = inventory.slots.at(slot);
            if (inventory.slots.at(slot).itemId == 0) {
                uint8_t availableSpace = maxStack - slotData.itemCount;
                uint8_t toAdd = std::min(availableSpace, remaining);
                slotData.itemCount += toAdd;
                slotData.itemId = id;
                remaining -= toAdd;
                SendSetContainerSlot(*client, 0, 0, slot, slotData);

                if(remaining == 0) return;
            }
        }
    }

    // Finally, add to empty slots in main inventory
    for(int slot = 9; slot <= 35; ++slot) {
        if (!inventory.slots.contains(slot)) {
            SlotData slotData;
            uint8_t toAdd = std::min(maxStack, remaining);
            slotData.itemId = id;
            slotData.itemCount = toAdd;
            remaining -= toAdd;
            inventory.slots[slot] = slotData;
            SendSetContainerSlot(*client, 0, 0, slot, slotData);

            if(remaining == 0) return;
        } else {
            SlotData& slotData = inventory.slots.at(slot);
            if (inventory.slots.at(slot).itemId == 0) {
                uint8_t availableSpace = maxStack - slotData.itemCount;
                uint8_t toAdd = std::min(availableSpace, remaining);
                slotData.itemCount += toAdd;
                slotData.itemId = id;
                remaining -= toAdd;
                SendSetContainerSlot(*client, 0, 0, slot, slotData);

                if(remaining == 0) return;
            }
        }
    }
}
