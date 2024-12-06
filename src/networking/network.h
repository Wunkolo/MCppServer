#ifndef NETWORK_H
#define NETWORK_H
#include <vector>
#include <cstdint>
#include <mutex>
#include <string>

#ifdef _WIN32
#include <winsock2.h>
typedef SOCKET SocketType;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
typedef int SocketType;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
#endif

struct SlotData;
struct ClientConnection;

int32_t readVarInt(SocketType sock);
void writeVarInt(std::vector<uint8_t>& buffer, int32_t value);
void writeString(std::vector<uint8_t>& buffer, const std::string& str);
void writeInt(std::vector<uint8_t>& buffer, int32_t value);
void writeLong(std::vector<uint8_t>& buffer, int64_t value);
void writeDouble(std::vector<uint8_t>& buffer, double value);
void writeFloat(std::vector<uint8_t>& buffer, float value);
void writeBytes(std::vector<uint8_t>& buffer, const std::vector<uint8_t>& data, const size_t length = -1);
void writeByte(std::vector<uint8_t>& buffer, int8_t value);
void writeUByte(std::vector<uint8_t>& buffer, uint8_t value);
void writeShort(std::vector<uint8_t>& buffer, int16_t value);
void writeVarLong(std::vector<uint8_t>& buffer, uint64_t value);
void writeSlotSimple(std::vector<uint8_t>& buffer, const SlotData& slot);
void writeUUID(std::vector<uint8_t>& buffer, const std::array<uint8_t, 16>& uuid);
int32_t parseVarInt(const std::vector<uint8_t>& data, size_t& index);
int64_t parseLong(const std::vector<uint8_t>& data, size_t& index);
short parseShort(const std::vector<uint8_t>& data, size_t& index);
float parseFloat(const std::vector<uint8_t>& data, size_t& index);
double parseDouble(const std::vector<uint8_t>& data, size_t& index);
std::string parseString(const std::vector<uint8_t>& data, size_t& index);
std::vector<uint8_t> parseBytes(const std::vector<uint8_t>& data, size_t& index, size_t length);
uint8_t parseByte(const std::vector<uint8_t>& data, size_t& index);
uint64_t parseVarLong(const std::vector<uint8_t>& data, size_t& index);
SlotData parseSlotData(const std::vector<uint8_t>& data, size_t& index);
bool readUnencryptedPacket(const ClientConnection& client, std::vector<uint8_t>& packetData);
bool readPacketTimeout(const ClientConnection& client, std::vector<uint8_t>& packetData, int timeout, bool unencrypted);
bool readPacket(const ClientConnection& client, std::vector<uint8_t>& packetData);
bool sendUnencryptedPacket(ClientConnection& client, const std::vector<uint8_t>& packetData);
bool sendPacket(ClientConnection& client, const std::vector<uint8_t>& packetData);
void broadcastToOthers(const std::vector<uint8_t>& packetData, const std::string& excludeUUID = "");

#endif // NETWORK_H

