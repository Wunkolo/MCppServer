#ifndef BIOME_H
#define BIOME_H
#include <optional>
#include <string>
#include <tag_compound.h>
#include <vector>

struct BiomeEffects {
    int fog_color;
    int water_color;
    int water_fog_color;
    int sky_color;
    std::optional<int> foliage_color;
    std::optional<int> grass_color;
    std::optional<int> grass_color_modifier;
    std::optional<nbt::tag_compound> particle;
    std::optional<std::string> ambient_sound;
    std::optional<nbt::tag_compound> mood_sound;
    std::optional<nbt::tag_compound> additions_sound;
    std::optional<nbt::tag_compound> music;

    // Serialize BiomeEffects to NBT CompoundTag
    nbt::tag_compound serialize() const;

    // Deserialize NBT CompoundTag to BiomeEffects
    static BiomeEffects deserialize(const nbt::tag_compound& compound);
};

struct Biome {
    std::string identifier;
    bool has_precipitation;
    float temperature;
    std::optional<std::string> temperature_modifier;
    float downfall;
    BiomeEffects effects;

    // Serialize Biome to NBT CompoundTag
    nbt::tag_compound serialize() const;

    // Deserialize NBT CompoundTag to Biome
    static Biome deserialize(const nbt::tag_compound& compound);
};

struct BiomeTag {
    std::string identifier;
    std::vector<std::string> biomes;

    nbt::tag_compound serialize() const;
    static BiomeTag deserialize(const nbt::tag_compound& compound);
};

// Union-like structure to hold either a Biome or a BiomeTag
struct BiomeRegistryEntry {
    enum class Type {
        Biome,
        Tag
    } type;

    std::optional<Biome> biome;
    std::optional<BiomeTag> tag;

    // Serialize the entry based on its type
    nbt::tag_compound serialize() const;

    // Deserialize the entry based on the presence of fields
    static BiomeRegistryEntry deserialize(const nbt::tag_compound& compound, const std::string& key);
};

bool loadBiome(const std::string& filename, Biome& biome);
bool loadBiomesFromCompoundFile(const std::string& filename, std::vector<BiomeRegistryEntry>& entries);

#endif //BIOME_H
