#include "config.h"
#include <fstream>
#include <iostream>

#include "networking/fetch.h"
#include "utils.h"
#include "nlohmann/json.hpp"

ServerConfig serverConfig;
std::atomic configChanged(false);
std::string configDirectory;

void loadConfig() {
    std::string configFilePath = "../config.json";
    std::ifstream configFile(configFilePath);
    if (!configFile) {
        // Set default config if file doesn't exist
        serverConfig.server_version = "1.21.3";
        serverConfig.protocol_version = 768;
        serverConfig.port = 25565;
        serverConfig.motd = "Welcome to my C++ Minecraft server!";
        serverConfig.maxPlayers = 100;
        serverConfig.icon = "server-icon.png";
        serverConfig.worldType = "flat";
        serverConfig.flatWorldPreset = "classic_flat";
        serverConfig.viewDistance = 10;
        serverConfig.onlineMode = true;
        serverConfig.enableEncryption = true;
        serverConfig.enableCompression = true;
        serverConfig.compressionThreshold = 256;
        serverConfig.enableSecureChat = true;
        if (serverConfig.serverId.empty()) {
            serverConfig.serverId = generateServerID();
        }
        serverConfig.opPermissionLevel = 4;
        serverConfig.enableQuery = false;
        serverConfig.queryPort = 25565;
        serverConfig.enableRcon = false;
        serverConfig.ticksPerSecond = 20;
        serverConfig.consoleLang = "en_us";
        logMessage("Failed to open config file: " + configFilePath, LOG_ERROR);
        return;
    }

    serverConfig.serverLinks.clear();

    nlohmann::json jsonConfig;
    configFile >> jsonConfig;
    // Get the directory of the config file
    configDirectory = getDirectoryFromPath(configFilePath);

    serverConfig.server_version = jsonConfig.value("server_version", "1.21.3");
    serverConfig.protocol_version = jsonConfig.value("protocol_version", 768);
    serverConfig.port = jsonConfig.value("server_port", 25565);
    serverConfig.motd = jsonConfig.value("motd", "Welcome to my C++ Minecraft server!");
    serverConfig.maxPlayers = jsonConfig.value("maxPlayers", 100);
    serverConfig.icon = jsonConfig.value("icon", "server-icon.png");
    // If icon path is relative, resolve it relative to the config file directory
    if (!serverConfig.icon.empty() && !isAbsolutePath(serverConfig.icon)) {
        serverConfig.icon = configDirectory + "/" + serverConfig.icon;
    }
    serverConfig.worldType = jsonConfig.value("world_type", "flat");
    serverConfig.flatWorldPreset = jsonConfig.value("flatworld_preset", "classic_flat");
    serverConfig.viewDistance = jsonConfig.value("view_distance", 10);
    serverConfig.viewDistance = std::clamp(serverConfig.viewDistance, 2, 32);
    serverConfig.onlineMode = jsonConfig.value("online_mode", true);
    serverConfig.enableEncryption = (jsonConfig.value("enable_encryption", true) || serverConfig.onlineMode);
    serverConfig.enableCompression = jsonConfig.value("enable_compression", true);
    serverConfig.compressionThreshold = jsonConfig.value("compression_threshold", 256);
    serverConfig.enableSecureChat = jsonConfig.value("enable_secure_chat", true) && serverConfig.enableEncryption;
    if (serverConfig.serverId.empty()) {
        serverConfig.serverId = generateServerID();
    }
    serverConfig.opPermissionLevel = jsonConfig.value("op_permission_level", 4);
    serverConfig.opPermissionLevel = std::clamp(serverConfig.opPermissionLevel, 0, 4);

    auto serverLinks = jsonConfig.value("server_links", nlohmann::json::array());
    for (const auto& link : serverLinks) {
        ServerLink serverLink;
        serverLink.label = link.value("label", "");
        serverLink.url = link.value("url", "");
        serverConfig.serverLinks.push_back(serverLink);
    }
    serverConfig.enableQuery = jsonConfig.value("enable_query", false);
    serverConfig.queryPort = jsonConfig.value("query_port", 25565);
    auto resourcePacksJson = jsonConfig.value("resource_packs", nlohmann::json::array());
    for (const auto& pack : resourcePacksJson) {
        ResourcePack resourcePack;
        resourcePack.uuid = pack.value("uuid", "");
        resourcePack.url = pack.value("url", "");
        resourcePack.hash = pack.value("hash", "");
        resourcePack.forced = pack.value("forced", false);
        resourcePack.hasPromptMessage = pack.value("has_prompt_message", false);
        resourcePack.promptMessage = pack.value("prompt_message", "");

        // Validate UUID format (basic check)
        if (resourcePack.uuid.empty()) {
            logMessage("Resource pack missing UUID. Generating...", LOG_DEBUG);
            resourcePack.uuid = bytesToUUIDString(generateUUID());
        }

        // Validate URL
        if (resourcePack.url.empty()) {
            logMessage("Resource pack missing URL.", LOG_WARNING);
            continue;
        }

        // Validate hash
        if (resourcePack.hash.empty()) {
            logMessage("Resource pack missing hash. Generating...", LOG_DEBUG);
        }

        // Validate hash length
        if (!resourcePack.hash.empty() && resourcePack.hash.length() != 40) {
            logMessage("Resource pack hash must be 40 characters long.", LOG_WARNING);
            continue;
        }

        if (resourcePack.uuid.size() != 32 && resourcePack.uuid.size() != 36) {
            logMessage("Invalid UUID length for resource pack: " + resourcePack.uuid + ". UUID must be 32 characters long without '-', or 36 character with '-'", LOG_WARNING);
            continue;
        }

        serverConfig.resourcePacks.push_back(resourcePack);
    }

    // Log the loaded resource packs
    for (const auto& pack : serverConfig.resourcePacks) {
        logMessage("Loaded resource pack: UUID=" + pack.uuid + ", URL=" + pack.url, LOG_DEBUG);
    }

    serverConfig.enableRcon = jsonConfig.value("enable_rcon", false);
    serverConfig.rconPassword = jsonConfig.value("rcon_password", "");
    serverConfig.rconPort = jsonConfig.value("rcon_port", 25575);
    serverConfig.broadcastRconToOps = jsonConfig.value("broadcast_rcon_to_ops", false); // TODO: Implement this feature

    if (serverConfig.enableRcon) {
        if (serverConfig.rconPassword.empty()) {
            logMessage("RCON is enabled but no password is set.", LOG_WARNING);
        }
        if (serverConfig.rconPort < 1 || serverConfig.rconPort > 65535) {
            logMessage("RCON port out of valid range (1-65535).", LOG_WARNING);
            serverConfig.enableRcon = false;
        }
    }

    // ********** Load World Border Settings **********
    if (jsonConfig.contains("world_border")) {
        const auto& wb = jsonConfig["world_border"];

        // Load World Border Size
        if (wb.contains("world_border_size")) {
            serverConfig.worldBorder.size = wb["world_border_size"].get<double>();
        } else {
            logMessage("Invalid or missing 'world_border_size' in config. Using default values.", LOG_WARNING);
        }

        // Load World Border Center
        if (wb.contains("world_border_center") && wb["world_border_center"].is_array() && wb["world_border_center"].size() == 2) {
            serverConfig.worldBorder.center[0] = wb["world_border_center"][0].get<double>();
            serverConfig.worldBorder.center[1] = wb["world_border_center"][1].get<double>();
        } else {
            logMessage("Invalid or missing 'world_border_center' in config. Using default values.", LOG_WARNING);
        }

        // Load Warning Time
        serverConfig.worldBorder.warningTime = wb.value("world_border_warning_time", 15);

        // Load Warning Blocks
        serverConfig.worldBorder.warningBlocks = wb.value("world_border_warning_blocks", 5);
    } else {
        logMessage("Missing 'world_border' section in config. Using default World Border settings.", LOG_WARNING);
    }
    // ********** End of World Border Settings **********

    serverConfig.ticksPerSecond = jsonConfig.value("ticks_per_second", 20);
    serverConfig.consoleLang = jsonConfig.value("console_language", "en_us");
}

