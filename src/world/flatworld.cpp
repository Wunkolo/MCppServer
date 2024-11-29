#include "flatworld.h"

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "core/utils.h"

std::unordered_map<std::string, FlatWorldSettings> loadFlatWorldPresets(const std::string& filepath) {
    std::unordered_map<std::string, FlatWorldSettings> presets;

    std::ifstream file(filepath);
    if (!file.is_open()) {
        logMessage("Failed to open flatworld_presets.json", LOG_ERROR);
        return presets;
    }

    nlohmann::json jsonData;
    file >> jsonData;

    for (auto& [presetName, presetData] : jsonData.items()) {
        FlatWorldSettings settings;
        settings.biome = presetData["settings"]["biome"].get<std::string>();
        settings.features = presetData["settings"]["features"].get<bool>();
        settings.lakes = presetData["settings"]["lakes"].get<bool>();

        // Parse layers
        for (auto& layer : presetData["settings"]["layers"]) {
            Layer l;
            l.block = layer["block"].get<std::string>();
            l.height = layer["height"].get<int>();
            settings.layers.push_back(l);
        }

        // Parse structure_overrides
        if (presetData["settings"]["structure_overrides"].is_array()) {
            for (auto& structOverride : presetData["settings"]["structure_overrides"]) {
                settings.structure_overrides.push_back(structOverride.get<std::string>());
            }
        } else if (presetData["settings"]["structure_overrides"].is_string()) {
            settings.structure_overrides.push_back(presetData["settings"]["structure_overrides"].get<std::string>());
        }

        presets[presetName] = settings;
    }

    return presets;
}
