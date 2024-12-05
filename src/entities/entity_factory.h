#ifndef ENTITY_FACTORY_H
#define ENTITY_FACTORY_H
#include <memory>

struct Player;
class Item;

class EntityFactory {
public:
    static std::shared_ptr<Item> createItem();
    static std::shared_ptr<Player> createPlayer(const std::array<uint8_t, 16>& uuidBytes, const std::string& playerName);
};

#endif //ENTITY_FACTORY_H
