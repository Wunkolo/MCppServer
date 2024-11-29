#ifndef FLATWORLD_H
#define FLATWORLD_H
#include <string>
#include <unordered_map>
#include <vector>

struct Layer {
    std::string block;
    int height;
};

struct FlatWorldSettings {
    std::string biome;
    bool features;
    bool lakes;
    std::vector<Layer> layers;
    std::vector<std::string> structure_overrides;
};

std::unordered_map<std::string, FlatWorldSettings> loadFlatWorldPresets(const std::string& filepath);

#endif //FLATWORLD_H
