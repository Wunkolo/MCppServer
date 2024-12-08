#ifndef PLAYER_INVENTORY_H
#define PLAYER_INVENTORY_H
#include "crafting_inventory.h"


class PlayerInventory final : public CraftingInventory {
public:
    PlayerInventory() : CraftingInventory(46, 2, 2) {}

    void HandleInventoryClick(
        uint8_t windowId,
        int32_t stateId,
        int16_t slotClicked,
        int8_t button,
        int32_t mode,
        const std::vector<std::pair<int16_t, SlotData>>& changedSlotsFromClient,
        const SlotData& clientCarriedItem
    ) override;
    void moveItemToOtherInventorySection(int slotIndex) override;
};



#endif //PLAYER_INVENTORY_H
