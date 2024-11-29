#include "world.h"

#include <iostream>
#include <fstream>
#include <chrono>
#ifdef _WIN32
#include <io.h>
#endif
#include <tag_string.h>
#include <io/stream_reader.h>

#include "chunk.h"
#include "core/server.h"
#include "core/utils.h"
#include "tag_primitive.h"

World::World(const std::string& worldPath) : path(worldPath) {}

World::~World() {
    releaseLock();
}

bool World::fileExists(const std::filesystem::path& p) {
    return exists(p);
}

bool World::acquireAndVerifyLock() {
    std::filesystem::path sessionLockPath = path / "session.lock";

#ifdef _WIN32
    // Open or create the session.lock file
    lockHandle = CreateFileA(
        sessionLockPath.string().c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0, // No sharing
        nullptr,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (lockHandle == INVALID_HANDLE_VALUE) {
        logMessage("Failed to open session.lock.", LOG_ERROR);
        return false;
    }

    // Try to acquire an exclusive lock
    OVERLAPPED overlapped = {0};
    if (!LockFileEx(lockHandle, LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &overlapped)) {
        logMessage("Failed to acquire session.lock.", LOG_ERROR);
        CloseHandle(lockHandle);
        lockHandle = INVALID_HANDLE_VALUE;
        return false;
    }

    // Determine if the file is newly created by checking its size
    DWORD fileSize = GetFileSize(lockHandle, nullptr);
    if (fileSize == 0) {
        // Write the ☃ character
        constexpr char lockChar[] = "\xE2\x98\x83"; // ☃ in UTF-8
        DWORD bytesWritten;
        SetFilePointer(lockHandle, 0, nullptr, FILE_BEGIN);
        if (!WriteFile(lockHandle, lockChar, sizeof(lockChar) - 1, &bytesWritten, nullptr)) {
            logMessage("Failed to write to session.lock.", LOG_ERROR);
            UnlockFileEx(lockHandle, 0, MAXDWORD, MAXDWORD, &overlapped);
            CloseHandle(lockHandle);
            lockHandle = INVALID_HANDLE_VALUE;
            return false;
        }
    } else {
        // Verify the existing session.lock content
        SetFilePointer(lockHandle, 0, nullptr, FILE_BEGIN);
        char c1, c2, c3;
        DWORD bytesRead;
        if (!ReadFile(lockHandle, &c1, 1, &bytesRead, nullptr) || bytesRead != 1 ||
            !ReadFile(lockHandle, &c2, 1, &bytesRead, nullptr) || bytesRead != 1 ||
            !ReadFile(lockHandle, &c3, 1, &bytesRead, nullptr) || bytesRead != 1) {
            logMessage("Failed to read session.lock content.", LOG_ERROR);
            UnlockFileEx(lockHandle, 0, MAXDWORD, MAXDWORD, &overlapped);
            CloseHandle(lockHandle);
            lockHandle = INVALID_HANDLE_VALUE;
            return false;
        }

        if (static_cast<unsigned char>(c1) != 0xE2 || static_cast<unsigned char>(c2) != 0x98 || static_cast<unsigned char>(c3) != 0x83) {
            logMessage("Invalid session.lock content.", LOG_ERROR);
            UnlockFileEx(lockHandle, 0, MAXDWORD, MAXDWORD, &overlapped);
            CloseHandle(lockHandle);
            lockHandle = INVALID_HANDLE_VALUE;
            return false;
        }
        logMessage("Loaded existing session.lock.", LOG_DEBUG);
    }

#else
    // POSIX implementation using flock
    lockFd = open(sessionLockPath.c_str(), O_RDWR | O_CREAT, 0666);
    if (lockFd == -1) {
        logMessage("Failed to open session.lock.", LOG_ERROR);
        return false;
    }

    if (flock(lockFd, LOCK_EX | LOCK_NB) != 0) {
        logMessage("Failed to acquire session.lock (already locked).", LOG_ERROR);
        close(lockFd);
        lockFd = -1;
        return false;
    }

    // Determine if the file is newly created by checking its size
    off_t fileSize = lseek(lockFd, 0, SEEK_END);
    if (fileSize == 0) {
        // Write the ☃ character
        const char lockChar[] = "\xE2\x98\x83"; // ☃ in UTF-8
        if (write(lockFd, lockChar, sizeof(lockChar) - 1) != sizeof(lockChar) - 1) {
            logMessage("Failed to write to session.lock.", LOG_ERROR);
            flock(lockFd, LOCK_UN);
            close(lockFd);
            lockFd = -1;
            return false;
        }
    } else {
        // Verify the existing session.lock content
        lseek(lockFd, 0, SEEK_SET);
        char c1, c2, c3;
        ssize_t bytesRead = read(lockFd, &c1, 1);
        if (bytesRead != 1) {
            logMessage("Failed to read session.lock content.", LOG_ERROR);
            flock(lockFd, LOCK_UN);
            close(lockFd);
            lockFd = -1;
            return false;
        }
        bytesRead = read(lockFd, &c2, 1);
        if (bytesRead != 1) {
            logMessage("Failed to read session.lock content.", LOG_ERROR);
            flock(lockFd, LOCK_UN);
            close(lockFd);
            lockFd = -1;
            return false;
        }
        bytesRead = read(lockFd, &c3, 1);
        if (bytesRead != 1) {
            logMessage("Failed to read session.lock content.", LOG_ERROR);
            flock(lockFd, LOCK_UN);
            close(lockFd);
            lockFd = -1;
            return false;
        }

        if (static_cast<unsigned char>(c1) != 0xE2 || static_cast<unsigned char>(c2) != 0x98 || static_cast<unsigned char>(c3) != 0x83) {
            logMessage("Invalid session.lock content.", LOG_ERROR);
            flock(lockFd, LOCK_UN);
            close(lockFd);
            lockFd = -1;
            return false;
        }

        logMessage("Loaded existing session.lock.", LOG_DEBUG);
    }

#endif

    return true;
}

bool World::releaseLock() {
#ifdef _WIN32
    if (lockHandle != INVALID_HANDLE_VALUE) {
        // Unlock the file
        OVERLAPPED overlapped = {0};
        UnlockFileEx(lockHandle, 0, MAXDWORD, MAXDWORD, &overlapped);
        CloseHandle(lockHandle);
        lockHandle = INVALID_HANDLE_VALUE;
    }
#else
    if (lockFd != -1) {
        // Unlock and close the file
        flock(lockFd, LOCK_UN);
        close(lockFd);
        lockFd = -1;
    }
#endif
    return true;
}

bool World::createDirectories() const {
    try {
        // List of required directories
        create_directories(path / "playerdata");
        create_directories(path / "stats");
        create_directories(path / "advancements");
        create_directories(path / "data");
        create_directories(path / "DIM-1" / "data");
        create_directories(path / "DIM1" / "data");
        create_directories(path / "datapacks");
        create_directories(path / "entities");
        create_directories(path / "region");

    } catch (const std::filesystem::filesystem_error& e) {
        logMessage("Error creating directories: " + std::string(e.what()), LOG_ERROR);
        return false;
    }
    return true;
}

bool World::load() {
    if (!createDirectories()) {
        return false;
    }

    if (!acquireAndVerifyLock()) {
        return false;
    }

    bool success = true;
    success &= loadPlayerData();
    success &= loadLevelDat();

    return success;
}

bool World::save() {
    // TODO: Implement saving logic
    return true;
}

bool World::loadLevelDat() const {
    std::filesystem::path levelDatPath = path / "level.dat";
    if (fileExists(levelDatPath)) {
        // Load existing level.dat
        std::ifstream file(levelDatPath, std::ios::binary);
        if (!file) {
            logMessage("Failed to open level.dat for reading.", LOG_ERROR);
            return false;
        }

        std::vector<uint8_t> compressedData;
        compressedData.assign(std::istreambuf_iterator(file), std::istreambuf_iterator<char>());
        if (compressedData.empty()) {
            logMessage("level.dat is empty.", LOG_ERROR);
            return false;
        }
        std::vector<uint8_t> decompressedData = decompressGZip(compressedData);
        std::istringstream stream(std::string(decompressedData.begin(), decompressedData.end()), std::ios::binary);

        std::pair<std::string, std::shared_ptr<nbt::tag_compound>> levelDat = nbt::io::read_compound(stream, endian::big);
        if (levelDat.second == nullptr) {
            logMessage("Failed to parse level.dat.", LOG_ERROR);
            return false;
        }
        nbt::tag_compound nbt = *(levelDat.second);
        if(nbt.has_key("Data")) {
            auto& data = nbt.at("Data").as<nbt::tag_compound>();
            spawnPosition.x = data.at("SpawnX").as<nbt::tag_int>().get();
            spawnPosition.y = data.at("SpawnY").as<nbt::tag_int>().get();
            spawnPosition.z = data.at("SpawnZ").as<nbt::tag_int>().get();

        } else {
            logMessage("level.dat is missing the Data tag.", LOG_ERROR);
            return false;
        }
    } else {
        // Create default level.dat
        if (!createDefaultLevelDat()) {
            logMessage("Failed to create default level.dat.", LOG_ERROR);
            return false;
        }
       logMessage("Created default level.dat.", LOG_DEBUG);
    }
    return true;
}

bool World::createDefaultLevelDat() const {
    std::filesystem::path levelDatPath = path / "level.dat";
    nbt::tag_compound nbt;

    // TODO: Construct the default NBT structure
    // minimal example
    nbt["LevelName"] = nbt::tag_string("New World");

    // Serialize to file
    std::ofstream file(levelDatPath, std::ios::binary);
    if (!file) {
        logMessage("Failed to open level.dat for writing.", LOG_ERROR);
        return false;
    }
    std::vector<uint8_t> serializedNBT = serializeNBT(nbt, false, "Data");
    if (serializedNBT.empty()) {
        logMessage("Serialized NBT data is empty.", LOG_ERROR);
        return false;
    }
    std::vector<uint8_t> compressedNBT = compressGZip(serializedNBT);
    file.write(reinterpret_cast<const char*>(compressedNBT.data()), compressedNBT.size());
    if (!file) {
        logMessage("Failed to write serialized NBT data to level.dat.", LOG_ERROR);
        return false;
    }

    file.close();
    return true;
}

bool World::loadSessionLock() const {
    std::filesystem::path sessionLockPath = path / "session.lock";
    if (fileExists(sessionLockPath)) {
        // Load existing session.lock
        std::ifstream file(sessionLockPath, std::ios::binary);
        if (!file) {
            logMessage("Failed to open session.lock for reading.", LOG_ERROR);
            return false;
        }
        char c;
        file.get(c);
        if (c != '\xE2' || file.get() != '\x98' || file.get() != '\x83') { // ☃ in UTF-8
            logMessage("Invalid session.lock content.", LOG_ERROR);
            return false;
        }
    } else {
        // Create session.lock
        if (!createSessionLock()) {
            logMessage("Failed to create session.lock.", LOG_ERROR);
            return false;
        }
        logMessage("Created session.lock.", LOG_DEBUG);
    }
    return true;
}

bool World::createSessionLock() const {
    std::filesystem::path sessionLockPath = path / "session.lock";
    std::ofstream file(sessionLockPath, std::ios::binary);
    if (!file) {
        logMessage("Failed to open session.lock for writing.", LOG_ERROR);
        return false;
    }
    // Write the single character ☃ in UTF-8
    file << "\xE2\x98\x83";
    if (!file) {
        logMessage("Failed to write to session.lock.", LOG_ERROR);
        return false;
    }
    return true;
}

bool World::loadPlayerData() const {
    std::filesystem::path playerDataPath = path / "playerdata";
    if (!exists(playerDataPath)) {
        logMessage("playerdata directory does not exist.", LOG_ERROR);
        return false;
    }

    // Iterate through all .dat files in playerdata
    for (const auto& entry : std::filesystem::directory_iterator(playerDataPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".dat") {
            std::ifstream file(entry.path(), std::ios::binary);
            if (!file) {
                logMessage("Failed to open " + entry.path().string() + " for reading.", LOG_ERROR);
                continue;
            }
            std::vector<uint8_t> compressedData;
            compressedData.assign(std::istreambuf_iterator(file), std::istreambuf_iterator <char>());
            if (compressedData.empty()) {
                logMessage(entry.path().string() + " is empty.", LOG_ERROR);
                continue;
            }
            std::vector<uint8_t> decompressedData = decompressGZip(compressedData);
            std::istringstream stream(std::string(decompressedData.begin(), decompressedData.end()), std::ios::binary);
            std::pair<std::string, std::shared_ptr<nbt::tag_compound>> nbt = nbt::io::read_compound(stream, endian::big);
            if (nbt.second == nullptr) {
                logMessage("Failed to parse " + entry.path().string(), LOG_ERROR);
                continue;
            }
            // Extract player UUID from filename or NBT data
            std::string uuid = entry.path().stem().string();
            // TODO: Store player data
            // playerData[uuid] = nbt;
            logMessage("Loaded player data for UUID: " + uuid, LOG_DEBUG);
        }
    }
    return true;
}
