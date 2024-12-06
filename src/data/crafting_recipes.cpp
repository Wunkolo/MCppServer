#include "crafting_recipes.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "core/utils.h"
#include "entities/slot_data.h"
#include "enums/enums.h"
#include "networking/network.h"

std::vector<uint8_t> CraftingRecipe::serialize(uint16_t id) const {
    std::vector<uint8_t> data;
    writeString(data, std::to_string(id));
    writeVarInt(data, shapeless ? 1 : 0);
    writeString(data, "Craft"); // TODO: Write group, need to write own extractor for this
    writeVarInt(data, 0); // Building = 0, Redstone = 1, Equipment = 2, Misc = 3 TODO: Write type, also extractor needed

    if (!shapeless) {
        // Shaped
        writeVarInt(data, width);
        writeVarInt(data, height);

        // Ingredients: width*height
        if (ingredients.size() != width * height) {
            logMessage("Invalid ingredient count for crafting recipe", LOG_WARNING);
            return {};
        }
        for (auto ing : ingredients) {
            writeVarInt(data, 1); // Count
            SlotData slotData;
            slotData.itemId = ing;
            slotData.itemCount = 1;
            writeSlotSimple(data, slotData);
        }

        // Result (Slot)
        SlotData slotData;
        slotData.itemId = result;
        slotData.itemCount = resultCount;
        writeSlotSimple(data, slotData);

        // Show notification
        writeByte(data, false);// TODO: Write showNotification, need to write own extractor for this
    } else {
        // Shapeless
        writeVarInt(data, static_cast<int>(ingredients.size()));
        for (auto ing : ingredients) {
            writeVarInt(data, 1); // Count
            SlotData slotData;
            slotData.itemId = ing;
            slotData.itemCount = 1;
            writeSlotSimple(data, slotData);
        }

        // Result (Slot)
        SlotData slotData;
        slotData.itemId = result;
        slotData.itemCount = resultCount;
        writeSlotSimple(data, slotData);
    }

    return data;
}

std::unordered_multimap<uint16_t, CraftingRecipe> loadCraftingRecipes(const std::string &filename) {
    std::unordered_multimap<uint16_t, CraftingRecipe> craftingRecipes;
    std::ifstream file(filename);
    if (!file) {
        logMessage("Failed to open crafting recipe file: " + filename, LOG_ERROR);
        return craftingRecipes;
    }
    try {
        // Parse the JSON file
        nlohmann::json j;
        file >> j;

        for (const auto& [key, recipeArray] : j.items()) {
            auto numericKey = static_cast<uint16_t>(std::stoi(key));
            for (const auto& recipeVariant : recipeArray) {
                CraftingRecipe craftingRecipe;
                craftingRecipe.result = recipeVariant["result"]["id"].get<uint16_t>();
                craftingRecipe.resultCount = recipeVariant["result"]["count"].get<uint8_t>();

                // Check if shapeless: `ingredients` array present without `inShape`
                if (recipeVariant.contains("ingredients")) {
                    // Shapeless recipe
                    craftingRecipe.shapeless = true;
                    craftingRecipe.height = 0;
                    craftingRecipe.width = 0;

                    craftingRecipe.ingredients.clear();
                    for (const auto &ingredient : recipeVariant["ingredients"]) {
                        // Push each ingredient in order
                        uint16_t ing = ingredient.is_null() ? 0 : ingredient.get<uint16_t>();
                        craftingRecipe.ingredients.push_back(ing);
                    }

                    craftingRecipes.emplace(numericKey, craftingRecipe);
                } else if (recipeVariant.contains("inShape")) {
                    // Shaped recipe
                    craftingRecipe.shapeless = false;
                    const auto &inShape = recipeVariant["inShape"];
                    // Dimensions
                    craftingRecipe.height = static_cast<uint8_t>(inShape.size());
                    if (craftingRecipe.height > 0) {
                        craftingRecipe.width = static_cast<uint8_t>(inShape[0].size());
                    } else {
                        craftingRecipe.width = 0;
                    }

                    // Flatten ingredients into a single array
                    // indexed by x + (y * width)
                    for (int y = 0; y < craftingRecipe.height; ++y) {
                        craftingRecipe.ingredients.clear();
                        for (int x = 0; x < craftingRecipe.width; ++x) {
                            const auto &ingredient = inShape[y][x];
                            uint16_t ing = ingredient.is_null() ? 0 : ingredient.get<uint16_t>();
                            craftingRecipe.ingredients.push_back(ing);
                        }
                    }

                    craftingRecipe.ingredients.clear();
                    for (int y = 0; y < craftingRecipe.height; ++y) {
                        for (int x = 0; x < craftingRecipe.width; ++x) {
                            const auto &ingredient = inShape[y][x];
                            uint16_t ing = ingredient.is_null() ? 0 : ingredient.get<uint16_t>();
                            craftingRecipe.ingredients.push_back(ing);
                        }
                    }

                    craftingRecipes.emplace(numericKey, craftingRecipe);
                } else {
                    // Unknown recipe type
                    logMessage("Unknown recipe format in: " + key, LOG_WARNING);
                }
            }
        }
    } catch (const nlohmann::json::parse_error &e) {
        logMessage("Failed to parse crafting recipe file: " + std::string(e.what()), LOG_ERROR);
    }

    return craftingRecipes;
}
