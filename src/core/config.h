//
// Created by noah1 on 13.11.2024.
//

#ifndef CONFIG_H
#define CONFIG_H

#include <array>
#include <string>
#include <thread>
#include <vector>

struct ResourcePack {
    std::string uuid;
    std::string url;
    std::string hash{};
    bool forced;
    bool hasPromptMessage;
    std::string promptMessage;
};

struct ServerLink {
    std::string label;
    std::string url;
};

struct WorldBorderConfig {
    double size = 60000000.0; // Diameter
    std::array<double, 2> center = {0.0, 0.0};             // Center X and Z
    int32_t warningTime = 15;                              // In seconds
    int32_t warningBlocks = 5;                             // In meters
};

struct ServerConfig {
    std::string server_version;
    int protocol_version;
    uint16_t port;
    std::string motd;
    int maxPlayers;
    std::string icon;
    std::string worldType;
    std::string flatWorldPreset;
    int viewDistance;
    bool onlineMode;
    bool enableEncryption;
    bool enableCompression;
    int compressionThreshold;
    bool enableSecureChat;
    std::string serverId{};
    int opPermissionLevel;
    std::vector<ServerLink> serverLinks;
    // Query Settings
    bool enableQuery;
    uint16_t queryPort;
    std::vector<ResourcePack> resourcePacks;
    // RCON Settings
    bool enableRcon;
    std::string rconPassword;
    int rconPort;
    bool broadcastRconToOps;
    WorldBorderConfig worldBorder;
    int ticksPerSecond;
};

extern ServerConfig serverConfig;

void loadConfig();

#endif // CONFIG_H

