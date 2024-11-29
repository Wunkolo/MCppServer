#ifndef FETCH_H
#define FETCH_H
#include <string>
#include <vector>

struct ResourcePack;

struct ParsedURL {
    std::string protocol;
    std::string host;
    int port;
    std::string path;
};

std::string fetchPlayerUUID(const std::string& name);
std::pair<std::string, std::string> fetchPlayerSkin(const std::string& uuid);
std::string extractSkinURL(const std::string& texturesBase64);
bool authenticatePlayer(const std::string& username, const std::string& serverHash, const std::string& ipAddress, std::string& outUUID, std::string& outName, std::pair<std::string, std::string>& skinTexturesPair);
std::vector<std::string> fetchMojangPublicKeys();
bool validateResourcePackURL(const ResourcePack& pack);
bool downloadResourcePack(const ResourcePack& pack, const std::string& downloadPath);

#endif //FETCH_H
