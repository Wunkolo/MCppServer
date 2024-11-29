#ifndef DAMAGE_TYPE_H
#define DAMAGE_TYPE_H
#include <optional>
#include <string>
#include <tag_compound.h>
#include <vector>

struct DamageType {
    std::string identifier;
    std::string message_id;
    std::string scaling;
    float exhaustion;
    std::optional<std::string> effects;
    std::optional<std::string> death_message_type;

    // Serialize Damage Type to NBT CompoundTag
    nbt::tag_compound serialize() const;

    // Deserialize NBT CompoundTag to Damage Type
    static DamageType deserialize(const nbt::tag_compound& compound);
};

bool loadDamageTypesFromCompoundFile(const std::string &filename, std::vector<DamageType> &types);

#endif //DAMAGE_TYPE_H
