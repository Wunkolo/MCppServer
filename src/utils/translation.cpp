#include "translation.h"

#include <format>
#include <fstream>
#include <nlohmann/json.hpp>

#include "core/server.h"
#include "core/utils.h"

bool isTranslationKey(const std::string &arg) {
    return arg.find("translate.") == 0;
}

std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
loadTranslations(const std::string &path) {
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> translations;

    // Open the JSON file
    std::ifstream file(path);
    if (!file.is_open()) {
        logMessage("Failed to open languages.json file at " + path, LOG_ERROR);
        return translations; // Return empty map on failure
    }

    // Parse the JSON content
    nlohmann::json languagesJson;
    try {
        file >> languagesJson;
    } catch (const nlohmann::json::parse_error& e) {
        logMessage("Failed to parse languages.json: " + std::string(e.what()), LOG_ERROR);
        return translations; // Return empty map on parse error
    }

    // Iterate over each language in the JSON
    for (const auto& [lang, value] : languagesJson.items()) {
        // Ensure that the value is an object containing key-text pairs
        if (!value.is_object()) {
            logMessage("Invalid format for language: " + lang + ". Expected an object of key-text pairs.", LOG_WARNING);
            continue; // Skip invalid entries
        }

        // Iterate over each translation key-text pair within the language
        for (const auto& [key, text] : value.items()) {
            // Ensure that the text is a string
            if (!text.is_string()) {
                logMessage("Invalid text for key: " + key + " in language: " + lang + ". Expected a string.", LOG_WARNING);
                continue; // Skip invalid text entries
            }

            // Assign the translation string to the map
            translations[lang][key] = text.get<std::string>();
        }
    }

    return translations;
}

std::string getTranslation(const std::string &key, const std::string &lang) {
    std::string translation;

    // Check if the desired language exists
    auto langIt = translations.find(lang);
    if (langIt != translations.end()) {
        // Check if the key exists in the desired language
        auto keyIt = langIt->second.find(key);
        if (keyIt != langIt->second.end()) {
            translation = keyIt->second;
        }
    }

    // Fallback to "en_US" if translation not found
    if (translation.empty() && lang != "en_us") { // Avoid redundant lookup
        auto defaultLangIt = translations.find("en_us");
        if (defaultLangIt != translations.end()) {
            auto defaultKeyIt = defaultLangIt->second.find(key);
            if (defaultKeyIt != defaultLangIt->second.end()) {
                translation = defaultKeyIt->second;
            }
        }
    }

    // If still empty, return the key itself as a fallback
    if (translation.empty()) {
        return key;
    }

    return translation;
}

std::string getTranslationString(const std::string &key, const std::string &lang, const std::vector<std::string> &args) {
    std::string translation;

    // Check if the desired language exists
    auto langIt = translations.find(lang);
    if (langIt != translations.end()) {
        // Check if the key exists in the desired language
        auto keyIt = langIt->second.find(key);
        if (keyIt != langIt->second.end()) {
            translation = keyIt->second;
        }
    }

    // Fallback to "en_US" if translation not found
    if (translation.empty()) {
        auto defaultLangIt = translations.find("en_us");
        if (defaultLangIt != translations.end()) {
            auto defaultKeyIt = defaultLangIt->second.find(key);
            if (defaultKeyIt != defaultLangIt->second.end()) {
                translation = defaultKeyIt->second;
            }
        }
    }

    // If still empty, return the key itself as a fallback
    if (translation.empty()) {
        return key;
    }

    // Translate arguments if they are translation keys
    std::vector<std::string> translatedArgs;
    translatedArgs.reserve(args.size());

    for (const auto &arg : args) {
        if (isTranslationKey(arg)) {
            // Remove the "translate." prefix to get the actual key
            std::string actualKey = arg.substr(10); // Length of "translate." is 10
            std::string translatedArg = getTranslationString(actualKey, lang, {});
            translatedArgs.emplace_back(translatedArg);
        } else {
            translatedArgs.emplace_back(arg);
        }
    }

    try {
        // Manual placeholder replacement: {0}, {1}, etc.
        for (size_t i = 0; i < translatedArgs.size(); ++i) {
            std::string placeholder = "{" + std::to_string(i) + "}";
            size_t pos = 0;
            while ((pos = translation.find(placeholder, pos)) != std::string::npos) {
                translation.replace(pos, placeholder.length(), translatedArgs[i]);
                pos += translatedArgs[i].length();
            }
        }
        return translation;
    } catch (const std::exception &e) {
        logMessage("Format error for key: " + key + ", error: " + e.what(), LOG_WARNING);
        return key;
    }
}
