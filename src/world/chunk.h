#ifndef CHUNK_H
#define CHUNK_H
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "block_states.h"
#include "flatworld.h"
#include "networking/network.h"
#include "region_file.h"
#include "core/server.h"

struct Player;
constexpr int MIN_Y = -64;
constexpr int CHUNK_WIDTH = 16;
constexpr int CHUNK_HEIGHT = 384;
constexpr int CHUNK_LENGTH = 16;
constexpr int SECTION_HEIGHT = 16;
constexpr int NUM_SECTIONS = CHUNK_HEIGHT / SECTION_HEIGHT;


struct Block {
    short blockStateID;

    Block() {
        blockStateID = blocks["air"].defaultState;
    }

    explicit Block(int state) : blockStateID(state) {}
};

struct Palette {
    std::unordered_map<int32_t, uint8_t> blockStateToIndex;
    std::vector<int32_t> indexToBlockState;

    uint8_t getIndex(int32_t blockStateID);
};

struct MemChunkSection {
    bool isEmpty = true; // True if the entire section is air
    int bitsPerEntry = 4;
    int16_t blockCount = 0;
    Palette palette; // Mapping of blockStateIDs to palette indices
    std::vector<uint8_t> blockIndices; // Bit-packed indices referencing the palette
    std::vector<uint8_t> tempBlockIndices; // Temporary buffer to store block indices before bit-packing

    // Biome data
    Palette biomePalette; // Mapping of biomeIDs to palette indices
    std::vector<uint8_t> biomeIndices; // Bit-packed indices referencing the biome palette
    std::vector<uint8_t> tempBiomeIndices; // Temporary buffer before bit-packing

    Lighting lighting;

    // Method to add a block to the palette and return its index
    uint8_t getOrAddBlockIndex(int32_t blockStateID);
    // Method to set a block's palette index in blockIndices
    void setBlockIndex(int32_t index, uint8_t paletteIndex);
    // Method to get a block's palette index from blockIndices
    uint8_t getBlockIndex(int32_t index) const;
    void addBlock(int32_t blockStateID);
    void addBiome(int32_t biomeID);
    void finalize();
};

struct Chunk {
    int32_t chunkX;
    int32_t chunkZ;
    std::array<std::optional<MemChunkSection>, NUM_SECTIONS> sections;
    std::mutex mutex;
    bool dirty;
    Heightmaps heightmaps;

    Chunk(int32_t x, int32_t z) : chunkX(x), chunkZ(z), dirty(false) {}

    Block getBlock(int32_t x, int32_t y, int32_t z) const;
    void setBlock(int32_t x, int32_t y, int32_t z, int32_t blockStateID, bool adjustY = false);
    void markDirty();
};

struct ChunkCoordinates {
    int32_t chunkX;
    int32_t chunkZ;

    bool operator==(const ChunkCoordinates & other) const {
        return chunkX == other.chunkX && chunkZ == other.chunkZ;
    }
};

// Hash function for ChunkCoordinates
template <>
struct std::hash<ChunkCoordinates> {
    std::size_t operator()(const ChunkCoordinates& coords) const noexcept {
        return ((std::hash<int32_t>()(coords.chunkX) ^ (std::hash<int32_t>()(coords.chunkZ) << 1)) >> 1);
    }
};

inline std::unordered_map<ChunkCoordinates, std::shared_ptr<Chunk>, std::hash<ChunkCoordinates>> globalChunkMap;
inline std::mutex chunkMapMutex;

// Global map from ChunkCoordinates to players viewing them
inline std::unordered_map<ChunkCoordinates, std::vector<std::shared_ptr<Player>>, std::hash<ChunkCoordinates>> chunkViewersMap;
inline std::mutex chunkViewersMutex;

int32_t getLocalCoordinate(int32_t coord);
int calculateBitsPerEntry(const Palette& palette, int min = 4);
std::vector<uint8_t> encodeBlockIndices(const std::vector<uint8_t>& indices, int bitsPerEntry);
std::shared_ptr<Chunk> getChunkContainingBlock(int32_t x, int32_t y, int32_t z);
void notifyChunkUpdate(const std::shared_ptr<Chunk> & chunk, int32_t x, int32_t y, int32_t z);
void updatePlayerChunkView(const std::shared_ptr<Player> & player, int32_t oldChunkX, int32_t oldChunkZ, int32_t newChunkX, int32_t newChunkZ);
std::shared_ptr<Chunk> loadChunkFromDisk(int chunkX, int chunkZ);
std::shared_ptr<Chunk> generateFlatChunk(const FlatWorldSettings& settings, int32_t chunkX, int32_t chunkZ, int& highestY);
void sendChunkDataToPlayer(ClientConnection& client, const std::shared_ptr<Chunk>& chunk);
std::shared_ptr<Chunk> getOrLoadChunk(int32_t chunkX, int32_t chunkZ);
bool sendCurrentChunkToPlayer(ClientConnection& client, int chunkX, int chunkZ);
std::vector<ChunkCoordinates> getChunksInView(int32_t centerChunkX, int32_t centerChunkZ, int viewDistance);

#endif //CHUNK_H
