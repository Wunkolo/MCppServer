#include "player.h"

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

void Player::serializeAdditionalData(std::vector<uint8_t>& packetData) const {
    nlohmann::json metadataJson;
    metadataJson["Flags"] = 0; // No flags set
}
