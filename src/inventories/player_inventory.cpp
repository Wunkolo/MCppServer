#include "player_inventory.h"

#include "core/server.h"
#include "core/utils.h"

void PlayerInventory::HandleInventoryClick(
    uint8_t windowId,
    int32_t stateId,
    int16_t slotClicked,
    int8_t button,
    int32_t mode,
    const std::vector<std::pair<int16_t, SlotData>>& changedSlotsFromClient,
    const SlotData& clientCarriedItem
) {
    // Check window ID. If not 0, we are dealing with another window type.
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

            if (CraftableRecipe() != -1) {
                do {
                    ConsumeCraftingIngredients();
                    moveItemToOtherInventorySection(slotClicked);
                    UpdateCraftingResult();
                } while (CraftableRecipe() == lastCraftedRecipeId && lastCraftedRecipeId != -1 && CraftableRecipe() != -1);
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
                    // For now, all inventory slots are considered.
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
    }) && mode != 1) {
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
        SendFullInventoryUpdate(0);
    } else {
        // Compare changedSlotsFromClient to our final state,
        // and if there's a discrepancy, send corrections.
        for (auto & [clientSlot, clientData] : changedSlotsFromClient) {
            SlotData &serverData = getSlotData(clientSlot);
            if (serverData.itemId != clientData.itemId || serverData.itemCount != clientData.itemCount) {
                // Send correction
                SendSlotUpdate(clientSlot, serverData, 0);
            }
            // Also check if crafting result slot is different
            if (clientSlot >= 1 && clientSlot <= 4) {
                SlotData &resultData = getSlotData(0);
                SendSlotUpdate(0, resultData, 0);
            }
        }
        // Also check if carriedItem matches clientCarriedItem
        if (carriedItem.itemId != clientCarriedItem.itemId || carriedItem.itemCount != clientCarriedItem.itemCount) {
            SendFullInventoryUpdate(0);
        }
    }
}

void PlayerInventory::moveItemToOtherInventorySection(int slotIndex) {
    SlotData &fromSlot = getSlotData(slotIndex);
    if (fromSlot.itemId == 0 || fromSlot.itemCount == 0) return;

    int startRange, endRange;
    bool reverse = false;
    // Determine source and target ranges
    // If clicked slot in main inventory (9-35), move to hotbar (36-44)
    if (slotIndex >= 9 && slotIndex <= 35) {
        startRange = 36; endRange = 44;
    } else if (slotIndex >= 36 && slotIndex <= 44) {
        startRange = 9; endRange = 35;
    } else if (slotIndex == 0) {
        // If clicked slot is result slot, move to hotbar, then main inventory in reverse
        startRange = 9; endRange = 44;
        reverse = true;
    } else {
        // If crafting or armor, decide a default: move to main inventory (9-35)
        startRange = 9; endRange = 35;
    }

    uint16_t id = fromSlot.itemId.value();
    uint8_t count = fromSlot.itemCount;
    uint8_t maxStack = itemIDs[id].stackSize;

    // First try to merge with existing stacks of the same type
    if (reverse) {
        for (int slot = endRange; slot >= startRange && count > 0; slot--) {
            SlotData &targetSlot = getSlotData(slot);
            if (targetSlot.itemId == id && targetSlot.itemCount < maxStack) {
                uint8_t space = maxStack - targetSlot.itemCount;
                uint8_t toAdd = std::min(space, count);
                targetSlot.itemCount += toAdd;
                count -= toAdd;
            }
        }

        // If still have remaining, try empty slots
        for (int slot = endRange; slot >= startRange && count > 0; slot--) {
            SlotData &targetSlot = getSlotData(slot);
            if (targetSlot.itemId == 0) {
                uint8_t toAdd = std::min((uint8_t)maxStack, count);
                targetSlot.itemId = id;
                targetSlot.itemCount = toAdd;
                count -= toAdd;
            }
        }
    } else {
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
    }

    // Update source slot with remaining items (if any)
    fromSlot.itemCount = count;
    if (count == 0) {
        fromSlot.itemId = 0;
    }
}


