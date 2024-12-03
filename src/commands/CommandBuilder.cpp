#include "CommandBuilder.h"

#include "networking/clientbound_packets.h"
#include "core/config.h"

void buildAllCommands() {
    CommandBuilder builder;

    // Op Command: /op <player>
    builder
        .literal("op", true)                               // /op
            .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                    // Handler for /op
                    sendEntityEventPacket(*player->client, player->entityID, 24 + serverConfig.opPermissionLevel); // Make the player an operator
                    sendOutput("commands.op.success", false, {player->name});
            })
            .argument("player", 7, true, true)
                .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                    const std::string& targetPlayer = args[0];
                    Player* target = getPlayer(targetPlayer);
                    sendEntityEventPacket(*target->client, target->entityID, 24 + serverConfig.opPermissionLevel); // Make the player an operator
                    sendOutput("commands.op.success", false, {target->name});
                })
                .end()                               // End argument node
        .end();                                      // End literal node

    // Deop Command: /deop <player>
    builder
        .literal("deop", true)                             // /deop
                .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                    // Handler for /op
                    sendEntityEventPacket(*player->client, player->entityID, 24); // Make the player an operator
                    sendOutput("commands.deop.success", false, {player->name});
                })
            .argument("player", 7, true, true)
                .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                    const std::string& targetPlayer = args[0];
                    Player* target = getPlayer(targetPlayer);
                    sendEntityEventPacket(*target->client, target->entityID, 24); // Remove operator status
                    sendOutput("commands.deop.success", false, {target->name});
                })
                .end()                               // End argument node
        .end();                                      // End literal node

    // Gamemode Command: /gamemode <mode> [player]
    builder
    .literal("gamemode")                        // /gamemode
        .argument("mode", 41, true)              // /gamemode <mode>, executable
            .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                const std::string& mode = args[0];
                if (player) {
                    std::string targetPlayer = player->name; // Self
                    Player* target = getPlayer(targetPlayer);
                    if (target && stringToGamemode(mode) != target->gameMode) {
                        target->gameMode = stringToGamemode(mode);
                        sendChangeGamemode(*target->client, *target, target->gameMode);
                        sendOutput( "commands.gamemode.success.self", false, {"translate.gameMode." + gamemodeToString(target->gameMode)});
                    }
                }
            })
            .argument("player", 7, true, true)            // /gamemode <mode> [player], executable
                .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                    const std::string& mode = args[0];
                    const std::string& targetPlayer = args[1];
                    Player* target = getPlayer(targetPlayer);
                    if (player && target->uuid == player->uuid) {
                        if (stringToGamemode(mode) != target->gameMode) {
                            target->gameMode = stringToGamemode(mode);
                            sendChangeGamemode(*target->client, *target, target->gameMode);
                            sendOutput( "commands.gamemode.success.self", false, {"translate.gameMode." + gamemodeToString(target->gameMode)});
                        }
                    } else {
                        if (stringToGamemode(mode) != target->gameMode) {
                            target->gameMode = stringToGamemode(mode);
                            sendChangeGamemode(*target->client, *target, target->gameMode);
                            sendOutput("commands.gamemode.success.other", false, {target->name, "translate.gameMode." + gamemodeToString(target->gameMode)});
                            std::vector<Player> players = {*target};
                            sendTranslatedChatMessage("gameMode.changed", false, "white", &players, false, "translate.gameMode." + gamemodeToString(target->gameMode));
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
                    .setIntegerRange(0, 0) // Set the range of the <time> argument (only min is used)
                    .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                        // Parse the <time> argument
                        std::string timeArg = args[0];
                        int ticksToAdd = 0;

                        ticksToAdd = parseDuration(timeArg);

                        worldTime.setTimeOfDay(worldTime.getTimeOfDay() + ticksToAdd);
                        sendOutput("commands.time.set", false, {std::to_string(worldTime.getTimeOfDay())});
                    })
                    .end() // End <time> argument
                .end() // End "add" subcommand

            // Subcommand: query
            .literal("query", true, true) // /time query
                .literal("daytime", true, true) // /time query daytime
                    .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                        int daytime = worldTime.getTimeOfDay() % 24000;
                        sendOutput("commands.time.query", false, {std::to_string(daytime)});
                    })
                    .end()
                .literal("gametime", true, true) // /time query gametime
                    .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                        sendOutput("commands.time.query", false, {std::to_string(worldTime.getWorldAge())});
                    })
                    .end()
                .literal("day", true, true) // /time query day
                    .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                        long long days = (worldTime.getWorldAge() / 24000) % 2147483647;
                        sendOutput("commands.time.query", false, {std::to_string(days)});
                    })
                    .end()
                .end() // End "query" subcommand

            // Subcommand: set
            .literal("set", true, true) // /time set
                // Set to specific time of day
                .literal("day", true, true) // /time set day
                    .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                        worldTime.setTimeOfDay(1000);
                        sendOutput("commands.time.set", false, {std::to_string(1000)});
                    })
                    .end()
                .literal("night", true, true) // /time set night
                    .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                        worldTime.setTimeOfDay(13000);
                        sendOutput("commands.time.set", false, {std::to_string(13000)});
                    })
                    .end()
                .literal("noon", true, true) // /time set noon
                    .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                        worldTime.setTimeOfDay(6000);
                        sendOutput("commands.time.set", false, {std::to_string(6000)});
                    })
                    .end()
                .literal("midnight", true, true) // /time set midnight
                    .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                        worldTime.setTimeOfDay(18000);
                        sendOutput("commands.time.set", false, {std::to_string(18000)});
                    })
                    .end()

                // Set to specific time using <time> argument
                .argument("time", 42, true, true) // <time> argument with parserId=42 (minecraft:time)
                    .setIntegerRange(0, 0) // Set the range of the <time> argument (only min is used)
                    .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                        // Parse the <time> argument
                        std::string timeArg = args[0];
                        int newTime = 0;

                        newTime = parseDuration(timeArg);

                        // Set the server's current time
                        worldTime.setTimeOfDay(newTime);
                        sendOutput("commands.time.set", false, {std::to_string(newTime)});
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
                    .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                        double distanceChange = 0.0;

                        try {
                            distanceChange = std::stod(args[0]);
                        }
                        catch (const std::exception& e) {
                            sendOutput("Invalid argument format.", true, {});
                            return;
                        }

                        // Update the world border
                        if (worldBorder.getDiameter() + distanceChange < 1.0) {
                            sendOutput("commands.worldborder.set.failed.small", true, {});
                            return;
                        }
                        if (worldBorder.getDiameter() + distanceChange == worldBorder.getDiameter()) {
                            sendOutput("commands.worldborder.set.failed.nochange", true, {});
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
                        sendOutput("commands.worldborder.set.immediate", false, {diameterStream.str()});
                    })
                        .argument("time", 3, true, true) // <time>: brigadier:integer (parserId=3)
                            .setIntegerRange(0, 2147483647) // Must be between 0 and 2147483647
                            .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                                double distanceChange = 0.0;
                                int timeSeconds = 0; // Default time

                                try {
                                    distanceChange = std::stod(args[0]);
                                    timeSeconds = std::stoi(args[1]);
                                }
                                catch (const std::exception& e) {
                                    sendOutput("Invalid argument format.", true, {});
                                    return;
                                }

                                // Update the world border
                                double currentDiameter = worldBorder.getDiameter();
                                if (worldBorder.getDiameter() + distanceChange < 1.0) {
                                    sendOutput("commands.worldborder.set.failed.small", true, {});
                                    return;
                                }
                                if (worldBorder.getDiameter() + distanceChange == worldBorder.getDiameter()) {
                                    sendOutput("commands.worldborder.set.failed.nochange", true, {});
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
                                    message = "commands.worldborder.set.immediate";
                                } else if (currentDiameter < worldBorder.getDiameter()) {
                                    message = "commands.worldborder.set.grow";
                                } else {
                                    message = "commands.worldborder.set.shrink";
                                }
                                sendOutput(message, false, {diameterStream.str(), std::to_string(timeSeconds)});
                            })
                        .end() // End <time> argument
                    .end() // End <distance> argument
                // [No further arguments]
                .end() // End "add" subcommand

            // Subcommand: center <pos>
            .literal("center", true, true) // /worldborder center
                .argument("pos", 11, true, true) // <pos>: minecraft:vec2 (parserId=11)
                    .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
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
                                sendOutput("Coordinates must be between ±29999984.", true, {});
                                return;
                            }
                        }
                        catch (const std::exception& e) {
                            sendOutput("Invalid position format.", true, {});
                            return;
                        }

                        // Update the world border center
                        double currentDiameter = worldBorder.getDiameter();
                        if (worldBorder.centerX == x && worldBorder.centerZ == z) {
                            sendOutput("commands.worldborder.center.failed", true, {});
                            return;
                        }
                        if (std::abs(x) + worldBorder.getDiameter() > worldBorder.biggestSize || std::abs(z) + worldBorder.getDiameter() > worldBorder.biggestSize) {
                            sendReInitializeWorldBorder(x, z, worldBorder.getDiameter() + (std::max(std::abs(x), std::abs(z)) * 2), 0, worldBorder.warningBlocks, worldBorder.warningTime);
                            sendSetBorderSize(currentDiameter);
                        } else {
                            sendSetBorderCenter(x, z);
                        }
                        std::ostringstream posXStream;
                        posXStream.precision(2);
                        posXStream << std::fixed << x;
                        std::ostringstream posZStream;
                        posZStream.precision(2);
                        posZStream << std::fixed << z;
                        sendOutput("commands.worldborder.center.success", false, {posXStream.str(), posZStream.str()});
                    })
                    .end() // End <pos> argument
                .end() // End "center" subcommand

            // Subcommand: damage
            .literal("damage", true, true) // /worldborder damage
                // Sub-subcommand: amount <damagePerBlock>
                .literal("amount", true, true) // /worldborder damage amount
                    .argument("damagePerBlock", 1, true, true) // <damagePerBlock>: brigadier:float (parserId=1)
                        .setFloatRange(0.0f, std::numeric_limits<float>::max()) // Must be >= 0.0
                        .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                            float damage = 0.0f;

                            try {
                                damage = std::stof(args[0]);
                            }
                            catch (const std::exception& e) {
                                sendOutput("Invalid damage per block format.", true, {});
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
                            sendOutput("commands.worldborder.damage.amount.success", false, {damageStream.str()});
                        })
                        .end() // End <damagePerBlock> argument
                    .end() // End "amount" sub-subcommand

                // Sub-subcommand: buffer <distance>
                .literal("buffer", true, true) // /worldborder damage buffer
                    .argument("distance", 1, true, true) // <distance>: brigadier:float (parserId=1)
                        .setFloatRange(0.0f, std::numeric_limits<float>::max()) // Must be >= 0.0
                        .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                            float bufferDistance = 0.0f;

                            try {
                                bufferDistance = std::stof(args[0]);
                            }
                            catch (const std::exception& e) {
                                sendOutput("Invalid buffer distance format.", true, {});
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
                            sendOutput("commands.worldborder.damage.buffer.success", false, {distStream.str()});
                        })
                        .end() // End <distance> argument
                    .end() // End "buffer" sub-subcommand
                .end() // End "damage" subcommand

            // Subcommand: get
            .literal("get", true, true) // /worldborder get
                .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                    std::ostringstream diameterStream;
                    diameterStream.precision(0);
                    diameterStream << std::fixed << worldBorder.getDiameter();
                    sendOutput("commands.worldborder.get", false, {diameterStream.str()});
                })
                .end() // End "get" subcommand

            // Subcommand: set <distance> [<time>]
            .literal("set", true, true) // /worldborder set
                .argument("distance", 2, true, true) // <distance>: brigadier:double (parserId=2)
                    .setDoubleRange(-59999968.0, 59999968.0) // As per specifications
                    .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                        double newDiameter = 0.0;

                        try {
                            newDiameter = std::stod(args[0]);
                        }
                        catch (const std::exception& e) {
                            sendOutput("Invalid argument format.", true, {});
                            return;
                        }

                        // Update the world border
                        if (newDiameter < 1.0) {
                            sendOutput("commands.worldborder.set.failed.small", true, {});
                            return;
                        }
                        if (newDiameter == worldBorder.getDiameter()) {
                            sendOutput("commands.worldborder.set.failed.nochange", true, {});
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
                        sendOutput("commands.worldborder.set.immediate", false, {diameterStream.str()});
                    })
                        .argument("time", 3, true, true) // <time>: brigadier:integer (parserId=3)
                            .setIntegerRange(0, 2147483647) // Must be between 0 and 2147483647
                            .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                                double newDiameter = 0.0;
                                int timeSeconds = 0;

                                try {
                                    newDiameter = std::stod(args[0]);
                                    timeSeconds = std::stoi(args[1]);
                                }
                                catch (const std::exception& e) {
                                    sendOutput("Invalid time format.", true, {});
                                    return;
                                }

                                // Update the world border
                                double currentDiameter = worldBorder.getDiameter();
                                if (newDiameter < 1.0) {
                                    sendOutput("commands.worldborder.set.failed.small", true, {});
                                    return;
                                }
                                if (newDiameter == worldBorder.getDiameter()) {
                                    sendOutput("commands.worldborder.set.failed.nochange", true, {});
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
                                    message = "commands.worldborder.set.immediate";
                                } else if (currentDiameter < worldBorder.getDiameter()) {
                                    message = "commands.worldborder.set.grow";
                                } else {
                                    message = "commands.worldborder.set.shrink";
                                }
                                sendOutput(message, false, {diameterStream.str(), std::to_string(timeSeconds)});
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
                        .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                            int warningDistance = 0;

                            try {
                                warningDistance = std::stoi(args[0]);
                            }
                            catch (const std::exception& e) {
                                sendOutput("Invalid warning distance format.", true, {});
                                return;
                            }

                            // Update the world border warning distance
                            if (worldBorder.warningBlocks == warningDistance) {
                                sendOutput("commands.worldborder.warning.distance.failed", true, {});
                                return;
                            }
                            sendSetBorderWarningDistance(warningDistance);

                            sendOutput("commands.worldborder.warning.distance.success", false, {std::to_string(warningDistance)});;
                        })
                        .end() // End <distance> argument
                    .end() // End "distance" sub-subcommand

                // Sub-subcommand: time <time>
                .literal("time", true, true) // /worldborder warning time
                    .argument("time", 3, true, true) // <time>: brigadier:integer (parserId=3)
                        .setIntegerRange(0, 2147483647) // Must be between 0 and 2147483647
                        .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                            int warningTime = 0;

                            try {
                                warningTime = std::stoi(args[0]);
                            }
                            catch (const std::exception& e) {
                                sendOutput("Invalid warning time format.", true, {});
                                return;
                            }

                            // Update the world border warning time
                            if (worldBorder.warningTime == warningTime) {
                                sendOutput("commands.worldborder.warning.time.failed", true, {});
                                return;
                            }
                            sendSetBorderWarningDelay(warningTime);

                            sendOutput("commands.worldborder.warning.time.success", false, {std::to_string(warningTime)});
                        })
                        .end() // End <time> argument
                    .end() // End "time" sub-subcommand
                .end() // End "warning" subcommand
            .end(); // End "worldborder" command
    // Bossbar Command: /bossbar <add|remove|list|set|get> [args]
    builder
        .literal("bossbar") // /bossbar
            // Subcommand: add <distance> [<time>]
            .literal("add") // /bossbar add
                .argument("id", 35) // <id>: minecraft:resource_location (parserId=35)
                    .argument("name", 17, true, true) // <name> minecraft:component (parserId=17) JSON text component
                    .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                        Bossbar bar;
                        std::string id = args[0];
                        // Test if the bossbar already exists
                        for (const auto &bossbar: bossBars | std::views::values) {
                            if (bossbar.getId() == id) {
                                sendOutput("commands.bossbar.create.failed", true, {id});
                                return;
                            }
                        }
                        const std::string& name = args[1];
                        nbt::tag_compound title = createTextComponent(name, "white");
                        const std::array<uint8_t, 16> uuid = bar.createBossbar(id, title);
                        bossBars[uuid] = bar;
                        sendOutput("commands.bossbar.create.success", false, {"[" + name + "]"});
                    })
                    .end() // End <name> argument
                .end() // End <id> argument
            .end() // End "add" subcommand
            .literal("get") // /bossbar get
                .argument("id", 35) // <id>: minecraft:resource (parserId=35)
                    .setHasSuggestions(true)
                    .suggestionIdentifier("bossbars") // Forces the client to request suggestions from the server
                    .literal("visible", true, true)
                        .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                            std::string id = args[0];
                            id = stripNamespace(id);
                            for (const auto &bossbar: bossBars | std::views::values) {
                                if (bossbar.getId() == id) {
                                    if (bossbar.isVisible()) {
                                        sendOutput("commands.bossbar.get.visible.visible", false, {colorBossbarMessage(bossbar) + "[" + bossbar.getTitleString() + "]§f"});
                                    } else {
                                        sendOutput("commands.bossbar.get.visible.hidden", false, {colorBossbarMessage(bossbar) + "[" + bossbar.getTitleString() + "]§f"});
                                    }
                                    return;
                                }
                            }
                            sendOutput("commands.bossbar.unknown", true, {id});
                        })
                    .end() // End "visible" subcommand
                    .literal("players", true, true)
                        .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                            std::string id = args[0];
                            id = stripNamespace(id);
                            for (const auto &bossbar: bossBars | std::views::values) {
                                if (bossbar.getId() == id) {
                                    if (bossbar.getOnlinePlayerCount() == 0) {
                                        sendOutput("commands.bossbar.get.players.none", false, {colorBossbarMessage(bossbar) + "[" + bossbar.getTitleString() + "]§f"});
                                    } else {
                                        sendOutput("commands.bossbar.get.players.some", false, {colorBossbarMessage(bossbar) + "[" + bossbar.getTitleString() + "]§f", std::to_string(bossbar.getPlayerCount()), bossbar.getOnlinePlayerList()});
                                    }
                                    return;
                                }
                            }
                            sendOutput("commands.bossbar.unknown", true, {id});
                        })
                    .end() // End "players" subcommand
                    .literal("max", true, true)
                        .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                            std::string id = args[0];
                            id = stripNamespace(id);
                            for (const auto &bossbar: bossBars | std::views::values) {
                                if (bossbar.getId() == id) {
                                    sendOutput("commands.bossbar.get.max", false, {colorBossbarMessage(bossbar) + "[" + bossbar.getTitleString() + "]§f", std::to_string(bossbar.getMax())});
                                    return;
                                }
                            }
                            sendOutput("commands.bossbar.unknown", true, {id});
                        })
                    .end() // End "max" subcommand
                    .literal("value", true, true)
                        .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                            std::string id = args[0];
                            id = stripNamespace(id);
                            for (const auto &bossbar: bossBars | std::views::values) {
                                if (bossbar.getId() == id) {
                                    sendOutput("commands.bossbar.get.value", false, {colorBossbarMessage(bossbar) + "[" + bossbar.getTitleString() + "]§f", std::to_string(bossbar.getValue())});
                                    return;
                                }
                            }
                            sendOutput("commands.bossbar.unknown", true, {id});
                        })
                    .end() // End "value" subcommand
                .end() // End <id> argument
            .end() // End "get" subcommand
            .literal("set") // /bossbar set
                .argument("id", 35) // <id>: minecraft:resource (parserId=35)
                    .setHasSuggestions(true)
                    .suggestionIdentifier("bossbars") // Forces the client to request suggestions from the server
                    .literal("color")
                        .argument("color", 35, true, true) // <color>: minecraft:resource (parserId=35)
                            .setHasSuggestions(true)
                            .suggestionIdentifier("bossbar_colors") // Forces the client to request suggestions from the server
                            .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                                std::string id = args[0];
                                id = stripNamespace(id);
                                std::string color = args[1];
                                BossbarColor bossbarColor = stringToBossbarColor(color);
                                for (auto &bossbar: bossBars | std::views::values) {
                                    if (bossbar.getId() == id) {
                                        if (bossbarColor == bossbar.getColor()) {
                                            sendOutput("commands.bossbar.set.color.unchanged", true, {});
                                            return;
                                        }
                                        bossbar.updateColor(bossbarColor);
                                        sendOutput("commands.bossbar.set.color.success", false, {colorBossbarMessage(bossbar) + "[" + bossbar.getTitleString() + "]§f"});
                                        return;
                                    }
                                }
                                sendOutput("commands.bossbar.unknown", true, {id});
                            })
                        .end() // End <color> argument
                    .end() // End "color" subcommand
                    .literal("max")
                        .argument("max", 3, true, true) // <max>: brigadier:integer (parserId=3)
                            .setIntegerRange(1, 2147483647)
                            .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                                std::string id = args[0];
                                id = stripNamespace(id);
                                int32_t max = std::stoi(args[1]);
                                for (auto &bossbar: bossBars | std::views::values) {
                                    if (bossbar.getId() == id) {
                                        if (max == bossbar.getMax()) {
                                            sendOutput("commands.bossbar.set.max.unchanged", true, {});
                                            return;
                                        }
                                        bossbar.updateMax(max);
                                        sendOutput("commands.bossbar.set.max.success", false, {colorBossbarMessage(bossbar) + "[" + bossbar.getTitleString() + "]§f", std::to_string(max)});
                                        return;
                                    }
                                }
                                sendOutput("commands.bossbar.unknown", true, {id});
                            })
                        .end() // End <max> argument
                    .end() // End "max" subcommand
                    .literal("name")
                        .argument("name", 17, true, true) // <name> minecraft:component (parserId=17) JSON text component
                            .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                                std::string id = args[0];
                                id = stripNamespace(id);
                                const std::string& name = args[1];
                                nbt::tag_compound title = createTextComponent(name, "white");
                                for (auto &bossbar: bossBars | std::views::values) {
                                    if (bossbar.getId() == id) {
                                        if (bossbar.getTitleString() == name) {
                                            sendOutput("commands.bossbar.set.name.unchanged", true, {});
                                            return;
                                        }
                                        bossbar.updateTitle(title);
                                        sendOutput("commands.bossbar.set.name.success", false, {colorBossbarMessage(bossbar) + "[" + bossbar.getTitleString() + "]§f"});
                                        return;
                                    }
                                }
                                sendOutput("commands.bossbar.unknown", true, {id});
                            })
                        .end() // End <name> argument
                    .end() // End "name" subcommand
                    .literal("players", true, true)
                        .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                            std::string id = args[0];
                            id = stripNamespace(id);
                            for (auto &bossbar: bossBars | std::views::values) {
                                if (bossbar.getId() == id) {
                                    if (bossbar.getPlayerCount() == 0) {
                                        sendOutput("commands.bossbar.set.players.unchanged", true, {colorBossbarMessage(bossbar) + "[" + bossbar.getTitleString() + "]§f"});
                                        return;
                                    }
                                    bossbar.removeAllPlayers();
                                    sendOutput("commands.bossbar.set.players.success.none", false, {colorBossbarMessage(bossbar) + "[" + bossbar.getTitleString() + "]§f"});
                                    return;
                                }
                            }
                            sendOutput("commands.bossbar.unknown", true, {id});
                        })
                        .argument("player", 6, true, true) // <player>: minecraft:entity (parserId=6)
                            .setEntityProperties(false, true)
                            .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                                std::string id = args[0];
                                id = stripNamespace(id);
                                std::string playerName = args[1];
                                for (auto &bossbar: bossBars | std::views::values) {
                                    if (bossbar.getId() == id) {
                                        if (bossbar.hasPlayer(playerName)) {
                                            sendOutput("commands.bossbar.set.players.unchanged", true, {});
                                            return;
                                        }
                                        bossbar.addPlayer(playerName);
                                        sendOutput("commands.bossbar.set.players.success.some", false, {colorBossbarMessage(bossbar) + "[" + bossbar.getTitleString() + "]§f", std::to_string(bossbar.getPlayerCount()), bossbar.getPlayerList()});
                                        return;
                                    }
                                }
                                sendOutput("commands.bossbar.unknown", true, {id});
                            })
                        .end() // End <player> argument
                    .end() // End "players" subcommand
                    .literal("style")
                        .argument("style", 35, true, true) // <style>: minecraft:resource (parserId=35)
                            .setHasSuggestions(true)
                            .suggestionIdentifier("bossbar_styles") // Forces the client to request suggestions from the server
                            .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                                std::string id = args[0];
                                id = stripNamespace(id);
                                std::string style = args[1];
                                BossbarDivision bossbarStyle = stringToBossbarDivision(style);
                                for (auto &bossbar: bossBars | std::views::values) {
                                    if (bossbar.getId() == id) {
                                        if (bossbarStyle == bossbar.getDivision()) {
                                            sendOutput("commands.bossbar.set.style.unchanged", true, {});
                                            return;
                                        }
                                        bossbar.updateDivision(bossbarStyle);
                                        sendOutput("commands.bossbar.set.style.success", false, {colorBossbarMessage(bossbar) + "[" + bossbar.getTitleString() + "]§f"});
                                        return;
                                    }
                                }
                                sendOutput("commands.bossbar.unknown", true, {id});
                            })
                        .end() // End <style> argument
                    .end() // End "style" subcommand
                    .literal("value")
                        .argument("value", 3, true, true) // <value>: brigadier:integer (parserId=3)
                            .setIntegerRange(0, 2147483647)
                            .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                                std::string id = args[0];
                                id = stripNamespace(id);
                                int32_t value = std::stoi(args[1]);
                                for (auto &bossbar: bossBars | std::views::values) {
                                    if (bossbar.getId() == id) {
                                        if (value == bossbar.getValue()) {
                                            sendOutput("commands.bossbar.set.value.unchanged", true, {});
                                            return;
                                        }
                                        bossbar.updateValue(value);
                                        sendOutput("commands.bossbar.set.value.success", false, {colorBossbarMessage(bossbar) + "[" + bossbar.getTitleString() + "]§f", std::to_string(value)});
                                        return;
                                    }
                                }
                                sendOutput("commands.bossbar.unknown", true, {id});
                            })
                        .end() // End <value> argument
                    .end() // End "value" subcommand
                    .literal("visible")
                        .argument("visible", 0, true, true) // <visible>: brigadier:bool (parserId=0)
                            .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                                std::string id = args[0];
                                id = stripNamespace(id);
                                bool visible = args[1] == "true";
                                for (auto &bossbar: bossBars | std::views::values) {
                                    if (bossbar.getId() == id) {
                                        if (visible == bossbar.isVisible()) {
                                            if (visible) {
                                                sendOutput("commands.bossbar.set.visibility.unchanged.visible", true, {});
                                                return;
                                            }
                                            sendOutput("commands.bossbar.set.visibility.unchanged.hidden", true, {});
                                            return;

                                        }
                                        bossbar.setVisible(visible);
                                        if (visible) {
                                            sendOutput("commands.bossbar.set.visible.success.visible", false, {colorBossbarMessage(bossbar) + "[" + bossbar.getTitleString() + "]§f"});
                                            return;
                                        }
                                        sendOutput("commands.bossbar.set.visible.success.hidden", false, {colorBossbarMessage(bossbar) + "[" + bossbar.getTitleString() + "]§f"});
                                        return;
                                    }
                                }
                                sendOutput("commands.bossbar.unknown", true, {id});
                            })
                        .end() // End <visible> argument
                    .end() // End "visible" subcommand
                .end() // End <id> argument
            .end() // End "set" subcommand
            .literal("remove") // /bossbar remove
                .argument("id", 35, true, true) // <id>: minecraft:resource (parserId=35)
                    .setHasSuggestions(true)
                    .suggestionIdentifier("bossbars") // Forces the client to request suggestions from the server
                    .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                        std::string id = args[0];
                        id = stripNamespace(id);
                        for (auto &bossbar: bossBars | std::views::values) {
                            if (bossbar.getId() == id) {
                                bossBars.erase(bossbar.getUUID());
                                sendOutput("commands.bossbar.remove.success", false, {colorBossbarMessage(bossbar) + "[" + bossbar.getTitleString() + "]§f"});
                                return;
                            }
                        }
                        sendOutput("commands.bossbar.unknown", true, {id});
                    })
                .end() // End <id> argument
            .end() // End "remove" subcommand
            .literal("list", true, true) // /bossbar list
                .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                    if (bossBars.empty()) {
                        sendOutput("commands.bossbar.list.bars.none", false, {});
                        return;
                    }
                    std::string bossbarList;
                    for (const auto &bossbar: bossBars | std::views::values) {
                        bossbarList += colorBossbarMessage(bossbar) + "[" + bossbar.getTitleString() + "]§f, ";
                    }
                    bossbarList = bossbarList.substr(0, bossbarList.size() - 2);
                    sendOutput("commands.bossbar.list.bars.some", false, {std::to_string(bossBars.size()), bossbarList});
                })
            .end() // End "list" subcommand
        .end(); // End "bossbar" command

    // Weather command: /weather <clear|rain|thunder> [duration]
    builder
        .literal("weather")
            .literal("clear", true, true)
                .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                    weather.setWeather(WeatherType::CLEAR, 0);
                    sendOutput("commands.weather.set.clear", false, {});
                })
                .argument("duration", 42, true, true) // <time> argument with parserId=42 (minecraft:time)
                    .setIntegerRange(0, 0) // Set the range of the <time> argument (only min is used)
                    .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                        // Parse the <time> argument
                        std::string timeArg = args[0];
                        int duration = 0;

                        duration = parseDuration(timeArg);

                        weather.setWeather(WeatherType::CLEAR, duration);
                        sendOutput("commands.weather.set.clear", false, {});
                    })
                .end() // End <duration> argument
            .end() // End "clear" subcommand
            .literal("rain", true, true)
                .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                    weather.setWeather(WeatherType::RAIN, 0);
                    sendOutput("commands.weather.set.rain", false, {});
                })
                .argument("duration", 42, true, true) // <time> argument with parserId=42 (minecraft:time)
                    .setIntegerRange(0, 0) // Set the range of the <time> argument (only min is used)
                    .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                        // Parse the <time> argument
                        std::string timeArg = args[0];
                        int duration = 0;

                        duration = parseDuration(timeArg);

                        weather.setWeather(WeatherType::RAIN, duration);
                        sendOutput("commands.weather.set.rain", false, {});
                    })
                .end() // End <duration> argument
            .end() // End "rain" subcommand
            .literal("thunder", true, true)
                .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                    weather.setWeather(WeatherType::THUNDER, 0);
                    sendOutput("commands.weather.set.thunder", false, {});
                })
                .argument("duration", 42, true, true) // <time> argument with parserId=42 (minecraft:time)
                    .setIntegerRange(0, 0) // Set the range of the <time> argument (only min is used)
                    .handler([](const Player* player, const std::vector<std::string>& args, const std::function<void(const std::string&, bool, const std::vector<std::string>& args)> &sendOutput) {
                        // Parse the <time> argument
                        std::string timeArg = args[0];
                        int duration = 0;

                        duration = parseDuration(timeArg);

                        weather.setWeather(WeatherType::THUNDER, duration);
                        sendOutput("commands.weather.set.thunder", false, {});
                    })
                .end() // End <duration> argument
            .end() // End "thunder" subcommand
        .end(); // End "weather" command

    // Build the command graph
    globalCommandGraph = builder.build();

    // Serialize the command graph
    CommandSerializer serializer;
    serializedCommandGraph = serializer.serializeCommands(globalCommandGraph, commandGraphNumOfNodes);
}
