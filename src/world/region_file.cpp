#include "region_file.h"

#include <iostream>
#include <sstream>
#include <tag_array.h>
#include <cstring>

#include "core/utils.h"
#include "zlib.h"
#include "io/stream_reader.h"

RegionFile::RegionFile(const std::filesystem::path& filepath, bool readOnly) : filepath(filepath) {
    // Open the file in binary read/write mode
    if (readOnly) {
        fileStream.open(filepath, std::ios::in | std::ios::binary);
    } else {
        fileStream.open(filepath, std::ios::in | std::ios::out | std::ios::binary);
    }

    // If the file doesn't exist, create it and initialize header
    if(!readOnly) {
        if (!fileStream.is_open()) {
            fileStream.clear();
            fileStream.open(filepath, std::ios::out | std::ios::binary);
            if (!fileStream) {
                logMessage("Failed to create/open region file: " + filepath.string(), LOG_ERROR);
                return;
            }

            // Initialize header with zeros
            std::array<uint8_t, 8192> emptyHeader = {0};
            fileStream.write(reinterpret_cast<const char*>(emptyHeader.data()), emptyHeader.size());
            fileStream.close();
            // Reopen in read/write mode
            fileStream.open(filepath, std::ios::in | std::ios::out | std::ios::binary);
            if (!fileStream) {
                logMessage("Failed to reopen region file after creation: " + filepath.string(), LOG_ERROR);
                return;
            }
        }
    } else {
        if (!fileStream.is_open()) {
            logMessage("Failed to open region file: " + filepath.string(), LOG_ERROR);
        }
    }

    // Load the header
    if (!loadHeader()) {
        logMessage("Failed to load header for region file: " + filepath.string(), LOG_ERROR);
    }
}

RegionFile::~RegionFile() {
    if (fileStream.is_open()) {
        // Write the header back before closing
        writeHeader();
        fileStream.close();
    }
}

bool RegionFile::loadHeader() {
    if (!fileStream.is_open()) {
        logMessage("Region file not open: " + filepath.string(), LOG_ERROR);
        return false;
    }

    // Read the first 4,096 bytes for chunk offset table
    fileStream.seekg(0, std::ios::beg);
    for (size_t i = 0; i < 1024; ++i) {
        uint32_t offset = 0;
        fileStream.read(reinterpret_cast<char*>(&offset), 4);
        // Convert from big-endian to host byte order
        offset = ((offset & 0xFF) << 16) | ((offset & 0xFF00) << 8) | ((offset & 0xFF0000) >> 8);
        chunkOffsetTable[i] = offset;
    }

    // Read the next 4,096 bytes for chunk timestamp table
    for (size_t i = 0; i < 1024; ++i) {
        uint32_t timestamp = 0;
        fileStream.read(reinterpret_cast<char*>(&timestamp), 4);
        // Convert from big-endian to host byte order
        timestamp = ((timestamp & 0xFF) << 24) | ((timestamp & 0xFF00) << 8) |
                    ((timestamp & 0xFF0000) >> 8) | ((timestamp & 0xFF000000) >> 24);
        chunkTimestampTable[i] = timestamp;
    }

    return true;
}

bool RegionFile::writeHeader() {
    if (!fileStream.is_open()) {
        logMessage("Region file not open: " + filepath.string(), LOG_ERROR);
        return false;
    }

    // Write the chunk offset table
    fileStream.seekp(0, std::ios::beg);
    for (size_t i = 0; i < 1024; ++i) {
        uint32_t offset = chunkOffsetTable[i];
        // Convert to big-endian
        uint32_t beOffset = ((offset & 0xFF0000) >> 8) |
							((offset & 0xFF00) << 8) |
							((offset & 0xFF) << 16);
        fileStream.write(reinterpret_cast<const char*>(&beOffset), 4);
    }

    // Write the chunk timestamp table
    for (size_t i = 0; i < 1024; ++i) {
        uint32_t timestamp = chunkTimestampTable[i];
        // Convert to big-endian
        uint32_t beTimestamp = ((timestamp & 0xFF000000) >> 24) |
                               ((timestamp & 0xFF0000) >> 8) |
                               ((timestamp & 0xFF00) << 8) |
                               ((timestamp & 0xFF) << 24);
        fileStream.write(reinterpret_cast<const char*>(&beTimestamp), 4);
    }

    fileStream.flush();
    return true;
}

int RegionFile::getChunkIndex(int localX, int localZ) {
    return (localZ * 32) + localX;
}

std::optional<std::pair<uint32_t, uint8_t>> RegionFile::getChunkLocation(int localX, int localZ) const {
    int index = getChunkIndex(localX, localZ);
    uint32_t offset = chunkOffsetTable[index] >> 8; // First 3 bytes
    uint8_t sectorCount = chunkOffsetTable[index] & 0xFF; // Last byte

    if (offset == 0 && sectorCount == 0) {
        return std::nullopt;
    }

    return std::make_pair(offset, sectorCount);
}

bool RegionFile::setChunkLocation(int localX, int localZ, uint32_t offset, uint8_t sectorCount) {
    int index = getChunkIndex(localX, localZ);
    chunkOffsetTable[index] = (offset << 8) | sectorCount;
    return true;
}

std::optional<ChunkData> RegionFile::loadChunk(int localX, int localZ, int regionX, int regionZ) {
    if (localX < 0 || localX >= 32 || localZ < 0 || localZ >= 32) {
        logMessage("Local chunk coordinates out of bounds: (" + std::to_string(localX) + ", " + std::to_string(localZ), LOG_ERROR);
        return std::nullopt;
    }

    int index = getChunkIndex(localX, localZ);
    uint32_t offset = chunkOffsetTable[index] >> 8; // The first 3 bytes
    uint8_t sectorCount = chunkOffsetTable[index] & 0xFF; // The last byte

    if (offset == 0 && sectorCount == 0) {
        // Chunk not present
        return std::nullopt;
    }

    // Calculate byte offset
    uint64_t byteOffset = static_cast<uint64_t>(offset) * 4096;

    // Read chunk length
    fileStream.seekg(byteOffset, std::ios::beg);
    uint32_t length = 0;
    fileStream.read(reinterpret_cast<char*>(&length), 4);
    // Convert from big-endian
    length = ((length & 0xFF) << 24) | ((length & 0xFF00) << 8) |
             ((length & 0xFF0000) >> 8) | ((length & 0xFF000000) >> 24);

    // Read compression type
    uint8_t compressionType = 0;
    fileStream.read(reinterpret_cast<char*>(&compressionType), 1);
    if (compressionType != 2) { // Only handle zlib compression
        logMessage("Unsupported compression type: " + static_cast<int>(compressionType), LOG_ERROR);
        return std::nullopt;
    }

    // Read compressed data
    size_t compressedSize = length - 1;
    std::vector<uint8_t> compressedData(compressedSize);
    fileStream.read(reinterpret_cast<char*>(compressedData.data()), compressedSize);

    // Decompress using zlib
    std::vector<uint8_t> decompressedData;
    {
        z_stream strm = {};
        strm.next_in = compressedData.data();
        strm.avail_in = compressedSize;

        if (inflateInit(&strm) != Z_OK) {
            logMessage("Failed to initialize zlib for decompression.", LOG_ERROR);
            return std::nullopt;
        }

        constexpr size_t bufferSize = 1024 * 1024; // 1 MiB buffer
        std::vector<uint8_t> buffer(bufferSize);

        int ret;
        do {
            strm.next_out = buffer.data();
            strm.avail_out = bufferSize;

            ret = inflate(&strm, Z_NO_FLUSH);
            if (ret == Z_STREAM_ERROR) {
                logMessage("Zlib stream error during decompression.", LOG_ERROR);
                inflateEnd(&strm);
                return std::nullopt;
            }

            switch (ret) {
                case Z_NEED_DICT:
                case Z_DATA_ERROR:
                case Z_MEM_ERROR:
                    logMessage("Zlib decompression error: " + ret, LOG_ERROR);
                    inflateEnd(&strm);
                    return std::nullopt;
                default: break;
            }

            size_t have = bufferSize - strm.avail_out;
            decompressedData.insert(decompressedData.end(), buffer.begin(), buffer.begin() + have);
        } while (ret != Z_STREAM_END);

        inflateEnd(&strm);
    }

    // Parse NBT data
    std::istringstream nbtStream(std::string(reinterpret_cast<char*>(decompressedData.data()), decompressedData.size()), std::ios::binary);
    nbt::tag_compound rootCompound;
    try {
        std::pair<std::string, std::unique_ptr<nbt::tag_compound>> nbtData = nbt::io::read_compound(nbtStream, endian::big);
        rootCompound = std::move(*nbtData.second);
    } catch (const std::exception& e) {
        logMessage("Failed to parse NBT data for chunk (" + std::to_string(localX) + ", " + std::to_string(localZ) + "): " + e.what(), LOG_ERROR);
        return std::nullopt;
    }

    // Extract Heightmaps
    Heightmaps heightmaps;
    if (rootCompound.has_key("Heightmaps")) {
        const nbt::tag_compound& hmTag = rootCompound["Heightmaps"].as<nbt::tag_compound>();
        for (const auto& [name, tag] : hmTag) {
            if (tag.get_type() == nbt::tag_type::Long_Array) {
                const std::vector<int64_t>& longs = tag.as<nbt::tag_long_array>().get();
                heightmaps.data[name] = longs;
            }
        }
    }

    // Populate ChunkData
    ChunkData chunkData;
    chunkData.nbt = rootCompound;
    chunkData.heightmaps = heightmaps;

    return chunkData;
}

bool RegionFile::saveChunk(int localX, int localZ, int regionX, int regionZ, const ChunkData& chunk) {
    if (localX < 0 || localX >= 32 || localZ < 0 || localZ >= 32) {
        logMessage("Local chunk coordinates out of bounds: (" + std::to_string(localX) + ", " + std::to_string(localZ), LOG_ERROR);
        return false;
    }

    int index = getChunkIndex(localX, localZ);
    uint32_t currentOffset = chunkOffsetTable[index] >> 8;
    uint8_t currentSectorCount = chunkOffsetTable[index] & 0xFF;

    // Serialize NBT data
    std::ostringstream nbtStream(std::ios::binary);
    try {
        auto nbtData = serializeNBT(chunk.nbt, false, "Chunk [" + std::to_string(regionX) + ", " + std::to_string(regionZ) + "]    in world at (" + std::to_string(localX) + ", " + std::to_string(localZ) + ")");
        nbtStream.write(reinterpret_cast<const char*>(nbtData.data()), nbtData.size());
    } catch (const std::exception& e) {
        logMessage("Failed to serialize NBT data for chunk (" + std::to_string(localX) + ", " + std::to_string(localZ) + "): " + e.what(), LOG_ERROR);
        return false;
    }
    std::string nbtDataStr = nbtStream.str();
    std::vector<uint8_t> nbtData(nbtDataStr.begin(), nbtDataStr.end());

    // Compress NBT data using zlib
    std::vector<uint8_t> compressedData;
    {
        z_stream strm = {};
        if (deflateInit(&strm, Z_BEST_COMPRESSION) != Z_OK) {
            logMessage("Failed to initialize zlib for compression.", LOG_ERROR);
            return false;
        }

        strm.next_in = nbtData.data();
        strm.avail_in = nbtData.size();

        constexpr size_t bufferSize = 1024 * 1024; // 1 MiB buffer
        std::vector<uint8_t> buffer(bufferSize);

        int ret;
        do {
            strm.next_out = buffer.data();
            strm.avail_out = bufferSize;

            ret = deflate(&strm, Z_FINISH);
            if (ret == Z_STREAM_ERROR) {
                logMessage("Zlib stream error during compression.", LOG_ERROR);
                deflateEnd(&strm);
                return false;
            }

            size_t have = bufferSize - strm.avail_out;
            compressedData.insert(compressedData.end(), buffer.begin(), buffer.begin() + have);
        } while (ret != Z_STREAM_END);

        deflateEnd(&strm);
    }

    // Determine compression type
    uint8_t compressionType = 2; // zlib

    // Prepare chunk data
    std::vector<uint8_t> chunkData;
    uint32_t chunkLength = static_cast<uint32_t>(1 + compressedData.size()); // 1 byte for compression type

    // Convert chunkLength to big-endian
    uint32_t beChunkLength = ((chunkLength & 0xFF000000) >> 24) |
                              ((chunkLength & 0x00FF0000) >> 8) |
                              ((chunkLength & 0x0000FF00) << 8) |
                              ((chunkLength & 0x000000FF) << 24);
    chunkData.resize(4 + 1 + compressedData.size());
    memcpy(chunkData.data(), &beChunkLength, 4);
    chunkData[4] = compressionType;
    memcpy(chunkData.data() + 5, compressedData.data(), compressedData.size());

    // Calculate required sectors
    size_t totalBytes = chunkData.size();
    uint8_t requiredSectors = static_cast<uint8_t>((totalBytes + 4095) / 4096); // Ceiling division

    // Find a suitable location (for simplicity, append to the end)
    fileStream.seekp(0, std::ios::end);
    uint64_t fileSize = fileStream.tellp();
    uint32_t newOffset = static_cast<uint32_t>((fileSize + 4095) / 4096); // Ceiling division

    // Update chunk offset table
    chunkOffsetTable[index] = (newOffset << 8) | requiredSectors;

    // Update chunk timestamp (current epoch time)
    uint32_t currentTimestamp = static_cast<uint32_t>(std::time(nullptr));
    chunkTimestampTable[index] = currentTimestamp;

    // Write chunk data
    uint64_t byteOffset = static_cast<uint64_t>(newOffset) * 4096;
    fileStream.seekp(byteOffset, std::ios::beg);
    fileStream.write(reinterpret_cast<const char*>(chunkData.data()), chunkData.size());

    // Pad the remaining space in the sector with zeros
    size_t paddingSize = (requiredSectors * 4096) - chunkData.size();
    if (paddingSize > 0) {
        std::vector<uint8_t> padding(paddingSize, 0);
        fileStream.write(reinterpret_cast<const char*>(padding.data()), padding.size());
    }

    fileStream.flush();

    return true;
}