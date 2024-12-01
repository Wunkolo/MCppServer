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

    // Time Command: /time <set|add|query> <value>
    builder
        .literal("time") // /time
            // Subcommand: add
            .literal("add", true, true) // /time add
                .argument("time", 42, true, true) // <time> argument with parserId=42 (minecraft:time)
                    .setIntegerRange(0, 0) // Set the range of the <time> argument
                    .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool)> &sendOutput) {
                        if (args.size() != 1) {
                            sendOutput("Usage: /time add <time>", true);
                            return;
                        }

                        // Parse the <time> argument
                        std::string timeArg = args[0];
                        int ticksToAdd = 0;

                        try {
                            // Handle unit suffixes: d, s, t
                            size_t len = timeArg.length();
                            char unit = 't'; // default unit
                            if (len > 0 && (timeArg[len - 1] == 'd' || timeArg[len - 1] == 's' || timeArg[len - 1] == 't')) {
                                unit = timeArg[len - 1];
                                timeArg = timeArg.substr(0, len - 1);
                            }

                            float timeValue = std::stof(timeArg);
                            switch (unit) {
                                case 'd':
                                    ticksToAdd = static_cast<int>(timeValue * 24000);
                                    break;
                                case 's':
                                    ticksToAdd = static_cast<int>(timeValue * 20);
                                    break;
                                case 't':
                                default:
                                    ticksToAdd = static_cast<int>(timeValue);
                                    break;
                            }
                        }
                        catch (const std::exception& e) {
                            sendOutput("Invalid time format.", true);
                            return;
                        }

                        worldTime.setTimeOfDay(worldTime.getTimeOfDay() + ticksToAdd);
                        sendOutput("Set time to " + std::to_string(worldTime.getTimeOfDay()), false);
                    })
                    .end() // End <time> argument
                .end() // End "add" subcommand

            // Subcommand: query
            .literal("query", true, true) // /time query
                .literal("daytime", true, true) // /time query daytime
                    .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool)> &sendOutput) {
                        int daytime = worldTime.getTimeOfDay() % 24000;
                        sendOutput("The time is " + std::to_string(daytime), false);
                    })
                    .end()
                .literal("gametime", true, true) // /time query gametime
                    .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool)> &sendOutput) {
                        sendOutput("The time is " + std::to_string(worldTime.getWorldAge()), false);
                    })
                    .end()
                .literal("day", true, true) // /time query day
                    .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool)> &sendOutput) {
                        long long days = (worldTime.getWorldAge() / 24000) % 2147483647;
                        sendOutput("The time is " +  std::to_string(days), false);
                    })
                    .end()
                .end() // End "query" subcommand

            // Subcommand: set
            .literal("set", true, true) // /time set
                // Set to specific time of day
                .literal("day", true, true) // /time set day
                    .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool)> &sendOutput) {
                        worldTime.setTimeOfDay(1000);
                        sendOutput("Time set to 1000", false);
                    })
                    .end()
                .literal("night", true, true) // /time set night
                    .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool)> &sendOutput) {
                        worldTime.setTimeOfDay(13000);
                        sendOutput("Time set to 13000", false);
                    })
                    .end()
                .literal("noon", true, true) // /time set noon
                    .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool)> &sendOutput) {
                        worldTime.setTimeOfDay(6000);
                        sendOutput("Time set to 6000", false);
                    })
                    .end()
                .literal("midnight", true, true) // /time set midnight
                    .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool)> &sendOutput) {
                        worldTime.setTimeOfDay(18000);
                        sendOutput("Time set to 18000", false);
                    })
                    .end()

                // Set to specific time using <time> argument
                .argument("time", 42, true, true) // <time> argument with parserId=42 (minecraft:time)
                    .setIntegerRange(0, 0) // Set the range of the <time> argument
                    .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool)> &sendOutput) {
                        if (args.size() != 1) {
                            sendOutput("Usage: /time set <time>", true);
                            return;
                        }

                        // Parse the <time> argument
                        std::string timeArg = args[0];
                        int newTime = 0;

                        try {
                            // Handle unit suffixes: d, s, t
                            size_t len = timeArg.length();
                            char unit = 't'; // default unit
                            if (len > 0 && (timeArg[len - 1] == 'd' || timeArg[len - 1] == 's' || timeArg[len - 1] == 't')) {
                                unit = timeArg[len - 1];
                                timeArg = timeArg.substr(0, len - 1);
                            }

                            float timeValue = std::stof(timeArg);
                            switch (unit) {
                                case 'd':
                                    newTime = static_cast<int>(timeValue * 24000);
                                    break;
                                case 's':
                                    newTime = static_cast<int>(timeValue * 20);
                                    break;
                                case 't':
                                default:
                                    newTime = static_cast<int>(timeValue);
                                    break;
                            }

                            if (newTime < 0) newTime += 24000;
                        }
                        catch (const std::exception& e) {
                            sendOutput("Invalid time format.", true);
                            return;
                        }

                        // Set the server's current time
                        worldTime.setTimeOfDay(newTime % 24000);
                        sendOutput("Set time to " + std::to_string(newTime), false);
                    })
                    .end() // End <time> argument
                .end() // End "set" subcommand
            .end(); // End "time" command

    // Worldborder Command: /worldborder <set|add|center|damage|get|warning> [args]
    builder
        .literal("worldborder") // /worldborder
            // Subcommand: add <distance> [<time>]
            .literal("add", true, true) // /worldborder add
                .argument("distance", 2, true, true) // <distance>: brigadier:double (parserId=2)
                    .setDoubleRange(-59999968.0, 59999968.0)
                    .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool)> &sendOutput) {
                        double distanceChange = 0.0;

                        try {
                            distanceChange = std::stod(args[0]);
                        }
                        catch (const std::exception& e) {
                            sendOutput("Invalid argument format.", true);
                            return;
                        }

                        // Update the world border
                        if (worldBorder.getDiameter() + distanceChange < 1.0) {
                            sendOutput("The world border cannot be smaller than 1 block wide", true);
                            return;
                        }
                        if (worldBorder.getDiameter() + distanceChange == worldBorder.getDiameter()) {
                            sendOutput("Nothing changed. The world border is already that size", true);
                            return;
                        }
                        if (worldBorder.getDiameter() + distanceChange > worldBorder.getDiameter()) {
                            sendReInitializeWorldBorder(worldBorder.centerX, worldBorder.centerZ, worldBorder.getDiameter() + distanceChange, 0, worldBorder.warningBlocks, worldBorder.warningTime);
                        }else {
                            sendSetBorderSize(worldBorder.getDiameter() + distanceChange);
                        }
                        std::ostringstream diameterStream;
                        diameterStream.precision(1);
                        diameterStream << std::fixed << worldBorder.getDiameter();
                        std::string message = "Set the world border to " + diameterStream.str() + " block(s) wide";
                        sendOutput(message, false);
                    })
                        .argument("time", 3, true, true) // <time>: brigadier:integer (parserId=3)
                            .setIntegerRange(0, 2147483647) // Must be between 0 and 2147483647
                            .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool)> &sendOutput) {
                                double distanceChange = 0.0;
                                int timeSeconds = 0; // Default time

                                try {
                                    distanceChange = std::stod(args[0]);
                                    timeSeconds = std::stoi(args[1]);
                                }
                                catch (const std::exception& e) {
                                    sendOutput("Invalid argument format.", true);
                                    return;
                                }

                                // Update the world border
                                double currentDiameter = worldBorder.getDiameter();
                                if (worldBorder.getDiameter() + distanceChange < 1.0) {
                                    sendOutput("The world border cannot be smaller than 1 block wide", true);
                                    return;
                                }
                                if (worldBorder.getDiameter() + distanceChange == worldBorder.getDiameter()) {
                                    sendOutput("Nothing changed. The world border is already that size", true);
                                    return;
                                }
                                if (worldBorder.getDiameter() + distanceChange > worldBorder.getDiameter()) {
                                    sendReInitializeWorldBorder(worldBorder.centerX, worldBorder.centerZ, worldBorder.getDiameter() + distanceChange, timeSeconds * 1000, worldBorder.warningBlocks, worldBorder.warningTime);
                                } else {
                                    sendSetBorderLerpSize(worldBorder.getDiameter() + distanceChange, timeSeconds * 1000);
                                }
                                std::ostringstream diameterStream;
                                diameterStream.precision(1);
                                diameterStream << std::fixed << worldBorder.getDiameter();
                                std::string message;
                                if (timeSeconds == 0) {
                                    message = "Set the world border to " + diameterStream.str() + " block(s) wide";
                                } else if (currentDiameter < worldBorder.getDiameter()) {
                                    message = "Growing the world border to " + diameterStream.str() + " block(s) wide over " + std::to_string(timeSeconds) + " second(s)";
                                } else {
                                    message = "Shrinking the world border to " + diameterStream.str() + " block(s) wide over " + std::to_string(timeSeconds) + " second(s)";
                                }
                                sendOutput(message, false);
                            })
                        .end() // End <time> argument
                    .end() // End <distance> argument
                // [No further arguments]
                .end() // End "add" subcommand

            // Subcommand: center <pos>
            .literal("center", true, true) // /worldborder center
                .argument("pos", 11, true, true) // <pos>: minecraft:vec2 (parserId=11)
                    .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool)> &sendOutput) {
                        // Parse the <pos> argument
                        std::string posArg = args[0];
                        float x = 0.0f, z = 0.0f;

                        try {
                            // pos is in "x,z" format
                            std::istringstream ss(posArg);
                            std::string token;
                            std::getline(ss, token, ',');
                            x = std::stof(token);
                            std::getline(ss, token, ',');
                            z = std::stof(token);

                            if (std::abs(x) > 29999984.0f || std::abs(z) > 29999984.0f) {
                                sendOutput("Coordinates must be between ±29999984.", true);
                                return;
                            }
                        }
                        catch (const std::exception& e) {
                            sendOutput("Invalid position format.", true);
                            return;
                        }

                        // Update the world border center
                        double currentDiameter = worldBorder.getDiameter();
                        if (worldBorder.centerX == x && worldBorder.centerZ == z) {
                            sendOutput("Nothing changed. The world border is already centered there", true);
                            return;
                        }
                        if (std::abs(x) + worldBorder.getDiameter() > worldBorder.biggestSize || std::abs(z) + worldBorder.getDiameter() > worldBorder.biggestSize) {
                            sendReInitializeWorldBorder(x, z, worldBorder.getDiameter() + (std::max(std::abs(x), std::abs(z)) * 2), 0, worldBorder.warningBlocks, worldBorder.warningTime);
                            sendSetBorderSize(currentDiameter);
                        } else {
                            sendSetBorderCenter(x, z);
                        }
                        std::ostringstream posStream;
                        posStream.precision(2);
                        posStream << std::fixed << x << ", " << z;
                        std::string message = "Set the center of the world border to " + posStream.str();
                        sendOutput(message, false);
                    })
                    .end() // End <pos> argument
                .end() // End "center" subcommand

            // Subcommand: damage
            .literal("damage", true, true) // /worldborder damage
                // Sub-subcommand: amount <damagePerBlock>
                .literal("amount", true, true) // /worldborder damage amount
                    .argument("damagePerBlock", 1, true, true) // <damagePerBlock>: brigadier:float (parserId=1)
                        .setFloatRange(0.0f, std::numeric_limits<float>::max()) // Must be >= 0.0
                        .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool)> &sendOutput) {
                            float damage = 0.0f;

                            try {
                                damage = std::stof(args[0]);
                                if (damage < 0.0f) {
                                    sendOutput("Damage per block must be >= 0.0.", true);
                                    return;
                                }
                            }
                            catch (const std::exception& e) {
                                sendOutput("Invalid damage per block format.", true);
                                return;
                            }

                            // TODO: Update the world border damage amount
                            /*
                            if (worldBorder.damage == damage) {
                                sendOutput("Nothing changed. The world border damage is already that amount", true);
                                return;
                            }
                            */
                            std::ostringstream damageStream;
                            damageStream.precision(2);
                            damageStream << std::fixed << damage;
                            std::string message = "Set the world border damage to " + damageStream.str() + " per block each second";
                            sendOutput(message, false);
                        })
                        .end() // End <damagePerBlock> argument
                    .end() // End "amount" sub-subcommand

                // Sub-subcommand: buffer <distance>
                .literal("buffer", true, true) // /worldborder damage buffer
                    .argument("distance", 1, true, true) // <distance>: brigadier:float (parserId=1)
                        .setFloatRange(0.0f, std::numeric_limits<float>::max()) // Must be >= 0.0
                        .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool)> &sendOutput) {
                            float bufferDistance = 0.0f;

                            try {
                                bufferDistance = std::stof(args[0]);
                                if (bufferDistance < 0.0f) {
                                    sendOutput("Buffer distance must be >= 0.0.", true);
                                    return;
                                }
                            }
                            catch (const std::exception& e) {
                                sendOutput("Invalid buffer distance format.", true);
                                return;
                            }

                            // TODO: Update the world border damage buffer

                            /*
                            if (worldBorder.damage == damage) {
                                sendOutput("Nothing changed. The world border damage buffer is already that distance", true);
                                return;
                            }
                            */
                            std::ostringstream distStream;
                            distStream.precision(2);
                            distStream << std::fixed << bufferDistance;
                            std::string message = "Set the world border damage buffer to " + distStream.str() + " block(s)";
                            sendOutput(message, false);
                        })
                        .end() // End <distance> argument
                    .end() // End "buffer" sub-subcommand
                .end() // End "damage" subcommand

            // Subcommand: get
            .literal("get", true, true) // /worldborder get
                .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool)> &sendOutput) {
                    if (!args.empty()) {
                        sendOutput("Usage: /worldborder get", true);
                        return;
                    }

                    std::ostringstream diameterStream;
                    diameterStream.precision(0);
                    diameterStream << std::fixed << worldBorder.getDiameter();
                    sendOutput("The world border is currently " + diameterStream.str() + " block(s) wide", false);
                })
                .end() // End "get" subcommand

            // Subcommand: set <distance> [<time>]
            .literal("set", true, true) // /worldborder set
                .argument("distance", 2, true, true) // <distance>: brigadier:double (parserId=2)
                    .setDoubleRange(-59999968.0, 59999968.0) // As per specifications
                    .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool)> &sendOutput) {
                        double newDiameter = 0.0;

                        try {
                            newDiameter = std::stod(args[0]);
                        }
                        catch (const std::exception& e) {
                            sendOutput("Invalid argument format.", true);
                            return;
                        }

                        // Update the world border
                        if (newDiameter < 1.0) {
                            sendOutput("The world border cannot be smaller than 1 block wide", true);
                            return;
                        }
                        if (newDiameter == worldBorder.getDiameter()) {
                            sendOutput("Nothing changed. The world border is already that size", true);
                            return;
                        }
                        if (newDiameter > worldBorder.getDiameter()) {
                            sendReInitializeWorldBorder(worldBorder.centerX, worldBorder.centerZ, newDiameter, 0, worldBorder.warningBlocks, worldBorder.warningTime);
                        } else {
                            sendSetBorderSize(newDiameter);
                        }

                        std::ostringstream diameterStream;
                        diameterStream.precision(1);
                        diameterStream << std::fixed << worldBorder.getDiameter();
                        std::string message = "Set the world border to " + diameterStream.str() + " block(s) wide";
                        sendOutput(message, false);
                    })
                        .argument("time", 3, true, true) // <time>: brigadier:integer (parserId=3)
                            .setIntegerRange(0, 2147483647) // Must be between 0 and 2147483647
                            .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool)> &sendOutput) {
                                double newDiameter = 0.0;
                                int timeSeconds = 0;

                                try {
                                    newDiameter = std::stod(args[0]);
                                    timeSeconds = std::stoi(args[1]);
                                    if (timeSeconds < 0) {
                                        sendOutput("Time must be >= 0.", true);
                                        return;
                                    }
                                }
                                catch (const std::exception& e) {
                                    sendOutput("Invalid time format.", true);
                                    return;
                                }

                                // Update the world border
                                double currentDiameter = worldBorder.getDiameter();
                                if (newDiameter < 1.0) {
                                    sendOutput("The world border cannot be smaller than 1 block wide", true);
                                    return;
                                }
                                if (newDiameter == worldBorder.getDiameter()) {
                                    sendOutput("Nothing changed. The world border is already that size", true);
                                    return;
                                }
                                if (newDiameter > worldBorder.getDiameter()) {
                                    sendReInitializeWorldBorder(worldBorder.centerX, worldBorder.centerZ, newDiameter, timeSeconds * 1000, worldBorder.warningBlocks, worldBorder.warningTime);
                                } else {
                                    sendSetBorderLerpSize(newDiameter, timeSeconds * 1000);
                                }

                                std::ostringstream diameterStream;
                                diameterStream.precision(1);
                                diameterStream << std::fixed << worldBorder.getDiameter();
                                std::string message;
                                if (timeSeconds == 0) {
                                    message = "Set the world border to " + diameterStream.str() + " block(s) wide";
                                } else if (currentDiameter < worldBorder.getDiameter()) {
                                    message = "Growing the world border to " + diameterStream.str() + " block(s) wide over " + std::to_string(timeSeconds) + " second(s)";
                                } else {
                                    message = "Shrinking the world border to " + diameterStream.str() + " block(s) wide over " + std::to_string(timeSeconds) + " second(s)";
                                }
                                sendOutput(message, false);
                            })
                            .end() // End <time> argument
                    .end() // End <distance> argument
                // [No further arguments]
                .end() // End "set" subcommand

            // Subcommand: warning
            .literal("warning", true, true) // /worldborder warning
                // Sub-subcommand: distance <distance>
                .literal("distance", true, true) // /worldborder warning distance
                    .argument("distance", 3, true, true) // <distance>: brigadier:integer (parserId=3)
                        .setIntegerRange(0, 2147483647) // Must be between 0 and 2147483647
                        .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool)> &sendOutput) {
                            int warningDistance = 0;

                            try {
                                warningDistance = std::stoi(args[0]);
                                if (warningDistance < 0) {
                                    sendOutput("Warning distance must be >= 0.", true);
                                    return;
                                }
                            }
                            catch (const std::exception& e) {
                                sendOutput("Invalid warning distance format.", true);
                                return;
                            }

                            // Update the world border warning distance
                            if (worldBorder.warningBlocks == warningDistance) {
                                sendOutput("Nothing changed. The world border warning is already that distance", true);
                                return;
                            }
                            sendSetBorderWarningDistance(warningDistance);

                            std::string message = "Set the world border warning distance to " + std::to_string(warningDistance) + " block(s)";
                            sendOutput(message, false);
                        })
                        .end() // End <distance> argument
                    .end() // End "distance" sub-subcommand

                // Sub-subcommand: time <time>
                .literal("time", true, true) // /worldborder warning time
                    .argument("time", 3, true, true) // <time>: brigadier:integer (parserId=3)
                        .setIntegerRange(0, 2147483647) // Must be between 0 and 2147483647
                        .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool)> &sendOutput) {
                            int warningTime = 0;

                            try {
                                warningTime = std::stoi(args[0]);
                                if (warningTime < 0) {
                                    sendOutput("Warning time must be >= 0.", true);
                                    return;
                                }
                            }
                            catch (const std::exception& e) {
                                sendOutput("Invalid warning time format.", true);
                                return;
                            }

                            // Update the world border warning time
                            if (worldBorder.warningTime == warningTime) {
                                sendOutput("Nothing changed. The world border warning is already that amount of time", true);
                                return;
                            }
                            sendSetBorderWarningDelay(warningTime);

                            std::string message = "Set the world border warning time to " + std::to_string(warningTime) + " second(s)";
                            sendOutput(message, false);
                        })
                        .end() // End <time> argument
                    .end() // End "time" sub-subcommand
                .end() // End "warning" subcommand



            .end(); // End "worldborder" command


    // Build the command graph
    globalCommandGraph = builder.build();

    // Serialize the command graph
    CommandSerializer serializer;
    serializedCommandGraph = serializer.serializeCommands(globalCommandGraph, commandGraphNumOfNodes);
}
