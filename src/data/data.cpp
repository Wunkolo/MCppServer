#include "data.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "core/utils.h"

std::unordered_map<std::string, BiomeData> loadBiomes(const std::string& filePath) {
    std::unordered_map<std::string, BiomeData> biomeMap;
    std::ifstream file(filePath);
    if (!file.is_open()) {
        logMessage("Failed to open biomes.json file at " + filePath, LOG_ERROR);
        return biomeMap;
    }

    nlohmann::json biomesJson;
    try {
        file >> biomesJson;
    } catch (const nlohmann::json::parse_error& e) {
        logMessage("Failed to parse biomes.json: " + std::string(e.what()), LOG_ERROR);
        return biomeMap;
    }

    for (const auto& biome : biomesJson) {
        if (biome.contains("name") && biome.contains("id")) {
            BiomeData biomeData;
            std::string name = biome.value("name", "unknown");
            biomeData.id = biome.value("id", 0);
            biomeData.category = biome.value("category", "none");
            biomeData.temperature = biome.value("temperature", 0.5f);
            biomeData.hasPrecipitation = biome.value("precipitation", false);
            biomeData.dimension = biome.value("dimension", "overworld");
            biomeData.displayName = biome.value("display_name", name);
            biomeData.color = biome.value("color", 0);

            biomeMap[name] = biomeData;
        } else {
            logMessage("Biome entry missing 'name' or 'id'", LOG_ERROR);
        }
    }

    return biomeMap;
}

std::unordered_map<std::string, BlockData> loadBlocks(const std::string& filePath) {
    std::unordered_map<std::string, BlockData> blockMap;
    std::ifstream file(filePath);
    if (!file.is_open()) {
        logMessage("Failed to open blocks.json file at " + filePath, LOG_ERROR);
        return blockMap;
    }

    nlohmann::json blocksJson;
    try {
        file >> blocksJson;
    } catch (const nlohmann::json::parse_error& e) {
        logMessage("Failed to parse blocks.json: " + std::string(e.what()), LOG_ERROR);
        return blockMap;
    }

    for (const auto& block : blocksJson) {
        if (block.contains("name") && block.contains("defaultState")) {
            BlockData blockData;
            std::string name = block["name"];
            blockData.id = block.value("id", 0);
            blockData.displayName = block.value("displayName", name);
            blockData.hardness = block.value("hardness", 0.0f);
            blockData.resistance = block.value("resistance", 0.0f);
            blockData.stackSize = block.value("stackSize", 64);
            blockData.diggable = block.value("diggable", true);
            blockData.material = block.value("material", "rock");
            blockData.transparent = block.value("transparent", false);
            blockData.emitLight = block.value("emitLight", 0);
            blockData.filterLight = block.value("filterLight", 0);
            blockData.defaultState = block.value("defaultState", 0);
            blockData.minStateId = block.value("minStateId", 0);
            blockData.maxStateId = block.value("maxStateId", 0);
            blockData.boundingBox = block.value("boundingBox", "block");
            blockData.states = getBlockStates(block["states"]);

            blockMap[name] = blockData;
        } else {
            logMessage("Block entry missing 'name' or 'defaultState'", LOG_ERROR);
        }
    }

    return blockMap;
}

std::unordered_map<std::string, ItemData> loadItems(const std::string& filePath) {
    std::unordered_map<std::string, ItemData> itemMap;
    std::ifstream file(filePath);
    if (!file.is_open()) {
        logMessage("Failed to open items.json file at " + filePath, LOG_ERROR);
        return itemMap;
    }

    nlohmann::json itemsJson;
    try {
        file >> itemsJson;
    } catch (const nlohmann::json::parse_error& e) {
        logMessage("Failed to parse items.json: " + std::string(e.what()), LOG_ERROR);
        return itemMap;
    }

    for (const auto& item : itemsJson) {
        if (item.contains("id") && item.contains("name") && item.contains("displayName") && item.contains("stackSize")) {
            ItemData itemData;
            std::string name = item["name"];
            itemData.name = name;
            itemData.id = item.value("id", 0);
            itemData.displayName = item.value("displayName", name);
            itemData.stackSize = item.value("stackSize", 64);

            itemMap[name] = itemData;
        } else {
            logMessage("Item entry missing 'id', 'name', 'displayName' or 'stackSize'", LOG_ERROR);
        }
    }

    return itemMap;
}

std::unordered_map<int, ItemData> loadItemIDs(const std::string& filePath) {
    std::unordered_map<int, ItemData> itemMap;
    std::ifstream file(filePath);
    if (!file.is_open()) {
        logMessage("Failed to open items.json file at " + filePath, LOG_ERROR);
        return itemMap;
    }

    nlohmann::json itemsJson;
    try {
        file >> itemsJson;
    } catch (const nlohmann::json::parse_error& e) {
        logMessage("Failed to parse items.json: " + std::string(e.what()), LOG_ERROR);
        return itemMap;
    }

    for (const auto& item : itemsJson) {
        if (item.contains("id") && item.contains("name") && item.contains("displayName") && item.contains("stackSize")) {
            ItemData itemData;
            std::string name = item["name"];
            itemData.name = name;
            itemData.id = item.value("id", 0);
            itemData.displayName = item.value("displayName", name);
            itemData.stackSize = item.value("stackSize", 64);

            itemMap[itemData.id] = itemData;
        } else {
            logMessage("Item entry missing 'id', 'name', 'displayName' or 'stackSize'", LOG_ERROR);
        }
    }

    return itemMap;
}
