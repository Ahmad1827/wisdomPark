#include "AIPanel.h"
#include "../core/ColorManager.h"
#include "../UI/UITheme.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <regex>
#include <cstdlib>

AIPanel::AIPanel()
    : position(64.f, 78.f),
    size(320.f, 310.f),
    isVisible(false),
    isDraggingPanel(false),
    selectedPaletteIdx(0),
    isDropdownOpen(false),
    dropdownScroll(0.f),
    dropdownMaxScroll(0.f),
    lastPickedColor(sf::Color::White) {}

void AIPanel::init() {
    font.loadFromFile("assets/font.otf");
    position = sf::Vector2f(64.f, 78.f);
    loadPalettes();
}

void AIPanel::toggle() {
    isVisible = !isVisible;
    if (!isVisible) {
        isDropdownOpen = false;
        pendingAction = "";
    }
}

bool AIPanel::getIsVisible() const {
    return isVisible;
}

std::string AIPanel::getSelectedPaletteName() const {
    if (selectedPaletteIdx >= 0 && selectedPaletteIdx < static_cast<int>(palettes.size())) {
        return palettes[selectedPaletteIdx].name;
    }
    return "";
}

const std::vector<sf::Color>& AIPanel::getSelectedPaletteColors() const {
    static const std::vector<sf::Color> s_empty;
    if (selectedPaletteIdx >= 0 && selectedPaletteIdx < static_cast<int>(palettes.size())) {
        return palettes[selectedPaletteIdx].colors;
    }
    return s_empty;
}

sf::Color AIPanel::getLastPickedColor() const {
    return lastPickedColor;
}

sf::Color AIPanel::hexToColor(const std::string& hex) const {
    std::string clean = hex;
    if (!clean.empty() && clean[0] == '#') clean = clean.substr(1);
    if (clean.length() == 6) {
        int r = std::stoi(clean.substr(0, 2), nullptr, 16);
        int g = std::stoi(clean.substr(2, 2), nullptr, 16);
        int b = std::stoi(clean.substr(4, 2), nullptr, 16);
        return sf::Color(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b), 255);
    }
    return sf::Color::White;
}

std::string AIPanel::colorToHex(sf::Color c) const {
    std::stringstream ss;
    ss << "#" << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(c.r)
        << std::setw(2) << static_cast<int>(c.g)
        << std::setw(2) << static_cast<int>(c.b);
    return ss.str();
}

void AIPanel::loadPalettes() {
    palettes.clear();

    palettes.push_back({ "Sunset Warmth (Desert 32)", {
        sf::Color(190,74,47), sf::Color(215,118,67), sf::Color(234,212,170), sf::Color(228,166,114),
        sf::Color(184,111,86), sf::Color(115,62,57), sf::Color(62,39,49), sf::Color(38,23,30),
        sf::Color(162,38,51), sf::Color(228,59,68), sf::Color(247,118,34), sf::Color(254,174,52),
        sf::Color(254,231,97), sf::Color(99,199,77), sf::Color(62,137,72), sf::Color(38,92,66)
    } });
    palettes.push_back({ "Neon Cyberpunk (Apollo 32)", {
        sf::Color(5,5,15), sf::Color(25,15,45), sf::Color(60,20,80), sf::Color(130,25,95),
        sf::Color(215,35,100), sf::Color(255,95,130), sf::Color(255,160,180), sf::Color(255,230,240),
        sf::Color(15,40,70), sf::Color(25,90,130), sf::Color(35,160,190), sf::Color(70,230,230)
    } });
    palettes.push_back({ "Pastel Fantasy (Sweetie 16)", {
        sf::Color(26,28,44), sf::Color(93,39,93), sf::Color(177,62,83), sf::Color(239,125,87),
        sf::Color(255,205,117), sf::Color(167,240,112), sf::Color(56,183,100), sf::Color(37,113,121),
        sf::Color(41,54,111), sf::Color(59,93,201), sf::Color(65,166,246), sf::Color(115,239,247)
    } });
    palettes.push_back({ "Gothic Dungeon (Resurrect 64)", {
        sf::Color(46,34,47), sf::Color(62,53,70), sf::Color(98,85,101), sf::Color(150,108,108),
        sf::Color(171,148,122), sf::Color(105,123,91), sf::Color(82,101,66), sf::Color(79,143,186),
        sf::Color(194,133,105), sf::Color(162,83,83), sf::Color(116,47,47), sf::Color(68,28,28)
    } });
    palettes.push_back({ "Retro 8-Bit (Pico-8)", {
        sf::Color(0,0,0), sf::Color(29,43,83), sf::Color(126,37,83), sf::Color(0,135,81),
        sf::Color(171,82,54), sf::Color(95,87,79), sf::Color(194,195,199), sf::Color(255,241,232),
        sf::Color(255,0,77), sf::Color(255,163,0), sf::Color(255,236,39), sf::Color(0,228,54)
    } });
    palettes.push_back({ "Deep Space (Cosmic 8)", {
        sf::Color(15,10,25), sf::Color(35,20,55), sf::Color(70,35,90), sf::Color(125,55,125),
        sf::Color(190,85,135), sf::Color(240,140,140), sf::Color(255,210,185), sf::Color(255,255,255)
    } });
    palettes.push_back({ "Volcanic Core (Magma 8)", {
        sf::Color(24,12,14), sf::Color(59,18,24), sf::Color(120,24,32), sf::Color(194,46,34),
        sf::Color(242,99,33), sf::Color(255,179,44), sf::Color(255,243,128), sf::Color(255,255,255)
    } });
    palettes.push_back({ "Glacial Frost (Ice 8)", {
        sf::Color(15,23,42), sf::Color(30,58,95), sf::Color(49,105,150), sf::Color(84,160,202),
        sf::Color(143,211,232), sf::Color(207,241,249), sf::Color(240,253,255), sf::Color(255,255,255)
    } });

    std::vector<LospecPalette> customPalettes;
    std::ifstream file("assets/palettes.txt");
    if (file.is_open()) {
        std::string nameLine, colorsLine;
        while (std::getline(file, nameLine) && std::getline(file, colorsLine)) {
            while (!nameLine.empty() && (nameLine.back() == '\r' || nameLine.back() == ' ')) nameLine.pop_back();
            if (nameLine.empty() || colorsLine.empty()) continue;

            auto it = std::find_if(customPalettes.begin(), customPalettes.end(), [&](const LospecPalette& p) {
                return p.name == nameLine;
                });
            if (it != customPalettes.end()) continue;

            LospecPalette p;
            p.name = nameLine;
            std::stringstream ss(colorsLine);
            std::string hexStr;
            while (ss >> hexStr) {
                p.colors.push_back(hexToColor(hexStr));
            }
            if (p.colors.size() >= 4) {
                customPalettes.push_back(p);
            }
        }
        file.close();

        std::ofstream cleanOut("assets/palettes.txt", std::ios::trunc);
        if (cleanOut.is_open()) {
            for (const auto& cp : customPalettes) {
                cleanOut << cp.name << "\n";
                for (size_t i = 0; i < cp.colors.size(); ++i) {
                    cleanOut << colorToHex(cp.colors[i]) << (i + 1 == cp.colors.size() ? "" : " ");
                }
                cleanOut << "\n";
            }
        }
    }

    for (const auto& cp : customPalettes) {
        auto it = std::find_if(palettes.begin(), palettes.end(), [&](const LospecPalette& p) {
            return p.name == cp.name;
            });
        if (it == palettes.end()) {
            palettes.push_back(cp);
        }
    }
}

bool AIPanel::importFromClipboard() {
    std::string clip = sf::Clipboard::getString().toAnsiString();
    clip.erase(0, clip.find_first_not_of(" \r\n\t"));
    clip.erase(clip.find_last_not_of(" \r\n\t") + 1);
    if (clip.empty()) return false;

    std::regex hexRegex("#?([0-9a-fA-F]{6})");
    std::sregex_iterator next(clip.begin(), clip.end(), hexRegex);
    std::sregex_iterator end;

    std::vector<sf::Color> foundColors;
    std::vector<std::string> foundHexes;
    while (next != end) {
        std::smatch match = *next;
        std::string h = "#" + match.str(1);
        foundColors.push_back(hexToColor(h));
        foundHexes.push_back(h);
        ++next;
    }

    if (foundColors.size() >= 4) {
        std::string name = "Custom Palette (" + std::to_string(foundColors.size()) + "c)";
        for (size_t i = 0; i < palettes.size(); ++i) {
            if (palettes[i].name == name) {
                selectedPaletteIdx = static_cast<int>(i);
                return true;
            }
        }
        std::ofstream out("assets/palettes.txt", std::ios::app);
        if (out.is_open()) {
            out << name << "\n";
            for (size_t i = 0; i < foundHexes.size(); ++i) {
                out << foundHexes[i] << (i + 1 == foundHexes.size() ? "" : " ");
            }
            out << "\n";
        }
        loadPalettes();
        for (size_t i = 0; i < palettes.size(); ++i) {
            if (palettes[i].name == name) {
                selectedPaletteIdx = static_cast<int>(i);
                break;
            }
        }
        return true;
    }

    if (clip.find("http") != std::string::npos || clip.find("lospec") != std::string::npos || clip.find("-") != std::string::npos) {
        std::string cmd = "python scripts/fetch_palettes.py \"" + clip + "\"";
        int res = std::system(cmd.c_str());
        if (res == 0) {
            loadPalettes();
            if (!palettes.empty()) {
                selectedPaletteIdx = static_cast<int>(palettes.size()) - 1;
                return true;
            }
        }
    }

    return false;
}

float AIPanel::getLuminance(sf::Color c) const {
    return 0.299f * c.r + 0.587f * c.g + 0.114f * c.b;
}

float AIPanel::getColorDistance(sf::Color c1, sf::Color c2) const {
    float dr = static_cast<float>(c1.r) - static_cast<float>(c2.r);
    float dg = static_cast<float>(c1.g) - static_cast<float>(c2.g);
    float db = static_cast<float>(c1.b) - static_cast<float>(c2.b);
    return std::sqrt(dr * dr + dg * dg + db * db);
}


void AIPanel::update(float dt) {
    if (!isVisible) return;

    float bx = position.x;
    float by = position.y;

    headerGripBounds = sf::FloatRect(bx + 8.f, by + 6.f, size.x - 16.f, 24.f);
    dropdownBtnBounds = sf::FloatRect(bx + 16.f, by + 56.f, size.x - 32.f, 28.f);

    float splitW = (size.x - 36.f) / 2.f;
    pasteBtnBounds = sf::FloatRect(bx + 16.f, by + 88.f, splitW, 24.f);
    randomBtnBounds = sf::FloatRect(bx + 16.f + splitW + 4.f, by + 88.f, splitW, 24.f);

    paletteSwatchBounds.clear();
    if (!palettes.empty() && selectedPaletteIdx < static_cast<int>(palettes.size())) {
        const auto& cols = palettes[selectedPaletteIdx].colors;
        float startX = bx + 16.f;
        float startY = by + 132.f;
        float swSize = 18.f;
        float spacing = 22.f;
        int perRow = 12;

        for (size_t i = 0; i < cols.size(); ++i) {
            if (i >= 24) break;
            int r = static_cast<int>(i / perRow);
            int c = static_cast<int>(i % perRow);
            sf::FloatRect swRect(startX + c * spacing, startY + r * spacing, swSize, swSize);
            paletteSwatchBounds.push_back({ swRect, cols[i] });
        }
    }

    suggestBtnBounds = sf::FloatRect(bx + 16.f, by + 188.f, size.x - 32.f, 38.f);
    sendAllBtnBounds = sf::FloatRect(bx + 16.f, by + 230.f, size.x - 32.f, 26.f);
    closeBtnBounds = sf::FloatRect(bx + 16.f, by + 262.f, size.x - 32.f, 24.f);

    float listY = dropdownBtnBounds.top + dropdownBtnBounds.height + 2.f;
    float listH = 150.f;
    dropdownListArea = sf::FloatRect(bx + 16.f, listY, size.x - 32.f, listH);

    dropdownItemBounds.clear();
    float itemH = 24.f;
    float curY = listY - dropdownScroll;
    for (size_t i = 0; i < palettes.size(); ++i) {
        dropdownItemBounds.push_back({ sf::FloatRect(bx + 16.f, curY, size.x - 32.f, itemH), static_cast<int>(i) });
        curY += itemH + 2.f;
    }
    dropdownMaxScroll = std::max(0.f, static_cast<float>(palettes.size()) * (itemH + 2.f) - listH);
}

void AIPanel::draw(sf::RenderWindow& window) {
    if (!isVisible) return;

    sf::FloatRect panelBounds(position.x, position.y, size.x, size.y);
    WisdomUI::Theme::DrawSunsetPanel(window, panelBounds, 1.0f);

    sf::RectangleShape gripBg(sf::Vector2f(headerGripBounds.width, headerGripBounds.height));
    gripBg.setPosition(headerGripBounds.left, headerGripBounds.top);
    gripBg.setFillColor(WisdomUI::Theme::SunsetDeepDark);
    gripBg.setOutlineThickness(1.f);
    gripBg.setOutlineColor(WisdomUI::Theme::SunsetPlum);
    window.draw(gripBg);

    WisdomUI::Theme::DrawCrispText(window, font, ":: COLOR ASSISTANT ::", 11, headerGripBounds.left + headerGripBounds.width / 2.0f, headerGripBounds.top + headerGripBounds.height / 2.0f, WisdomUI::Theme::SunsetAmber, sf::Color(14, 6, 20), true, true);

    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    WisdomUI::Theme::DrawCrispText(window, font, "Vibe / Preset Palette:", 10, position.x + 16.f, position.y + 38.f, WisdomUI::Theme::TextSecondary);

    std::string currentPalName = palettes.empty() ? "None" : palettes[selectedPaletteIdx].name;
    bool hovDrop = dropdownBtnBounds.contains(mPos);
    WisdomUI::Theme::DrawSunsetButton(window, dropdownBtnBounds, currentPalName + "  v", font, 11, false, hovDrop, isDropdownOpen, 1.0f);

    bool hovPaste = pasteBtnBounds.contains(mPos);
    WisdomUI::Theme::DrawSunsetButton(window, pasteBtnBounds, "+ Paste Link", font, 10, false, hovPaste, false, 1.0f);

    bool hovRand = randomBtnBounds.contains(mPos);
    WisdomUI::Theme::DrawSunsetButton(window, randomBtnBounds, "Random Vibe", font, 10, false, hovRand, false, 1.0f);

    size_t colCount = (!palettes.empty() && selectedPaletteIdx < static_cast<int>(palettes.size())) ? palettes[selectedPaletteIdx].colors.size() : 0;
    std::string swLabel = "Palette Swatches (" + std::to_string(colCount) + "):";
    WisdomUI::Theme::DrawCrispText(window, font, swLabel, 9, position.x + 16.f, position.y + 116.f, WisdomUI::Theme::SunsetGold);

    for (const auto& item : paletteSwatchBounds) {
        sf::RectangleShape r(sf::Vector2f(item.first.width, item.first.height));
        r.setPosition(item.first.left, item.first.top);
        r.setFillColor(item.second);
        r.setOutlineThickness(1.f);
        bool hovSw = item.first.contains(mPos);
        r.setOutlineColor(hovSw ? sf::Color::White : WisdomUI::Theme::Border);
        window.draw(r);
    }

    bool hovSuggest = suggestBtnBounds.contains(mPos);
    WisdomUI::Theme::DrawSunsetButton(window, suggestBtnBounds, "+ Get 4-Color Ramp Advice", font, 11, false, hovSuggest, true, 1.0f);

    bool hovSend = sendAllBtnBounds.contains(mPos);
    WisdomUI::Theme::DrawSunsetButton(window, sendAllBtnBounds, "--> Send All to Colors Panel", font, 10, false, hovSend, false, 1.0f);

    bool hovClose = closeBtnBounds.contains(mPos);
    WisdomUI::Theme::DrawSunsetButton(window, closeBtnBounds, "Close", font, 11, false, hovClose, false, 1.0f);

    if (isDropdownOpen) {
        sf::RectangleShape shadow(sf::Vector2f(dropdownListArea.width + 6.f, dropdownListArea.height + 6.f));
        shadow.setPosition(dropdownListArea.left - 3.f, dropdownListArea.top - 3.f);
        shadow.setFillColor(sf::Color(0, 0, 0, 220));
        window.draw(shadow);

        sf::RectangleShape listBg(sf::Vector2f(dropdownListArea.width, dropdownListArea.height));
        listBg.setPosition(dropdownListArea.left, dropdownListArea.top);
        listBg.setFillColor(sf::Color(16, 10, 22, 252));
        listBg.setOutlineThickness(1.5f);
        listBg.setOutlineColor(WisdomUI::Theme::SunsetGold);
        window.draw(listBg);

        for (const auto& item : dropdownItemBounds) {
            if (item.first.top + item.first.height < dropdownListArea.top || item.first.top > dropdownListArea.top + dropdownListArea.height) {
                continue;
            }
            bool isHov = item.first.contains(mPos);
            bool isSel = (item.second == selectedPaletteIdx);

            sf::RectangleShape r(sf::Vector2f(item.first.width - 4.f, item.first.height));
            r.setPosition(item.first.left + 2.f, item.first.top);
            r.setFillColor(isSel ? WisdomUI::Theme::SunsetSkyMid : (isHov ? sf::Color(45, 28, 55) : sf::Color::Transparent));
            window.draw(r);

            WisdomUI::Theme::DrawCrispText(window, font, palettes[item.second].name, 10, item.first.left + 6.f, item.first.top + 6.f, isSel ? WisdomUI::Theme::SunsetGold : WisdomUI::Theme::TextPrimary);

            float pX = item.first.left + item.first.width - 50.f;
            for (size_t c = 0; c < std::min(static_cast<size_t>(4), palettes[item.second].colors.size()); ++c) {
                sf::RectangleShape sw(sf::Vector2f(9.f, 9.f));
                sw.setPosition(pX + c * 11.f, item.first.top + 8.f);
                sw.setFillColor(palettes[item.second].colors[c]);
                window.draw(sw);
            }
        }
    }
}

bool AIPanel::handleEvent(const sf::Event& event, sf::Vector2f mousePos) {
    if (!isVisible) return false;

    if (isDropdownOpen && event.type == sf::Event::MouseWheelScrolled) {
        if (dropdownListArea.contains(mousePos)) {
            dropdownScroll = std::clamp(dropdownScroll - event.mouseWheelScroll.delta * 24.0f, 0.0f, dropdownMaxScroll);
            return true;
        }
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (isDropdownOpen) {
            if (dropdownListArea.contains(mousePos)) {
                for (const auto& item : dropdownItemBounds) {
                    if (item.first.contains(mousePos)) {
                        selectedPaletteIdx = item.second;
                        isDropdownOpen = false;
                        pendingAction = "";
                        return true;
                    }
                }
                return true;
            }
            if (dropdownBtnBounds.contains(mousePos)) {
                isDropdownOpen = false;
                return true;
            }
            isDropdownOpen = false;
            return true;
        }

        if (headerGripBounds.contains(mousePos)) {
            isDraggingPanel = true;
            dragOffset = mousePos - position;
            return true;
        }

        if (dropdownBtnBounds.contains(mousePos)) {
            isDropdownOpen = true;
            dropdownScroll = 0.f;
            return true;
        }

        if (pasteBtnBounds.contains(mousePos)) {
            pendingAction = "action:paste_palette";
            return true;
        }

        if (randomBtnBounds.contains(mousePos)) {
            pendingAction = "action:random_palette";
            return true;
        }

        for (const auto& item : paletteSwatchBounds) {
            if (item.first.contains(mousePos)) {
                lastPickedColor = item.second;
                pendingAction = "action:swatch_picked";
                return true;
            }
        }

        if (suggestBtnBounds.contains(mousePos)) {
            pendingAction = "action:get_advice";
            return true;
        }

        if (sendAllBtnBounds.contains(mousePos)) {
            pendingAction = "action:send_all_swatches";
            return true;
        }

        if (closeBtnBounds.contains(mousePos)) {
            toggle();
            pendingAction = "close";
            return true;
        }

        if (sf::FloatRect(position.x, position.y, size.x, size.y).contains(mousePos)) {
            return true;
        }
    }
    else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        if (isDraggingPanel) {
            isDraggingPanel = false;
            return true;
        }
    }
    else if (event.type == sf::Event::MouseMoved && isDraggingPanel) {
        position = mousePos - dragOffset;
        position.x = std::clamp(position.x, 56.f, 1920.f - size.x);
        position.y = std::clamp(position.y, 40.f, 1080.f - size.y);
        return true;
    }

    if (isDropdownOpen && dropdownListArea.contains(mousePos)) {
        return true;
    }

    return sf::FloatRect(position.x, position.y, size.x, size.y).contains(mousePos);
}

std::string AIPanel::handleClick(sf::Vector2f mousePos) {
    std::string act = pendingAction;
    pendingAction = "";
    return act;
}

std::vector<sf::Color> AIPanel::generateAdvice(sf::Color base, int variation) {
    if (palettes.empty() || selectedPaletteIdx >= static_cast<int>(palettes.size())) {
        return { base, base, base, base };
    }

    auto pColors = palettes[selectedPaletteIdx].colors;
    if (pColors.size() < 4) {
        return { base, base, base, base };
    }

    std::sort(pColors.begin(), pColors.end(), [this](sf::Color a, sf::Color b) {
        return getLuminance(a) < getLuminance(b);
        });

    int N = static_cast<int>(pColors.size());
    int bestIdx = 0;
    float bestDist = getColorDistance(base, pColors[0]);
    for (int i = 1; i < N; ++i) {
        float d = getColorDistance(base, pColors[i]);
        if (d < bestDist) {
            bestDist = d;
            bestIdx = i;
        }
    }

    int varOffset = (variation % 4) - 1;
    int baseIdx = std::clamp(bestIdx + varOffset, 0, N - 1);

    int highIdx = 0, midIdx = 0, shdIdx = 0, deepIdx = 0;

    if (baseIdx <= 1) {
        deepIdx = 0;
        shdIdx = std::min(N - 1, 1 + (variation % 2));
        midIdx = std::min(N - 1, 3 + (variation % 3));
        highIdx = std::min(N - 1, 6 + (variation % (N - 6 > 0 ? N - 6 : 1)));
    }
    else if (baseIdx >= N - 2) {
        highIdx = N - 1;
        midIdx = std::max(0, N - 2 - (variation % 2));
        shdIdx = std::max(0, N - 4 - (variation % 3));
        deepIdx = std::max(0, N - 7 - (variation % (N - 7 > 0 ? N - 7 : 1)));
    }
    else {
        midIdx = baseIdx;
        int stepUp = std::max(1, (N - 1 - midIdx) / 2);
        highIdx = std::min(N - 1, midIdx + stepUp + (variation % 2));
        int stepDown = std::max(1, midIdx / 2);
        shdIdx = std::max(0, midIdx - stepDown);
        deepIdx = std::max(0, shdIdx - stepDown - (variation % 2));
    }

    sf::Color highlight = pColors[highIdx];
    sf::Color midtone = pColors[midIdx];
    sf::Color shadow = pColors[shdIdx];
    sf::Color deepShadow = pColors[deepIdx];

    return { highlight, midtone, shadow, deepShadow };
}

void AIPanel::pickRandomPalette() {
    static const std::vector<LospecPalette> builtInPool = {
        { "Neon Nights (Cyber 8)", {
            sf::Color(10, 8, 26), sf::Color(44, 21, 72), sf::Color(114, 25, 102), sf::Color(199, 36, 107),
            sf::Color(245, 115, 84), sf::Color(254, 219, 104), sf::Color(42, 237, 217), sf::Color(255, 255, 255)
        }},
        { "Autumn Woodland (Amber 8)", {
            sf::Color(34, 24, 21), sf::Color(68, 44, 33), sf::Color(133, 66, 38), sf::Color(190, 106, 47),
            sf::Color(235, 168, 77), sf::Color(247, 226, 143), sf::Color(92, 105, 60), sf::Color(147, 160, 94)
        }},
        { "Frozen Glaciers (Frost 8)", {
            sf::Color(15, 23, 42), sf::Color(30, 58, 95), sf::Color(49, 105, 150), sf::Color(84, 160, 202),
            sf::Color(143, 211, 232), sf::Color(207, 241, 249), sf::Color(240, 253, 255), sf::Color(255, 255, 255)
        }},
        { "Candy Bubblegum (Pop 8)", {
            sf::Color(46, 21, 56), sf::Color(107, 39, 99), sf::Color(189, 63, 131), sf::Color(240, 117, 169),
            sf::Color(255, 184, 209), sf::Color(255, 237, 243), sf::Color(103, 232, 249), sf::Color(255, 255, 255)
        }},
        { "Volcanic Core (Magma 8)", {
            sf::Color(24, 12, 14), sf::Color(59, 18, 24), sf::Color(120, 24, 32), sf::Color(194, 46, 34),
            sf::Color(242, 99, 33), sf::Color(255, 179, 44), sf::Color(255, 243, 128), sf::Color(255, 255, 255)
        }},
        { "Enchanted Garden (Verdant 8)", {
            sf::Color(16, 28, 22), sf::Color(28, 59, 41), sf::Color(45, 105, 65), sf::Color(78, 158, 89),
            sf::Color(142, 207, 103), sf::Color(212, 245, 137), sf::Color(250, 255, 209), sf::Color(255, 220, 120)
        }},
        { "Deep Twilight (Purple 8)", {
            sf::Color(20, 12, 28), sf::Color(45, 20, 59), sf::Color(82, 34, 99), sf::Color(130, 56, 140),
            sf::Color(181, 88, 175), sf::Color(224, 138, 207), sf::Color(250, 202, 233), sf::Color(255, 245, 250)
        }}
    };

    static size_t s_randomCounter = 0;
    size_t pick = s_randomCounter++ % builtInPool.size();
    const auto& chosen = builtInPool[pick];

    for (size_t i = 0; i < palettes.size(); ++i) {
        if (palettes[i].name == chosen.name) {
            selectedPaletteIdx = static_cast<int>(i);
            return;
        }
    }

    palettes.push_back(chosen);
    selectedPaletteIdx = static_cast<int>(palettes.size()) - 1;
}