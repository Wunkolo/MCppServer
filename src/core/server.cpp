#include "server.h"
#include "networking/network.h"
#include "networking/client.h"
#include "config.h"
#include <iostream>
#include <thread>

#include "commands/CommandBuilder.h"
#include "networking/clientbound_packets.h"
#include "server/query_server.h"
#include "server/rcon_server.h"
#include "world/world.h"

void tickingSystem() {
    using namespace std::chrono;
    auto tickCount = 0;

    // Calculate the duration between ticks based on ticksPerSecond
    double ticksPerSecond = static_cast<double>(serverConfig.ticksPerSecond);
    double millisecondsPerTick = 1000.0 / ticksPerSecond;

    // Convert to chrono duration with floating-point precision
    auto tickInterval = duration<double, std::milli>(millisecondsPerTick);

    auto nextTick = steady_clock::now() + tickInterval;

    while (true) {
        // Wait until the next tick
        std::this_thread::sleep_until(nextTick);
        // Increment world time
        worldTime.tick();
        if (tickCount % 20 == 0) {
            // Every second

            // Notify all connected clients about the updated time
            {
                std::lock_guard lock(connectedClientsMutex);
                for (auto &existingClient: connectedClients | std::views::values) {
                    sendTimeUpdatePacket(*existingClient);
                }
            }
        }

        // Schedule the next tick
        nextTick += tickInterval;
        tickCount++;
    }
}

void runServer() {
    auto startTime = std::chrono::system_clock::now();

    loadConfig();
    manageResourcePacks();
    buildAllCommands();

#ifdef _WIN32
    // Initialize Winsock
    WSADATA wsaData;
    int wsaStartup = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaStartup != 0) {
        logMessage("Failed to initialize Winsock.", LOG_ERROR);
        return;
    }
#endif

    SocketType serverSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSock == INVALID_SOCKET) {
        logMessage("Failed to create socket.", LOG_ERROR);
#ifdef _WIN32
        WSACleanup();
#endif
        return;
    }

    // Set socket options
    int opt = 1;
#ifdef _WIN32
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));
#else
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
#ifdef _WIN32
    serverAddr.sin_addr.s_addr = INADDR_ANY;
#else
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
#endif
    serverAddr.sin_port = htons(serverConfig.port);

    if (bind(serverSock, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        logMessage("Failed to bind socket.", LOG_ERROR);
#ifdef _WIN32
        closesocket(serverSock);
        WSACleanup();
#else
        close(serverSock);
#endif
        return;
    }

    if (listen(serverSock, SOMAXCONN) == SOCKET_ERROR) {
        logMessage("Failed to listen on socket.", LOG_ERROR);
#ifdef _WIN32
        closesocket(serverSock);
        WSACleanup();
#else
        close(serverSock);
#endif
        return;
    }

    // Load world
    auto world = new World("world");
    if (!world->load()) {
        logMessage("Failed to load world.", LOG_ERROR);
    }

    blocks = loadBlocks("../resources/blocks.json");
    biomes = loadBiomes("../resources/biomes.json");
    items = loadItems("../resources/items.json");
    itemIDs = loadItemIDs("../resources/items.json");

    auto endTime = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsedSeconds = endTime - startTime;
    logMessage("Server started in " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(elapsedSeconds).count()) + "ms.", LOG_INFO);
    logMessage("Minecraft server is running on port " + std::to_string(serverConfig.port) + ".", LOG_INFO);

    // Initialize and start the QueryServer if enabled
    std::unique_ptr<QueryServer> queryServer;
    if (serverConfig.enableQuery) {
        queryServer = std::make_unique<QueryServer>(serverConfig.queryPort);
        queryServer->start();
    }

    // Initialize and start the RCON if enabled
    if (serverConfig.enableRcon) {
        rconServer = std::make_unique<RCONServer>(serverConfig.rconPort, serverConfig.rconPassword);
        if (!rconServer->start()) {
            logMessage("Failed to start RCON server.", LOG_ERROR);
        }
    }

    // Start the console input thread
    std::thread consoleThread([&]() {
        std::string input;
        while (std::getline(std::cin, input)) {
            if (input.empty()) continue;
            handleConsoleCommand(input);
        }
    });

    // Detach the thread to allow it to run independently
    consoleThread.detach();

    std::thread tickThread(tickingSystem);
    tickThread.detach();

    while (true) {
        sockaddr_in clientAddr{};
#ifdef _WIN32
        int clientAddrLen = sizeof(clientAddr);
#else
        socklen_t clientAddrLen = sizeof(clientAddr);
#endif
        SocketType clientSock = accept(serverSock, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrLen);
        if (clientSock == INVALID_SOCKET) {
            logMessage("Failed to accept client connection.", LOG_ERROR);
            continue;
        }

        std::thread(handleClient, clientSock).detach();
    }

    if (serverConfig.enableRcon) {
        rconServer->stop();
    }

    if (queryServer) {
        queryServer->stop();
    }

#ifdef _WIN32
    closesocket(serverSock);
    WSACleanup();
#else
    close(serverSock);
#endif
}
