#include "network.h"
#include <cstring>
#include <random>
#ifdef _WIN32
#include <ws2tcpip.h>
#endif
#include <openssl/err.h>
#include <openssl/evp.h>

#include "client.h"
#include "core/config.h"
#include "core/server.h"

int32_t readVarInt(SocketType sock) {
    int32_t numRead = 0;
    int32_t result = 0;
    uint8_t readByte;
    do {
        int recvResult = recv(sock, reinterpret_cast<char*>(&readByte), 1, 0);
        if (recvResult <= 0) {
            return -1;
        }
        int32_t value = (readByte & 0b01111111);
        result |= (value << (7 * numRead));

        numRead++;
        if (numRead > 5) {
            return -1;
        }
    } while ((readByte & 0b10000000) != 0);

    return result;
}

void logicalShiftRightAssign(int32_t& value, int shift) {
    // Cast to unsigned to perform logical shift
    uint32_t unsignedValue = static_cast<uint32_t>(value);
    unsignedValue >>= shift; // Logical shift
    value = static_cast<int32_t>(unsignedValue);
}

void writeVarInt(std::vector<uint8_t>& buffer, int32_t value) {
    while (true) {
        if ((value & ~0x7F) == 0) {
            writeByte(buffer, static_cast<uint8_t>(value));
            break;
        }

        writeByte(buffer, static_cast<uint8_t>((value & 0x7F) | 0x80));
        logicalShiftRightAssign(value, 7);
    }
}

void writeString(std::vector<uint8_t>& buffer, const std::string& str) {
    writeVarInt(buffer, static_cast<int32_t>(str.size()));
    buffer.insert(buffer.end(), str.begin(), str.end());
}

void writeInt(std::vector<uint8_t>& buffer, int32_t value) {
    buffer.push_back((value >> 24) & 0xFF);
    buffer.push_back((value >> 16) & 0xFF);
    buffer.push_back((value >> 8) & 0xFF);
    buffer.push_back(value & 0xFF);
}

void writeLong(std::vector<uint8_t>& buffer, int64_t value) {
    for (int i = 7; i >= 0; --i) {
        buffer.push_back((value >> (8 * i)) & 0xFF);
    }
}

void writeDouble(std::vector<uint8_t>& buffer, double value) {
    uint64_t data;
    memcpy(&data, &value, sizeof(double));
    writeLong(buffer, data);
}

void writeFloat(std::vector<uint8_t>& buffer, float value) {
    uint32_t data;
    memcpy(&data, &value, sizeof(float));
    writeInt(buffer, data);
}

void writeBytes(std::vector<uint8_t>& buffer, const std::vector<uint8_t>& data, const size_t length) {
    if (length != -1) {
        buffer.insert(buffer.end(), data.begin(), data.begin() + length);
        return;
    }
    buffer.insert(buffer.end(), data.begin(), data.end());
}


void writeByte(std::vector<uint8_t>& buffer, uint8_t value) {
    buffer.push_back(value);
}

void writeShort(std::vector<uint8_t>& buffer, int16_t value) {
    buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
}

void writeVarLong(std::vector<uint8_t>& buffer, uint64_t value) {
    do {
        uint8_t temp = value & 0b01111111;
        value >>= 7;
        if (value != 0) {
            temp |= 0b10000000;
        }
        buffer.push_back(temp);
    } while (value != 0);
}

void writeSlotSimple(std::vector<uint8_t>& buffer, const SlotData& slot) {
    writeVarInt(buffer, slot.itemCount);
    if(slot.itemCount == 0) {
        return;
    }
    if(slot.itemId.has_value()) {
        writeVarInt(buffer, slot.itemId.value());
    } else {
        writeVarInt(buffer, -1);
    }
    writeVarInt(buffer, 0);
    writeVarInt(buffer, 0);
}

void writeUUID(std::vector<uint8_t>& buffer, const std::array<uint8_t, 16>& uuid) {
    for (int i = 0; i < 16; i += 8) {
        for (int j = 7; j >= 0; --j) {
            buffer.push_back(uuid[i + j]);
        }
    }
}

int32_t parseVarInt(const std::vector<uint8_t>& data, size_t& index) {
    int32_t value = 0;
    int32_t position = 0;

    while (true) {
        uint8_t currentByte = parseByte(data, index);
        value |= (currentByte & 0x7F) << position;

        if ((currentByte & 0x80) == 0) {
            break;
        }

        position += 7;

        if (position >= 32) {
            throw std::runtime_error("Error: VarLong is too big.");
        }
    }

    return value;
}

int64_t parseLong(const std::vector<uint8_t>& data, size_t& index) {
    if (index + 8 > data.size()) {
        // Error: Not enough data
        return 0;
    }

    uint64_t value = 0;
    for (int i = 0; i < 8; ++i) {
        value |= static_cast<uint64_t>(data[index + i]) << (8 * (7 - i));
    }

    index += 8;
    return value;
}

short parseShort(const std::vector<uint8_t>& data, size_t& index) {
    if (index + 2 > data.size()) {
        // Error: Not enough data
        return 0;
    }

    short value = (data[index] << 8) | data[index + 1];
    index += 2;
    return value;
}

float parseFloat(const std::vector<uint8_t>& data, size_t& index) {
    if (index + 4 > data.size()) {
        // Error: Not enough data
        return 0.0f;
    }

    uint32_t value = (data[index] << 24) | (data[index + 1] << 16) | (data[index + 2] << 8) | data[index + 3];
    float result;
    memcpy(&result, &value, sizeof(float));
    index += 4;
    return result;
}

double parseDouble(const std::vector<uint8_t>& data, size_t& index) {
    if (index + 8 > data.size()) {
        // Error: Not enough data
        return 0.0;
    }

    uint64_t value = 0;
    for (int i = 0; i < 8; ++i) {
        value |= static_cast<uint64_t>(data[index + i]) << (8 * (7 - i));
    }

    double result;
    memcpy(&result, &value, sizeof(double));
    index += 8;
    return result;
}

std::string parseString(const std::vector<uint8_t>& data, size_t& index) {
    int32_t length = parseVarInt(data, index);
    std::string result(data.begin() + index, data.begin() + index + length);
    index += length;
    return result;
}

std::vector<uint8_t> parseBytes(const std::vector<uint8_t>& data, size_t& index, size_t length) {
    if (index + length > data.size()) {
        // Error: Not enough data
        return {};
    }

    std::vector<uint8_t> result(data.begin() + index, data.begin() + index + length);
    index += length;
    return result;
}

uint64_t parseVarLong(const std::vector<uint8_t>& data, size_t& index) {
    int64_t value = 0;
    int32_t position = 0;

    while (true) {
        uint8_t currentByte = parseByte(data, index);
        value |= static_cast<int64_t>(currentByte & 0x7F) << position;

        if ((currentByte & 0x80) == 0) {
            break;
        }

        position += 7;

        if (position >= 64) {
            throw std::runtime_error("Error: VarLong is too big.");
        }
    }

    return value;
}

uint8_t parseByte(const std::vector<uint8_t>& data, size_t& index) {
    if (index >= data.size()) {
        // Error: Not enough data
        return 0;
    }

    return data[index++];
}

bool readUnencryptedPacket(const ClientConnection& client, std::vector<uint8_t>& packetData) {
    // Read the length prefix (VarInt)
    int32_t length = readVarInt(client.socket);
    if (length == -1) {
        // Connection closed or error
        return false;
    }

    // Read the packet payload based on the length
    packetData.resize(length);
    size_t totalRead = 0;
    while (totalRead < static_cast<size_t>(length)) {
        ssize_t bytesRead = recv(client.socket, reinterpret_cast<char*>(packetData.data()) + totalRead, length - totalRead, 0);
        if (bytesRead <= 0) {
            // Connection closed or error
            return false;
        }
        totalRead += bytesRead;
    }

    return true;
}

bool readPacketTimeout(const ClientConnection& client, std::vector<uint8_t>& packetData, int timeout, bool unencrypted) {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(client.socket, &readSet);

    timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;

    int selectResult = select(client.socket + 1, &readSet, nullptr, nullptr, &tv);
    if (selectResult == -1) {
        // Error occurred
        return false;
    }
    if (selectResult == 0) {
        // Timeout
        return false;
    }
    if (unencrypted) {
        return readUnencryptedPacket(client, packetData);
    }
    return readPacket(client, packetData);
}

bool readPacket(const ClientConnection& client, std::vector<uint8_t>& packetData) {
    if (serverConfig.enableEncryption && client.decryptCtx) {
        // Step 1: Read encrypted length prefix (VarInt up to 5 bytes)
        int32_t length = 0;
        int32_t numRead = 0;
        uint8_t decryptedByte;
        bool lengthRead = false;

        while (numRead < 5 && !lengthRead) {
            uint8_t encryptedByte;
            ssize_t bytesRead = recv(client.socket, reinterpret_cast<char*>(&encryptedByte), 1, 0);
            if (bytesRead <= 0) {
                // Connection closed or error
                return false;
            }

            // Decrypt the byte
            int outLen = 0;
            if (EVP_DecryptUpdate(client.decryptCtx, &decryptedByte, &outLen, &encryptedByte, 1) != 1) {
                logMessage("Failed to decrypt length byte", LOG_ERROR);
                ERR_print_errors_fp(stderr);
                return false;
            }

            if (outLen != 1) {
                logMessage("Unexpected decrypted length for length byte", LOG_ERROR);
                return false;
            }

            // Accumulate the length
            length |= (decryptedByte & 0x7F) << (7 * numRead);
            if ((decryptedByte & 0x80) == 0) {
                lengthRead = true;
            }
            numRead++;
        }

        if (!lengthRead) {
            logMessage("VarInt length prefix is too big", LOG_ERROR);
            return false;
        }

        // Step 2: Read encrypted payload based on the decrypted length
        std::vector<uint8_t> encryptedPayload(length);
        size_t totalRead = 0;
        while (totalRead < static_cast<size_t>(length)) {
            ssize_t bytesRead = recv(client.socket, reinterpret_cast<char*>(encryptedPayload.data()) + totalRead, length - totalRead, 0);
            if (bytesRead <= 0) {
                // Connection closed or error
                return false;
            }
            totalRead += bytesRead;
        }

        // Step 3: Decrypt the payload
        std::vector<uint8_t> decryptedPayload(length);
        int decryptedLen = 0;
        if (EVP_DecryptUpdate(client.decryptCtx, decryptedPayload.data(), &decryptedLen, encryptedPayload.data(), length) != 1) {
            logMessage("Failed to decrypt payload", LOG_ERROR);
            ERR_print_errors_fp(stderr);
            return false;
        }
        decryptedPayload.resize(decryptedLen);

        // Assign the decrypted payload to packetData
        packetData = decryptedPayload;

        // Check for compression
        if (client.compressionEnabled && serverConfig.enableCompression) {
            size_t index = 0;
            // Read the Data Length VarInt
            int32_t dataLength = parseVarInt(decryptedPayload, index);

            if (dataLength == 0) {
                // Packet is not compressed
                packetData = std::vector<uint8_t>(decryptedPayload.begin() + index, decryptedPayload.end());
            } else {
                // Packet is compressed
                // Extract compressed data
                std::vector<uint8_t> compressedData(decryptedPayload.begin() + index, decryptedPayload.end());

                try {
                    // Decompress data
                    std::vector<uint8_t> decompressedData = decompressData(compressedData);

                    // Replace packetData with decompressed data
                    packetData = decompressedData;
                } catch (const std::exception& e) {
                    logMessage("Decompression failed: " + std::string(e.what()), LOG_ERROR);
                    return false;
                }
            }
        }

        return true;
    }
    // Existing unencrypted readPacket logic
    // Read the length prefix (VarInt)
    int32_t length = 0;
    int32_t numRead = 0;
    uint8_t read;
    do {
        ssize_t bytesRead = recv(client.socket, reinterpret_cast<char*>(&read), 1, 0);
        if (bytesRead <= 0) {
            // Connection closed or error
            return false;
        }
        length |= (read & 0x7F) << (7 * numRead);
        numRead++;
        if (numRead > 5) {
            // VarInt is too big
            logMessage("VarInt is too big", LOG_ERROR);
            return false;
        }
    } while ((read & 0x80) != 0);

    // Read the packet payload based on the length
    std::vector<uint8_t> receivedData(length);
    size_t totalRead = 0;
    while (totalRead < static_cast<size_t>(length)) {
        ssize_t bytesRead = recv(client.socket, reinterpret_cast<char*>(receivedData.data()) + totalRead, length - totalRead, 0);
        if (bytesRead <= 0) {
            // Connection closed or error
            return false;
        }
        totalRead += bytesRead;
    }

    // Check for compression
    if (client.compressionEnabled && serverConfig.enableCompression) {
        size_t index = 0;
        // Read the Data Length VarInt
        int32_t dataLength = parseVarInt(receivedData, index);

        if (dataLength == 0) {
            // Packet is not compressed
            packetData = std::vector<uint8_t>(receivedData.begin() + index, receivedData.end());
        } else {
            // Packet is compressed
            // Extract compressed data
            std::vector<uint8_t> compressedData(receivedData.begin() + index, receivedData.end());

            try {
                // Decompress data
                std::vector<uint8_t> decompressedData = decompressData(compressedData);

                // Replace packetData with decompressed data
                packetData = decompressedData;
            } catch (const std::exception& e) {
                logMessage("Decompression failed: " + std::string(e.what()), LOG_ERROR);
                return false;
            }
        }
    } else {
        // No compression; assign received data directly
        packetData = receivedData;
    }

    return true;
}

std::vector<uint8_t> buildPacket(const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> packet;
    writeVarInt(packet, static_cast<int32_t>(payload.size()));
    packet.insert(packet.end(), payload.begin(), payload.end());
    return packet;
}

std::vector<uint8_t> writeLength(const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> packet;
    writeVarInt(packet, static_cast<int32_t>(payload.size()));
    return packet;
}

bool sendUnencryptedPacket(ClientConnection& client, const std::vector<uint8_t>& packetData) {
    if (client.connectionClosed) {
        return false;
    }
    std::lock_guard lock(client.sendMutex);
    std::vector<uint8_t> dataToSend = buildPacket(packetData);
    size_t totalSent = 0;
    size_t packetSize = dataToSend.size();
    const char* dataPtr = reinterpret_cast<const char*>(dataToSend.data());

    while (totalSent < packetSize) {
        ssize_t sent = send(client.socket, dataPtr + totalSent, packetSize - totalSent, 0);
        if (sent == -1) {
            logMessage("Failed to send packet: " + std::string(strerror(errno)), LOG_DEBUG);
            return false;
        }
        totalSent += sent;
    }
    return true;
}

bool sendPacket(ClientConnection& client, const std::vector<uint8_t>& packetData) {
    if (client.connectionClosed) {
        return false;
    }
    std::lock_guard<std::mutex> lock(client.sendMutex);
    std::vector<uint8_t> dataToSend;

    // Determine if compression should be applied
    bool shouldCompress = serverConfig.enableCompression &&
                          packetData.size() >= serverConfig.compressionThreshold;

    if (shouldCompress && client.compressionEnabled) {
        try {
            // Compress the (Packet ID + Data)
            std::vector<uint8_t> compressedData = compressData(packetData);

            // Create a new packet with Data Length (uncompressed size) + Compressed Data
            std::vector<uint8_t> compressedPacket;
            writeVarInt(compressedPacket, packetData.size()); // Data Length
            compressedPacket.insert(compressedPacket.end(), compressedData.begin(), compressedData.end());

            dataToSend = buildPacket(compressedPacket);
        } catch (const std::exception& e) {
            logMessage("Compression failed: " + std::string(e.what()), LOG_ERROR);
            return false;
        }
    } else if (client.compressionEnabled) {
        // No compression: Data Length is 0
        std::vector<uint8_t> uncompressedPacket;
        writeVarInt(uncompressedPacket, 0); // Data Length = 0
        uncompressedPacket.insert(uncompressedPacket.end(), packetData.begin(), packetData.end());

        // Build the final packet with length prefix
        dataToSend = buildPacket(uncompressedPacket);
    } else {
        // No compression and compression not enabled
        dataToSend = buildPacket(packetData);
    }

    // Handle encryption if enabled
    if (serverConfig.enableEncryption && client.encryptCtx) {
        // Encrypt dataToSend
        std::vector<uint8_t> encryptedData(dataToSend.size() + EVP_CIPHER_block_size(EVP_aes_128_cfb8()));
        int outLen = 0;
        if (EVP_EncryptUpdate(client.encryptCtx, encryptedData.data(), &outLen, dataToSend.data(), dataToSend.size()) != 1) {
            logMessage("Failed to encrypt packet data", LOG_ERROR);
            ERR_print_errors_fp(stderr);
            return false;
        }
        encryptedData.resize(outLen);
        dataToSend = encryptedData;
    }

    // Send the packet
    size_t totalSent = 0;
    size_t packetSize = dataToSend.size();
    const char* dataPtr = reinterpret_cast<const char*>(dataToSend.data());

    while (totalSent < packetSize) {
        ssize_t sent = send(client.socket, dataPtr + totalSent, packetSize - totalSent, 0);
        if (sent == -1) {
            logMessage("Failed to send packet: " + std::string(strerror(errno)), LOG_ERROR);
            return false;
        }
        totalSent += sent;
    }
    return true;
}

void broadcastToOthers(const std::vector<uint8_t>& packetData, const std::string& excludeUUID) {
    std::lock_guard lock(connectedClientsMutex);
    for (const auto& [uuid, client] : connectedClients) {
        if (uuid != excludeUUID) {
            sendPacket(*client, packetData);
        }
    }
}