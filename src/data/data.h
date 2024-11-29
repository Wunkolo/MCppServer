#ifndef DATA_H
#define DATA_H
#include <string>
#include <unordered_map>

#include "world/block_states.h"


struct ItemData {
    int id;
    std::string name;
    std::string displayName;
    int stackSize;
};

struct BlockData {
    int id;
    std::string displayName;
    float hardness;
    float resistance;
    int stackSize;
    bool diggable;
    std::string material;
    bool transparent;
    int emitLight;
    int filterLight;
    int defaultState;
    int minStateId;
    int maxStateId;
    std::vector<BlockState> states;
    // TODO: Drops
    std::string boundingBox;
};

struct BiomeData {
    int id;
    std::string category;
    float temperature;
    bool hasPrecipitation;
    std::string dimension;
    std::string displayName;
    int color;
};

std::unordered_map<std::string, BiomeData> loadBiomes(const std::string& filePath);
std::unordered_map<std::string, BlockData> loadBlocks(const std::string& filePath);
std::unordered_map<std::string, ItemData> loadItems(const std::string& filePath);
std::unordered_map<int, ItemData> loadItemIDs(const std::string& filePath);

#endif //DATA_H
