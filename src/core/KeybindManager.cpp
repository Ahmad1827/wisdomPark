#include "KeybindManager.h"
#include <fstream>
#include <sstream>

KeybindManager::KeybindManager() : configPath("keybinds.txt") {}

void KeybindManager::registerDefault(const std::string& id, const std::string& name, const std::string& category, sf::Keyboard::Key key, bool ctrl, bool shift, bool alt) {
    Keybind kb{ key, ctrl, shift, alt };
    actions[id] = { id, name, category, kb, kb };
    orderedActionIds.push_back(id);
}

void KeybindManager::init() {
    orderedActionIds.clear();
    actions.clear();

    registerDefault("tool_brush", "Brush", "Tools", sf::Keyboard::B);
    registerDefault("tool_pencil", "Pencil", "Tools", sf::Keyboard::P);
    registerDefault("tool_eraser", "Eraser", "Tools", sf::Keyboard::E);
    registerDefault("tool_fill", "Fill", "Tools", sf::Keyboard::G);
    registerDefault("tool_select", "Select", "Tools", sf::Keyboard::S);
    registerDefault("tool_move", "Move", "Tools", sf::Keyboard::V);
    registerDefault("tool_text", "Text", "Tools", sf::Keyboard::T);
    registerDefault("tool_search", "Search Actions", "Tools", sf::Keyboard::K, true);

    registerDefault("brush_presets", "Brush Presets", "Brushes", sf::Keyboard::B, false, true);

    registerDefault("proj_new", "New Project", "Project", sf::Keyboard::N, true);
    registerDefault("proj_open", "Open Project", "Project", sf::Keyboard::O, true);
    registerDefault("proj_save", "Save Project", "Project", sf::Keyboard::S, true);
    registerDefault("proj_save_as", "Save As", "Project", sf::Keyboard::S, true, true);
    registerDefault("proj_close", "Close Project", "Project", sf::Keyboard::W, true);
    registerDefault("proj_pixel_mode", "Toggle Pixel Mode", "Project", sf::Keyboard::P, true);

    registerDefault("edit_undo", "Undo", "Edit", sf::Keyboard::Z, true);
    registerDefault("edit_redo", "Redo", "Edit", sf::Keyboard::Y, true);
    registerDefault("edit_copy", "Copy", "Edit", sf::Keyboard::C, true);
    registerDefault("edit_paste", "Paste", "Edit", sf::Keyboard::V, true);

    registerDefault("sel_dup", "Duplicate Selection", "Selection", sf::Keyboard::D, true);
    registerDefault("sel_del", "Delete Selection", "Selection", sf::Keyboard::Delete);
    registerDefault("sel_deselect", "Deselect", "Selection", sf::Keyboard::Escape);
    registerDefault("sel_flip_h", "Flip Horizontal", "Selection", sf::Keyboard::H);
    registerDefault("sel_flip_v", "Flip Vertical", "Selection", sf::Keyboard::H, false, true);
    registerDefault("sel_magic_wand", "Magic Wand", "Selection", sf::Keyboard::W);
    registerDefault("sel_rotate", "Rotate Selection", "Selection", sf::Keyboard::R);
    registerDefault("sel_scale", "Scale Selection", "Selection", sf::Keyboard::R, false, true);

    registerDefault("time_next", "Next Frame", "Timeline", sf::Keyboard::Right);
    registerDefault("time_prev", "Previous Frame", "Timeline", sf::Keyboard::Left);
    registerDefault("time_add", "Add Frame", "Timeline", sf::Keyboard::Add);
    registerDefault("time_del", "Delete Frame", "Timeline", sf::Keyboard::Subtract);
    registerDefault("time_play", "Play/Pause", "Timeline", sf::Keyboard::Space);
    registerDefault("time_zoom_in", "Timeline Zoom In", "Timeline", sf::Keyboard::Equal, true);
    registerDefault("time_zoom_out", "Timeline Zoom Out", "Timeline", sf::Keyboard::Dash, true);

    registerDefault("layer_new", "New Layer", "Layers", sf::Keyboard::L, true);
    registerDefault("layer_dup", "Duplicate Layer", "Layers", sf::Keyboard::L, true, true);
    registerDefault("layer_del", "Delete Layer", "Layers", sf::Keyboard::Delete, true);
    registerDefault("layer_folder", "Layer Folder", "Layers", sf::Keyboard::L, true, false, true);
    registerDefault("layer_up", "Move Layer Up", "Layers", sf::Keyboard::RBracket);
    registerDefault("layer_down", "Move Layer Down", "Layers", sf::Keyboard::LBracket);
    registerDefault("layer_vis", "Toggle Visibility", "Layers", sf::Keyboard::V, false, true);
    registerDefault("layer_lock", "Toggle Lock", "Layers", sf::Keyboard::K, false, true);
    registerDefault("layer_persist", "Toggle Persistence", "Layers", sf::Keyboard::P, false, true);

    registerDefault("layer_merge_down", "Merge Down", "Layers", sf::Keyboard::E, true);
    registerDefault("layer_merge_vis", "Merge Visible", "Layers", sf::Keyboard::E, true, true);

    registerDefault("view_zoom_in", "Zoom In", "View", sf::Keyboard::Unknown);
    registerDefault("view_zoom_out", "Zoom Out", "View", sf::Keyboard::Unknown);
    registerDefault("view_zoom_reset", "Reset Zoom", "View", sf::Keyboard::Num0, true);
    registerDefault("view_grid", "Toggle Grid", "View", sf::Keyboard::G, true);

    registerDefault("audio_import", "Import Audio", "Audio", sf::Keyboard::M, true);

    registerDefault("export_gif", "Export GIF", "Export", sf::Keyboard::G, true, true);

    registerDefault("ui_settings", "Toggle Keybinds", "UI", sf::Keyboard::Tab);

    loadConfig();
}

std::string KeybindManager::keybindToString(const Keybind& kb) const {
    if (kb.key == sf::Keyboard::Unknown) return "MouseWheel";
    std::string res = "";
    if (kb.ctrl) res += "Ctrl+";
    if (kb.shift) res += "Shift+";
    if (kb.alt) res += "Alt+";

    if (kb.key >= sf::Keyboard::A && kb.key <= sf::Keyboard::Z) res += (char)('A' + (kb.key - sf::Keyboard::A));
    else if (kb.key >= sf::Keyboard::Num0 && kb.key <= sf::Keyboard::Num9) res += (char)('0' + (kb.key - sf::Keyboard::Num0));
    else if (kb.key == sf::Keyboard::Escape) res += "Esc";
    else if (kb.key == sf::Keyboard::Space) res += "Space";
    else if (kb.key == sf::Keyboard::Delete) res += "Del";
    else if (kb.key == sf::Keyboard::Right) res += "Right";
    else if (kb.key == sf::Keyboard::Left) res += "Left";
    else if (kb.key == sf::Keyboard::Home) res += "Home";
    else if (kb.key == sf::Keyboard::End) res += "End";
    else if (kb.key == sf::Keyboard::Add) res += "+";
    else if (kb.key == sf::Keyboard::Subtract) res += "-";
    else if (kb.key == sf::Keyboard::Equal) res += "=";
    else if (kb.key == sf::Keyboard::Dash) res += "-";
    else if (kb.key == sf::Keyboard::Tab) res += "Tab";
    else if (kb.key == sf::Keyboard::LBracket) res += "[";
    else if (kb.key == sf::Keyboard::RBracket) res += "]";
    else res += "Key";

    return res;
}

void KeybindManager::saveConfig() {
    std::ofstream file(configPath);
    if (!file.is_open()) return;
    for (const auto& id : orderedActionIds) {
        const auto& a = actions[id];
        file << id << " " << static_cast<int>(a.currentBind.key) << " " << a.currentBind.ctrl << " " << a.currentBind.shift << " " << a.currentBind.alt << "\n";
    }
}

void KeybindManager::loadConfig() {
    std::ifstream file(configPath);
    if (!file.is_open()) return;
    std::string id;
    int k, c, s, a;
    while (file >> id >> k >> c >> s >> a) {
        if (actions.find(id) != actions.end()) {
            actions[id].currentBind.key = static_cast<sf::Keyboard::Key>(k);
            actions[id].currentBind.ctrl = (c == 1);
            actions[id].currentBind.shift = (s == 1);
            actions[id].currentBind.alt = (a == 1);
        }
    }
}

void KeybindManager::restoreDefaults() {
    for (auto& pair : actions) {
        pair.second.currentBind = pair.second.defaultBind;
    }
    saveConfig();
}

bool KeybindManager::isActionTriggered(const std::string& actionId, const sf::Event& event) const {
    if (event.type != sf::Event::KeyPressed) return false;
    auto it = actions.find(actionId);
    if (it == actions.end()) return false;

    const Keybind& kb = it->second.currentBind;
    if (kb.key == sf::Keyboard::Unknown) return false;

    bool c = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::RControl);
    bool s = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);
    bool a = sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt) || sf::Keyboard::isKeyPressed(sf::Keyboard::RAlt);

    return event.key.code == kb.key && c == kb.ctrl && s == kb.shift && a == kb.alt;
}

bool KeybindManager::isActionPressed(const std::string& actionId) const {
    auto it = actions.find(actionId);
    if (it == actions.end()) return false;

    const Keybind& kb = it->second.currentBind;
    if (kb.key == sf::Keyboard::Unknown) return false;

    if (!sf::Keyboard::isKeyPressed(kb.key)) return false;

    bool c = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::RControl);
    bool s = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);
    bool a = sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt) || sf::Keyboard::isKeyPressed(sf::Keyboard::RAlt);

    return c == kb.ctrl && s == kb.shift && a == kb.alt;
}

std::string KeybindManager::getActionConflict(const Keybind& bind, const std::string& ignoreId) const {
    for (const auto& pair : actions) {
        if (pair.first != ignoreId && pair.second.currentBind == bind) {
            return pair.second.name;
        }
    }
    return "";
}

bool KeybindManager::setKeybind(const std::string& actionId, const Keybind& newBind) {
    if (actions.find(actionId) != actions.end()) {
        actions[actionId].currentBind = newBind;
        saveConfig();
        return true;
    }
    return false;
}

const std::vector<std::string>& KeybindManager::getActionOrder() const {
    return orderedActionIds;
}

const KeyAction& KeybindManager::getAction(const std::string& id) const {
    return actions.at(id);
}

std::string KeybindManager::getActionString(const std::string& id) const {
    auto it = actions.find(id);
    if (it != actions.end()) return keybindToString(it->second.currentBind);
    return "";
}