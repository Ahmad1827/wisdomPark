#include "AIPanel.h"
#include "../core/ColorManager.h"
#include "../UI/UITheme.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>

AIPanel::AIPanel()
    : position(64.f, 78.f),
    size(320.f, 250.f),
    isVisible(false),
    isDraggingPanel(false),
    selectedPaletteIdx(0),
    isDropdownOpen(false),
    dropdownScroll(0.f),
    dropdownMaxScroll(0.f) {}

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

void AIPanel::loadPalettes() {
    palettes.clear();
    std::ifstream file("assets/palettes.txt");
    if (file.is_open()) {
        std::string nameLine, colorsLine;
        while (std::getline(file, nameLine) && std::getline(file, colorsLine)) {
            if (nameLine.empty() || colorsLine.empty()) continue;
            LospecPalette p;
            p.name = nameLine;
            std::stringstream ss(colorsLine);
            std::string hexStr;
            while (ss >> hexStr) {
                p.colors.push_back(hexToColor(hexStr));
            }
            if (p.colors.size() >= 4) {
                palettes.push_back(p);
            }
        }
    }

    if (palettes.empty()) {
        palettes.push_back({ "Sunset Warmth (Desert 32)", {
            sf::Color(190,74,47), sf::Color(215,118,67), sf::Color(234,212,170), sf::Color(228,166,114),
            sf::Color(184,111,86), sf::Color(115,62,57), sf::Color(62,39,49), sf::Color(38,23,30),
            sf::Color(162,38,51), sf::Color(228,59,68), sf::Color(247,118,34), sf::Color(254,174,52),
            sf::Color(254,231,97), sf::Color(99,199,77), sf::Color(62,137,72), sf::Color(38,92,66),
            sf::Color(25,60,62), sf::Color(18,78,137), sf::Color(0,153,219), sf::Color(44,232,245)
        } });
        palettes.push_back({ "Neon Cyberpunk (Apollo 32)", {
            sf::Color(5,5,15), sf::Color(25,15,45), sf::Color(60,20,80), sf::Color(130,25,95),
            sf::Color(215,35,100), sf::Color(255,95,130), sf::Color(255,160,180), sf::Color(255,230,240),
            sf::Color(15,40,70), sf::Color(25,90,130), sf::Color(35,160,190), sf::Color(70,230,230),
            sf::Color(40,180,100), sf::Color(120,240,110), sf::Color(245,235,90), sf::Color(255,140,40)
        } });
        palettes.push_back({ "Pastel Fantasy (Sweetie 16)", {
            sf::Color(26,28,44), sf::Color(93,39,93), sf::Color(177,62,83), sf::Color(239,125,87),
            sf::Color(255,205,117), sf::Color(167,240,112), sf::Color(56,183,100), sf::Color(37,113,121),
            sf::Color(41,54,111), sf::Color(59,93,201), sf::Color(65,166,246), sf::Color(115,239,247),
            sf::Color(244,244,244), sf::Color(148,176,194), sf::Color(86,108,134), sf::Color(51,60,87)
        } });
        palettes.push_back({ "Gothic Dungeon (Resurrect 64)", {
            sf::Color(46,34,47), sf::Color(62,53,70), sf::Color(98,85,101), sf::Color(150,108,108),
            sf::Color(171,148,122), sf::Color(105,123,91), sf::Color(82,101,66), sf::Color(79,143,186),
            sf::Color(194,133,105), sf::Color(162,83,83), sf::Color(116,47,47), sf::Color(68,28,28)
        } });
        palettes.push_back({ "Retro 8-Bit (Pico-8)", {
            sf::Color(0,0,0), sf::Color(29,43,83), sf::Color(126,37,83), sf::Color(0,135,81),
            sf::Color(171,82,54), sf::Color(95,87,79), sf::Color(194,195,199), sf::Color(255,241,232),
            sf::Color(255,0,77), sf::Color(255,163,0), sf::Color(255,236,39), sf::Color(0,228,54),
            sf::Color(41,173,255), sf::Color(131,118,156), sf::Color(255,119,168), sf::Color(255,204,170)
        } });
        palettes.push_back({ "Deep Space (Cosmic 8)", {
            sf::Color(15,10,25), sf::Color(35,20,55), sf::Color(70,35,90), sf::Color(125,55,125),
            sf::Color(190,85,135), sf::Color(240,140,140), sf::Color(255,210,185), sf::Color(255,255,255)
        } });
    }
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

    headerGripBounds = sf::FloatRect(bx + 8.f, by + 6.f, size.x - 16.f, 26.f);
    dropdownBtnBounds = sf::FloatRect(bx + 16.f, by + 68.f, size.x - 32.f, 32.f);
    suggestBtnBounds = sf::FloatRect(bx + 16.f, by + 120.f, size.x - 32.f, 44.f);
    closeBtnBounds = sf::FloatRect(bx + 16.f, by + 182.f, size.x - 32.f, 28.f);

    float listY = dropdownBtnBounds.top + dropdownBtnBounds.height + 3.f;
    float listH = 175.f;
    dropdownListArea = sf::FloatRect(bx + 16.f, listY, size.x - 32.f, listH);

    dropdownItemBounds.clear();
    float itemH = 26.f;
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

    WisdomUI::Theme::DrawCrispText(window, font, ":: COLOR ASSISTANT ::", 12, headerGripBounds.left + headerGripBounds.width / 2.0f, headerGripBounds.top + headerGripBounds.height / 2.0f, WisdomUI::Theme::SunsetAmber, sf::Color(14, 6, 20), true, true);

    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    WisdomUI::Theme::DrawCrispText(window, font, "Vibe / Palette Preset:", 10, position.x + 16.f, position.y + 44.f, WisdomUI::Theme::TextSecondary);

    std::string currentPalName = palettes.empty() ? "None" : palettes[selectedPaletteIdx].name;
    bool hovDrop = dropdownBtnBounds.contains(mPos);
    WisdomUI::Theme::DrawSunsetButton(window, dropdownBtnBounds, currentPalName + "  v", font, 11, false, hovDrop, isDropdownOpen, 1.0f);

    bool hovSuggest = suggestBtnBounds.contains(mPos);
    WisdomUI::Theme::DrawSunsetButton(window, suggestBtnBounds, "+ Get Palette Advice", font, 12, false, hovSuggest, true, 1.0f);

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
            dropdownScroll = std::clamp(dropdownScroll - event.mouseWheelScroll.delta * 26.0f, 0.0f, dropdownMaxScroll);
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

        if (suggestBtnBounds.contains(mousePos)) {
            pendingAction = "action:get_advice";
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