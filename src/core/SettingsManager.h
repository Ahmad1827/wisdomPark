#pragma once
#include <string>
#include <fstream>
#include <map>

struct AppSettings {
    std::string activeProvider = "none";
    std::map<std::string, std::string> apiKeys;

    bool isConfigured() const {
        return activeProvider != "none" &&
            apiKeys.count(activeProvider) > 0 &&
            apiKeys.at(activeProvider).length() > 10;
    }
};

class SettingsManager {
public:
    static AppSettings loadSettings() {
        AppSettings settings;
        std::ifstream file("config.txt");
        if (file.is_open()) {
            std::string line;
            if (std::getline(file, line)) settings.activeProvider = line;
            while (std::getline(file, line)) {
                size_t pos = line.find('=');
                if (pos != std::string::npos) {
                    settings.apiKeys[line.substr(0, pos)] = line.substr(pos + 1);
                }
            }
        }
        return settings;
    }

    static void saveSettings(const AppSettings& settings) {
        std::ofstream file("config.txt", std::ios::trunc);
        if (file.is_open()) {
            file << settings.activeProvider << "\n";
            for (const auto& pair : settings.apiKeys) {
                file << pair.first << "=" << pair.second << "\n";
            }
        }
    }
};