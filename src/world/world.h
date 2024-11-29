#ifndef WORLD_H
#define WORLD_H
#include <string>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <sys/file.h>
#endif

class World {
public:
    World(const std::string& worldPath);

    ~World();

    bool load();

    static bool save();

private:
    std::filesystem::path path;

#ifdef _WIN32
    HANDLE lockHandle = INVALID_HANDLE_VALUE;
#else
    int lockFd = -1;
#endif

    bool loadLevelDat() const;
    bool createDefaultLevelDat() const;

    bool loadSessionLock() const;
    bool createSessionLock() const;

    bool loadPlayerData() const;

    // Utility methods
    static bool fileExists(const std::filesystem::path& p);
    bool acquireAndVerifyLock();
    bool releaseLock();
    bool createDirectories() const;

};

#endif //WORLD_H
