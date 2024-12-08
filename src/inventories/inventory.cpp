#include "inventory.h"

#include "networking/clientbound_packets.h"

SlotData& Inventory::getSlotData(int slotIndex) {
    // If the slot doesn't exist, create an empty slot
    if (!slots.contains(slotIndex)) {
        SlotData empty;
        empty.itemId = 0;
        empty.itemCount = 0;
        slots[slotIndex] = empty;
    }
    return slots[slotIndex];
}

void Inventory::clearSlot(int slotIndex) {
    if (slots.contains(slotIndex)) {
        slots[slotIndex].itemId = 0;
        slots[slotIndex].itemCount = 0;
    }
}

Inventory::Inventory(int16_t size) {
    slots.reserve(size);
    this->size = size;
}

void Inventory::SendFullInventoryUpdate(const uint8_t windowID) {
    sendContainerContent(*client, windowID, this->lastStateId, *this);
}

void Inventory::SendSlotUpdate(const int16_t slot, const SlotData &data, const uint8_t windowID) const {
    SendSetContainerSlot(*client, windowID, this->lastStateId, slot, data);
}