#ifndef TRANSLATION_H
#define TRANSLATION_H
#include <format>
#include <string>
#include <unordered_map>

#include "core/server.h"

bool isTranslationKey(const std::string &arg);
std::unordered_map<std::string, std::unordered_map<std::string, std::string>> loadTranslations(const std::string& path);
std::string getTranslation(const std::string &key, const std::string &lang = "en_us");
std::string getTranslationString(const std::string &key, const std::string &lang, const std::vector<std::string>& args);

template<typename... Args>
std::string getTranslation(const std::string &key, const std::string &lang, Args&&... args) {
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

    // Collect args into a vector
    std::vector<std::string> argVector = { std::forward<Args>(args)... };

    // Translate arguments if they are translation keys
    std::vector<std::string> translatedArgs;
    translatedArgs.reserve(argVector.size());

    for (const auto &arg : argVector) {
        if (isTranslationKey(arg)) {
            std::string actualKey = arg.substr(10);
            translatedArgs.emplace_back(getTranslation(actualKey, lang));
        } else {
            translatedArgs.emplace_back(arg);
        }
    }

    try {
        // Format the translation string with the provided arguments
        for (size_t i = 0; i < translatedArgs.size(); ++i) {
            std::string placeholder = "{" + std::to_string(i) + "}";
            size_t pos = 0;
            while ((pos = translation.find(placeholder, pos)) != std::string::npos) {
                translation.replace(pos, placeholder.length(), translatedArgs[i]);
                pos += translatedArgs[i].length();
            }
        }
        return translation;
    } catch (const std::format_error &e) {
        logMessage("Format error for key: " + key + ", error: " + e.what(), LOG_WARNING);
        return key;
    }
}

#endif //TRANSLATION_H
