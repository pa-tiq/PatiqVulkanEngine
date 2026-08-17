#pragma once

#include <string>
#include <unordered_map>
#include <fstream>
#include <json.hpp>

namespace pve {

class I18n {
public:
    static I18n& getInstance() {
        static I18n instance;
        return instance;
    }

    void setLanguage(const std::string& lang) {
        currentLanguage = lang;
        loadLanguage(lang);
    }

    std::string get(const std::string& key) const {
        auto it = translations.find(key);
        if (it != translations.end()) {
            return it->second;
        }
        return key; // Return key if translation not found
    }

private:
    I18n() {
        setLanguage("en"); // Default language
    }

    void loadLanguage(const std::string& lang) {
        translations.clear();
        std::string filename = "assets/i18n/" + lang + ".json";
        
        std::ifstream file(filename);
        if (!file.is_open()) {
            return; // File not found, keep empty translations
        }

        try {
            nlohmann::json jsonData;
            file >> jsonData;
            parseJsonRecursive(jsonData, "");
        } catch (const nlohmann::json::exception& e) {
            // JSON parsing error, keep empty translations
        }
    }

    void parseJsonRecursive(const nlohmann::json& json, const std::string& prefix) {
        if (json.is_object()) {
            for (auto it = json.begin(); it != json.end(); ++it) {
                std::string key = it.key();
                std::string fullKey = prefix.empty() ? key : prefix + "." + key;
                
                if (it.value().is_string()) {
                    translations[fullKey] = it.value().get<std::string>();
                } else if (it.value().is_object()) {
                    parseJsonRecursive(it.value(), fullKey);
                }
            }
        }
    }

    std::string currentLanguage;
    std::unordered_map<std::string, std::string> translations;
};

} // namespace pve
