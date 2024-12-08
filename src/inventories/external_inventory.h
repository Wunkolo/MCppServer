#ifndef EXTERNAL_INVENTORY_H
#define EXTERNAL_INVENTORY_H

class PlayerInventory;

class ExternalInventory {
public:
    virtual ~ExternalInventory() = default;

    virtual void copyPlayerInventory(const PlayerInventory& playerInventory) = 0;
    virtual void copyToPlayerInventory(PlayerInventory& playerInventory) = 0;
};

#endif //EXTERNAL_INVENTORY_H
