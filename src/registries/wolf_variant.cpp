#include "wolf_variant.h"

#include <fstream>
#include <iostream>
#include <tag_list.h>
#include <tag_string.h>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "core/utils.h"

nbt::tag_compound WolfVariant::serialize() const {
    nbt::tag_compound compound;
    compound["wild_texture"] = nbt::tag_string(wild_texture);
    compound["tame_texture"] = nbt::tag_string(tame_texture);
    compound["angry_texture"] = nbt::tag_string(angry_texture);

    // Handle 'biomes' which can be either a String or a List
    if (std::holds_alternative<std::string>(biomes)) {
        compound["biomes"] = nbt::tag_string(std::get<std::string>(biomes));
    } else {
        const auto& biomesList = std::get<std::vector<std::string>>(biomes);
        nbt::tag_list biomeListTag(nbt::tag_type::String);
        for (const auto& biome : biomesList) {
            biomeListTag.push_back(nbt::tag_string(biome));
        }
        compound["biomes"] = std::move(biomeListTag);
    }

    return compound;
}

WolfVariant WolfVariant::deserialize(const nbt::tag_compound &compound) {
    WolfVariant variant;

    try {
        variant.wild_texture = compound.at("wild_texture").as<nbt::tag_string>().get();
        variant.tame_texture = compound.at("tame_texture").as<nbt::tag_string>().get();
        variant.angry_texture = compound.at("angry_texture").as<nbt::tag_string>().get();

        const auto& biomesTag = compound.at("biomes");
        switch (biomesTag.get_type()) {
            case nbt::tag_type::String:
                variant.biomes = biomesTag.as<nbt::tag_string>().get();
            break;
            case nbt::tag_type::List: {
                const auto& biomeList = biomesTag.as<nbt::tag_list>();
                std::vector<std::string> biomesVec;
                for (const auto& tag : biomeList) {
                    if (tag.get_type() == nbt::tag_type::String) {
                        biomesVec.push_back(tag.as<nbt::tag_string>().get());
                    } else {
                        throw std::runtime_error("Invalid tag type in 'biomes' list.");
                    }
                }
                variant.biomes = std::move(biomesVec);
                break;
            }
            default:
                throw std::runtime_error("Invalid tag type for 'biomes' field.");
        }
    }
    catch (const std::exception& e) {
        logMessage("Error deserializing WolfVariant: " + std::string(e.what()), LOG_ERROR);
        // TODO: set default values or rethrow the exception
    }

    return variant;
}

bool loadWolfVariantsFromCompoundFile(const std::string &filename, std::vector<WolfVariant> &variants) {
    std::ifstream file(filename);
    if (!file) {
        logMessage("Failed to open compound file: " + filename, LOG_ERROR);
        return false;
    }
    try {
        // Parse the JSON file
        nlohmann::json j;
        file >> j;

        // Check if the expected registry exists
        if (j.contains("minecraft:wolf_variant") && j["minecraft:wolf_variant"].is_object()) {
            // Iterate over each wolf variant entry
            for (auto& [key, value] : j["minecraft:wolf_variant"].items()) {
                // Load each field
                if (value.contains("wild_texture") && value["wild_texture"].is_string() &&
                    value.contains("tame_texture") && value["tame_texture"].is_string() &&
                    value.contains("angry_texture") && value["angry_texture"].is_string()) {
                    WolfVariant variant;

                    variant.wild_texture = value["wild_texture"].get<std::string>();
                    variant.tame_texture = value["tame_texture"].get<std::string>();
                    variant.angry_texture = value["angry_texture"].get<std::string>();

                    // Handle 'biomes' field which can be a string or a list
                    if (value.contains("biomes")) {
                        if (value["biomes"].is_string()) {
                            variant.biomes = value["biomes"].get<std::string>();
                        }
                        else if (value["biomes"].is_array()) {
                            std::vector<std::string> biomesVec;
                            for (const auto& biome : value["biomes"]) {
                                if (biome.is_string()) {
                                    biomesVec.push_back(biome.get<std::string>());
                                }
                                else {
                                    logMessage("Invalid biome entry type in 'biomes' list for variant: " + key, LOG_ERROR);
                                    continue; // Skip invalid entries
                                }
                            }
                            variant.biomes = std::move(biomesVec);
                        }
                        else {
                            logMessage("Invalid 'biomes' field type for variant: " + key, LOG_ERROR);
                            continue; // Skip invalid entries
                        }
                    }
                    else {
                        logMessage("Missing 'biomes' field for variant: " + key, LOG_ERROR);
                        continue; // Skip invalid entries
                    }
                    variant.identifier = key;
                    variants.push_back(variant);
                    }
                else {
                    logMessage("Missing or invalid fields in variant: " + key, LOG_ERROR);
                    continue; // Skip invalid entries
                }
            }
        }
        else {
            logMessage("'minecraft:wolf_variant' registry not found or is not an object.", LOG_ERROR);
            return false;
        }
        return true;
    }
    catch (const nlohmann::json::parse_error& e) {
        logMessage("JSON parse error in file " + filename + ": " + e.what(), LOG_ERROR);
        return false;
    }
    catch (const nlohmann::json::type_error& e) {
        logMessage("JSON type error in file " + filename + ": " + e.what(), LOG_ERROR);
        return false;
    }
    catch (const std::exception& e) {
        logMessage("Error loading wolf variants from file " + filename + ": " + e.what(), LOG_ERROR);
        return false;
    }
}
