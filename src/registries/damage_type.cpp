#include "damage_type.h"

#include <fstream>
#include <iostream>
#include <tag_primitive.h>
#include <tag_string.h>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "core/utils.h"

nbt::tag_compound DamageType::serialize() const {
    nbt::tag_compound compound;
    compound["message_id"] = nbt::tag_string(message_id);
    compound["scaling"] = nbt::tag_string(scaling);
    compound["exhaustion"] = nbt::tag_float(exhaustion);
    if (effects.has_value()) {
        compound["effects"] = nbt::tag_string(effects.value());
    }
    if (death_message_type.has_value()) {
        compound["death_message_type"] = nbt::tag_string(death_message_type.value());
    }

    return compound;
}

DamageType DamageType::deserialize(const nbt::tag_compound &compound) {
    DamageType type;
    type.message_id = compound.at("message_id").as<nbt::tag_string>().get();
    type.scaling = compound.at("scaling").as<nbt::tag_string>().get();
    type.exhaustion = compound.at("exhaustion").as<nbt::tag_float>().get();
    if (compound.has_key("effects")) {
        type.effects = compound.at("effects").as<nbt::tag_string>().get();
    }
    if (compound.has_key("death_message_type")) {
        type.death_message_type = compound.at("death_message_type").as<nbt::tag_string>().get();
    }

    return type;
}

bool loadDamageTypesFromCompoundFile(const std::string &filename, std::vector<DamageType> &types) {
    std::ifstream file(filename);
    if (!file) {
        logMessage("Failed to open compound file: " + filename, LOG_ERROR);
        return false;
    }
    try {
        // Parse the JSON file
        nlohmann::json j;
        file >> j;

        if (j.contains("minecraft:damage_type") && j["minecraft:damage_type"].is_object()) {
            // for every element in the object
            for (auto& [key, value] : j["minecraft:damage_type"].items()) {
                DamageType type;
                type.identifier = key;

                if (value.contains("message_id") && value["message_id"].is_string()) {
                    type.message_id = value["message_id"].get<std::string>();
                } else {
                    logMessage("Missing or invalid message_id field", LOG_ERROR);
                    return false;
                }

                if (value.contains("scaling") && value["scaling"].is_string()) {
                    type.scaling = value["scaling"].get<std::string>();
                } else {
                    logMessage("Missing or invalid scaling field", LOG_ERROR);
                    return false;
                }

                if (value.contains("exhaustion") && value["exhaustion"].is_number_float()) {
                    type.exhaustion = value["exhaustion"].get<float>();
                } else {
                    logMessage("Missing or invalid exhaustion field", LOG_ERROR);
                    return false;
                }

                if (value.contains("effects") && value["effects"].is_string()) {
                    type.effects = value["effects"].get<std::string>();
                }

                if (value.contains("death_message_type") && value["death_message_type"].is_string()) {
                    type.death_message_type = value["death_message_type"].get<std::string>();
                }
                types.push_back(type);
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
        logMessage("Error loading painting variants from file " + filename + ": " + e.what(), LOG_ERROR);
        return false;
    }
}
