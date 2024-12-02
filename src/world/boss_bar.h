//
// Created by noah1 on 02.12.2024.
//

#ifndef BOSS_BAR_H
#define BOSS_BAR_H
#include <array>
#include <cstdint>
#include <tag_compound.h>
#include <vector>

#include "enums/enums.h"

struct Player;

class Bossbar {
public:
    Bossbar() = default;
    std::array<uint8_t, 16> createBossbar(std::string& id, nbt::tag_compound& title);
    void updateMax(int32_t max);
    void updateValue(int32_t value);
    void updateTitle(nbt::tag_compound& title);
    void updateColor(BossbarColor color);
    void updateDivision(BossbarDivision division);
    void updateFlags(uint8_t flags);
    void setVisible(bool visible);
    void addPlayer(const std::string& playerName);
    void removeAllPlayers();

    ~Bossbar();

    [[nodiscard]] std::string getId() const;
    [[nodiscard]] std::array<uint8_t, 16> getUUID() const;
    [[nodiscard]] nbt::tag_compound getTitle() const;
    [[nodiscard]] std::string getTitleString() const;
    [[nodiscard]] int32_t getMax() const;
    [[nodiscard]] int32_t getValue() const;
    [[nodiscard]] float getHealth() const;
    [[nodiscard]] BossbarColor getColor() const;
    [[nodiscard]] BossbarDivision getDivision() const;
    [[nodiscard]] uint8_t getFlags() const;
    [[nodiscard]] bool isVisible() const;
    bool hasPlayer(const std::string& playerName) const;
    std::vector<Player*>& getPlayers();
    int32_t getPlayerCount() const;
    int32_t getOnlinePlayerCount() const;
    std::string getPlayerList() const;
    std::string getOnlinePlayerList() const;

private:
    std::vector<Player*> players;
    std::array<uint8_t, 16> uuid{};
    bool visible{true};
    std::string id;
    nbt::tag_compound title; // Text component
    int32_t max{100};
    int32_t value{};
    BossbarColor color{};
    BossbarDivision division{};
    uint8_t flags{};
};

#endif //BOSS_BAR_H
