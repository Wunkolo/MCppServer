#include "painting_variant.h"

#include <fstream>
#include <iostream>
#include <tag_primitive.h>
#include <tag_string.h>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include "core/utils.h"

nbt::tag_compound PaintingVariant::serialize() const {
    nbt::tag_compound compound;
    compound["asset_id"] = nbt::tag_string(asset_id);
    compound["width"] = nbt::tag_int(width);
    compound["height"] = nbt::tag_int(height);

    return compound;
}

PaintingVariant PaintingVariant::deserialize(const nbt::tag_compound &compound) {
    PaintingVariant variant;
    variant.asset_id = compound.at("asset_id").as<nbt::tag_string>().get();
    variant.width = compound.at("width").as<nbt::tag_int>().get();
    variant.height = compound.at("height").as<nbt::tag_int>().get();

    return variant;
}

bool loadPaintingVariant(PaintingVariant &variant, nlohmann::basic_json<>& json) {
    if (json.contains("asset_id") && json["asset_id"].is_string()) {
        variant.asset_id = json["asset_id"].get<std::string>();
    } else {
        logMessage("Missing or invalid asset_id field", LOG_ERROR);
        return false;
    }

    if (json.contains("width") && json["width"].is_number_integer()) {
        variant.width = json["width"].get<int>();
    } else {
        logMessage("Missing or invalid width field", LOG_ERROR);
        return false;
    }

    if (json.contains("height") && json["height"].is_number_integer()) {
        variant.height = json["height"].get<int>();
    } else {
        logMessage("Missing or invalid height field", LOG_ERROR);
        return false;
    }

    return true;
}

bool loadPaintingVariantsFromCompoundFile(const std::string &filename, std::vector<PaintingVariant> &variants) {
    std::ifstream file(filename);
    if (!file) {
        logMessage("Failed to open compound file: " + filename, LOG_ERROR);
        return false;
    }
    try {
        // Parse the JSON file
        nlohmann::json j;
        file >> j;

        if (j.contains("minecraft:painting_variant") && j["minecraft:painting_variant"].is_object()) {
            // for every element in the object
            for (auto& [key, value] : j["minecraft:painting_variant"].items()) {
                PaintingVariant variant;
                if (loadPaintingVariant(variant, value)) {
                    variants.push_back(variant);
                } else {
                    logMessage("Failed to load painting variant from compound file: " + filename, LOG_ERROR);
                    return false;
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
        logMessage("Error loading painting variants from file " + filename + ": " + e.what(), LOG_ERROR);
        return false;
    }
}
