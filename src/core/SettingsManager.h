#pragma once
#include <string>
#include <fstream>
#include <map>

struct AppSettings {
    std::string activeProvider = "none";
    std::map<std::string, std::string> apiKeys;

    bool fullscreen = true;
    bool borderless = false;
    bool vsync = true;
    int fpsLimit = 60;
    int resWidth = 1280;
    int resHeight = 720;
    bool autoBackup = true;
    bool hwAccel = true;
    int animFps = 12;
    int historySize = 30;

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
                    std::string key = line.substr(0, pos);
                    std::string val = line.substr(pos + 1);

                    if (key == "fullscreen") settings.fullscreen = (val == "1");
                    else if (key == "borderless") settings.borderless = (val == "1");
                    else if (key == "vsync") settings.vsync = (val == "1");
                    else if (key == "fpsLimit") settings.fpsLimit = std::stoi(val);
                    else if (key == "resWidth") settings.resWidth = std::stoi(val);
                    else if (key == "resHeight") settings.resHeight = std::stoi(val);
                    else if (key == "autoBackup") settings.autoBackup = (val == "1");
                    else if (key == "hwAccel") settings.hwAccel = (val == "1");
                    else if (key == "animFps") settings.animFps = std::stoi(val);
                    else if (key == "historySize") settings.historySize = std::stoi(val);
                    else settings.apiKeys[key] = val;
                }
            }
        }
        return settings;
    }

    static void saveSettings(const AppSettings& settings) {
        std::ofstream file("config.txt", std::ios::trunc);
        if (file.is_open()) {
            file << settings.activeProvider << "\n";
            file << "fullscreen=" << settings.fullscreen << "\n";
            file << "borderless=" << settings.borderless << "\n";
            file << "vsync=" << settings.vsync << "\n";
            file << "fpsLimit=" << settings.fpsLimit << "\n";
            file << "resWidth=" << settings.resWidth << "\n";
            file << "resHeight=" << settings.resHeight << "\n";
            file << "autoBackup=" << settings.autoBackup << "\n";
            file << "hwAccel=" << settings.hwAccel << "\n";
            file << "animFps=" << settings.animFps << "\n";
            file << "historySize=" << settings.historySize << "\n";

            for (const auto& pair : settings.apiKeys) {
                file << pair.first << "=" << pair.second << "\n";
            }
        }
    }
};