#ifndef PAINTING_VARIANT_H
#define PAINTING_VARIANT_H
#include <string>
#include <tag_compound.h>
#include <vector>

struct PaintingVariant {
    std::string asset_id;
    int width;
    int height;

    // Serialize Painting Variant to NBT CompoundTag
    nbt::tag_compound serialize() const;

    // Deserialize NBT CompoundTag to Painting Variant
    static PaintingVariant deserialize(const nbt::tag_compound& compound);
};

bool loadPaintingVariantsFromCompoundFile(const std::string &filename, std::vector<PaintingVariant> &variants);

#endif //PAINTING_VARIANT_H
