//
// Created by noah1 on 13.11.2024.
//

#ifndef CLIENT_H
#define CLIENT_H

#include <array>
#include <mutex>
#include <unordered_set>
#include <openssl/types.h>

#include "network.h"

struct Player;

enum class ClientState {
    Handshake,
    Status,
    Login,
    Configuration,
    Play,
    AwaitingTeleportConfirm,
};

struct ClientConnection {
    SocketType socket;
    ClientState state;

    EVP_CIPHER_CTX* encryptCtx;
    EVP_CIPHER_CTX* decryptCtx;
    // Shared secret
    std::array<uint8_t, 16> sharedSecret;

    bool compressionEnabled = false;
    std::mutex sendMutex;

    // For teleport confirmation tracking
    std::mutex mutex;
    std::unordered_set<int32_t> pendingTeleportIDs;
    bool connectionClosed = false;
    int64_t keepAliveID = 0;
};

void disconnectClient(const Player& player, const std::string& reason, bool disconnectPacket);
void handleClient(SocketType clientSock);
void handleConsoleCommand(const std::string & command);


#endif // CLIENT_H

