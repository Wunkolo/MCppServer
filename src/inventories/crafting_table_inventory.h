#ifndef CRAFTING_TABLE_INVENTORY_H
#define CRAFTING_TABLE_INVENTORY_H
#include "crafting_inventory.h"
#include "external_inventory.h"

class PlayerInventory;

class CraftingTableInventory final : public CraftingInventory, public ExternalInventory {
public:
    explicit CraftingTableInventory(const uint8_t windowID) : CraftingInventory(55, 3, 3),
                                                              windowID(windowID) {
    }

    void copyPlayerInventory(const PlayerInventory& playerInventory) override;
    void copyToPlayerInventory(PlayerInventory& playerInventory) override;

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

private:
    uint8_t windowID;
};

#endif //CRAFTING_TABLE_INVENTORY_H
