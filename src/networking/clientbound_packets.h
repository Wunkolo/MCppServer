#ifndef CLIENTBOUND_PACKETS_H
#define CLIENTBOUND_PACKETS_H
#include <cstdint>

#include "client.h"
#include "enums/enums.h"
#include "registries/registry_manager.h"

struct WorldBorder;
struct EquipmentSlot;
struct MetadataEntry;
enum class GameEvent : uint8_t;
class Entity;
struct Position;
struct Player;

void sendRemoveEntityPacket(const int32_t& entityID);
void sendPlayerInfoRemove(const Player& player);
void sendRegistryDataPacket(ClientConnection& client, RegistryManager& registryManager);
void sendWorldEventPacket(ClientConnection& client, const int& worldEvent, const Position& position, const int& data);
bool sendUpdateTagsPacket(ClientConnection& client);
void sendJoinGamePacket(ClientConnection& client, int32_t entityID);
void sendSynchronizePlayerPositionPacket(ClientConnection& client, const std::shared_ptr<Player> &player);
void sendEntityRelativeMovePacket(const std::shared_ptr<Player>& player, short deltaX, short deltaY, short deltaZ);
void sendEntityLookAndRelativeMovePacket(const std::shared_ptr<Player>& player, short deltaX, short deltaY, short deltaZ, float yaw, float pitch);
void sendEntityRotationPacket(const std::shared_ptr<Player>& player);
void sendHeadRotationPacket(const std::shared_ptr<Player>& player);
void sendEntityTeleportPacket(const std::shared_ptr<Player>& player);
void sendSpawnEntityPacket(ClientConnection& client, const std::shared_ptr<Entity>& entity);
void sendEntityEventPacket(ClientConnection& client, int32_t entityID, uint8_t entityStatus);
void sendPlayerInfoUpdate(ClientConnection& targetClient, const std::vector<Player>& playersToUpdate, uint8_t actions);
void sendGameEventPacket(ClientConnection& targetClient, GameEvent event, float value);
void sendChangeGamemode(ClientConnection& client, const Player& player, Gamemode gameMode);
void sendRemoveEntitiesPacket(const std::vector<int32_t>& entityIDs);
void sendChatMessage(const std::string& message, const bool actionBar, const std::string& color, const Player& player, bool log = true);
void sendChatMessage(const std::string& message, const bool actionBar = false, const std::string& color = "white", const std::vector<Player>* players = nullptr, bool log = true);
void sendEntityEventPacket(ClientConnection& client, int32_t entityID, uint8_t entityStatus);
void sendChangeGamemode(ClientConnection& client, const Player& player, Gamemode gameMode);
void sendDisconnectionPacket(ClientConnection& client, const std::string& reason);
void sendSetCenterChunkPacket(ClientConnection& targetClient, int32_t chunkX, int32_t chunkZ);
void sendResourcePacks(ClientConnection& client);
void sendRemoveResourcePacks(ClientConnection& client, const std::vector<std::string>& uuidsToRemove = {});
bool sendKeepAlivePacket(ClientConnection& client);
void sendEntityMetadataPacket(const std::shared_ptr<Player>& player, const std::vector<MetadataEntry>& metadataEntries, int32_t entityID);
void sendEntityAnimation(const std::shared_ptr<Player> & player, EntityAnimation animation);
void sendAcknowledgeBlockChange(ClientConnection& client, size_t sequenceID);
void sendEquipmentPacket(const std::shared_ptr<Player> & player, int32_t entityID, const EquipmentSlot& slot);
void broadcastPlayerChatMessage(const std::shared_ptr<Player>& sender, const std::string& message, long timestamp, long salt, const std::vector<uint8_t>* signature, const RegistryManager& registryManager, const std::string& chatTypeIdentifier = "minecraft:chat", const std::string& targetName = "");
void sendCommandsPacket(ClientConnection& client);
void sendFinishConfigurationPacket(ClientConnection& client);
void sendKnownPacksPacket(ClientConnection& client);
void sendSetCompressionPacket(ClientConnection& client, int32_t threshold);
void sendServerLinksPacket(ClientConnection & client);
void sendServerPluginMessages(ClientConnection & client);
void sendInitializeWorldBorder(ClientConnection& client, const WorldBorder& border);
void sendTimeUpdatePacket(ClientConnection& client);

#endif //CLIENTBOUND_PACKETS_H
