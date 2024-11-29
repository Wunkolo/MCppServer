#ifndef DIMENSION_TYPE_H
#define DIMENSION_TYPE_H

#include <string>
#include <variant>
#include <optional>
#include <tag_compound.h>
#include <vector>

struct DimensionType {
    std::string identifier;
    std::optional<long> fixed_time;
    bool has_skylight;
    bool has_ceiling;
    bool ultrawarm;
    bool natural;
    double coordinate_scale;
    bool bed_works;
    bool respawn_anchor_works;
    int min_y;
    int height;
    int logical_height;
    std::string infiniburn;
    std::string effects;
    float ambient_light;
    bool piglin_safe;
    bool has_raids;
    std::variant<int, nbt::tag_compound> monster_spawn_light_level; // int or compound
    int monster_spawn_block_light_limit;

    // Serialize DimensionType to NBT CompoundTag
    nbt::tag_compound serialize() const;

    // Deserialize NBT CompoundTag to DimensionType
    static DimensionType deserialize(const nbt::tag_compound& compound);
};

bool loadDimensionTypesFromCompoundFile(const std::string &filename, std::vector<DimensionType> &dimensions);

#endif // DIMENSION_TYPE_H
