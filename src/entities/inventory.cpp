#include "inventory.h"

#include "player.h"
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

void Inventory::HandleInventoryClick(
    uint8_t windowId,
    int32_t stateId,
    int16_t slotClicked,
    int8_t button,
    int32_t mode,
    const std::vector<std::pair<int16_t, SlotData>>& changedSlotsFromClient,
    const SlotData& clientCarriedItem
) {
    // Check window ID. If not 0, might be dealing with another window type.
    if (windowId != 0) {
        logMessage("Received inventory click for window ID " + std::to_string(windowId), LOG_DEBUG);
        return;
    }

    // Check State ID. If mismatched with lastStateId, we will need to resync.
    bool needFullResync = (stateId != this->lastStateId);

    SlotData oldCarried = this->carriedItem;

    // Process the click according to mode and button
    switch (mode) {
        // TODO: Check if items are allowed in equipment slots
        case 0: {
            // Normal left/right click
            if (slotClicked >= 0) {
                // A slot in the inventory
                SlotData &clickedSlot = getSlotData(slotClicked);
                if (button == 0) {
                    // Left click
                    // If the carried item is empty, pick up whatever is in the slot.
                    // If the carried item is not empty and the slot is the same item type,
                    // then place items.
                    if (this->carriedItem.itemCount == 0) {
                        // Pick up the entire stack from the slot
                        this->carriedItem = clickedSlot;
                        clearSlot(slotClicked);
                    } else {
                        // If same type, merge; else swap
                        if ((this->carriedItem.itemId == clickedSlot.itemId || clickedSlot.itemId == 0) && slotClicked > 0) {
                            // Attempt to place items from carried into slot
                            uint8_t maxStack = itemIDs[carriedItem.itemId.value()].stackSize;
                            int space = maxStack - clickedSlot.itemCount;
                            int toMove = std::min((int)carriedItem.itemCount, space);
                            clickedSlot.itemId = carriedItem.itemId;
                            clickedSlot.itemCount += toMove;
                            carriedItem.itemCount -= toMove;

                            // If carried stack is now empty, clear it
                            if (carriedItem.itemCount == 0) {
                                carriedItem.itemId = 0;
                            }
                        } else if ((this->carriedItem.itemId == clickedSlot.itemId || clickedSlot.itemId == 0) && slotClicked == 0) {
                            // Attempt to add items to carried stack
                            uint8_t maxStack = itemIDs[carriedItem.itemId.value()].stackSize;
                            int space = maxStack - carriedItem.itemCount;
                            int toMove = std::min(static_cast<int>(clickedSlot.itemCount), space);
                            carriedItem.itemCount += toMove;
                            clickedSlot.itemCount -= toMove;
                        } else {
                            // Different item, just swap
                            if (slotClicked > 0) {
                                SlotData temp = clickedSlot;
                                clickedSlot = carriedItem;
                                carriedItem = temp;
                            }
                        }
                    }
                } else if (button == 1) {
                    // Right click
                    // Similar logic but only move half (or one item) when placing
                    if (carriedItem.itemCount == 0) {
                        // Pick up half the stack from the slot
                        int half = (clickedSlot.itemCount + 1) / 2;
                        carriedItem = clickedSlot;
                        carriedItem.itemCount = half;
                        clickedSlot.itemCount -= half;
                        if (clickedSlot.itemCount == 0) {
                            clickedSlot.itemId = 0;
                        }
                    } else {
                        // Place one item if same type or if slot empty
                        if ((carriedItem.itemId == clickedSlot.itemId || clickedSlot.itemId == 0) && slotClicked > 0) {
                            if (clickedSlot.itemId == 0) {
                                clickedSlot.itemId = carriedItem.itemId;
                            }
                            if (clickedSlot.itemCount < itemIDs[clickedSlot.itemId.value()].stackSize) {
                                clickedSlot.itemCount += 1;
                                carriedItem.itemCount -= 1;
                                if (carriedItem.itemCount == 0) {
                                    carriedItem.itemId = 0;
                                }
                            }
                        } else if ((this->carriedItem.itemId == clickedSlot.itemId || clickedSlot.itemId == 0) && slotClicked == 0) {
                            // Attempt to add items to carried stack
                            uint8_t maxStack = itemIDs[carriedItem.itemId.value()].stackSize;
                            int space = maxStack - carriedItem.itemCount;
                            int toMove = std::min(static_cast<int>(clickedSlot.itemCount), space);
                            carriedItem.itemCount += toMove;
                            clickedSlot.itemCount -= toMove;
                        } else {
                            // Swap only one item? Usually right-click on different item does a full swap.
                            // Actually, in vanilla MC: right-click with different item swaps entire stack.
                            // Let’s do full swap:
                            SlotData temp = clickedSlot;
                            clickedSlot = carriedItem;
                            carriedItem = temp;
                        }
                    }
                }
            } else {
                // Slot = -999 means click outside inventory
                if (button == 0) {
                    // Left click outside: drop entire carried stack
                    // TODO: spawn an item entity in the world here.
                    carriedItem.itemId = 0;
                    carriedItem.itemCount = 0;
                } else if (button == 1) {
                    // Right click outside: drop single item
                    if (carriedItem.itemCount > 0) {
                        // spawn single item
                        carriedItem.itemCount -= 1;
                        if (carriedItem.itemCount == 0) {
                            carriedItem.itemId = 0;
                        }
                    }
                }
            }
            break;
        }
        case 1: {
            // Shift-click
            if (slotClicked > 0) {
                moveItemToOtherInventorySection(slotClicked);
            }
            while (CanCraft()) {
                ConsumeCraftingIngredients();
                moveItemToOtherInventorySection(slotClicked);
                UpdateCraftingResult();
            }
            break;
        }
        case 2: {
            // Number key
            // If slotClicked >= 0, we swap the item in the clicked slot with the corresponding hotbar slot.
            // Hotbar slots are 36 to 44. Key 1 (button=0) -> slot 36; Key 2 (button=1) -> slot 37; ... Key 9 (button=8) -> slot 44.
            if (slotClicked >= 0 && button >= 0 && button <= 8) {
                int hotbarSlot = 36 + button;
                SlotData &clickedSlot = getSlotData(slotClicked);
                SlotData &hbSlot = getSlotData(hotbarSlot);

                // Swap them
                SlotData temp = clickedSlot;
                clickedSlot = hbSlot;
                hbSlot = temp;
            }
            break;
        }
        case 5: {
            // Painting mode
            // We have three phases: start, add slot, end
            if (slotClicked == -999) {
                // This is either start or end depending on button
                if (button == 0 || button == 4 || button == 8) {
                    // Start drag
                    dragging = true;
                    dragButtonType = button; // 0=left, 4=right, 8=middle
                    dragSlots.clear();
                } else if (button == 2 || button == 6 || button == 10) {
                    // End drag
                    if (!dragging) break; // sanity check
                    // Distribute items
                    if (carriedItem.itemCount > 0 && !dragSlots.empty()) {
                        // How we distribute depends on button type
                        if (dragButtonType == 0) {
                            // Left drag: distribute evenly
                            int totalSlots = (int)dragSlots.size();
                            int totalItems = carriedItem.itemCount;
                            int perSlot = totalSlots > 0 ? (totalItems / totalSlots) : 0;
                            // No distributing remainder here. If totalItems doesn't divide evenly,
                            // remainder stays in carriedItem.

                            for (int slotIndex : dragSlots) {
                                SlotData &ds = getSlotData(slotIndex);
                                // If same type or empty
                                if (ds.itemId == 0 || ds.itemId == carriedItem.itemId) {
                                    uint8_t maxStack = itemIDs[carriedItem.itemId.value()].stackSize;
                                    int space = maxStack - ds.itemCount;
                                    int toAdd = std::min(perSlot, space);
                                    if (toAdd > 0) {
                                        if (ds.itemId == 0) ds.itemId = carriedItem.itemId;
                                        ds.itemCount += (uint8_t)toAdd;
                                        carriedItem.itemCount -= (uint8_t)toAdd;
                                    }
                                }
                            }
                        } else if (dragButtonType == 4) {
                            // Right drag: place one item in each slot if possible
                            // Only one per slot
                            for (int slotIndex : dragSlots) {
                                if (carriedItem.itemCount == 0) break;
                                SlotData &ds = getSlotData(slotIndex);
                                // If same type or empty
                                if (ds.itemId == 0 || ds.itemId == carriedItem.itemId) {
                                    uint8_t maxStack = itemIDs[carriedItem.itemId.value()].stackSize;
                                    if (ds.itemCount < maxStack) {
                                        if (ds.itemId == 0) ds.itemId = carriedItem.itemId;
                                        ds.itemCount += 1;
                                        carriedItem.itemCount -= 1;
                                    }
                                }
                            }
                        } else if (dragButtonType == 8) {
                            // Middle drag (creative): similar to left drag but creative rules apply.
                            // In creative mode, you can just set full stacks. For simplicity:
                            for (int slotIndex : dragSlots) {
                                SlotData &ds = getSlotData(slotIndex);
                                ds.itemId = carriedItem.itemId;
                                ds.itemCount = itemIDs[carriedItem.itemId.value()].stackSize;
                                // carried item not reduced in creative
                            }
                        }
                    }

                    // Drag ended
                    dragging = false;
                    dragSlots.clear();
                }
            } else {
                // Adding a slot to the drag
                // For left drag: button = 1 when adding slots
                // For right drag: button = 5 when adding slots
                // For middle drag: button = 9 when adding slots
                if (dragging) {
                    // Add this slot to the dragSlots if not already added
                    if (std::find(dragSlots.begin(), dragSlots.end(), slotClicked) == dragSlots.end()) {
                        dragSlots.push_back(slotClicked);
                    }
                }
            }
            break;
        }
        case 6: {
            // Double click
            // Conditions:
            //  - mode = 6, button = 0, slot >= 0
            //  The player attempts to gather all items of the clicked item's type into the cursor.
            if (slotClicked >= 0 && button == 0) {
                SlotData &clickedSlot = getSlotData(slotClicked);

                // Determine the item type we are trying to gather
                // If carriedItem is empty, we need to pick up the clicked slot's item first
                if (carriedItem.itemCount == 0 && clickedSlot.itemId != 0) {
                    // Pick up the entire stack from the clicked slot
                    carriedItem = clickedSlot;
                    clearSlot(slotClicked);
                }

                // If we now have an item in carriedItem, try to gather more of that type
                if (carriedItem.itemCount > 0) {
                    uint16_t targetId = carriedItem.itemId.value();
                    uint8_t maxStack = itemIDs[targetId].stackSize;

                    // Iterate over all inventory slots to find the same item
                    // Note: This includes main inventory, hotbar, potentially crafting slots, armor, offhand.
                    // For now, assume all inventory slots are considered.
                    for (auto &slotPair : slots) {
                        SlotData &slotData = slotPair.second;

                        // If the slot has the same item
                        if (slotData.itemId == targetId && carriedItem.itemCount < maxStack) {
                            // Calculate how many we can take
                            uint8_t canTake = (uint8_t)std::min<int>(slotData.itemCount, maxStack - carriedItem.itemCount);
                            if (canTake > 0) {
                                carriedItem.itemCount += canTake;
                                slotData.itemCount -= canTake;
                                if (slotData.itemCount == 0) {
                                    slotData.itemId = 0;
                                }
                            }

                            // If carried item is now full, break early
                            if (carriedItem.itemCount == maxStack) {
                                break;
                            }
                        }
                    }
                }
            }
            break;
        }
        default:
            // Not implemented modes
            break;
    }

    // If crafting slots changed (slots 1-4), update the result slot (0)
    if (std::ranges::any_of(changedSlotsFromClient, [](const auto &pair) {
        return pair.first >= 1 && pair.first <= 4;
    })) {
        OnCraftingSlotChanged();
    }

    // If player takes items out of result slot, update the craft slots
    if (std::ranges::any_of(changedSlotsFromClient, [](const auto &pair) {
        return pair.first == 0;
    })) {
        ConsumeCraftingIngredients();
        UpdateCraftingResult();
    }

    // After handling the logic, we should update the State ID
    this->lastStateId = stateId;

    // TODO: Compare final state with client’s expectation
    // If mismatch: send SetContainerSlot for each slot that's different
    // or if State ID mismatch was detected: SendFullInventoryUpdate()

    // For simplicity:
    if (needFullResync) {
        SendFullInventoryUpdate();
    } else {
        // Compare changedSlotsFromClient to our final state,
        // and if there's a discrepancy, send corrections.
        for (auto & [clientSlot, clientData] : changedSlotsFromClient) {
            SlotData &serverData = getSlotData(clientSlot);
            if (serverData.itemId != clientData.itemId || serverData.itemCount != clientData.itemCount) {
                // Send correction
                SendSlotUpdate(clientSlot, serverData);
            }
            // Also check if crafting result slot is different
            if (clientSlot >= 1 && clientSlot <= 4) {
                SlotData &resultData = getSlotData(0);
                SendSlotUpdate(0, resultData);
            }
        }
        // Also check if carriedItem matches clientCarriedItem
        if (carriedItem.itemId != clientCarriedItem.itemId || carriedItem.itemCount != clientCarriedItem.itemCount) {
            SendFullInventoryUpdate();
        }
    }
}

void Inventory::SendFullInventoryUpdate() {
    sendContainerContent(*client, 0 /* windowId */, this->lastStateId, *this);
}

void Inventory::SendSlotUpdate(int16_t slot, const SlotData &data) const {
    SendSetContainerSlot(*client, 0 /* windowId */, this->lastStateId, slot, data);
}

void Inventory::moveItemToOtherInventorySection(int slotIndex) {
    SlotData &fromSlot = getSlotData(slotIndex);
    if (fromSlot.itemId == 0 || fromSlot.itemCount == 0) return;

    int startRange, endRange;
    // Determine source and target ranges
    // If clicked slot in main inventory (9-35), move to hotbar (36-44)
    if (slotIndex >= 9 && slotIndex <= 35) {
        startRange = 36; endRange = 44;
    } else if (slotIndex >= 36 && slotIndex <= 44) {
        startRange = 9; endRange = 35;
    } else {
        // If crafting or armor, decide a default: move to main inventory (9-35)
        startRange = 9; endRange = 35;
    }

    uint16_t id = fromSlot.itemId.value();
    uint8_t count = fromSlot.itemCount;
    uint8_t maxStack = itemIDs[id].stackSize;

    // First try to merge with existing stacks of the same type
    for (int slot = startRange; slot <= endRange && count > 0; slot++) {
        SlotData &targetSlot = getSlotData(slot);
        if (targetSlot.itemId == id && targetSlot.itemCount < maxStack) {
            uint8_t space = maxStack - targetSlot.itemCount;
            uint8_t toAdd = std::min(space, count);
            targetSlot.itemCount += toAdd;
            count -= toAdd;
        }
    }

    // If still have remaining, try empty slots
    for (int slot = startRange; slot <= endRange && count > 0; slot++) {
        SlotData &targetSlot = getSlotData(slot);
        if (targetSlot.itemId == 0) {
            uint8_t toAdd = std::min((uint8_t)maxStack, count);
            targetSlot.itemId = id;
            targetSlot.itemCount = toAdd;
            count -= toAdd;
        }
    }

    // Update source slot with remaining items (if any)
    fromSlot.itemCount = count;
    if (count == 0) {
        fromSlot.itemId = 0;
    }
}

// Updates the crafting result slot (0) based on what's in slots 1-4.
void Inventory::UpdateCraftingResult() {
    // The player crafting grid: 2x2
    // Slots:
    // 0 = result
    // 1,2
    // 3,4

    // Gather input items from slots 1-4
    std::array<uint16_t,4> inputItems;
    for (int i = 0; i < 4; i++) {
        SlotData &slot = getSlotData(i+1);
        inputItems[i] = slot.itemId.value();
    }

    // Try to find a matching recipe
    SlotData resultSlotData;
    resultSlotData.itemId = 0;
    resultSlotData.itemCount = 0;

    if (FindMatchingRecipe(inputItems, resultSlotData)) {
        // Place the result in slot 0
        SlotData &resultSlot = getSlotData(0);
        resultSlot = resultSlotData;
    } else {
        // No recipe matched, clear slot 0
        clearSlot(0);
    }
}

// Attempts to find a recipe that matches the given input and sets resultSlotData accordingly.
// Returns true if a match is found, false otherwise.
auto Inventory::FindMatchingRecipe(const std::array<uint16_t, 4> &inputItems, SlotData &resultSlotData) -> bool {
    // We must check all recipes to see if any match this 2x2 input.
    // Note: In a complex system, you'd store separate maps for shaped and shapeless recipes,
    // or index by multiple criteria. Here we brute force through all recipes.

    for (auto &pair : craftingRecipes) {
        const CraftingRecipe &recipe = pair.second;

        if (!MatchesRecipe(recipe, inputItems)) {
            continue;
        }

        // If matched, set the result
        resultSlotData.itemId = recipe.result;
        resultSlotData.itemCount = recipe.resultCount;
        return true;
    }

    return false;
}

// Checks if the given 2x2 inputItems match a particular recipe.
bool Inventory::MatchesRecipe(const CraftingRecipe &recipe, const std::array<uint16_t,4> &inputItems) {
    if (!recipe.shapeless) {
        // Shaped recipe
        // Player crafting grid is always 2x2 here, recipe.width <=2 and recipe.height <=2.

        if (recipe.width > 2 || recipe.height > 2) return false;

        // Determine possible x and y offsets
        // xOffsets: If width=2, xOffsets={0}, if width=1, xOffsets={0,1}
        std::vector<int> xOffsets;
        xOffsets.push_back(0);
        if (recipe.width == 1) {
            xOffsets.push_back(1);
        }

        std::vector<int> yOffsets;
        yOffsets.push_back(0);
        if (recipe.height == 1) {
            yOffsets.push_back(1);
        }

        // We'll try placing the recipe at all (xOffset, yOffset) combinations
        // and see if one matches exactly.
        for (int yOff : yOffsets) {
            for (int xOff : xOffsets) {
                if (CheckShapedRecipePlacement(recipe, inputItems, xOff, yOff)) {
                    return true;
                }
            }
        }

        return false;
    } else {
        // Shapeless logic
        std::unordered_map<uint16_t,int> needed;
        for (uint16_t ing : recipe.ingredients) {
            if (ing != 0) {
                needed[ing]++;
            }
        }

        for (auto iid : inputItems) {
            if (iid != 0) {
                if (needed.find(iid) == needed.end()) {
                    return false; // Extra item not in recipe
                } else {
                    needed[iid]--;
                    if (needed[iid] < 0) {
                        return false; // More of this item than needed
                    }
                }
            }
        }

        for (auto &kv : needed) {
            if (kv.second > 0) return false; // Not all ingredients provided
        }

        return true;
    }
}

// This helper checks if placing the shaped recipe at a given offset matches the inputItems.
bool Inventory::CheckShapedRecipePlacement(const CraftingRecipe &recipe, const std::array<uint16_t,4> &inputItems, int xOff, int yOff) {
    // recipeIngredients indexed by x+(y*width)
    // inputItems mapping:
    // inputItems:
    //   [0] = slot 1 (top-left)
    //   [1] = slot 2 (top-right)
    //   [2] = slot 3 (bottom-left)
    //   [3] = slot 4 (bottom-right)
    //
    // Let's create a function to get item from inputItems by grid coords:
    auto getInput = [&](int x, int y) -> uint16_t {
        // x and y in [0,1]
        int index = x + (y * 2);
        return inputItems[index];
    };

    // Check every cell of the recipe
    for (int ry = 0; ry < recipe.height; ry++) {
        for (int rx = 0; rx < recipe.width; rx++) {
            uint16_t required = recipe.ingredients[rx + ry * recipe.width];
            uint16_t actual = getInput(rx + xOff, ry + yOff);
            if (actual != required) {
                return false; // Mismatch at this cell
            }
        }
    }

    // Also check that other slots not covered by recipe are empty (0)
    // Because if the player has extra items in the crafting grid that are not part of the recipe pattern,
    // shaped recipes won't match.
    for (int y = 0; y < 2; y++) {
        for (int x = 0; x < 2; x++) {
            bool inRecipeArea = (x >= xOff && x < xOff + recipe.width &&
                                 y >= yOff && y < yOff + recipe.height);
            if (!inRecipeArea) {
                // This slot should be empty (0) for a proper shaped match
                if (getInput(x, y) != 0) {
                    return false;
                }
            }
        }
    }

    return true;
}

// Called after the player takes the crafted item from slot 0.
// This consumes the ingredients from slots 1-4 based on the matched recipe.
void Inventory::ConsumeCraftingIngredients() {
    std::array<uint16_t,4> inputItems;
    for (int i = 0; i < 4; i++) {
        SlotData &slot = getSlotData(i+1);
        inputItems[i] = slot.itemId.value();
    }

    CraftingRecipe matchedRecipe;
    bool found = false;
    for (auto &pair : craftingRecipes) {
        const CraftingRecipe &recipe = pair.second;
        if (MatchesRecipe(recipe, inputItems)) {
            matchedRecipe = recipe;
            found = true;
            break;
        }
    }

    if (!found) {
        // No recipe matched to consume
        return;
    }

    if (!matchedRecipe.shapeless) {
        // Shaped: We need to find the offsets used in the match
        int xOff, yOff;
        if (!FindShapedRecipePlacement(matchedRecipe, inputItems, xOff, yOff)) {
            // This should not happen if MatchesRecipe returned true, but just in case:
            return;
        }

        // Now consume ingredients using the found offsets
        for (int y = 0; y < matchedRecipe.height; y++) {
            for (int x = 0; x < matchedRecipe.width; x++) {
                uint16_t ing = matchedRecipe.ingredients[x + y * matchedRecipe.width];
                if (ing != 0) {
                    // Apply offsets to find correct slot
                    int inputIndex = (x + xOff) + (y + yOff)*2; // adjusted with offsets
                    int slotIndex = inputIndex + 1; // slot 1..4
                    SlotData &sd = getSlotData(slotIndex);
                    if (sd.itemCount > 0) {
                        sd.itemCount--;
                        if (sd.itemCount == 0) {
                            sd.itemId = 0;
                        }
                    }
                }
            }
        }
    } else {
        // Shapeless
        std::unordered_map<uint16_t,int> needed;
        for (uint16_t ing : matchedRecipe.ingredients) {
            if (ing != 0) {
                needed[ing]++;
            }
        }

        for (int i = 1; i <= 4; i++) {
            SlotData &sd = getSlotData(i);
            if (sd.itemId != 0 && needed.contains(sd.itemId.value()) && needed[sd.itemId.value()] > 0) {
                sd.itemCount--;
                needed[sd.itemId.value()]--;
                if (sd.itemCount == 0) {
                    sd.itemId = 0;
                }
            }
        }
    }
}

bool Inventory::FindShapedRecipePlacement(const CraftingRecipe &recipe, const std::array<uint16_t,4> &inputItems, int &outXOff, int &outYOff) {
    if (recipe.width > 2 || recipe.height > 2) return false;

    // Determine possible x and y offsets
    std::vector<int> xOffsets; xOffsets.push_back(0);
    if (recipe.width == 1) xOffsets.push_back(1);

    std::vector<int> yOffsets; yOffsets.push_back(0);
    if (recipe.height == 1) yOffsets.push_back(1);

    for (int yOff : yOffsets) {
        for (int xOff : xOffsets) {
            if (CheckShapedRecipePlacement(recipe, inputItems, xOff, yOff)) {
                outXOff = xOff;
                outYOff = yOff;
                return true;
            }
        }
    }
    return false;
}

// Call this after any change in crafting slots or after taking the result item:
void Inventory::OnCraftingSlotChanged() {
    UpdateCraftingResult();
}

bool Inventory::CanCraft() {
    std::array<uint16_t,4> inputItems;
    for (int i = 0; i < 4; i++) {
        SlotData &slot = getSlotData(i+1);
        inputItems[i] = slot.itemId.value();
    }

    // Find the same recipe again
    bool found = false;
    for (auto &pair : craftingRecipes) {
        const CraftingRecipe &recipe = pair.second;
        if (MatchesRecipe(recipe, inputItems)) {
            found = true;
            break;
        }
    }
    return found;
}
