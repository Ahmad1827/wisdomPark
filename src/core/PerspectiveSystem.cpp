#include "PerspectiveSystem.h"
#include <cmath>
#include <algorithm>

VanishingPoint::VanishingPoint(sf::Vector2f pos, std::string n)
    : position(pos), locked(false), visible(true), color(sf::Color(0, 191, 255, 180)), name(n) {}

PerspectiveGuide::PerspectiveGuide()
    : density(20), thickness(1.0f), guideColor(sf::Color(255, 255, 255, 60)), activeColor(sf::Color(0, 255, 100, 200)),
    brushSnap(true), shapeSnap(true), transformSnap(true), selectionSnap(true),
    visible(true), locked(false) {}

PerspectiveConfig::PerspectiveConfig(std::string n, PerspectiveMode m)
    : name(n), mode(m) {}

sf::Vector2f PerspectiveSnapper::snapLine(sf::Vector2f startPos, sf::Vector2f currentPos, const PerspectiveConfig& config, int& outActiveVPIndex) {
    if (!config.guideSettings.brushSnap) return currentPos;

    sf::Vector2f dir = currentPos - startPos;
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (len < 0.1f) {
        outActiveVPIndex = -1;
        return currentPos;
    }

    float currentAngle = std::atan2(dir.y, dir.x);
    float minAngleDiff = 999.0f;
    sf::Vector2f bestSnap = currentPos;
    outActiveVPIndex = -1;

    std::vector<std::pair<sf::Vector2f, int>> validDirs;

    for (size_t i = 0; i < config.vps.size(); ++i) {
        if (!config.vps[i].visible) continue;
        sf::Vector2f vpDir = startPos - config.vps[i].position;
        float vpLen = std::sqrt(vpDir.x * vpDir.x + vpDir.y * vpDir.y);
        if (vpLen > 0.1f) {
            validDirs.push_back({ vpDir / vpLen, static_cast<int>(i) });
            validDirs.push_back({ -vpDir / vpLen, static_cast<int>(i) });
        }
    }

    if (config.mode == PerspectiveMode::OnePoint || config.mode == PerspectiveMode::TwoPoint) {
        validDirs.push_back({ sf::Vector2f(0, 1), -1 });
        validDirs.push_back({ sf::Vector2f(0, -1), -1 });
        if (config.mode == PerspectiveMode::OnePoint) {
            validDirs.push_back({ sf::Vector2f(1, 0), -1 });
            validDirs.push_back({ sf::Vector2f(-1, 0), -1 });
        }
    }
    else if (config.mode == PerspectiveMode::Isometric) {
        float a30 = 30.0f * 3.14159f / 180.0f;
        float a150 = 150.0f * 3.14159f / 180.0f;
        validDirs.push_back({ sf::Vector2f(std::cos(a30), std::sin(a30)), -1 });
        validDirs.push_back({ sf::Vector2f(-std::cos(a30), -std::sin(a30)), -1 });
        validDirs.push_back({ sf::Vector2f(std::cos(a150), std::sin(a150)), -1 });
        validDirs.push_back({ sf::Vector2f(-std::cos(a150), -std::sin(a150)), -1 });
        validDirs.push_back({ sf::Vector2f(0, 1), -1 });
        validDirs.push_back({ sf::Vector2f(0, -1), -1 });
    }

    for (const auto& vd : validDirs) {
        float ang = std::atan2(vd.first.y, vd.first.x);
        float diff = std::abs(currentAngle - ang);
        while (diff > 3.14159f) diff -= 2.0f * 3.14159f;
        diff = std::abs(diff);

        if (diff < minAngleDiff) {
            minAngleDiff = diff;
            float dot = dir.x * vd.first.x + dir.y * vd.first.y;
            bestSnap = startPos + vd.first * dot;
            outActiveVPIndex = vd.second;
        }
    }

    if (minAngleDiff > 0.26f) {
        outActiveVPIndex = -1;
        return currentPos;
    }

    return bestSnap;
}

sf::Vector2f PerspectiveSnapper::snapToPixelGrid(sf::Vector2f pos) {
    return sf::Vector2f(std::round(pos.x), std::round(pos.y));
}

void PerspectiveRenderer::render(sf::RenderWindow& window, const PerspectiveConfig& config, const sf::Transform& canvasTransform, sf::Vector2u canvasSize, int hoveredVPIndex, int activeVPIndex) {
    if (!config.guideSettings.visible) return;

    sf::RenderStates states;
    states.transform = canvasTransform;
    float diagonal = std::sqrt(canvasSize.x * canvasSize.x + canvasSize.y * canvasSize.y) * 4.0f;

    for (size_t i = 0; i < config.vps.size(); ++i) {
        if (!config.vps[i].visible) continue;
        sf::Color gColor = (i == static_cast<size_t>(activeVPIndex)) ? config.guideSettings.activeColor : config.guideSettings.guideColor;

        if (config.guideSettings.density > 0) {
            float step = 360.0f / config.guideSettings.density;
            for (float a = 0; a < 360.0f; a += step) {
                float rad = a * 3.14159f / 180.f;
                sf::Vector2f endPoint = config.vps[i].position + sf::Vector2f(std::cos(rad), std::sin(rad)) * diagonal;

                sf::VertexArray line(sf::Lines, 2);
                line[0].position = config.vps[i].position;
                line[0].color = gColor;
                line[1].position = endPoint;
                line[1].color = gColor;
                window.draw(line, states);
            }
        }
    }

    if (config.mode == PerspectiveMode::Isometric) {
        sf::Color gColor = config.guideSettings.guideColor;
        float a30 = 30.0f * 3.14159f / 180.0f;
        float a150 = 150.0f * 3.14159f / 180.0f;
        sf::Vector2f dir1(std::cos(a30), std::sin(a30));
        sf::Vector2f dir2(std::cos(a150), std::sin(a150));
        sf::Vector2f dir3(0.0f, 1.0f);

        float spacing = 100.0f;
        if (config.guideSettings.density > 0) spacing = 1000.0f / config.guideSettings.density;

        sf::Vector2f center(canvasSize.x / 2.0f, canvasSize.y / 2.0f);
        for (int i = -20; i <= 20; ++i) {
            sf::VertexArray l1(sf::Lines, 2), l2(sf::Lines, 2), l3(sf::Lines, 2);
            sf::Vector2f off1 = center + dir2 * (i * spacing);
            l1[0].position = off1 - dir1 * diagonal; l1[0].color = gColor;
            l1[1].position = off1 + dir1 * diagonal; l1[1].color = gColor;

            sf::Vector2f off2 = center + dir1 * (i * spacing);
            l2[0].position = off2 - dir2 * diagonal; l2[0].color = gColor;
            l2[1].position = off2 + dir2 * diagonal; l2[1].color = gColor;

            sf::Vector2f off3 = center + dir1 * (i * spacing);
            l3[0].position = off3 - dir3 * diagonal; l3[0].color = gColor;
            l3[1].position = off3 + dir3 * diagonal; l3[1].color = gColor;

            window.draw(l1, states); window.draw(l2, states); window.draw(l3, states);
        }
    }

    if (!config.guideSettings.locked) {
        for (size_t i = 0; i < config.vps.size(); ++i) {
            if (!config.vps[i].visible) continue;
            sf::CircleShape handle(10.0f);
            handle.setOrigin(10.0f, 10.0f);
            handle.setPosition(config.vps[i].position);
            handle.setFillColor(config.vps[i].color);
            if (i == static_cast<size_t>(hoveredVPIndex) || i == static_cast<size_t>(activeVPIndex)) {
                handle.setOutlineThickness(2.0f);
                handle.setOutlineColor(sf::Color::White);
            }
            window.draw(handle, states);
        }
    }
}

std::string PerspectiveSerializer::serialize(const std::vector<PerspectiveConfig>& configs) { return ""; }
std::vector<PerspectiveConfig> PerspectiveSerializer::deserialize(const std::string& data) { return {}; }

PerspectiveManager::PerspectiveManager() : activeConfigIndex(-1) {}

void PerspectiveManager::init() {
    configs.clear();
    undoStack.clear();
    redoStack.clear();
}

PerspectiveConfig* PerspectiveManager::getActiveConfig() {
    if (activeConfigIndex >= 0 && activeConfigIndex < static_cast<int>(configs.size())) return &configs[activeConfigIndex];
    return nullptr;
}

const PerspectiveConfig* PerspectiveManager::getActiveConfigReadOnly() const {
    if (activeConfigIndex >= 0 && activeConfigIndex < static_cast<int>(configs.size())) return &configs[activeConfigIndex];
    return nullptr;
}

int PerspectiveManager::getActiveConfigIndex() const { return activeConfigIndex; }

void PerspectiveManager::setActiveConfig(int index) {
    if (index >= 0 && index < static_cast<int>(configs.size())) activeConfigIndex = index;
}

void PerspectiveManager::addConfig(PerspectiveMode mode, sf::Vector2u canvasSize) {
    saveUndoState();
    PerspectiveConfig cfg("New Guide", mode);
    float cx = canvasSize.x / 2.0f;
    float cy = canvasSize.y / 2.0f;

    if (mode == PerspectiveMode::OnePoint) {
        cfg.vps.push_back(VanishingPoint(sf::Vector2f(cx, cy), "VP 1"));
    }
    else if (mode == PerspectiveMode::TwoPoint) {
        cfg.vps.push_back(VanishingPoint(sf::Vector2f(-cx, cy), "Left VP"));
        cfg.vps.push_back(VanishingPoint(sf::Vector2f(cx * 3.0f, cy), "Right VP"));
    }
    else if (mode == PerspectiveMode::ThreePoint) {
        cfg.vps.push_back(VanishingPoint(sf::Vector2f(-cx, cy), "Left VP"));
        cfg.vps.push_back(VanishingPoint(sf::Vector2f(cx * 3.0f, cy), "Right VP"));
        cfg.vps.push_back(VanishingPoint(sf::Vector2f(cx, cy * 4.0f), "Bottom VP"));
    }

    configs.push_back(cfg);
    activeConfigIndex = static_cast<int>(configs.size()) - 1;
}

void PerspectiveManager::removeConfig(int index) {
    if (index >= 0 && index < static_cast<int>(configs.size())) {
        saveUndoState();
        configs.erase(configs.begin() + index);
        if (activeConfigIndex >= static_cast<int>(configs.size())) activeConfigIndex = static_cast<int>(configs.size()) - 1;
    }
}

std::vector<std::string> PerspectiveManager::getPresetNames() const {
    return { "Interior Room", "City Street", "House", "Railway", "Bridge", "Isometric Game", "Road", "Technical Drawing" };
}

void PerspectiveManager::loadPreset(const std::string& presetName, sf::Vector2u canvasSize) {
    saveUndoState();
    PerspectiveConfig cfg(presetName, PerspectiveMode::OnePoint);
    float w = static_cast<float>(canvasSize.x);
    float h = static_cast<float>(canvasSize.y);

    if (presetName == "Interior Room") {
        cfg.mode = PerspectiveMode::OnePoint;
        cfg.vps.push_back(VanishingPoint(sf::Vector2f(w * 0.5f, h * 0.5f), "Center"));
    }
    else if (presetName == "City Street") {
        cfg.mode = PerspectiveMode::OnePoint;
        cfg.vps.push_back(VanishingPoint(sf::Vector2f(w * 0.3f, h * 0.6f), "Off-Center"));
    }
    else if (presetName == "House") {
        cfg.mode = PerspectiveMode::TwoPoint;
        cfg.vps.push_back(VanishingPoint(sf::Vector2f(-w * 0.5f, h * 0.5f), "Left"));
        cfg.vps.push_back(VanishingPoint(sf::Vector2f(w * 1.5f, h * 0.5f), "Right"));
    }
    else if (presetName == "Railway") {
        cfg.mode = PerspectiveMode::OnePoint;
        cfg.vps.push_back(VanishingPoint(sf::Vector2f(w * 0.5f, h * 0.8f), "Low Horizon"));
    }
    else if (presetName == "Bridge") {
        cfg.mode = PerspectiveMode::TwoPoint;
        cfg.vps.push_back(VanishingPoint(sf::Vector2f(-w * 0.2f, h * 0.8f), "Low Left"));
        cfg.vps.push_back(VanishingPoint(sf::Vector2f(w * 1.2f, h * 0.8f), "Low Right"));
    }
    else if (presetName == "Isometric Game" || presetName == "Technical Drawing") {
        cfg.mode = PerspectiveMode::Isometric;
    }
    else if (presetName == "Road") {
        cfg.mode = PerspectiveMode::OnePoint;
        cfg.vps.push_back(VanishingPoint(sf::Vector2f(w * 0.5f, h * 0.3f), "High Horizon"));
    }

    configs.push_back(cfg);
    activeConfigIndex = static_cast<int>(configs.size()) - 1;
}

void PerspectiveManager::saveUndoState() {
    undoStack.push_back(configs);
    if (undoStack.size() > 20) undoStack.erase(undoStack.begin());
    redoStack.clear();
}

void PerspectiveManager::undo() {
    if (!undoStack.empty()) {
        redoStack.push_back(configs);
        configs = undoStack.back();
        undoStack.pop_back();
        if (activeConfigIndex >= static_cast<int>(configs.size())) activeConfigIndex = static_cast<int>(configs.size()) - 1;
    }
}

void PerspectiveManager::redo() {
    if (!redoStack.empty()) {
        undoStack.push_back(configs);
        configs = redoStack.back();
        redoStack.pop_back();
    }
}