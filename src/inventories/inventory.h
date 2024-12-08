#ifndef INVENTORY_H
#define INVENTORY_H
#include <cstdint>
#include <unordered_map>

#include "entities/slot_data.h"

class CraftingRecipe;
struct ClientConnection;

class Inventory {
public:
    virtual ~Inventory() = default;

    // Inventory slots
    std::unordered_map<int, SlotData> slots;

    // Item currently "carried" by the player's cursor
    SlotData carriedItem;

    // State ID to track inventory synchronization
    int32_t lastStateId = 0;

    int16_t size;

    ClientConnection *client = nullptr;

    bool dragging = false;
    uint8_t dragButtonType = 0; // 0 = left, 4 = right, 8 = middle
    std::vector<int> dragSlots;

    explicit Inventory(int16_t size);

    // Method to handle inventory click
    virtual void HandleInventoryClick(
        uint8_t windowId,
        int32_t stateId,
        int16_t slotClicked,
        int8_t button,
        int32_t mode,
        const std::vector<std::pair<int16_t, SlotData>>& changedSlotsFromClient,
        const SlotData& clientCarriedItem
    ) = 0;

    void SendFullInventoryUpdate(uint8_t windowID);
    void SendSlotUpdate(int16_t slot, const SlotData &data, uint8_t windowID) const;
    SlotData & getSlotData(int slotIndex);
    void clearSlot(int slotIndex);
    virtual void moveItemToOtherInventorySection(int slotIndex) = 0;

};

#endif //INVENTORY_H
