#include "fetch.h"

#include "core/config.h"
#include "core/utils.h"
#define CPPHTTPLIB_OPENSSL_SUPPORT
constexpr int MAX_RETRIES = 3;
constexpr int RETRY_DELAY_MS = 1000; // 1 second

#include <iostream>
#include <string>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>
#include "cppcodec/base64_rfc4648.hpp"
#include "../thirdparty/httplib.h"

std::vector<std::string> ca_paths = {
        "/etc/ssl/certs/ca-certificates.crt",               // Debian/Ubuntu/Gentoo etc.
        "/etc/pki/tls/certs/ca-bundle.crt",                 // Fedora/RHEL 6
        "/etc/ssl/ca-bundle.pem",                           // OpenSUSE
        "/etc/pki/tls/cacert.pem",                          // OpenELEC
        "/etc/pki/ca-trust/extracted/pem/tls-ca-bundle.pem",// CentOS/RHEL 7
        "/etc/ssl/cert.pem",                                // Alpine Linux, macOS
};

std::string httpGet(const std::string& host, const std::string& path) {
    httplib::SSLClient cli(host.c_str());

#ifndef _WIN32
    bool set_ca_cert = false;
    for (const auto& path : ca_paths) {
        if (access(path.c_str(), R_OK) == 0) {
            cli.set_ca_cert_path(path.c_str());
            set_ca_cert = true;
            break;
        }
    }
    if (!set_ca_cert) {
        logMessage("No CA certificates found on system. SSL connections may fail.", LOG_ERROR);
    }
    cli.enable_server_certificate_verification(true);
    cli.set_follow_location(true);
#endif

    auto res = cli.Get(path.c_str());

    if (res && res->status == 200) {
        return res->body;
    }

    logMessage("HTTP GET request failed for " + host + path, LOG_ERROR);
    return "";
}

std::string fetchPlayerUUID(const std::string& name) {
    std::string host = "api.mojang.com";
    std::string path = "/users/profiles/minecraft/" + name;

    std::string response = httpGet(host, path);

    if (response.empty()) {
        logMessage("Failed to fetch UUID for: " + name, LOG_ERROR);
        return "";
    }

    // Parse JSON response
    nlohmann::json profileJson;
    try {
        profileJson = nlohmann::json::parse(response);
    }
    catch (const nlohmann::json::parse_error& e) {
        logMessage("JSON parse error for name: " + name + " - " + e.what(), LOG_ERROR);
        return "";
    }

    if (!profileJson.contains("id")) {
        logMessage("No UUID found in profile for name: " + name, LOG_ERROR);
        return "";
    }

    return profileJson["id"];
}

std::pair<std::string, std::string> fetchPlayerSkin(const std::string& uuid) {
    std::string host = "sessionserver.mojang.com";
    std::string path = "/session/minecraft/profile/" + uuid + "?unsigned=false";

    std::string response = httpGet(host, path);

    if (response.empty()) {
        logMessage("Failed to fetch profile for UUID: " + uuid, LOG_ERROR);
        return { "", "" };
    }

    // Parse JSON response
    nlohmann::json profileJson;
    try {
        profileJson = nlohmann::json::parse(response);
    }
    catch (const nlohmann::json::parse_error& e) {
        logMessage("JSON parse error for UUID: " + uuid + " - " + e.what(), LOG_ERROR);
        return { "", "" };
    }

    if (!profileJson.contains("properties")) {
        logMessage("No properties found in profile for UUID: " + uuid, LOG_ERROR);
        return { "", "" };
    }

    for (const auto& prop : profileJson["properties"]) {
        if (prop["name"] == "textures" && prop.contains("value")) {
            return { prop["value"], prop["signature"] };
        }
    }

    logMessage("No textures property found for UUID: " + uuid, LOG_ERROR);
    return { "", "" };
}

// Function to extract skin URL from base64-encoded textures
std::string extractSkinURL(const std::string& texturesBase64) {
    // Decode base64
    std::string decodedStr;
    try {
        decodedStr = cppcodec::base64_rfc4648::decode<std::string>(texturesBase64);
    }
    catch (const std::exception& e) {
        logMessage("Base64 decoding failed: " + std::string(e.what()), LOG_ERROR);
        return "";
    }

    // Parse JSON
    nlohmann::json texturesJson;
    try {
        texturesJson = nlohmann::json::parse(decodedStr);
    }
    catch (const nlohmann::json::parse_error& e) {
        logMessage("Failed to parse decoded textures JSON: " + std::string(e.what()), LOG_ERROR);
        return "";
    }

    if (!texturesJson.contains("textures") || !texturesJson["textures"].contains("SKIN")) {
        logMessage("No SKIN texture found.", LOG_ERROR);
        return "";
    }

    if (!texturesJson["textures"]["SKIN"].contains("url")) {
        logMessage("No URL found for SKIN texture.", LOG_ERROR);
        return "";
    }

    return texturesJson["textures"]["SKIN"]["url"].get<std::string>();
}

bool authenticatePlayer(const std::string& username, const std::string& serverHash, const std::string& ipAddress, std::string& outUUID, std::string& outName, std::pair<std::string, std::string>& skinTexturesPair) {
    httplib::Client sessionClient("sessionserver.mojang.com");

    std::string endpoint = "/session/minecraft/hasJoined?username=" + httplib::detail::encode_query_param(username) + "&serverId=" + httplib::detail::encode_query_param(serverHash);

#ifndef _WIN32
    bool set_ca_cert = false;
    for (const auto& path : ca_paths) {
        if (access(path.c_str(), R_OK) == 0) {
            sessionClient.set_ca_cert_path(path.c_str());
            set_ca_cert = true;
            break;
        }
    }
    if (!set_ca_cert) {
        logMessage("No CA certificates fouifnd on system. SSL connections may fail.", LOG_ERROR);
    }
    sessionClient.enable_server_certificate_verification(true);
    sessionClient.set_follow_location(true);
#endif

    for (int tryCount = 0; tryCount < MAX_RETRIES; ++tryCount) {
        auto res = sessionClient.Get(endpoint.c_str());

        if (res && res->status == 200) {
            // Parse JSON response
            nlohmann::json responseJson;
            try {
                responseJson = nlohmann::json::parse(res->body);
            } catch (const nlohmann::json::parse_error& e) {
                logMessage("JSON parse error for authentication response: " + std::string(e.what()), LOG_ERROR);
                return false;
            }

            if (!responseJson.contains("id") || !responseJson.contains("name")) {
                logMessage("No UUID or name found in authentication response.", LOG_ERROR);
                return false;
            }

            outUUID = responseJson["id"];
            outName = responseJson["name"];

            for (const auto& prop : responseJson["properties"]) {
                if (prop["name"] == "textures" && prop.contains("value") && prop.contains("signature")) {
                    skinTexturesPair = { prop["value"], prop["signature"] };
                }
            }
            return true;
        }
        if (res && res->status == 204) {
            // 204 No Content: Authentication failed
            logMessage("Authentication failed for player " + username + ": 204 No Content", LOG_ERROR);
            if (tryCount < MAX_RETRIES - 1) {
                logMessage("Retrying authentication (" + std::to_string(tryCount + 1) + "/" + std::to_string(MAX_RETRIES) + ")...", LOG_ERROR);
                std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY_MS));
                continue; // Retry the loop
            }
            return false;
        }
        if (res && res->status == 403) {
            // 403 Forbidden: Player has multiplayer disabled or is banned
            try {
                auto jsonResponse = nlohmann::json::parse(res->body);
                logMessage("Authentication Error: " + jsonResponse.at("error").get<std::string>(), LOG_ERROR);
            } catch (...) {
                logMessage("Authentication Error: 403 Forbidden", LOG_ERROR);
            }
            return false;
        }
        // Handle other HTTP statuses
        if (res) {
            logMessage("Authentication failed for player " + username + ". HTTP Status: " + std::to_string(res->status), LOG_ERROR);
        } else {
            logMessage("Authentication request failed: No response from session server.", LOG_ERROR);
        }
        return false;
    }

    // If all retries are exhausted
    logMessage("Authentication failed for player " + username + " after " + std::to_string(MAX_RETRIES) + " attempts.", LOG_ERROR);
    return false;
}

std::vector<std::string> fetchMojangPublicKeys() {
    httplib::Client cli("https://api.minecraftservices.com");
    auto res = cli.Get("/publickeys");

    if (!res || res->status != 200) {
        // Handle HTTP request failure
        logMessage("Failed to fetch Mojang public keys.", LOG_ERROR);
        return {};
    }

    // Parse JSON response
    nlohmann::json jsonResponse = nlohmann::json::parse(res->body);

    std::vector<std::string> publicKeys;
    for (const auto &keyObj : jsonResponse["playerCertificateKeys"]) {
        publicKeys.push_back(keyObj["publicKey"].get<std::string>());
    }
    for (const auto &keyObj : jsonResponse["profilePropertyKeys"]) {
        publicKeys.push_back(keyObj["publicKey"].get<std::string>());
    }

    return publicKeys;
}

bool validateResourcePackURL(const ResourcePack& pack) {
    try {
        ParsedURL parsed = parseURL(pack.url);
        httplib::Client cli(parsed.host.c_str());

        auto res = cli.Head(parsed.path.c_str());
        if (res && res->status == 200) {
            logMessage("Resource pack URL is valid: " + pack.url, LOG_DEBUG);
            return true;
        }
        logMessage("Failed to validate resource pack URL: " + pack.url + " Status: " + std::to_string(res ? res->status : 0), LOG_WARNING);
        return false;
    } catch (const std::exception& e) {
        logMessage(std::string("Exception during URL validation: ") + e.what(), LOG_ERROR);
        return false;
    }
}

bool downloadResourcePack(const ResourcePack& pack, const std::string& downloadPath) {
    try {
        ParsedURL parsed = parseURL(pack.url);
        httplib::Client cli(parsed.host.c_str());

        std::ofstream file(downloadPath, std::ios::binary);
        if (!file) {
            logMessage("Failed to create file for resource pack: " + downloadPath, LOG_ERROR);
            return false;
        }

        // Stream the response directly into the file
        bool success = false;
        auto res = cli.Get(parsed.path.c_str());
        if (res->status == 200 && res->body.size() > 0) {
            file.write(res->body.data(), res->body.size());
            success = true;
            logMessage("Successfully downloaded resource pack: " + pack.url, LOG_DEBUG);
        } else {
            logMessage("Failed to download resource pack: " + pack.url + " Status: " + std::to_string(res->status), LOG_ERROR);
        }

        file.close();
        return success;
    } catch (const std::exception& e) {
        logMessage(std::string("Exception during resource pack download: ") + e.what(), LOG_ERROR);
        return false;
    }
}