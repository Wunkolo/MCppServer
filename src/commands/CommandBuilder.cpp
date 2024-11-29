#include "CommandBuilder.h"

#include "networking/clientbound_packets.h"
#include "core/config.h"

void buildAllCommands() {
    CommandBuilder builder;

    // Op Command: /op <player>
    builder
        .literal("op", true)                               // /op
            .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool)> &sendOutput) {
                    // Handler for /op
                    sendEntityEventPacket(*player->client, player->entityID, 24 + serverConfig.opPermissionLevel); // Make the player an operator
                    sendOutput("You are now an operator", false);
            })
            .argument("player", 7, true, true)
                .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool)> &sendOutput) {
                    // Handler for /op <player>
                    if (args.size() != 1 && player) {
                        sendOutput("Invalid number of arguments for /op", true);
                        return;
                    }
                    std::string targetPlayer = args[0];
                    Player* target = getPlayer(targetPlayer);
                    sendEntityEventPacket(*target->client, target->entityID, 24 + serverConfig.opPermissionLevel); // Make the player an operator
                    sendOutput(targetPlayer + " is now an operator", false);
                    sendChatMessage("You are now an operator", false, "white", *target, false);
                })
                .end()                               // End argument node
        .end();                                      // End literal node

    // Deop Command: /deop <player>
    builder
        .literal("deop", true)                             // /deop
                .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool)> &sendOutput) {
                    // Handler for /op
                    sendEntityEventPacket(*player->client, player->entityID, 24); // Make the player an operator
                    sendOutput("You are not an operator anymore", false);
                })
            .argument("player", 7, true, true)
                .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool)> &sendOutput) {
                    // Handler for /deop <player>
                    if (args.size() != 1 && player) {
                        sendOutput("Invalid number of arguments for /deop", true);
                        return;
                    }
                    std::string targetPlayer = args[0];
                    Player* target = getPlayer(targetPlayer);
                    sendEntityEventPacket(*target->client, target->entityID, 24); // Remove operator status
                    sendOutput(targetPlayer + " is not an operator anymore", false);
                    sendChatMessage("You are not an operator anymore", false, "white", *target, false);
                })
                .end()                               // End argument node
        .end();                                      // End literal node

    // Gamemode Command: /gamemode <mode> [player]
    builder
    .literal("gamemode")                        // /gamemode
        .argument("mode", 41, true)              // /gamemode <mode>, executable
            .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool)> &sendOutput) {
                // Handler for /gamemode <mode>
                if (args.size() != 1 && player) {
                    sendOutput("Usage: /gamemode <mode>", true);
                    return;
                }
                std::string mode = args[0];
                if (player) {
                    std::string targetPlayer = player->name; // Self
                    Player* target = getPlayer(targetPlayer);
                    if (target && stringToGamemode(mode) != target->gameMode) {
                        target->gameMode = stringToGamemode(mode);
                        sendChangeGamemode(*target->client, *target, target->gameMode);
                        sendChatMessage("Set own gamemode to " + gamemodeToString(target->gameMode) + " Mode", false, "white", *target, false);
                    }
                }
            })
            .argument("player", 7, true, true)            // /gamemode <mode> [player], executable
                .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool)> &sendOutput) {
                    // Handler for /gamemode <mode> [player]
                    if (args.size() != 2 && player) {
                        sendOutput("Usage: /gamemode <mode> [player]", true);
                        return;
                    }
                    std::string mode = args[0];
                    std::string targetPlayer = args[1];
                    Player* target = getPlayer(targetPlayer);
                    if (player && target->uuid == player->uuid) {
                        if (stringToGamemode(mode) != target->gameMode) {
                            target->gameMode = stringToGamemode(mode);
                            sendChangeGamemode(*target->client, *target, target->gameMode);
                            sendChatMessage("Set own gamemode to " + gamemodeToString(target->gameMode) + " Mode", false, "white", *target, false);
                        }
                    } else {
                        if (stringToGamemode(mode) != target->gameMode) {
                            target->gameMode = stringToGamemode(mode);
                            sendChangeGamemode(*target->client, *target, target->gameMode);
                            sendOutput("Set " + targetPlayer + "'s gamemode to " + gamemodeToString(target->gameMode) + " Mode", false);
                            sendChatMessage("Your gamemode was set to " + gamemodeToString(target->gameMode) + " Mode", false, "white", *target, false);
                        }
                    }

                })
                .end()                                   // End player argument
            .end()                                       // End mode argument
    .end();                                          // End literal node

    // Build the command graph
    globalCommandGraph = builder.build();

    // Serialize the command graph
    CommandSerializer serializer;
    serializedCommandGraph = serializer.serializeCommands(globalCommandGraph, commandGraphNumOfNodes);
}
