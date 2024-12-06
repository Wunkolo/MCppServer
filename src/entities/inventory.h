#ifndef INVENTORY_H
#define INVENTORY_H
#include <unordered_map>

#include "slot_data.h"

class CraftingRecipe;
struct ClientConnection;

class Inventory {
public:
    // Inventory slots
    std::unordered_map<int, SlotData> slots;

    // Item currently "carried" by the player's cursor
    SlotData carriedItem;

    // State ID to track inventory synchronization
    int32_t lastStateId = 0;

    int16_t size;

    ClientConnection *client = nullptr;

    Inventory(int16_t size);

    // Method to handle inventory click
    void HandleInventoryClick(
        uint8_t windowId,
        int32_t stateId,
        int16_t slotClicked,
        int8_t button,
        int32_t mode,
        const std::vector<std::pair<int16_t, SlotData>>& changedSlotsFromClient,
        const SlotData& clientCarriedItem
    );

    void SendFullInventoryUpdate();
    void SendSlotUpdate(int16_t slot, const SlotData &data) const;
    SlotData & getSlotData(int slotIndex);
    void clearSlot(int slotIndex);
    void moveItemToOtherInventorySection(int slotIndex);
    void UpdateCraftingResult();
    void ConsumeCraftingIngredients();
    void OnCraftingSlotChanged();
    bool CanCraft();
    auto FindMatchingRecipe(const std::array<uint16_t, 4> &inputItems, SlotData &resultSlotData) -> bool;
    static auto MatchesRecipe(const CraftingRecipe &recipe, const std::array<uint16_t, 4> &inputItems) -> bool;
    static bool CheckShapedRecipePlacement(const CraftingRecipe &recipe, const std::array<uint16_t,4> &inputItems, int xOff, int yOff);
    static bool FindShapedRecipePlacement(const CraftingRecipe &recipe, const std::array<uint16_t,4> &inputItems, int &outXOff, int &outYOff);

private:
    bool dragging = false;
    uint8_t dragButtonType = 0; // 0 = left, 4 = right, 8 = middle
    std::vector<int> dragSlots;
};

#endif //INVENTORY_H
