#ifndef EQUIPMENT_H
#define EQUIPMENT_H
#include <cstdint>

#include "slot_data.h"

struct EquipmentSlot {
    uint8_t slotId;
    SlotData slotData;
};

struct Equipment {
    EquipmentSlot mainHand;
    EquipmentSlot offHand;
    EquipmentSlot boots;
    EquipmentSlot leggings;
    EquipmentSlot chestplate;
    EquipmentSlot helmet;
    EquipmentSlot body;
};


#endif //EQUIPMENT_H
