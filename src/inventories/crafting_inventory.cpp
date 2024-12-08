#include "crafting_inventory.h"

#include <array>

#include "core/server.h"
#include "data/crafting_recipes.h"
#include "entities/slot_data.h"

// Updates the crafting result slot (0) based on what's in the crafting grid.
void CraftingInventory::UpdateCraftingResult() {
    int totalSlots = craftWidth * craftHeight;
    std::vector<uint16_t> inputItems(totalSlots);
    for (int i = 0; i < totalSlots; i++) {
        SlotData &slot = getSlotData(i+1);
        inputItems[i] = slot.itemId.value();
    }

    SlotData resultSlotData;
    resultSlotData.itemId = 0;
    resultSlotData.itemCount = 0;

    if (FindMatchingRecipe(inputItems, resultSlotData)) {
        SlotData &resultSlot = getSlotData(0);
        resultSlot = resultSlotData;
    } else {
        clearSlot(0);
    }
}

bool CraftingInventory::FindMatchingRecipe(const std::vector<uint16_t> &inputItems, SlotData &resultSlotData) const {
    for (auto &pair : craftingRecipes) {
        const CraftingRecipe &recipe = pair.second;
        if (MatchesRecipe(recipe, inputItems, craftWidth, craftHeight)) {
            resultSlotData.itemId = recipe.result;
            resultSlotData.itemCount = recipe.resultCount;
            return true;
        }
    }
    return false;
}


bool CraftingInventory::MatchesRecipe(const CraftingRecipe &recipe, const std::vector<uint16_t> &inputItems, int craftWidth, int craftHeight) {
    if (!recipe.shapeless) {
        // Shaped recipe
        if (recipe.width > craftWidth || recipe.height > craftHeight) return false;

        // Possible offsets
        // xOffsets = [0 .. craftWidth - recipe.width]
        // yOffsets = [0 .. craftHeight - recipe.height]
        for (int yOff = 0; yOff <= craftHeight - recipe.height; yOff++) {
            for (int xOff = 0; xOff <= craftWidth - recipe.width; xOff++) {
                if (CheckShapedRecipePlacement(recipe, inputItems, craftWidth, craftHeight, xOff, yOff)) {
                    return true;
                }
            }
        }

        return false;
    } else {
        // Shapeless
        std::unordered_map<uint16_t,int> needed;
        for (uint16_t ing : recipe.ingredients) {
            if (ing != 0) {
                needed[ing]++;
            }
        }

        for (auto iid : inputItems) {
            if (iid != 0) {
                if (needed.find(iid) == needed.end()) {
                    return false; // Extra item
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

bool CraftingInventory::CheckShapedRecipePlacement(const CraftingRecipe &recipe, const std::vector<uint16_t> &inputItems, int craftWidth, int craftHeight, int xOff, int yOff) {
    auto getInput = [&](int x, int y) -> uint16_t {
        int index = x + y*craftWidth;
        return inputItems[index];
    };

    // Check every cell of the recipe
    for (int ry = 0; ry < recipe.height; ry++) {
        for (int rx = 0; rx < recipe.width; rx++) {
            uint16_t required = recipe.ingredients[rx + ry * recipe.width];
            uint16_t actual = getInput(rx + xOff, ry + yOff);
            if (actual != required) {
                return false;
            }
        }
    }

    // Check other slots not in recipe are empty
    for (int y = 0; y < craftHeight; y++) {
        for (int x = 0; x < craftWidth; x++) {
            bool inRecipeArea = (x >= xOff && x < xOff + recipe.width &&
                                 y >= yOff && y < yOff + recipe.height);
            if (!inRecipeArea) {
                if (getInput(x, y) != 0) {
                    return false;
                }
            }
        }
    }

    return true;
}

void CraftingInventory::ConsumeCraftingIngredients() {
    int totalSlots = craftWidth * craftHeight;
    std::vector<uint16_t> inputItems(totalSlots);
    for (int i = 0; i < totalSlots; i++) {
        SlotData &slot = getSlotData(i+1);
        inputItems[i] = slot.itemId.value();
    }

    CraftingRecipe matchedRecipe;
    bool found = false;
    for (auto &pair : craftingRecipes) {
        const CraftingRecipe &recipe = pair.second;
        if (MatchesRecipe(recipe, inputItems, craftWidth, craftHeight)) {
            lastCraftedRecipeId = pair.first;
            matchedRecipe = recipe;
            found = true;
            break;
        }
    }

    if (!found) return;

    if (!matchedRecipe.shapeless) {
        int xOff, yOff;
        if (!FindShapedRecipePlacement(matchedRecipe, inputItems, craftWidth, craftHeight, xOff, yOff)) {
            return; // Should not happen if MatchesRecipe was true
        }

        // Consume ingredients
        for (int y = 0; y < matchedRecipe.height; y++) {
            for (int x = 0; x < matchedRecipe.width; x++) {
                uint16_t ing = matchedRecipe.ingredients[x + y*matchedRecipe.width];
                if (ing != 0) {
                    int inputIndex = (x + xOff) + (y + yOff)*craftWidth;
                    int slotIndex = inputIndex + 1;
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

        for (int i = 1; i <= totalSlots; i++) {
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


bool CraftingInventory::FindShapedRecipePlacement(const CraftingRecipe &recipe, const std::vector<uint16_t> &inputItems, int craftWidth, int craftHeight, int &outXOff, int &outYOff) {
    if (recipe.width > craftWidth || recipe.height > craftHeight) return false;

    for (int yOff = 0; yOff <= craftHeight - recipe.height; yOff++) {
        for (int xOff = 0; xOff <= craftWidth - recipe.width; xOff++) {
            if (CheckShapedRecipePlacement(recipe, inputItems, craftWidth, craftHeight, xOff, yOff)) {
                outXOff = xOff;
                outYOff = yOff;
                return true;
            }
        }
    }
    return false;
}


void CraftingInventory::OnCraftingSlotChanged() {
    UpdateCraftingResult();
}

int32_t CraftingInventory::CraftableRecipe() {
    int totalSlots = craftWidth * craftHeight;
    std::vector<uint16_t> inputItems(totalSlots);
    for (int i = 0; i < totalSlots; i++) {
        SlotData &slot = getSlotData(i+1);
        inputItems[i] = slot.itemId.value();
    }

    for (auto &pair : craftingRecipes) {
        const CraftingRecipe &recipe = pair.second;
        if (MatchesRecipe(recipe, inputItems, craftWidth, craftHeight)) {
            return pair.first;
        }
    }
    return -1;
}
