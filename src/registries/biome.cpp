#include "biome.h"

#include <fstream>
#include <iostream>
#include <tag_list.h>
#include <tag_primitive.h>
#include <tag_string.h>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "core/utils.h"

nbt::tag_compound BiomeEffects::serialize() const {
    nbt::tag_compound compound;

    compound["fog_color"] = nbt::tag_int(fog_color);
    compound["water_color"] = nbt::tag_int(water_color);
    compound["water_fog_color"] = nbt::tag_int(water_fog_color);
    compound["sky_color"] = nbt::tag_int(sky_color);
    if(foliage_color.has_value()) {
        compound["foliage_color"] = nbt::tag_int(foliage_color.value());
    }
    if(grass_color.has_value()) {
        compound["grass_color"] = nbt::tag_int(grass_color.value());
    }
    if(grass_color_modifier.has_value()) {
        compound["grass_color_modifier"] = nbt::tag_int(grass_color_modifier.value());
    }
    if(particle.has_value()) {
        compound["particle"] = nbt::tag_compound(particle.value());
    }
    if(ambient_sound.has_value()) {
        compound["ambient_sound"] = nbt::tag_string(ambient_sound.value());
    }
    if(mood_sound.has_value()) {
        compound["mood_sound"] = nbt::tag_compound(mood_sound.value());
    }
    if(additions_sound.has_value()) {
        compound["additions_sound"] = nbt::tag_compound(additions_sound.value());
    }
    if(music.has_value()) {
        compound["music"] = nbt::tag_compound(music.value());
    }

    return compound;
}

BiomeEffects BiomeEffects::deserialize(const nbt::tag_compound& compound) {
    BiomeEffects effects;

    effects.fog_color = compound.at("fog_color").as<nbt::tag_int>().get();
    effects.water_color = compound.at("water_color").as<nbt::tag_int>().get();
    effects.water_fog_color = compound.at("water_fog_color").as<nbt::tag_int>().get();
    effects.sky_color = compound.at("sky_color").as<nbt::tag_int>().get();
    if (compound.has_key("foliage_color")) {
        effects.foliage_color = compound.at("foliage_color").as<nbt::tag_int>().get();
    }
    if (compound.has_key("grass_color")) {
        effects.grass_color = compound.at("grass_color").as<nbt::tag_int>().get();
    }
    if (compound.has_key("grass_color_modifier")) {
        effects.grass_color_modifier = compound.at("grass_color_modifier").as<nbt::tag_int>().get();
    }
    if (compound.has_key("particle")) {
        effects.particle = compound.at("particle").as<nbt::tag_compound>();
    }
    if (compound.has_key("ambient_sound")) {
        effects.ambient_sound = compound.at("ambient_sound").as<nbt::tag_string>().get();
    }
    if (compound.has_key("mood_sound")) {
        effects.mood_sound = compound.at("mood_sound").as<nbt::tag_compound>();
    }
    if (compound.has_key("additions_sound")) {
        effects.additions_sound = compound.at("additions_sound").as<nbt::tag_compound>();
    }
    if (compound.has_key("music")) {
        effects.music = compound.at("music").as<nbt::tag_compound>();
    }

    return effects;
}

nbt::tag_compound Biome::serialize() const {
    nbt::tag_compound compound;

    compound["has_precipitation"] = nbt::tag_byte(has_precipitation ? 1 : 0);
    compound["temperature"] = nbt::tag_float(temperature);

    if (temperature_modifier.has_value()) {
        compound["temperature_modifier"] = nbt::tag_string(temperature_modifier.value());
    }

    compound["downfall"] = nbt::tag_float(downfall);
    compound["effects"] = effects.serialize();

    return compound;
}

Biome Biome::deserialize(const nbt::tag_compound& compound) {
    Biome biome;

    biome.has_precipitation = compound.at("has_precipitation").as<nbt::tag_byte>().get() != 0;
    biome.temperature = compound.at("temperature").as<nbt::tag_float>().get();

    if (compound.has_key("temperature_modifier")) {
        biome.temperature_modifier = compound.at("temperature_modifier").as<nbt::tag_string>().get();
    }

    biome.downfall = compound.at("downfall").as<nbt::tag_float>().get();
    biome.effects = BiomeEffects::deserialize(compound.at("effects").as<nbt::tag_compound>());

    return biome;
}

nbt::tag_compound BiomeTag::serialize() const {
    nbt::tag_compound compound;

    nbt::tag_list biomeListTag(nbt::tag_type::String);
    for (const auto& biome : biomes) {
        biomeListTag.push_back(nbt::tag_string(biome));
    }
    compound[identifier] = std::move(biomeListTag);

    return compound;
}

BiomeTag BiomeTag::deserialize(const nbt::tag_compound &compound) {
    BiomeTag tag;

    try {
        tag.identifier = compound.at("name").as<nbt::tag_string>().get();

        const auto& biomeListTag = compound.at("biomes").as<nbt::tag_list>();
        for (const auto& biomeTag : biomeListTag) {
            if (biomeTag.get_type() == nbt::tag_type::String) {
                tag.biomes.push_back(biomeTag.as<nbt::tag_string>().get());
            }
        }
    }
    catch (const std::exception& e) {
        logMessage("Error deserializing BiomeTag: " + std::string(e.what()), LOG_ERROR);
    }

    return tag;
}

nbt::tag_compound BiomeRegistryEntry::serialize() const {
    if (type == Type::Biome && biome.has_value()) {
        return biome.value().serialize();
    }
    if (type == Type::Tag && tag.has_value()) {
        return tag.value().serialize();
    }

    // Return an empty compound
    return nbt::tag_compound();
}

BiomeRegistryEntry BiomeRegistryEntry::deserialize(const nbt::tag_compound& compound, const std::string& key) {
    BiomeRegistryEntry entry;

    // Simple heuristic: if 'biomes' field exists, it's a tag; else, it's a biome
    if (compound.has_key("biomes")) {
        entry.type = Type::Tag;
        BiomeTag tag = BiomeTag::deserialize(compound);
        tag.identifier = key;
        entry.tag = tag;
    }
    else {
        entry.type = Type::Biome;
        Biome biome = Biome::deserialize(compound);
        biome.identifier = key;
        entry.biome = biome;
    }

    return entry;
}

bool loadBiome( Biome &biome, nlohmann::basic_json<>& j) {
    // Parse Biome fields
    if (j.contains("has_precipitation") && j["has_precipitation"].is_boolean()) {
        biome.has_precipitation = j["has_precipitation"].get<bool>();
    } else {
        logMessage("Missing or invalid 'has_precipitation' field.", LOG_ERROR);
        return false;
    }

    if (j.contains("temperature") && j["temperature"].is_number_float()) {
        biome.temperature = j["temperature"].get<float>();
    } else {
        logMessage("Missing or invalid 'temperature' field.", LOG_ERROR);
        return false;
    }

    if (j.contains("temperature_modifier") && j["temperature_modifier"].is_string()) {
        biome.temperature_modifier = j["temperature_modifier"].get<std::string>();
    } else {
        // Optional field; not an error if missing
        biome.temperature_modifier = std::nullopt;
    }

    if (j.contains("downfall") && j["downfall"].is_number_float()) {
        biome.downfall = j["downfall"].get<float>();
    } else {
        logMessage("Missing or invalid 'downfall' field.", LOG_ERROR);
        return false;
    }

    // Parse BiomeEffects
    if (j.contains("effects") && j["effects"].is_object()) {
        const nlohmann::json& effectsJson = j["effects"];

        BiomeEffects effects;

        // Mandatory fields
        if (effectsJson.contains("fog_color") && effectsJson["fog_color"].is_number_integer()) {
            effects.fog_color = effectsJson["fog_color"].get<int>();
        } else {
            logMessage("Missing or invalid 'effects.fog_color' field.", LOG_ERROR);
            return false;
        }

        if (effectsJson.contains("water_color") && effectsJson["water_color"].is_number_integer()) {
            effects.water_color = effectsJson["water_color"].get<int>();
        } else {
            logMessage("Missing or invalid 'effects.water_color' field.", LOG_ERROR);
            return false;
        }

        if (effectsJson.contains("water_fog_color") && effectsJson["water_fog_color"].is_number_integer()) {
            effects.water_fog_color = effectsJson["water_fog_color"].get<int>();
        } else {
            logMessage("Missing or invalid 'effects.water_fog_color' field.", LOG_ERROR);
            return false;
        }

        if (effectsJson.contains("sky_color") && effectsJson["sky_color"].is_number_integer()) {
            effects.sky_color = effectsJson["sky_color"].get<int>();
        } else {
            logMessage("Missing or invalid 'effects.sky_color' field.", LOG_ERROR);
            return false;
        }

        // Optional fields
        if (effectsJson.contains("foliage_color") && effectsJson["foliage_color"].is_number_integer()) {
            effects.foliage_color = effectsJson["foliage_color"].get<int>();
        }

        if (effectsJson.contains("grass_color") && effectsJson["grass_color"].is_number_integer()) {
            effects.grass_color = effectsJson["grass_color"].get<int>();
        }

        if (effectsJson.contains("grass_color_modifier") && effectsJson["grass_color_modifier"].is_number_integer()) {
            effects.grass_color_modifier = effectsJson["grass_color_modifier"].get<int>();
        }

        if (effectsJson.contains("particle") && effectsJson["particle"].is_object()) {
            nbt::tag_compound particleTag = jsonToTagCompound(effectsJson["particle"]);
            effects.particle = particleTag;
        }

        if (effectsJson.contains("ambient_sound") && effectsJson["ambient_sound"].is_string()) {
            effects.ambient_sound = effectsJson["ambient_sound"].get<std::string>();
        }

        if (effectsJson.contains("mood_sound") && effectsJson["mood_sound"].is_object()) {
            nbt::tag_compound moodSoundTag = jsonToTagCompound(effectsJson["mood_sound"]);
            effects.mood_sound = moodSoundTag;
        }

        if (effectsJson.contains("additions_sound") && effectsJson["additions_sound"].is_object()) {
            nbt::tag_compound additionsSoundTag = jsonToTagCompound(effectsJson["additions_sound"]);
            effects.additions_sound = additionsSoundTag;
        }

        if (effectsJson.contains("music") && effectsJson["music"].is_object()) {
            nbt::tag_compound musicTag = jsonToTagCompound(effectsJson["music"]);
            effects.music = musicTag;
        }

        biome.effects = effects;
    } else {
        logMessage("Missing or invalid 'effects' field.", LOG_ERROR);
        return false;
    }

    // Successfully loaded Biome
    return true;
}

bool loadBiomesFromCompoundFile(const std::string &filename, std::vector<BiomeRegistryEntry> &entries) {
    std::ifstream file(filename);
    if (!file) {
        logMessage("Failed to open compound file: " + filename, LOG_ERROR);
        return false;
    }
    try {
        // Parse the JSON file
        nlohmann::json j;
        file >> j;

        if (j.contains("minecraft:worldgen/biome") && j["minecraft:worldgen/biome"].is_object()) {
            for (auto& [key, value] : j["minecraft:worldgen/biome"].items()) {
                // Determine if the entry is a biome or a tag
                bool isTag = false;
                if (value.contains("values")) {
                    isTag = true;
                }
                if (isTag) {
                    // It's a biome tag
                    BiomeTag tag;
                    tag.identifier = key;

                    if (value["values"].is_string()) {
                        tag.biomes.push_back(value["values"].get<std::string>());
                    }
                    else if (value["values"].is_array()) {
                        for (const auto& biome : value["values"]) {
                            if (biome.is_string()) {
                                tag.biomes.push_back(biome.get<std::string>());
                            }
                            else {
                                logMessage("Invalid biome entry type in 'biomes' list for tag: " + key, LOG_ERROR);
                            }
                        }
                    }
                    else {
                        logMessage("Invalid 'biomes' field type for tag: " + key, LOG_ERROR);
                        continue;
                    }

                    BiomeRegistryEntry entry;
                    entry.type = BiomeRegistryEntry::Type::Tag;
                    entry.tag = tag;
                    entries.push_back(entry);
                } else {
                    // It's a biome
                    Biome biome;
                    if (loadBiome(biome, value)) {
                        biome.identifier = key;
                        BiomeRegistryEntry entry;
                        entry.type = BiomeRegistryEntry::Type::Biome;
                        entry.biome = biome;
                        entries.push_back(entry);
                    } else {
                        logMessage("Failed to load biomes from compound file: " + filename, LOG_ERROR);
                        return false;
                    }
                }
            }
        }
        return true;
    } catch (const nlohmann::json::parse_error& e) {
        logMessage("JSON parse error in file " + filename + ": " + e.what(), LOG_ERROR);
        return false;
    } catch (const nlohmann::json::type_error& e) {
        logMessage("JSON type error in file " + filename + ": " + e.what(), LOG_ERROR);
        return false;
    } catch (const std::exception& e) {
        logMessage("Error loading biome from file " + filename + ": " + e.what(), LOG_ERROR);
        return false;
    }
}

