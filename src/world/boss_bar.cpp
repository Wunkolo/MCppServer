#include "boss_bar.h"

#include <utility>

#include "core/utils.h"
#include "entities/player.h"
#include "networking/clientbound_packets.h"

std::array<uint8_t, 16> Bossbar::createBossbar(std::string& id, nbt::tag_compound& title) {
    std::array<uint8_t, 16> uuid = generateUUID();
    this->id = id;
    this->uuid = uuid;
    this->title = title;
    this->color = WHITE;
    this->division = NO_DIVISION;
    return uuid;
}

void Bossbar::updateMax(int32_t max) {
    this->max = max;
    sendBossbar(*this, 2);
}

void Bossbar::updateValue(int32_t value) {
    this->value = value;
    sendBossbar(*this, 2);
}

void Bossbar::updateTitle(nbt::tag_compound &title) {
    this->title = title;
    sendBossbar(*this, 3);
}

void Bossbar::updateColor(BossbarColor color) {
    this->color = color;
    sendBossbar(*this, 4);
}

void Bossbar::updateDivision(BossbarDivision division) {
    this->division = division;
    sendBossbar(*this, 4);
}

void Bossbar::updateFlags(uint8_t flags) {
    this->flags = flags;
    sendBossbar(*this, 5);
}

void Bossbar::setVisible(bool visible) {
    if (this->visible == visible) {
        return;
    }
    this->visible = visible;
    for (auto& player : players) {
        if (visible) {
            sendBossbar(*this, 0);
        } else {
            sendBossbar(*this, 1);
        }
    }
}

void Bossbar::addPlayer(const std::string &playerName) {
    std::shared_ptr<Player> player = getPlayer(playerName);
    players.push_back(player);
    sendBossbar(*this, 0);
}

void Bossbar::removeAllPlayers() {
    for (auto& player : players) {
        sendBossbar(*this, 1);
    }
    players.clear();
}

Bossbar::~Bossbar() {
    sendBossbar(*this, 1);
}

std::string Bossbar::getId() const {
    return id;
}

std::array<uint8_t, 16> Bossbar::getUUID() const {
        return uuid;
}

nbt::tag_compound Bossbar::getTitle() const {
    return title;
}

std::string Bossbar::getTitleString() const {
    return title.at("text").as<nbt::tag_string>().get();
}

int32_t Bossbar::getMax() const {
    return max;
}

int32_t Bossbar::getValue() const {
    return value;
}

float Bossbar::getHealth() const {
    return max == 0 ? 0 : static_cast<float>(value) / max;
}

BossbarColor Bossbar::getColor() const {
    return color;
}

BossbarDivision Bossbar::getDivision() const {
    return division;
}

uint8_t Bossbar::getFlags() const {
    return flags;
}

bool Bossbar::isVisible() const {
    return visible;
}

bool Bossbar::hasPlayer(const std::string &playerName) const {
    for (const auto& player : players) {
        if (player->name == playerName) {
            return true;
        }
    }
    return false;
}

std::vector<std::shared_ptr<Player>>& Bossbar::getPlayers() {
    return players;
}

int32_t Bossbar::getPlayerCount() const {
    return players.size();
}

int32_t Bossbar::getOnlinePlayerCount() const {
    int32_t count = 0;
    for (const auto& player : players) {
        for (const auto &val: globalPlayers | std::views::values) {
            if (val->uuid == player->uuid) {
                count++;
                break;
            }
        }
    }
    return count;
}

std::string Bossbar::getPlayerList() const {
    std::string list;
    for (const auto& player : players) {
        list += player->name + ", ";
    }
    if (!list.empty()) {
        list.pop_back();
        list.pop_back();
    }
    return list;
}

std::string Bossbar::getOnlinePlayerList() const {
    std::string list;
    for (const auto& player : players) {
        for (const auto &val: globalPlayers | std::views::values) {
            if (val->uuid == player->uuid) {
                list += player->name + ", ";
                break;
            }
        }
    }
    if (!list.empty()) {
        list.pop_back();
        list.pop_back();
    }
    return list;
}
