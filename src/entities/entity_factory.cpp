#include "entity_factory.h"

#include "item_entity.h"
#include "player.h"
#include "core/server.h"

std::shared_ptr<Item> EntityFactory::createItem() {
    auto item = std::make_shared<Item>();
    entityManager.addEntity(item);
    return item;
}

std::shared_ptr<Player> EntityFactory::createPlayer(const std::array<uint8_t, 16>& uuidBytes, const std::string& playerName) {
    auto player = std::make_shared<Player>(uuidBytes, playerName, EntityType::Player);
    entityManager.addEntity(player);
    return player;
}