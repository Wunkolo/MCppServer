#ifndef QUERY_SERVER_H
#define QUERY_SERVER_H
#include <chrono>
#include <thread>
#include <unordered_map>

#include "networking/network.h"

struct ChallengeToken {
    int32_t token;
    sockaddr_in clientAddr;
    std::chrono::steady_clock::time_point timestamp;
};

class QueryServer {
public:
    QueryServer(uint16_t port);
    ~QueryServer();
    void start();
    void stop();

private:
    void listenLoop();
    void handlePacket(const uint8_t* data, size_t length, sockaddr_in clientAddr);
    void sendChallengeResponse(uint8_t type, uint32_t sessionId, const std::string& payload, sockaddr_in clientAddr) const;
    void sendStatResponse(uint32_t sessionId, int32_t challengeToken, sockaddr_in clientAddr, bool fullStat) const;

    SocketType udpSocket;
    uint16_t port;
    std::thread listenerThread;
    std::atomic<bool> running;

    // Stores challenge tokens mapped by client address
    std::unordered_map<std::string, ChallengeToken> challengeTokens;
    std::mutex tokensMutex;
};

#endif //QUERY_SERVER_H
