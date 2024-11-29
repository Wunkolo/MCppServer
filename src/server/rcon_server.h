#ifndef RCON_SERVER_H
#define RCON_SERVER_H

#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>

#ifdef _WIN32
#include <winsock2.h>
typedef SOCKET SocketType;
#else
#include <sys/socket.h>
#include <unistd.h>
typedef int SocketType;
#endif

class RCONServer {
public:
    RCONServer(int port, const std::string& password);
    ~RCONServer();

    bool start();
    void stop();

private:
    void listenLoop();
    void handleClient(SocketType clientSock) const;
    bool authenticateClient(SocketType clientSock) const;

    static void processCommand(SocketType clientSock, const std::string& command, int requestId);

    int port;
    std::string password;
    std::atomic<bool> running;
    std::thread listenThread;
    std::vector<std::thread> clientThreads;
    std::mutex clientsMutex;
};

#endif //RCON_SERVER_H
