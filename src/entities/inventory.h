#ifndef INVENTORY_H
#define INVENTORY_H
#include <unordered_map>

#include "slot_data.h"

struct Inventory {
    // Inventory slots
    std::unordered_map<int, SlotData> slots;
};

#endif //INVENTORY_H
