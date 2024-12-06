#ifndef CRAFTING_RECIPES_H
#define CRAFTING_RECIPES_H
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class CraftingRecipe {
public:
    uint16_t result;
    uint8_t resultCount;
    std::vector<uint16_t> ingredients;
    bool shapeless;
    uint8_t width;
    uint8_t height;

    std::vector<uint8_t> serialize(uint16_t id) const;
};

std::unordered_multimap<uint16_t, CraftingRecipe> loadCraftingRecipes(const std::string& path);

#endif //CRAFTING_RECIPES_H
