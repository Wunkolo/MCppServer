#ifndef REGION_FILE_H
#define REGION_FILE_H
#include <array>
#include <cstdint>
#include <fstream>
#include <optional>
#include <string>
#include <tag_compound.h>
#include <unordered_map>
#include <vector>
#include <filesystem>

struct Heightmaps {
    std::unordered_map<std::string, std::vector<int64_t>> data;
};

struct Lighting {
    std::vector<uint8_t> blockLight; // 2048 bytes
    std::vector<uint8_t> skyLight;   // 2048 bytes
};

struct ChunkData {
    nbt::tag_compound nbt;
    Heightmaps heightmaps;
};

class RegionFile {
public:
    RegionFile(const std::filesystem::path &filepath, bool readOnly);

    ~RegionFile();

    // Load a chunk at local (x, z) within the region (0-31)
    std::optional<ChunkData> loadChunk(int localX, int localZ, int regionX, int regionZ);

    // Save a chunk at local (x, z) within the region (0-31)
    bool saveChunk(int localX, int localZ, int regionX, int regionZ, const ChunkData &chunk);

private:
    std::filesystem::path filepath;
    std::fstream fileStream;

    // Header data
    std::array<uint32_t, 1024> chunkOffsetTable;
    std::array<uint32_t, 1024> chunkTimestampTable;

    bool loadHeader();
    bool writeHeader();

    // Utility functions
    static int getChunkIndex(int localX, int localZ);
    std::optional<std::pair<uint32_t, uint8_t>> getChunkLocation(int localX, int localZ) const;
    bool setChunkLocation(int localX, int localZ, uint32_t offset, uint8_t sectorCount);
};

#endif //REGION_FILE_H
