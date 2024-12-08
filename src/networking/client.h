#ifndef CLIENT_H
#define CLIENT_H

#include <array>
#include <memory>
#include <mutex>
#include <unordered_map>
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

void disconnectClient(const std::shared_ptr<Player>& player, const std::string& reason, bool disconnectPacket);
void handleClient(SocketType clientSock);
void handleConsoleCommand(const std::string & command);
void miningScheduler(std::unordered_map<std::string, std::shared_ptr<Player>> &players, std::atomic<bool> &running);


#endif // CLIENT_H

