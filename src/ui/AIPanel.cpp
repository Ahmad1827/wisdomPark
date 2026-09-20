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
#include <array>

#if defined(_WIN32)
#define POPEN_CMD _popen
#define PCLOSE_CMD _pclose
#else
#define POPEN_CMD popen
#define PCLOSE_CMD pclose
#endif

AIPanel::AIPanel()
    : position(64.f, 78.f),
    size(360.f, 350.f),
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
    std::string text = sf::Clipboard::getString().toAnsiString();
    while (!text.empty() && (text.back() == '\r' || text.back() == '\n' || text.back() == ' ' || text.back() == '\t')) {
        text.pop_back();
    }
    while (!text.empty() && (text.front() == ' ' || text.front() == '\t')) {
        text.erase(text.begin());
    }

    if (text.empty()) return false;

    bool isUrl = (text.find("http://") == 0 || text.find("https://") == 0 || text.find("lospec.com") != std::string::npos);

    std::vector<sf::Color> extractedColors;

    // 1. If not a link, parse raw hex color codes (#cc4622 or space-separated list)
    if (!isUrl) {
        std::regex hexRegex(R"(#?([0-9a-fA-F]{6}))");
        auto words_begin = std::sregex_iterator(text.begin(), text.end(), hexRegex);
        auto words_end = std::sregex_iterator();

        for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
            std::smatch match = *i;
            std::string hexStr = match[1].str();
            unsigned int val = std::stoul(hexStr, nullptr, 16);
            extractedColors.push_back(sf::Color(
                (val >> 16) & 0xFF,
                (val >> 8) & 0xFF,
                val & 0xFF
            ));
        }

        if (!extractedColors.empty()) {
            std::string palName = (extractedColors.size() == 1) ? ("Hex_" + text) : ("Pasted_" + std::to_string(std::time(nullptr)));

            std::ofstream f("assets/palettes.txt", std::ios::app);
            if (f.is_open()) {
                f << palName << "\n";
                for (size_t i = 0; i < extractedColors.size(); ++i) {
                    f << colorToHex(extractedColors[i]) << (i + 1 == extractedColors.size() ? "" : " ");
                }
                f << "\n";
            }

            palettes.push_back({ palName, extractedColors });
            selectedPaletteIdx = static_cast<int>(palettes.size()) - 1;
            return true;
        }
    }

    // 2. Extract clean slug for Lospec (handles full URLs, trailing slashes, and raw slugs)
    std::string slug = text;
    while (!slug.empty() && (slug.back() == '/' || slug.back() == ' ' || slug.back() == '\r' || slug.back() == '\n')) {
        slug.pop_back();
    }
    size_t qMark = slug.find('?');
    if (qMark != std::string::npos) {
        slug = slug.substr(0, qMark);
    }
    if (slug.length() > 4 && slug.substr(slug.length() - 4) == ".hex") {
        slug = slug.substr(0, slug.length() - 4);
    }
    size_t lastSlash = slug.find_last_of('/');
    if (lastSlash != std::string::npos) {
        slug = slug.substr(lastSlash + 1);
    }

    if (slug.empty()) return false;

    // Check if palette is already loaded
    for (size_t i = 0; i < palettes.size(); ++i) {
        if (palettes[i].name == slug || palettes[i].name.find(slug) != std::string::npos) {
            selectedPaletteIdx = static_cast<int>(i);
            return true;
        }
    }

    // 3. Fetch directly from Lospec using built-in Windows curl (bypasses Python environment issues)
    std::string curlCmd = "curl -s -L -A \"Mozilla/5.0\" \"https://lospec.com/palette-list/" + slug + ".hex\"";
    std::array<char, 256> buffer;
    std::string curlOutput;
    FILE* pipe = POPEN_CMD(curlCmd.c_str(), "r");
    if (pipe) {
        while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
            curlOutput += buffer.data();
        }
        PCLOSE_CMD(pipe);
    }

    if (!curlOutput.empty()) {
        std::istringstream stream(curlOutput);
        std::string line;
        while (std::getline(stream, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n' || line.back() == ' ')) {
                line.pop_back();
            }
            if (!line.empty() && line.front() == '#') line = line.substr(1);
            if (line.length() == 6) {
                bool valid = true;
                for (char c : line) {
                    if (!std::isxdigit(static_cast<unsigned char>(c))) {
                        valid = false;
                        break;
                    }
                }
                if (valid) {
                    extractedColors.push_back(hexToColor(line));
                }
            }
        }
    }

    // 4. Fallback to Python script if curl returned empty
    if (extractedColors.empty()) {
        std::string cmd = "python scripts/fetch_palettes.py \"" + slug + "\"";
        int res = std::system(cmd.c_str());
        if (res != 0) {
            cmd = "py scripts/fetch_palettes.py \"" + slug + "\"";
            std::system(cmd.c_str());
        }

        std::ifstream file("assets/palettes.txt");
        if (file.is_open()) {
            std::string lineName, lineColors;
            while (std::getline(file, lineName) && std::getline(file, lineColors)) {
                while (!lineName.empty() && (lineName.back() == '\r' || lineName.back() == ' ')) lineName.pop_back();
                if (lineName.rfind(slug, 0) == 0 || lineName.find(slug) != std::string::npos) {
                    std::istringstream ss(lineColors);
                    std::string h;
                    extractedColors.clear();
                    while (ss >> h) {
                        extractedColors.push_back(hexToColor(h));
                    }
                    if (!extractedColors.empty()) {
                        slug = lineName;
                        break;
                    }
                }
            }
        }
    }

    // 5. Store palette into assets/palettes.txt and select it in memory
    if (!extractedColors.empty()) {
        std::ofstream f("assets/palettes.txt", std::ios::app);
        if (f.is_open()) {
            f << slug << "\n";
            for (size_t i = 0; i < extractedColors.size(); ++i) {
                f << colorToHex(extractedColors[i]) << (i + 1 == extractedColors.size() ? "" : " ");
            }
            f << "\n";
        }

        palettes.push_back({ slug, extractedColors });
        selectedPaletteIdx = static_cast<int>(palettes.size()) - 1;
        return true;
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

void AIPanel::drawTooltip(sf::RenderWindow& window, const std::string& text, sf::Vector2f pos) {
    sf::Text tipText(text, font, 13);
    sf::FloatRect bounds = tipText.getLocalBounds();

    float padX = 10.f;
    float padY = 6.f;
    float w = bounds.width + padX * 2.f;
    float h = bounds.height + padY * 2.f + 4.f;

    float x = pos.x - w - 12.f;
    float y = pos.y + 14.f;

    if (x < 10.f) x = pos.x + 16.f;
    if (y + h > 1070.f) y = pos.y - h - 6.f;

    sf::RectangleShape bg(sf::Vector2f(w, h));
    bg.setPosition(x, y);
    bg.setFillColor(sf::Color(18, 12, 24, 250));
    bg.setOutlineThickness(1.5f);
    bg.setOutlineColor(WisdomUI::Theme::Gold);

    tipText.setPosition(x + padX, y + padY - 2.f);
    tipText.setFillColor(sf::Color::White);

    window.draw(bg);
    window.draw(tipText);
}

void AIPanel::update(float dt) {
    if (!isVisible) return;

    float bx = position.x;
    float by = position.y;

    headerGripBounds = sf::FloatRect(bx + 8.f, by + 6.f, size.x - 16.f, 30.f);
    dropdownBtnBounds = sf::FloatRect(bx + 16.f, by + 60.f, size.x - 32.f, 32.f);

    float splitW = (size.x - 40.f) / 2.f;
    pasteBtnBounds = sf::FloatRect(bx + 16.f, by + 98.f, splitW, 28.f);
    randomBtnBounds = sf::FloatRect(bx + 20.f + splitW, by + 98.f, splitW, 28.f);

    paletteSwatchBounds.clear();
    if (!palettes.empty() && selectedPaletteIdx < static_cast<int>(palettes.size())) {
        const auto& cols = palettes[selectedPaletteIdx].colors;
        float startX = bx + 16.f;
        float startY = by + 150.f;
        float swSize = 20.f;
        float spacing = 26.f;
        int perRow = 12;

        for (size_t i = 0; i < cols.size(); ++i) {
            if (i >= 24) break;
            int r = static_cast<int>(i / perRow);
            int c = static_cast<int>(i % perRow);
            sf::FloatRect swRect(startX + c * spacing, startY + r * spacing, swSize, swSize);
            paletteSwatchBounds.push_back({ swRect, cols[i] });
        }
    }

    suggestBtnBounds = sf::FloatRect(bx + 16.f, by + 214.f, size.x - 32.f, 40.f);
    sendAllBtnBounds = sf::FloatRect(bx + 16.f, by + 260.f, size.x - 32.f, 32.f);
    closeBtnBounds = sf::FloatRect(bx + 16.f, by + 298.f, size.x - 32.f, 28.f);

    float listY = dropdownBtnBounds.top + dropdownBtnBounds.height + 2.f;
    float listH = 160.f;
    dropdownListArea = sf::FloatRect(bx + 16.f, listY, size.x - 32.f, listH);

    dropdownItemBounds.clear();
    float itemH = 28.f;
    float curY = listY - dropdownScroll;
    for (size_t i = 0; i < palettes.size(); ++i) {
        dropdownItemBounds.push_back({ sf::FloatRect(bx + 16.f, curY, size.x - 32.f, itemH), static_cast<int>(i) });
        curY += itemH + 2.f;
    }
    dropdownMaxScroll = std::max(0.f, static_cast<float>(palettes.size()) * (itemH + 2.f) - listH);
}

void AIPanel::draw(sf::RenderWindow& window) {
    if (!isVisible) return;

    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    std::string hoveredTooltip = "";

    sf::FloatRect panelBounds(position.x, position.y, size.x, size.y);
    WisdomUI::Theme::DrawSunsetPanel(window, panelBounds, 1.0f);

    sf::RectangleShape gripBg(sf::Vector2f(headerGripBounds.width, headerGripBounds.height));
    gripBg.setPosition(headerGripBounds.left, headerGripBounds.top);
    gripBg.setFillColor(WisdomUI::Theme::SunsetDeepDark);
    gripBg.setOutlineThickness(1.f);
    gripBg.setOutlineColor(WisdomUI::Theme::SunsetPlum);
    window.draw(gripBg);

    WisdomUI::Theme::DrawCrispText(window, font, ":: COLOR ASSISTANT ::", 15, headerGripBounds.left + headerGripBounds.width / 2.0f, headerGripBounds.top + headerGripBounds.height / 2.0f, WisdomUI::Theme::SunsetAmber, sf::Color(14, 6, 20), true, true);

    WisdomUI::Theme::DrawCrispText(window, font, "Preset Palette:", 13, position.x + 16.f, position.y + 40.f, WisdomUI::Theme::TextSecondary);

    std::string currentPalName = palettes.empty() ? "None" : palettes[selectedPaletteIdx].name;
    bool hovDrop = dropdownBtnBounds.contains(mPos);
    if (hovDrop && !isDropdownOpen) hoveredTooltip = "Choose active palette preset";
    WisdomUI::Theme::DrawSunsetButton(window, dropdownBtnBounds, currentPalName + "  v", font, 13, false, hovDrop, isDropdownOpen, 1.0f);

    bool hovPaste = pasteBtnBounds.contains(mPos);
    if (hovPaste) hoveredTooltip = "Import from Lospec link or hex list in clipboard";
    WisdomUI::Theme::DrawSunsetButton(window, pasteBtnBounds, "+ Paste Link", font, 12, false, hovPaste, false, 1.0f);

    bool hovRand = randomBtnBounds.contains(mPos);
    if (hovRand) hoveredTooltip = "Pick a random stylized palette";
    WisdomUI::Theme::DrawSunsetButton(window, randomBtnBounds, "Random Vibe", font, 12, false, hovRand, false, 1.0f);

    size_t colCount = (!palettes.empty() && selectedPaletteIdx < static_cast<int>(palettes.size())) ? palettes[selectedPaletteIdx].colors.size() : 0;
    std::string swLabel = "Palette Swatches (" + std::to_string(colCount) + "):";
    WisdomUI::Theme::DrawCrispText(window, font, swLabel, 12, position.x + 16.f, position.y + 132.f, WisdomUI::Theme::SunsetGold);

    for (const auto& item : paletteSwatchBounds) {
        sf::RectangleShape r(sf::Vector2f(item.first.width, item.first.height));
        r.setPosition(item.first.left, item.first.top);
        r.setFillColor(item.second);
        r.setOutlineThickness(1.f);
        bool hovSw = item.first.contains(mPos);
        if (hovSw) hoveredTooltip = "Click swatch to set primary color (" + colorToHex(item.second) + ")";
        r.setOutlineColor(hovSw ? sf::Color::White : WisdomUI::Theme::Border);
        window.draw(r);
    }

    bool hovSuggest = suggestBtnBounds.contains(mPos);
    if (hovSuggest) hoveredTooltip = "Calculate 4-color shading ramp advice from current color";
    WisdomUI::Theme::DrawSunsetButton(window, suggestBtnBounds, "+ Get 4-Color Ramp Advice", font, 14, false, hovSuggest, true, 1.0f);

    bool hovSend = sendAllBtnBounds.contains(mPos);
    if (hovSend) hoveredTooltip = "Transfer all swatches from this palette to Colors panel";
    WisdomUI::Theme::DrawSunsetButton(window, sendAllBtnBounds, "--> Send All to Colors Panel", font, 13, false, hovSend, false, 1.0f);

    bool hovClose = closeBtnBounds.contains(mPos);
    if (hovClose) hoveredTooltip = "Close Color Assistant";
    WisdomUI::Theme::DrawSunsetButton(window, closeBtnBounds, "Close", font, 13, false, hovClose, false, 1.0f);

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

            WisdomUI::Theme::DrawCrispText(window, font, palettes[item.second].name, 12, item.first.left + 8.f, item.first.top + 6.f, isSel ? WisdomUI::Theme::SunsetGold : WisdomUI::Theme::TextPrimary);

            float pX = item.first.left + item.first.width - 56.f;
            for (size_t c = 0; c < std::min(static_cast<size_t>(4), palettes[item.second].colors.size()); ++c) {
                sf::RectangleShape sw(sf::Vector2f(10.f, 10.f));
                sw.setPosition(pX + c * 12.f, item.first.top + 9.f);
                sw.setFillColor(palettes[item.second].colors[c]);
                window.draw(sw);
            }
        }
    }

    if (!hoveredTooltip.empty() && !isDropdownOpen) {
        drawTooltip(window, hoveredTooltip, mPos);
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