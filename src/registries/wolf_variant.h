#ifndef WOLF_VARIANT_H
#define WOLF_VARIANT_H
#include <string>
#include <tag_compound.h>
#include <variant>
#include <vector>

struct WolfVariant {
    std::string identifier;
    std::string wild_texture;
    std::string tame_texture;
    std::string angry_texture;
    std::variant<std::string, std::vector<std::string>> biomes;

    // Serialize Wolf Variant to NBT CompoundTag
    nbt::tag_compound serialize() const;

    // Deserialize NBT CompoundTag to Wolf Variant
    static WolfVariant deserialize(const nbt::tag_compound& compound);
};

bool loadWolfVariantsFromCompoundFile(const std::string& filename, std::vector<WolfVariant>& variants);

#endif //WOLF_VARIANT_H
