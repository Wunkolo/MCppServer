#ifndef ITEM_ENTITY_H
#define ITEM_ENTITY_H
#include "entity.h"
#include "core/utils.h"
#include "data/data.h"

class Item : public Entity {
public:
    Item();

    void serializeAdditionalData(std::vector<uint8_t> &packetData) const override;
    int32_t id() const;
    void setItemId(int16_t);
    void setItemCount(int8_t count);

    [[nodiscard]] std::vector<MetadataEntry> getMetadata() const;

    void tryMerge();
    uint8_t getCount() const { return slotData.itemCount; }
    void setCooldown(uint8_t cooldown) { pickUpCooldown = cooldown; }
    uint8_t getCooldown() const { return pickUpCooldown; }

private:
    SlotData slotData;
    uint8_t pickUpCooldown{};
};

#endif //ITEM_ENTITY_H
