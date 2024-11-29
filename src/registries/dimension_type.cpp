#include "dimension_type.h"

#include <fstream>
#include <iostream>
#include <tag_primitive.h>
#include <tag_string.h>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "core/utils.h"

nbt::tag_compound DimensionType::serialize() const {
    nbt::tag_compound compound;

    if (fixed_time.has_value()) {
        compound["fixed_time"] = nbt::tag_long(fixed_time.value());
    }
    compound["has_skylight"] = nbt::tag_byte(has_skylight ? 1 : 0);
    compound["has_ceiling"] = nbt::tag_byte(has_ceiling ? 1 : 0);
    compound["ultrawarm"] = nbt::tag_byte(ultrawarm ? 1 : 0);
    compound["natural"] = nbt::tag_byte(natural ? 1 : 0);
    compound["coordinate_scale"] = nbt::tag_double(coordinate_scale);
    compound["bed_works"] = nbt::tag_byte(bed_works ? 1 : 0);
    compound["respawn_anchor_works"] = nbt::tag_byte(respawn_anchor_works ? 1 : 0);
    compound["min_y"] = nbt::tag_int(min_y);
    compound["height"] = nbt::tag_int(height);
    compound["logical_height"] = nbt::tag_int(logical_height);
    compound["infiniburn"] = nbt::tag_string(infiniburn);
    compound["effects"] = nbt::tag_string(effects);
    compound["ambient_light"] = nbt::tag_float(ambient_light);
    compound["piglin_safe"] = nbt::tag_byte(piglin_safe ? 1 : 0);
    compound["has_raids"] = nbt::tag_byte(has_raids ? 1 : 0);
    if (std::holds_alternative<int>(monster_spawn_light_level)) {
        compound["monster_spawn_light_level"] = nbt::tag_int(std::get<int>(monster_spawn_light_level));
    } else {
       compound["monster_spawn_light_level"] = nbt::tag_compound(std::get<nbt::tag_compound>(monster_spawn_light_level));
    }
    compound["monster_spawn_block_light_limit"] = nbt::tag_int(monster_spawn_block_light_limit);

    return compound;
}

DimensionType DimensionType::deserialize(const nbt::tag_compound& compound) {
    DimensionType dimension;

    if (compound.has_key("fixed_time")) {
        dimension.fixed_time = compound.at("fixed_time").as<nbt::tag_long>().get();
    }

    dimension.has_skylight = compound.at("has_skylight").as<nbt::tag_byte>().get() != 0;
    dimension.has_ceiling = compound.at("has_ceiling").as<nbt::tag_byte>().get() != 0;
    dimension.ultrawarm = compound.at("ultrawarm").as<nbt::tag_byte>().get() != 0;
    dimension.natural = compound.at("natural").as<nbt::tag_byte>().get() != 0;
    dimension.coordinate_scale = compound.at("coordinate_scale").as<nbt::tag_double>().get();
    dimension.bed_works = compound.at("bed_works").as<nbt::tag_byte>().get() != 0;
    dimension.respawn_anchor_works = compound.at("respawn_anchor_works").as<nbt::tag_byte>().get() != 0;
    dimension.min_y = compound.at("min_y").as<nbt::tag_int>().get();
    dimension.height = compound.at("height").as<nbt::tag_int>().get();
    dimension.logical_height = compound.at("logical_height").as<nbt::tag_int>().get();
    dimension.infiniburn = compound.at("infiniburn").as<nbt::tag_string>().get();
    dimension.effects = compound.at("effects").as<nbt::tag_string>().get();
    dimension.ambient_light = compound.at("ambient_light").as<nbt::tag_float>().get();
    dimension.piglin_safe = compound.at("piglin_safe").as<nbt::tag_byte>().get() != 0;
    dimension.has_raids = compound.at("has_raids").as<nbt::tag_byte>().get() != 0;
    if (compound.at("monster_spawn_light_level").get_type() == nbt::tag_type::Int) {
        dimension.monster_spawn_light_level = compound.at("monster_spawn_light_level").as<nbt::tag_int>().get();
    } else if (compound.at("monster_spawn_light_level").get_type() == nbt::tag_type::Compound) {
        dimension.monster_spawn_light_level = compound.at("monster_spawn_light_level").as<nbt::tag_compound>();
    } else {
        throw std::runtime_error("Invalid Tag type for MonsterSpawnLightLevel");
    }
    dimension.monster_spawn_block_light_limit = compound.at("monster_spawn_block_light_limit").as<nbt::tag_int>().get();

    return dimension;
}

bool loadDimensionTypesFromCompoundFile(const std::string &filename, std::vector<DimensionType> &dimensions) {
    // Open the JSON file
    std::ifstream file(filename);
    if (!file) {
        logMessage("Failed to open dimension file: " + filename, LOG_ERROR);
        return false;
    }

    try {
        // Parse the JSON file
        nlohmann::json j;
        file >> j;
        // Check if the expected registry exists
        if (j.contains("minecraft:dimension_type") && j["minecraft:dimension_type"].is_object()) {
            // Iterate over each dimension type entry
            for (auto& [key, value] : j["minecraft:dimension_type"].items()) {
                DimensionType dimension;
                dimension.identifier = key;
                if (value.contains("has_skylight") && value["has_skylight"].is_number_integer()) {
                    dimension.has_skylight = value["has_skylight"].get<int>();
                } else {
                    logMessage("Missing or invalid 'has_skylight' field.", LOG_ERROR);
                    return false;
                }

                if (value.contains("has_ceiling") && value["has_ceiling"].is_number_integer()) {
                    dimension.has_ceiling = value["has_ceiling"].get<int>();
                } else {
                    logMessage("Missing or invalid 'has_ceiling' field.", LOG_ERROR);
                    return false;
                }

                if (value.contains("ultrawarm") && value["ultrawarm"].is_number_integer()) {
                    dimension.ultrawarm = value["ultrawarm"].get<int>();
                } else {
                    logMessage("Missing or invalid 'ultrawarm' field.", LOG_ERROR);
                    return false;
                }

                if (value.contains("natural") && value["natural"].is_number_integer()) {
                    dimension.natural = value["natural"].get<int>();
                } else {
                    logMessage("Missing or invalid 'natural' field.", LOG_ERROR);
                    return false;
                }

                if (value.contains("coordinate_scale") && value["coordinate_scale"].is_number_float()) {
                    dimension.coordinate_scale = value["coordinate_scale"].get<double>();
                } else {
                    logMessage("Missing or invalid 'natural' field.", LOG_ERROR);
                    return false;
                }

                if (value.contains("bed_works") && value["bed_works"].is_number_integer()) {
                    dimension.bed_works = value["bed_works"].get<int>();
                } else {
                    logMessage("Missing or invalid 'bed_works' field.", LOG_ERROR);
                    return false;
                }

                if (value.contains("respawn_anchor_works") && value["respawn_anchor_works"].is_number_integer()) {
                    dimension.respawn_anchor_works = value["respawn_anchor_works"].get<int>();
                } else {
                    logMessage("Missing or invalid 'respawn_anchor_works' field.", LOG_ERROR);
                    return false;
                }

                if (value.contains("min_y") && value["min_y"].is_number_integer()) {
                    dimension.min_y = value["min_y"].get<int>();
                } else {
                    logMessage("Missing or invalid 'min_y' field.", LOG_ERROR);
                    return false;
                }

                if (value.contains("height") && value["height"].is_number_integer()) {
                    dimension.height = value["height"].get<int>();
                } else {
                    logMessage("Missing or invalid 'height' field.", LOG_ERROR);
                    return false;
                }

                if (value.contains("logical_height") && value["logical_height"].is_number_integer()) {
                    dimension.logical_height = value["logical_height"].get<int>();
                } else {
                    logMessage("Missing or invalid 'logical_height' field.", LOG_ERROR);
                    return false;
                }

                if (value.contains("infiniburn") && value["infiniburn"].is_string()) {
                    dimension.infiniburn = value["infiniburn"].get<std::string>();
                } else {
                    logMessage("Missing or invalid 'infiniburn' field.", LOG_ERROR);
                    return false;
                }

                if (value.contains("effects") && value["effects"].is_string()) {
                    dimension.effects = value["effects"].get<std::string>();
                } else {
                    logMessage("Missing or invalid 'effects' field.", LOG_ERROR);
                    return false;
                }

                if (value.contains("ambient_light") && value["ambient_light"].is_number_float()) {
                    dimension.ambient_light = value["ambient_light"].get<float>();
                } else {
                    logMessage("Missing or invalid 'ambient_light' field.", LOG_ERROR);
                    return false;
                }

                if (value.contains("piglin_safe") && value["piglin_safe"].is_number_integer()) {
                    dimension.piglin_safe = value["piglin_safe"].get<int>();
                } else {
                    logMessage("Missing or invalid 'piglin_safe' field.", LOG_ERROR);
                    return false;
                }

                if (value.contains("has_raids") && value["has_raids"].is_number_integer()) {
                    dimension.has_raids = value["has_raids"].get<int>();
                } else {
                    logMessage("Missing or invalid 'has_raids' field.", LOG_ERROR);
                    return false;
                }

                if (value.contains("monster_spawn_light_level") && value["monster_spawn_light_level"].is_object()) {
                    if(value["monster_spawn_light_level"].contains("type") && value["monster_spawn_light_level"]["type"].is_string()) {
                        if(value["monster_spawn_light_level"]["type"].get<std::string>() == "minecraft:uniform") {
                            nbt::tag_compound monster_spawn_light_level;
                            monster_spawn_light_level["type"] = nbt::tag_string("minecraft:uniform");
                            monster_spawn_light_level["min_inclusive"] = nbt::tag_int(value["monster_spawn_light_level"]["min_inclusive"].get<int>());
                            monster_spawn_light_level["max_inclusive"] = nbt::tag_int(value["monster_spawn_light_level"]["max_inclusive"].get<int>());
                            dimension.monster_spawn_light_level = monster_spawn_light_level;
                        }
                    } else {
                        logMessage("Missing or invalid 'monster_spawn_light_level type' field.", LOG_ERROR);
                        return false;
                    }
                } else if (value.contains("monster_spawn_light_level") && value["monster_spawn_light_level"].is_number_integer()) {
                    dimension.monster_spawn_light_level = value["monster_spawn_light_level"].get<int>();
                } else {
                    logMessage("Missing or invalid 'monster_spawn_light_level' field.", LOG_ERROR);
                    return false;
                }

                if (value.contains("monster_spawn_block_light_limit") && value["monster_spawn_block_light_limit"].is_number_integer()) {
                    dimension.monster_spawn_block_light_limit = value["monster_spawn_block_light_limit"].get<int>();
                } else {
                    logMessage("Missing or invalid 'monster_spawn_block_light_limit' field.", LOG_ERROR);
                    return false;
                }
                dimensions.push_back(dimension);
            }
            return true;
        }

        return false;
    } catch (const nlohmann::json::parse_error& e) {
        logMessage("JSON parse error in file " + filename + ": " + e.what(), LOG_ERROR);
        return false;
    } catch (const nlohmann::json::type_error& e) {
        logMessage("JSON type error in file " + filename + ": " + e.what(), LOG_ERROR);
        return false;
    } catch (const std::exception& e) {
        logMessage("Error loading dimension type from file " + filename + ": " + e.what(), LOG_ERROR);
        return false;
    }
}