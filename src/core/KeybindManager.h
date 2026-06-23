#pragma once
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Event.hpp>
#include <string>
#include <map>
#include <vector>

struct Keybind {
    sf::Keyboard::Key key = sf::Keyboard::Unknown;
    bool ctrl = false;
    bool shift = false;
    bool alt = false;

    bool operator==(const Keybind& other) const {
        return key == other.key && ctrl == other.ctrl && shift == other.shift && alt == other.alt;
    }
};

struct KeyAction {
    std::string id = "";
    std::string name = "";
    std::string category = "";
    Keybind defaultBind{};
    Keybind currentBind{};
};

class KeybindManager {
private:
    std::map<std::string, KeyAction> actions;
    std::vector<std::string> orderedActionIds;
    std::string configPath;

    void registerDefault(const std::string& id, const std::string& name, const std::string& category, sf::Keyboard::Key key, bool ctrl = false, bool shift = false, bool alt = false);

public:
    KeybindManager();
    void init();

    void saveConfig();
    void loadConfig();
    void restoreDefaults();

    bool isActionTriggered(const std::string& actionId, const sf::Event& event) const;
    bool isActionPressed(const std::string& actionId) const;

    std::string getActionConflict(const Keybind& bind, const std::string& ignoreId = "") const;
    bool setKeybind(const std::string& actionId, const Keybind& newBind);

    const std::vector<std::string>& getActionOrder() const;
    const KeyAction& getAction(const std::string& id) const;
    std::string getActionString(const std::string& id) const;
    std::string keybindToString(const Keybind& kb) const;
};