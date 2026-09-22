#include "ColorPalettePanel.h"
#include "../UI/UITheme.h"
#include <iomanip>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <iostream>

static uint32_t s_orderCounter = 1;
static float s_palettesScroll = 0.f;
static float s_palettesMaxScroll = 0.f;
static sf::FloatRect s_palettesAreaBounds;

ColorPalettePanel::ColorPalettePanel()
    : width(310.f), currentX(1920.f), targetX(1920.f), state(PalettePanelState::Hidden),
    hovered(false), isDetached(false), detachedPos(1480.f, 75.f), detachedSize(310.f, 660.f),
    isDraggingWindow(false), windowDragOffset(0.f, 0.f), isResizing(false),
    activeResizeDir(PaletteResizeDir::None), pickerSize(220.f), pickerX(0.f), pickerY(0.f),
    currentHue(0.f), currentSat(1.f), currentVal(1.f), currentAlpha(1.f),
    isDraggingSV(false), isDraggingHue(false), isDraggingAlpha(false),
    activeInputIndex(-1), isEyedropperActive(false) {}

void ColorPalettePanel::init() {
    colorManager.init();
    font.loadFromFile("assets/font.otf");

    background.setFillColor(WisdomUI::Theme::Panel);
    background.setOutlineThickness(1.f);
    background.setOutlineColor(WisdomUI::Theme::Border);

    headerBg.setFillColor(WisdomUI::Theme::PanelInset);
    headerText.setFont(font);
    headerText.setString("COLORS");
    headerText.setCharacterSize(16);
    headerText.setFillColor(WisdomUI::Theme::Gold);

    closeBtn.setSize(sf::Vector2f(28.f, 28.f));
    closeBtn.setFillColor(WisdomUI::Theme::PanelInset);
    closeBtn.setOutlineThickness(1.f);
    closeBtn.setOutlineColor(WisdomUI::Theme::Border);
    closeText.setFont(font);
    closeText.setString("X");
    closeText.setCharacterSize(14);
    closeText.setFillColor(WisdomUI::Theme::TextSecondary);

    pinBtn.setSize(sf::Vector2f(48.f, 28.f));
    pinBtn.setFillColor(WisdomUI::Theme::PanelInset);
    pinBtn.setOutlineThickness(1.f);
    pinBtn.setOutlineColor(WisdomUI::Theme::Border);
    pinLabel.setFont(font);
    pinLabel.setString("Pin");
    pinLabel.setCharacterSize(13);
    pinLabel.setFillColor(WisdomUI::Theme::TextSecondary);

    detachBtn.setSize(sf::Vector2f(56.f, 28.f));
    detachBtn.setFillColor(WisdomUI::Theme::PanelInset);
    detachBtn.setOutlineThickness(1.f);
    detachBtn.setOutlineColor(WisdomUI::Theme::Border);
    detachLabel.setFont(font);
    detachLabel.setString("Float");
    detachLabel.setCharacterSize(13);
    detachLabel.setFillColor(WisdomUI::Theme::TextSecondary);

    primaryBox.setSize(sf::Vector2f(42.f, 42.f));
    primaryBox.setOutlineThickness(1.5f);
    primaryBox.setOutlineColor(WisdomUI::Theme::BorderHighlight);

    secondaryBox.setSize(sf::Vector2f(42.f, 42.f));
    secondaryBox.setOutlineThickness(1.f);
    secondaryBox.setOutlineColor(WisdomUI::Theme::Border);

    eyedropperBtn.setSize(sf::Vector2f(108.f, 30.f));
    eyedropperBtn.setFillColor(WisdomUI::Theme::PanelInset);
    eyedropperBtn.setOutlineThickness(1.f);
    eyedropperBtn.setOutlineColor(WisdomUI::Theme::Border);

    eyedropperLabel.setFont(font);
    eyedropperLabel.setString("Eyedropper");
    eyedropperLabel.setCharacterSize(13);
    eyedropperLabel.setFillColor(WisdomUI::Theme::TextPrimary);

    svImage.create(200, 200, sf::Color::Black);
    hueImage.create(200, 15, sf::Color::Black);
    alphaImage.create(200, 15, sf::Color::Black);

    for (int x = 0; x < 200; ++x) {
        float h = (x / 200.0f) * 360.0f;
        sf::Color c = ColorManager::hsvToRgb(h, 1.0f, 1.0f);
        for (int y = 0; y < 15; ++y) hueImage.setPixel(x, y, c);
    }
    hueTexture.loadFromImage(hueImage);
    hueSprite.setTexture(hueTexture);

    svSelector.setSize(sf::Vector2f(10.f, 10.f));
    svSelector.setOrigin(5.f, 5.f);
    svSelector.setOutlineThickness(1.5f);
    svSelector.setOutlineColor(WisdomUI::Theme::Gold);
    svSelector.setFillColor(sf::Color::Transparent);

    hueSelector.setSize(sf::Vector2f(6.f, 20.f));
    hueSelector.setOrigin(3.f, 0.f);
    hueSelector.setOutlineThickness(1.f);
    hueSelector.setOutlineColor(sf::Color::Black);
    hueSelector.setFillColor(WisdomUI::Theme::Gold);

    alphaSelector.setSize(sf::Vector2f(6.f, 20.f));
    alphaSelector.setOrigin(3.f, 0.f);
    alphaSelector.setOutlineThickness(1.f);
    alphaSelector.setOutlineColor(sf::Color::Black);
    alphaSelector.setFillColor(WisdomUI::Theme::Gold);

    updatePickerImages();
    loadAdvicePalettes();
}

void ColorPalettePanel::saveAdvicePalettes() const {
    std::ofstream out("projects/color_advice.dat");
    if (!out.is_open()) return;
    out << m_advicePalettes.size() << "\n";
    for (const auto& pal : m_advicePalettes) {
        out << (pal.isPinned ? 1 : 0) << " " << pal.useCount << " " << pal.order << " " << pal.colors.size() << " ";
        for (const auto& c : pal.colors) {
            out << static_cast<int>(c.r) << " " << static_cast<int>(c.g) << " " << static_cast<int>(c.b) << " " << static_cast<int>(c.a) << " ";
        }
        out << "\n";
    }
}

void ColorPalettePanel::loadAdvicePalettes() {
    std::ifstream in("projects/color_advice.dat");
    if (!in.is_open()) {
        in.open("color_advice.dat");
        if (!in.is_open()) return;
    }

    size_t total = 0;
    if (!(in >> total)) return;

    m_advicePalettes.clear();
    for (size_t i = 0; i < total; ++i) {
        int pinned = 0, useCount = 0;
        uint32_t order = 0;
        size_t numCols = 0;
        if (!(in >> pinned >> useCount >> order >> numCols)) break;

        AdvicePalette pal;
        pal.isPinned = (pinned != 0);
        pal.useCount = useCount;
        pal.order = order;
        if (order >= s_orderCounter) s_orderCounter = order + 1;

        for (size_t c = 0; c < numCols; ++c) {
            int r = 0, g = 0, b = 0, a = 255;
            in >> r >> g >> b >> a;
            pal.colors.push_back(sf::Color(static_cast<uint8_t>(r), static_cast<uint8_t>(g), static_cast<uint8_t>(b), static_cast<uint8_t>(a)));
        }
        if (!pal.colors.empty()) {
            m_advicePalettes.push_back(pal);
        }
    }
}

bool ColorPalettePanel::addAdvicePalette(const std::vector<sf::Color>& ramp) {
    if (ramp.empty()) return false;

    for (size_t i = 0; i < m_advicePalettes.size(); ++i) {
        if (m_advicePalettes[i].colors == ramp) {
            m_advicePalettes[i].useCount++;
            m_advicePalettes[i].order = s_orderCounter++;
            saveAdvicePalettes();
            return true;
        }
    }

    m_advicePalettes.push_back({ ramp, 0, false, s_orderCounter++ });
    saveAdvicePalettes();
    return true;
}

void ColorPalettePanel::removeAdvicePalette(size_t index) {
    if (index < m_advicePalettes.size()) {
        if (m_advicePalettes[index].isPinned) return;
        m_advicePalettes.erase(m_advicePalettes.begin() + index);
        saveAdvicePalettes();
    }
}

void ColorPalettePanel::clearAdvicePalettes() {
    m_advicePalettes.erase(
        std::remove_if(m_advicePalettes.begin(), m_advicePalettes.end(),
            [](const AdvicePalette& p) { return !p.isPinned; }),
        m_advicePalettes.end()
    );
    saveAdvicePalettes();
}

std::string ColorPalettePanel::colorToHex(sf::Color c) const {
    std::stringstream ss;
    ss << "#" << std::uppercase << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(c.r)
        << std::setw(2) << static_cast<int>(c.g)
        << std::setw(2) << static_cast<int>(c.b);
    return ss.str();
}

void ColorPalettePanel::updateFromRGB(sf::Color c) {
    ColorManager::rgbToHsv(c, currentHue, currentSat, currentVal);
    currentAlpha = c.a / 255.0f;
    updatePickerImages();
}

void ColorPalettePanel::updatePickerImages() {
    for (int y = 0; y < 200; ++y) {
        for (int x = 0; x < 200; ++x) {
            float s = x / 200.0f;
            float v = 1.0f - (y / 200.0f);
            svImage.setPixel(x, y, ColorManager::hsvToRgb(currentHue, s, v));
        }
    }
    svTexture.loadFromImage(svImage);
    svSprite.setTexture(svTexture);

    sf::Color baseC = ColorManager::hsvToRgb(currentHue, currentSat, currentVal);
    for (int x = 0; x < 200; ++x) {
        float a = x / 200.0f;
        sf::Color c = baseC;
        c.a = static_cast<sf::Uint8>(a * 255.0f);
        for (int y = 0; y < 15; ++y) alphaImage.setPixel(x, y, c);
    }
    alphaTexture.loadFromImage(alphaImage);
    alphaSprite.setTexture(alphaTexture);
}

PaletteResizeDir ColorPalettePanel::getResizeDirection(sf::Vector2f mousePos) const {
    if (!isDetached) return PaletteResizeDir::None;

    sf::FloatRect bounds(detachedPos, detachedSize);
    const float m = 8.f;

    sf::FloatRect outer(bounds.left - m, bounds.top - m, bounds.width + 2.f * m, bounds.height + 2.f * m);
    if (!outer.contains(mousePos)) return PaletteResizeDir::None;

    bool onLeft = (mousePos.x >= bounds.left - m && mousePos.x <= bounds.left + m);
    bool onRight = (mousePos.x >= bounds.left + bounds.width - m && mousePos.x <= bounds.left + bounds.width + m);
    bool onTop = (mousePos.y >= bounds.top - m && mousePos.y <= bounds.top + m);
    bool onBottom = (mousePos.y >= bounds.top + bounds.height - m && mousePos.y <= bounds.top + bounds.height + m);

    if (onTop && onLeft) return PaletteResizeDir::TopLeft;
    if (onTop && onRight) return PaletteResizeDir::TopRight;
    if (onBottom && onLeft) return PaletteResizeDir::BottomLeft;
    if (onBottom && onRight) return PaletteResizeDir::BottomRight;
    if (onLeft) return PaletteResizeDir::Left;
    if (onRight) return PaletteResizeDir::Right;
    if (onTop) return PaletteResizeDir::Top;
    if (onBottom) return PaletteResizeDir::Bottom;

    return PaletteResizeDir::None;
}

void ColorPalettePanel::update(float dt, bool focusMode, Canvas& canvas, bool isOpen) {
    if (isDetached) {
        currentX = detachedPos.x;
        width = detachedSize.x;
    }
    else {
        width = 310.f;
        if (focusMode || !isOpen) targetX = 1920.f;
        else targetX = 1920.f - 44.f - width;

        currentX += (targetX - currentX) * 16.0f * dt;
    }

    float topBarsH = WisdomUI::Theme::TopBarHeight + WisdomUI::Theme::OptionsBarHeight;
    float panelX = std::floor(isDetached ? detachedPos.x : currentX);
    float panelY = std::floor(isDetached ? detachedPos.y : topBarsH);
    float panelW = std::floor(isDetached ? detachedSize.x : width);
    float panelH = std::floor(isDetached ? detachedSize.y : (1080.f - topBarsH - WisdomUI::Theme::StatusBarHeight));

    background.setPosition(panelX, panelY);
    background.setSize(sf::Vector2f(panelW, panelH));

    headerBg.setPosition(panelX, panelY);
    headerBg.setSize(sf::Vector2f(panelW, 36.f));
    headerText.setPosition(panelX + 14.f, panelY + 8.f);

    closeBtn.setPosition(panelX + panelW - 34.f, panelY + 4.f);
    closeText.setPosition(std::floor(closeBtn.getPosition().x + 8.f), std::floor(closeBtn.getPosition().y + 4.f));

    detachBtn.setPosition(panelX + panelW - 96.f, panelY + 4.f);
    detachLabel.setString(isDetached ? "Dock" : "Float");
    detachLabel.setPosition(std::floor(detachBtn.getPosition().x + 12.f), std::floor(detachBtn.getPosition().y + 5.f));

    pinBtn.setPosition(panelX + panelW - 150.f, panelY + 4.f);
    pinLabel.setPosition(std::floor(pinBtn.getPosition().x + 13.f), std::floor(pinBtn.getPosition().y + 5.f));

    primaryBox.setPosition(panelX + 16.f, panelY + 44.f);
    secondaryBox.setPosition(panelX + 36.f, panelY + 62.f);

    primaryBox.setFillColor(canvas.getPrimaryColor());
    secondaryBox.setFillColor(canvas.getSecondaryColor());

    eyedropperBtn.setPosition(panelX + 96.f, panelY + 52.f);
    if (isEyedropperActive) {
        eyedropperBtn.setFillColor(WisdomUI::Theme::Accent);
        eyedropperBtn.setOutlineColor(WisdomUI::Theme::BorderHighlight);
        eyedropperLabel.setFillColor(sf::Color::White);
    }
    else {
        eyedropperBtn.setFillColor(WisdomUI::Theme::PanelInset);
        eyedropperBtn.setOutlineColor(WisdomUI::Theme::Border);
        eyedropperLabel.setFillColor(WisdomUI::Theme::TextSecondary);
    }

    sf::FloatRect dropBounds = eyedropperLabel.getLocalBounds();
    float dropTextX = eyedropperBtn.getPosition().x + (eyedropperBtn.getSize().x - dropBounds.width) / 2.0f;
    float dropTextY = eyedropperBtn.getPosition().y + (eyedropperBtn.getSize().y - dropBounds.height) / 2.0f - 2.0f;
    eyedropperLabel.setPosition(std::floor(dropTextX), std::floor(dropTextY));

    float maxSVFromHeight = panelH - 310.f;
    float maxSVFromWidth = panelW - 32.f;
    pickerSize = std::floor(std::clamp(std::min(maxSVFromWidth, maxSVFromHeight), 140.f, 320.f));

    pickerX = std::floor(panelX + (panelW - pickerSize) * 0.5f);
    pickerY = std::floor(panelY + 112.f);

    svSprite.setPosition(pickerX, pickerY);
    svSprite.setScale(pickerSize / 200.f, pickerSize / 200.f);
    svSelector.setPosition(std::floor(pickerX + currentSat * pickerSize), std::floor(pickerY + (1.0f - currentVal) * pickerSize));

    float hueY = std::floor(pickerY + pickerSize + 10.f);
    hueSprite.setPosition(pickerX, hueY);
    hueSprite.setScale(pickerSize / 200.f, 20.f / 15.f);
    hueSelector.setSize(sf::Vector2f(6.f, 20.f));
    hueSelector.setPosition(std::floor(pickerX + (currentHue / 360.f) * pickerSize), hueY);

    float alphaY = std::floor(hueY + 26.f);
    alphaSprite.setPosition(pickerX, alphaY);
    alphaSprite.setScale(pickerSize / 200.f, 20.f / 15.f);
    alphaSelector.setSize(sf::Vector2f(6.f, 20.f));
    alphaSelector.setPosition(std::floor(pickerX + currentAlpha * pickerSize), alphaY);
}

void ColorPalettePanel::updateHover(sf::Vector2f mousePos, bool canOpen) {
    bool inPanel = background.getGlobalBounds().contains(mousePos);
    if (state == PalettePanelState::Hidden) {
        if (canOpen && inPanel) state = PalettePanelState::Visible;
    }
    else if (state == PalettePanelState::Visible) {
        if (!inPanel && !isDetached) state = PalettePanelState::Hidden;
    }
}

void ColorPalettePanel::drawTooltip(sf::RenderWindow& window, const std::string& text, sf::Vector2f pos) {
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

void ColorPalettePanel::draw(sf::RenderWindow& window) {
    if (!isDetached && currentX >= 1918.f) return;

    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    std::string hoveredTooltip = "";

    WisdomUI::Theme::DrawFiligreePanel(window, background.getGlobalBounds(), 1.0f);

    window.draw(headerBg);
    window.draw(headerText);

    auto styleBtn = [&](sf::RectangleShape& r, sf::Text& t, const std::string& tip) {
        bool hov = r.getGlobalBounds().contains(mousePos);
        if (hov) hoveredTooltip = tip;
        r.setFillColor(hov ? WisdomUI::Theme::PanelHover : WisdomUI::Theme::PanelInset);
        r.setOutlineColor(hov ? WisdomUI::Theme::BorderHighlight : WisdomUI::Theme::Border);
        window.draw(r);
        window.draw(t);
        };

    styleBtn(closeBtn, closeText, "Close Color Panel");
    styleBtn(detachBtn, detachLabel, isDetached ? "Dock panel back to edge" : "Detach panel into floating window");
    if (!isDetached) {
        styleBtn(pinBtn, pinLabel, (state == PalettePanelState::Pinned) ? "Unpin color panel" : "Pin color panel open");
    }

    if (primaryBox.getGlobalBounds().contains(mousePos)) hoveredTooltip = "Active Primary Color";
    if (secondaryBox.getGlobalBounds().contains(mousePos)) hoveredTooltip = "Secondary / Background Color";
    window.draw(secondaryBox);
    window.draw(primaryBox);

    bool hovEye = eyedropperBtn.getGlobalBounds().contains(mousePos);
    if (hovEye) hoveredTooltip = "Eyedropper: Pick color from canvas";
    window.draw(eyedropperBtn);
    window.draw(eyedropperLabel);

    if (svSprite.getGlobalBounds().contains(mousePos)) hoveredTooltip = "Drag to adjust Saturation and Value";
    if (hueSprite.getGlobalBounds().contains(mousePos)) hoveredTooltip = "Drag to adjust Hue angle";
    if (alphaSprite.getGlobalBounds().contains(mousePos)) hoveredTooltip = "Drag to adjust Opacity (Alpha)";

    sf::RectangleShape svOutline(sf::Vector2f(pickerSize + 2.f, pickerSize + 2.f));
    svOutline.setPosition(pickerX - 1.f, pickerY - 1.f);
    svOutline.setFillColor(sf::Color::Transparent);
    svOutline.setOutlineThickness(1.f);
    svOutline.setOutlineColor(WisdomUI::Theme::Border);
    window.draw(svOutline);

    window.draw(svSprite);
    window.draw(svSelector);

    window.draw(hueSprite);
    window.draw(hueSelector);

    window.draw(alphaSprite);
    window.draw(alphaSelector);

    float panelY = background.getPosition().y;
    float panelH = background.getSize().y;

    float inputY = std::floor(alphaSprite.getPosition().y + 28.f);

    auto drawInput = [&](int index, const std::string& label, const std::string& val, float x, float w, const std::string& tip) {
        sf::FloatRect wholeBox(x, inputY, w, 44.f);
        if (wholeBox.contains(mousePos)) hoveredTooltip = tip;

        sf::Text t(label, font, 13);
        t.setPosition(std::floor(x), std::floor(inputY));
        t.setFillColor(WisdomUI::Theme::TextSecondary);
        window.draw(t);

        sf::RectangleShape box(sf::Vector2f(w, 26.f));
        box.setPosition(std::floor(x), std::floor(inputY + 16.f));
        box.setFillColor(WisdomUI::Theme::PanelInset);
        box.setOutlineThickness(1.f);
        box.setOutlineColor(activeInputIndex == index ? WisdomUI::Theme::BorderHighlight : WisdomUI::Theme::Border);
        window.draw(box);

        std::string displayVal = val;
        if (activeInputIndex == index) {
            if (index == 4 && (inputBuffer.empty() || inputBuffer[0] != '#')) {
                displayVal = "#" + inputBuffer + "_";
            }
            else {
                displayVal = inputBuffer + "_";
            }
        }
        sf::Text v(displayVal, font, 13);
        sf::FloatRect vBounds = v.getLocalBounds();
        v.setPosition(std::floor(x + (w - vBounds.width) / 2.f), std::floor(inputY + 20.f));
        v.setFillColor(WisdomUI::Theme::Gold);
        window.draw(v);
        };

    sf::Color curC = ColorManager::hsvToRgb(currentHue, currentSat, currentVal);
    curC.a = static_cast<sf::Uint8>(currentAlpha * 255.f);

    float boxW = 38.f;
    float hexW = 74.f;
    float boxSpacing = (pickerSize - (4.f * boxW) - hexW) / 4.f;
    boxSpacing = std::max(2.f, boxSpacing);

    float curInputX = pickerX;
    drawInput(0, "R", std::to_string(curC.r), curInputX, boxW, "Red Channel (0-255)"); curInputX += boxW + boxSpacing;
    drawInput(1, "G", std::to_string(curC.g), curInputX, boxW, "Green Channel (0-255)"); curInputX += boxW + boxSpacing;
    drawInput(2, "B", std::to_string(curC.b), curInputX, boxW, "Blue Channel (0-255)"); curInputX += boxW + boxSpacing;
    drawInput(3, "A", std::to_string(curC.a), curInputX, boxW, "Alpha Channel (0-255)"); curInputX += boxW + boxSpacing;
    drawInput(4, "Hex", colorToHex(curC), curInputX, hexW, "Hexadecimal Color Code");

    float sx = pickerX;
    float sy = std::floor(inputY + 48.f);
    if (sy + 22.f < panelY + panelH) {
        sf::Text rt("Recent", font, 13);
        rt.setPosition(std::floor(sx), std::floor(sy));
        rt.setFillColor(WisdomUI::Theme::Gold);
        window.draw(rt);
        sy += 18.f;

        for (const auto& c : colorManager.getRecentColors()) {
            if (sy + 22.f >= panelY + panelH - 4.f) break;
            sf::FloatRect swRect(std::floor(sx), std::floor(sy), 22.f, 22.f);
            if (swRect.contains(mousePos)) hoveredTooltip = "Recently used color";
            sf::RectangleShape s(sf::Vector2f(22.f, 22.f));
            s.setPosition(swRect.left, swRect.top);
            s.setFillColor(c);
            s.setOutlineThickness(1.f);
            s.setOutlineColor(WisdomUI::Theme::Border);
            window.draw(s);
            sx += 26.f;
            if (sx > pickerX + pickerSize - 22.f) { sx = pickerX; sy += 26.f; }
        }

        sx = pickerX;
        sy += 30.f;
        if (sy + 22.f < panelY + panelH) {
            sf::Text ct("Swatches", font, 13);
            ct.setPosition(std::floor(sx), std::floor(sy));
            ct.setFillColor(WisdomUI::Theme::Gold);
            window.draw(ct);

            sf::FloatRect addBtnRect(std::floor(pickerX + pickerSize - 22.f), std::floor(sy), 22.f, 22.f);
            m_addSwatchBtnBounds = sf::FloatRect(std::floor(pickerX + pickerSize - 22.f), std::floor(sy), 22.f, 22.f);
            bool hovAdd = m_addSwatchBtnBounds.contains(mousePos);
            if (hovAdd) hoveredTooltip = "+ Save current color to swatches";
            sf::RectangleShape addSwatchBtn(sf::Vector2f(m_addSwatchBtnBounds.width, m_addSwatchBtnBounds.height));
            addSwatchBtn.setPosition(m_addSwatchBtnBounds.left, m_addSwatchBtnBounds.top);
            addSwatchBtn.setFillColor(hovAdd ? WisdomUI::Theme::PanelHover : WisdomUI::Theme::PanelInset);
            addSwatchBtn.setOutlineThickness(1.f);
            addSwatchBtn.setOutlineColor(hovAdd ? WisdomUI::Theme::BorderHighlight : WisdomUI::Theme::Border);
            window.draw(addSwatchBtn);
            sf::Text plus("+", font, 15);
            plus.setPosition(std::floor(addSwatchBtn.getPosition().x + 5.f), std::floor(addSwatchBtn.getPosition().y - 1.f));
            plus.setFillColor(WisdomUI::Theme::Gold);
            window.draw(plus);

            sy += 24.f;
            float swatchStartY = sy;
            m_customSwatchBounds.clear();

            for (size_t i = 0; i < colorManager.getCustomSwatches().size(); ++i) {
                if (sy + 24.f >= panelY + panelH - 4.f) break;
                sf::Color c = colorManager.getCustomSwatches()[i];
                sf::FloatRect swRect(std::floor(sx), std::floor(sy), 24.f, 24.f);
                m_customSwatchBounds.push_back({ swRect, i });

                if (swRect.contains(mousePos)) hoveredTooltip = "Swatch (Drag box to select multiple, Right-Click to delete)";

                bool isSelected = std::find(m_selectedSwatchIndices.begin(), m_selectedSwatchIndices.end(), i) != m_selectedSwatchIndices.end();

                sf::RectangleShape s(sf::Vector2f(24.f, 24.f));
                s.setPosition(std::floor(sx), std::floor(sy));
                s.setFillColor(c);
                s.setOutlineThickness(isSelected ? 2.f : 1.f);
                s.setOutlineColor(isSelected ? sf::Color(0, 220, 255) : WisdomUI::Theme::Border);
                window.draw(s);

                if (isSelected) {
                    sf::RectangleShape selCover(sf::Vector2f(24.f, 24.f));
                    selCover.setPosition(std::floor(sx), std::floor(sy));
                    selCover.setFillColor(sf::Color(0, 220, 255, 60));
                    window.draw(selCover);
                }

                sx += 28.f;
                if (sx > pickerX + pickerSize - 24.f) { sx = pickerX; sy += 28.f; }
            }

            m_swatchAreaBounds = sf::FloatRect(pickerX, swatchStartY, pickerSize, std::max(28.f, sy - swatchStartY + 28.f));
        }

        m_adviceSwatchBounds.clear();
        m_advicePinBtnBounds.clear();
        m_adviceDeleteBtnBounds.clear();

        sx = pickerX;
        sy += 34.f;

        if (sy + 24.f < panelY + panelH) {
            sf::Text at("Palettes", font, 13);
            at.setPosition(std::floor(sx), std::floor(sy));
            at.setFillColor(WisdomUI::Theme::Gold);
            window.draw(at);

            m_adviceClearAllBounds = sf::FloatRect(pickerX + pickerSize - 48.f, sy - 2.f, 48.f, 20.f);
            bool hovClear = m_adviceClearAllBounds.contains(mousePos);
            if (hovClear) hoveredTooltip = "Clear all unpinned palettes";
            sf::RectangleShape clearBtn(sf::Vector2f(m_adviceClearAllBounds.width, m_adviceClearAllBounds.height));
            clearBtn.setPosition(m_adviceClearAllBounds.left, m_adviceClearAllBounds.top);
            clearBtn.setFillColor(hovClear ? sf::Color(140, 30, 45) : WisdomUI::Theme::PanelInset);
            clearBtn.setOutlineThickness(1.f);
            clearBtn.setOutlineColor(WisdomUI::Theme::Border);
            window.draw(clearBtn);

            sf::Text clrTxt("Clear", font, 11);
            clrTxt.setPosition(m_adviceClearAllBounds.left + 8.f, m_adviceClearAllBounds.top + 2.f);
            clrTxt.setFillColor(hovClear ? sf::Color::White : WisdomUI::Theme::TextSecondary);
            window.draw(clrTxt);

            sy += 24.f;

            float areaH = std::max(20.f, (panelY + panelH) - sy - 8.f);
            s_palettesAreaBounds = sf::FloatRect(pickerX, sy, pickerSize, areaH);

            float curEntryY = sy - s_palettesScroll;
            float totalH = 0.f;

            for (size_t pIdx = 0; pIdx < m_advicePalettes.size(); ++pIdx) {
                const auto& pal = m_advicePalettes[pIdx];

                int perRow = std::max(1, static_cast<int>(pickerSize / 26.f));
                int rows = (static_cast<int>(pal.colors.size()) + perRow - 1) / perRow;
                if (rows == 0) rows = 1;
                float entryH = 22.f + (rows * 26.f) + 6.f;
                totalH += entryH;

                bool isVisibleRow = (curEntryY + entryH >= s_palettesAreaBounds.top && curEntryY <= s_palettesAreaBounds.top + s_palettesAreaBounds.height);

                if (isVisibleRow) {
                    if (pal.isPinned) {
                        sf::RectangleShape pinGlow(sf::Vector2f(pickerSize, entryH - 2.f));
                        pinGlow.setPosition(pickerX, curEntryY);
                        pinGlow.setFillColor(sf::Color(255, 180, 40, 20));
                        pinGlow.setOutlineThickness(1.f);
                        pinGlow.setOutlineColor(WisdomUI::Theme::Gold);
                        window.draw(pinGlow);
                    }

                    std::string numLabel = std::to_string(pIdx + 1) + ".";
                    sf::Text numText(numLabel, font, 12);
                    numText.setPosition(pickerX + 2.f, curEntryY);
                    numText.setFillColor(WisdomUI::Theme::Gold);
                    window.draw(numText);

                    sf::FloatRect pinRect(pickerX + pickerSize - 44.f, curEntryY, 20.f, 18.f);
                    bool hovPin = pinRect.contains(mousePos);
                    sf::RectangleShape pBtn(sf::Vector2f(pinRect.width, pinRect.height));
                    pBtn.setPosition(pinRect.left, pinRect.top);
                    pBtn.setFillColor(pal.isPinned ? sf::Color(240, 175, 45) : (hovPin ? WisdomUI::Theme::PanelHover : WisdomUI::Theme::PanelInset));
                    pBtn.setOutlineThickness(1.f);
                    pBtn.setOutlineColor(pal.isPinned ? WisdomUI::Theme::Gold : WisdomUI::Theme::Border);
                    window.draw(pBtn);

                    sf::Text pinTxt("P", font, 11);
                    pinTxt.setPosition(pinRect.left + 6.f, pinRect.top - 1.f);
                    pinTxt.setFillColor(pal.isPinned ? sf::Color(14, 6, 20) : (hovPin ? sf::Color::White : WisdomUI::Theme::TextSecondary));
                    window.draw(pinTxt);
                    m_advicePinBtnBounds.push_back({ pinRect, pIdx });

                    sf::FloatRect delRect(pickerX + pickerSize - 20.f, curEntryY, 18.f, 18.f);
                    bool hovDel = delRect.contains(mousePos);
                    sf::RectangleShape delBtn(sf::Vector2f(delRect.width, delRect.height));
                    delBtn.setPosition(delRect.left, delRect.top);
                    delBtn.setFillColor(pal.isPinned ? sf::Color(35, 25, 40, 100) : (hovDel ? sf::Color(180, 40, 55) : WisdomUI::Theme::PanelInset));
                    delBtn.setOutlineThickness(1.f);
                    delBtn.setOutlineColor(WisdomUI::Theme::Border);
                    window.draw(delBtn);

                    sf::Text xTxt("x", font, 11);
                    xTxt.setPosition(delRect.left + 5.f, delRect.top - 2.f);
                    xTxt.setFillColor(pal.isPinned ? sf::Color(90, 80, 95) : (hovDel ? sf::Color::White : WisdomUI::Theme::TextSecondary));
                    window.draw(xTxt);
                    m_adviceDeleteBtnBounds.push_back({ delRect, pIdx });

                    float swX = pickerX + 2.f;
                    float swY = curEntryY + 20.f;

                    for (size_t cIdx = 0; cIdx < pal.colors.size(); ++cIdx) {
                        sf::FloatRect swRect(std::floor(swX), std::floor(swY), 22.f, 22.f);
                        if (swRect.top + 22.f >= s_palettesAreaBounds.top && swRect.top <= s_palettesAreaBounds.top + s_palettesAreaBounds.height) {
                            bool hovSw = swRect.contains(mousePos);
                            if (hovSw) hoveredTooltip = colorToHex(pal.colors[cIdx]);

                            sf::RectangleShape s(sf::Vector2f(swRect.width, swRect.height));
                            s.setPosition(swRect.left, swRect.top);
                            s.setFillColor(pal.colors[cIdx]);
                            s.setOutlineThickness(1.f);
                            s.setOutlineColor(hovSw ? sf::Color::White : (pal.isPinned ? WisdomUI::Theme::Gold : WisdomUI::Theme::BorderHighlight));
                            window.draw(s);

                            m_adviceSwatchBounds.push_back({ swRect, { pal.colors[cIdx], pIdx } });
                        }

                        swX += 26.f;
                        if (swX + 22.f > pickerX + pickerSize) {
                            swX = pickerX + 2.f;
                            swY += 26.f;
                        }
                    }
                }

                curEntryY += entryH;
            }

            s_palettesMaxScroll = std::max(0.f, totalH - s_palettesAreaBounds.height);
        }
    }

    if (m_isBoxSelectingSwatches) {
        float left = std::min(m_swatchSelectStart.x, m_swatchSelectEnd.x);
        float top = std::min(m_swatchSelectStart.y, m_swatchSelectEnd.y);
        float w = std::abs(m_swatchSelectEnd.x - m_swatchSelectStart.x);
        float h = std::abs(m_swatchSelectEnd.y - m_swatchSelectStart.y);
        sf::RectangleShape selRect(sf::Vector2f(w, h));
        selRect.setPosition(left, top);
        selRect.setFillColor(sf::Color(0, 200, 255, 40));
        selRect.setOutlineThickness(1.f);
        selRect.setOutlineColor(sf::Color(0, 220, 255, 200));
        window.draw(selRect);
    }

    if (m_showSwatchContextMenu) {
        sf::RectangleShape menuBox(sf::Vector2f(m_swatchDeleteBtnBounds.width, m_swatchDeleteBtnBounds.height));
        menuBox.setPosition(m_swatchDeleteBtnBounds.left, m_swatchDeleteBtnBounds.top);
        menuBox.setFillColor(sf::Color(18, 10, 24, 250));
        menuBox.setOutlineThickness(1.5f);
        menuBox.setOutlineColor(WisdomUI::Theme::SunsetGold);
        window.draw(menuBox);

        bool hovMenuDel = m_swatchDeleteBtnBounds.contains(mousePos);
        sf::RectangleShape btnInner(sf::Vector2f(m_swatchDeleteBtnBounds.width - 4.f, m_swatchDeleteBtnBounds.height - 4.f));
        btnInner.setPosition(m_swatchDeleteBtnBounds.left + 2.f, m_swatchDeleteBtnBounds.top + 2.f);
        btnInner.setFillColor(hovMenuDel ? sf::Color(180, 35, 50) : sf::Color(120, 25, 35));
        window.draw(btnInner);

        std::string delStr = "Delete (" + std::to_string(m_selectedSwatchIndices.size()) + ")";
        WisdomUI::Theme::DrawCrispText(window, font, delStr, 13, m_swatchDeleteBtnBounds.left + m_swatchDeleteBtnBounds.width / 2.f, m_swatchDeleteBtnBounds.top + m_swatchDeleteBtnBounds.height / 2.f, sf::Color::White, sf::Color::Transparent, true, true);
    }

    if (isDetached) {
        PaletteResizeDir dir = isResizing ? activeResizeDir : getResizeDirection(mousePos);
        if (dir != PaletteResizeDir::None) {
            sf::FloatRect b = background.getGlobalBounds();
            sf::RectangleShape guide(sf::Vector2f(b.width, b.height));
            guide.setPosition(b.left, b.top);
            guide.setFillColor(sf::Color::Transparent);
            guide.setOutlineThickness(1.5f);
            guide.setOutlineColor(WisdomUI::Theme::Gold);
            window.draw(guide);
        }
    }

    if (!hoveredTooltip.empty() && !m_isBoxSelectingSwatches && !isDraggingSV && !isDraggingHue && !isDraggingAlpha) {
        drawTooltip(window, hoveredTooltip, mousePos);
    }
}

bool ColorPalettePanel::handleEvent(const sf::Event& event, sf::Vector2f mousePos, Canvas& canvas) {
    if (event.type == sf::Event::MouseWheelScrolled) {
        if (s_palettesAreaBounds.contains(mousePos)) {
            s_palettesScroll = std::clamp(s_palettesScroll - event.mouseWheelScroll.delta * 32.f, 0.f, s_palettesMaxScroll);
            return true;
        }
    }

    if (activeInputIndex != -1) {
        if (event.type == sf::Event::KeyPressed) {
            if (event.key.control && event.key.code == sf::Keyboard::V) {
                std::string clip = sf::Clipboard::getString().toAnsiString();
                if (activeInputIndex == 4) {
                    std::string clean;
                    for (char c : clip) {
                        if (c == '#' || c == ' ' || c == '\r' || c == '\n') continue;
                        if (std::isxdigit(static_cast<unsigned char>(c))) clean += static_cast<char>(std::toupper(c));
                    }
                    if (!clean.empty()) inputBuffer = clean.substr(0, 6);
                }
                else {
                    std::string digits;
                    for (char c : clip) {
                        if (std::isdigit(static_cast<unsigned char>(c))) digits += c;
                    }
                    if (!digits.empty()) inputBuffer = digits.substr(0, 3);
                }
                return true;
            }

            if (event.key.code == sf::Keyboard::Escape) {
                activeInputIndex = -1;
                inputBuffer.clear();
                return true;
            }

            if (event.key.code == sf::Keyboard::Enter) {
                sf::Color curC = ColorManager::hsvToRgb(currentHue, currentSat, currentVal);
                curC.a = static_cast<sf::Uint8>(currentAlpha * 255.f);
                try {
                    if (activeInputIndex == 0 && !inputBuffer.empty()) curC.r = std::clamp(std::stoi(inputBuffer), 0, 255);
                    else if (activeInputIndex == 1 && !inputBuffer.empty()) curC.g = std::clamp(std::stoi(inputBuffer), 0, 255);
                    else if (activeInputIndex == 2 && !inputBuffer.empty()) curC.b = std::clamp(std::stoi(inputBuffer), 0, 255);
                    else if (activeInputIndex == 3 && !inputBuffer.empty()) curC.a = std::clamp(std::stoi(inputBuffer), 0, 255);
                    else if (activeInputIndex == 4 && !inputBuffer.empty()) {
                        std::string hex = inputBuffer;
                        if (!hex.empty() && hex[0] == '#') hex = hex.substr(1);
                        if (hex.length() == 6) {
                            curC.r = std::stoi(hex.substr(0, 2), nullptr, 16);
                            curC.g = std::stoi(hex.substr(2, 2), nullptr, 16);
                            curC.b = std::stoi(hex.substr(4, 2), nullptr, 16);
                        }
                    }
                }
                catch (...) {}
                updateFromRGB(curC);
                canvas.setPrimaryColor(curC);
                colorManager.addRecentColor(curC);
                activeInputIndex = -1;
                inputBuffer.clear();
                return true;
            }
            return true;
        }

        if (event.type == sf::Event::KeyReleased) return true;

        if (event.type == sf::Event::TextEntered) {
            if (event.text.unicode == '\b') {
                if (!inputBuffer.empty()) inputBuffer.pop_back();
                return true;
            }
            if (event.text.unicode < 32 || event.text.unicode == 127) return true;
            char c = static_cast<char>(event.text.unicode);
            if (activeInputIndex == 4) {
                if (c != '#' && std::isxdigit(static_cast<unsigned char>(c)) && inputBuffer.length() < 6) {
                    inputBuffer += static_cast<char>(std::toupper(c));
                }
            }
            else {
                if (std::isdigit(static_cast<unsigned char>(c)) && inputBuffer.length() < 3) {
                    inputBuffer += c;
                }
            }
            return true;
        }
    }

    if (event.type == sf::Event::KeyPressed || event.type == sf::Event::KeyReleased || event.type == sf::Event::TextEntered) {
        return false;
    }

    if (m_showSwatchContextMenu) {
        if (event.type == sf::Event::MouseButtonPressed) {
            if (event.mouseButton.button == sf::Mouse::Left && m_swatchDeleteBtnBounds.contains(mousePos)) {
                std::sort(m_selectedSwatchIndices.rbegin(), m_selectedSwatchIndices.rend());
                m_selectedSwatchIndices.erase(std::unique(m_selectedSwatchIndices.begin(), m_selectedSwatchIndices.end()), m_selectedSwatchIndices.end());
                for (size_t idx : m_selectedSwatchIndices) {
                    colorManager.removeCustomSwatch(static_cast<int>(idx));
                }
                m_selectedSwatchIndices.clear();
                m_showSwatchContextMenu = false;
                return true;
            }
            m_showSwatchContextMenu = false;
        }
    }

    if (event.type == sf::Event::MouseButtonPressed) {
        if (event.mouseButton.button == sf::Mouse::Right) {
            if (!m_selectedSwatchIndices.empty() && m_swatchAreaBounds.contains(mousePos)) {
                m_showSwatchContextMenu = true;
                m_swatchDeleteBtnBounds = sf::FloatRect(mousePos.x, mousePos.y, 120.f, 30.f);
                return true;
            }
        }
        else if (event.mouseButton.button == sf::Mouse::Left) {
            if (m_swatchAreaBounds.contains(mousePos)) {
                m_isBoxSelectingSwatches = true;
                m_swatchSelectStart = mousePos;
                m_swatchSelectEnd = mousePos;
                m_selectedSwatchIndices.clear();
                m_showSwatchContextMenu = false;
            }

            if (closeBtn.getGlobalBounds().contains(mousePos)) {
                forceClose();
                return true;
            }

            if (detachBtn.getGlobalBounds().contains(mousePos)) {
                isDetached = !isDetached;
                if (isDetached) state = PalettePanelState::Visible;
                return true;
            }

            if (!isDetached && pinBtn.getGlobalBounds().contains(mousePos)) {
                state = (state == PalettePanelState::Pinned) ? PalettePanelState::Visible : PalettePanelState::Pinned;
                return true;
            }

            if (isDetached) {
                PaletteResizeDir dir = getResizeDirection(mousePos);
                if (dir != PaletteResizeDir::None) {
                    isResizing = true;
                    activeResizeDir = dir;
                    resizeStartMouse = mousePos;
                    resizeStartBounds = sf::FloatRect(detachedPos, detachedSize);
                    return true;
                }
            }

            if (headerBg.getGlobalBounds().contains(mousePos)) {
                if (!isDetached) {
                    isDetached = true;
                    detachedPos = sf::Vector2f(currentX, 68.f);
                    detachedSize = sf::Vector2f(width, 640.f);
                }
                isDraggingWindow = true;
                windowDragOffset = mousePos - detachedPos;
                return true;
            }

            if (eyedropperBtn.getGlobalBounds().contains(mousePos)) {
                isEyedropperActive = !isEyedropperActive;
                return true;
            }

            if (svSprite.getGlobalBounds().contains(mousePos)) isDraggingSV = true;
            else if (hueSprite.getGlobalBounds().contains(mousePos)) isDraggingHue = true;
            else if (alphaSprite.getGlobalBounds().contains(mousePos)) isDraggingAlpha = true;

            float inputY = std::floor(alphaSprite.getPosition().y + 28.f + 16.f);
            float boxW = 38.f;
            float hexW = 74.f;
            float boxSpacing = (pickerSize - (4.f * boxW) - hexW) / 4.f;
            boxSpacing = std::max(2.f, boxSpacing);

            float curInputX = pickerX;
            if (sf::FloatRect(curInputX, inputY, boxW, 26.f).contains(mousePos)) { activeInputIndex = 0; inputBuffer = ""; return true; } curInputX += boxW + boxSpacing;
            if (sf::FloatRect(curInputX, inputY, boxW, 26.f).contains(mousePos)) { activeInputIndex = 1; inputBuffer = ""; return true; } curInputX += boxW + boxSpacing;
            if (sf::FloatRect(curInputX, inputY, boxW, 26.f).contains(mousePos)) { activeInputIndex = 2; inputBuffer = ""; return true; } curInputX += boxW + boxSpacing;
            if (sf::FloatRect(curInputX, inputY, boxW, 26.f).contains(mousePos)) { activeInputIndex = 3; inputBuffer = ""; return true; } curInputX += boxW + boxSpacing;
            if (sf::FloatRect(curInputX, inputY, hexW, 26.f).contains(mousePos)) { activeInputIndex = 4; inputBuffer = ""; return true; }

            activeInputIndex = -1;

            if (m_adviceClearAllBounds.contains(mousePos)) {
                clearAdvicePalettes();
                return true;
            }

            for (const auto& item : m_advicePinBtnBounds) {
                if (item.first.contains(mousePos)) {
                    size_t pIdx = item.second;
                    if (pIdx < m_advicePalettes.size()) {
                        m_advicePalettes[pIdx].isPinned = !m_advicePalettes[pIdx].isPinned;
                        saveAdvicePalettes();
                    }
                    return true;
                }
            }

            for (const auto& item : m_adviceDeleteBtnBounds) {
                if (item.first.contains(mousePos)) {
                    removeAdvicePalette(item.second);
                    return true;
                }
            }

            for (const auto& item : m_adviceSwatchBounds) {
                if (item.first.contains(mousePos)) {
                    sf::Color picked = item.second.first;
                    size_t palIdx = item.second.second;
                    if (palIdx < m_advicePalettes.size()) {
                        m_advicePalettes[palIdx].useCount++;
                        m_advicePalettes[palIdx].order = s_orderCounter++;
                        saveAdvicePalettes();
                    }
                    updateFromRGB(picked);
                    canvas.setPrimaryColor(picked);
                    return true;
                }
            }

            sf::Color curC = ColorManager::hsvToRgb(currentHue, currentSat, currentVal);
            curC.a = static_cast<sf::Uint8>(currentAlpha * 255.f);

            float sy = std::floor(inputY + 48.f + 18.f);
            float sx = pickerX;
            for (const auto& c : colorManager.getRecentColors()) {
                if (sf::FloatRect(sx, sy, 22.f, 22.f).contains(mousePos)) {
                    updateFromRGB(c);
                    canvas.setPrimaryColor(c);
                    return true;
                }
                sx += 26.f;
                if (sx > pickerX + pickerSize - 22.f) { sx = pickerX; sy += 26.f; }
            }

            sx = pickerX;
            sy += 30.f;
            if (m_addSwatchBtnBounds.contains(mousePos)) {
                colorManager.addCustomSwatch(curC);
                return true;
            }
        }
    }

    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        if (m_isBoxSelectingSwatches) {
            float dist = std::hypot(m_swatchSelectEnd.x - m_swatchSelectStart.x, m_swatchSelectEnd.y - m_swatchSelectStart.y);
            if (dist < 5.f) {
                for (const auto& item : m_customSwatchBounds) {
                    if (item.first.contains(m_swatchSelectStart)) {
                        sf::Color c = colorManager.getCustomSwatches()[item.second];
                        updateFromRGB(c);
                        canvas.setPrimaryColor(c);
                        break;
                    }
                }
                m_selectedSwatchIndices.clear();
            }
            m_isBoxSelectingSwatches = false;
        }

        if (isDraggingSV || isDraggingHue || isDraggingAlpha) {
            sf::Color curC = ColorManager::hsvToRgb(currentHue, currentSat, currentVal);
            curC.a = static_cast<sf::Uint8>(currentAlpha * 255.f);
            canvas.setPrimaryColor(curC);
            colorManager.addRecentColor(curC);
        }
        isDraggingSV = false;
        isDraggingHue = false;
        isDraggingAlpha = false;
        isDraggingWindow = false;
        isResizing = false;
        activeResizeDir = PaletteResizeDir::None;
    }

    if (event.type == sf::Event::MouseMoved) {
        if (m_isBoxSelectingSwatches) {
            m_swatchSelectEnd = mousePos;
            float left = std::min(m_swatchSelectStart.x, m_swatchSelectEnd.x);
            float top = std::min(m_swatchSelectStart.y, m_swatchSelectEnd.y);
            float w = std::abs(m_swatchSelectEnd.x - m_swatchSelectStart.x);
            float h = std::abs(m_swatchSelectEnd.y - m_swatchSelectStart.y);
            sf::FloatRect box(left, top, w, h);

            m_selectedSwatchIndices.clear();
            for (const auto& item : m_customSwatchBounds) {
                if (box.intersects(item.first)) {
                    m_selectedSwatchIndices.push_back(item.second);
                }
            }
            return true;
        }

        if (isResizing && isDetached) {
            float dx = mousePos.x - resizeStartMouse.x;
            float dy = mousePos.y - resizeStartMouse.y;

            const float minW = 270.f;
            const float minH = 460.f;

            float newX = resizeStartBounds.left;
            float newY = resizeStartBounds.top;
            float newW = resizeStartBounds.width;
            float newH = resizeStartBounds.height;

            if (activeResizeDir == PaletteResizeDir::Right || activeResizeDir == PaletteResizeDir::TopRight || activeResizeDir == PaletteResizeDir::BottomRight) {
                newW = std::max(minW, resizeStartBounds.width + dx);
            }
            if (activeResizeDir == PaletteResizeDir::Bottom || activeResizeDir == PaletteResizeDir::BottomLeft || activeResizeDir == PaletteResizeDir::BottomRight) {
                newH = std::max(minH, resizeStartBounds.height + dy);
            }
            if (activeResizeDir == PaletteResizeDir::Left || activeResizeDir == PaletteResizeDir::TopLeft || activeResizeDir == PaletteResizeDir::BottomLeft) {
                float possibleW = resizeStartBounds.width - dx;
                if (possibleW >= minW) {
                    newX = resizeStartBounds.left + dx;
                    newW = possibleW;
                }
            }
            if (activeResizeDir == PaletteResizeDir::Top || activeResizeDir == PaletteResizeDir::TopLeft || activeResizeDir == PaletteResizeDir::TopRight) {
                float possibleH = resizeStartBounds.height - dy;
                if (possibleH >= minH) {
                    newY = resizeStartBounds.top + dy;
                    newH = possibleH;
                }
            }

            detachedPos = sf::Vector2f(newX, newY);
            detachedSize = sf::Vector2f(newW, newH);
            return true;
        }

        if (isDraggingWindow && isDetached) {
            detachedPos = mousePos - windowDragOffset;
            detachedPos.x = std::clamp(detachedPos.x, 0.f, 1920.f - detachedSize.x);
            detachedPos.y = std::clamp(detachedPos.y, 0.f, 1080.f - detachedSize.y);
            return true;
        }

        if (isDraggingSV) {
            currentSat = std::clamp((mousePos.x - pickerX) / pickerSize, 0.f, 1.f);
            currentVal = 1.0f - std::clamp((mousePos.y - pickerY) / pickerSize, 0.f, 1.f);
            updatePickerImages();
            sf::Color curC = ColorManager::hsvToRgb(currentHue, currentSat, currentVal);
            curC.a = static_cast<sf::Uint8>(currentAlpha * 255.f);
            canvas.setPrimaryColor(curC);
            return true;
        }
        else if (isDraggingHue) {
            currentHue = std::clamp((mousePos.x - pickerX) / pickerSize, 0.f, 1.f) * 360.f;
            updatePickerImages();
            sf::Color curC = ColorManager::hsvToRgb(currentHue, currentSat, currentVal);
            curC.a = static_cast<sf::Uint8>(currentAlpha * 255.f);
            canvas.setPrimaryColor(curC);
            return true;
        }
        else if (isDraggingAlpha) {
            currentAlpha = std::clamp((mousePos.x - pickerX) / pickerSize, 0.f, 1.f);
            updatePickerImages();
            sf::Color curC = ColorManager::hsvToRgb(currentHue, currentSat, currentVal);
            curC.a = static_cast<sf::Uint8>(currentAlpha * 255.f);
            canvas.setPrimaryColor(curC);
            return true;
        }
    }

    if (event.type == sf::Event::MouseButtonPressed || event.type == sf::Event::MouseButtonReleased ||
        event.type == sf::Event::MouseMoved || event.type == sf::Event::MouseWheelScrolled) {
        float topBarsH = WisdomUI::Theme::TopBarHeight + WisdomUI::Theme::OptionsBarHeight;
        sf::FloatRect panelBounds(currentX, topBarsH, width, 1080.f - topBarsH - WisdomUI::Theme::StatusBarHeight);
        return panelBounds.contains(mousePos);
    }

    return false;
}

std::string ColorPalettePanel::processClick(sf::Vector2f mousePos, Canvas& canvas) {
    if (closeBtn.getGlobalBounds().contains(mousePos)) {
        forceClose();
        return "color_close";
    }

    if (detachBtn.getGlobalBounds().contains(mousePos)) {
        isDetached = !isDetached;
        if (isDetached) state = PalettePanelState::Visible;
        return "color_detach";
    }

    if (!isDetached && pinBtn.getGlobalBounds().contains(mousePos)) {
        state = (state == PalettePanelState::Pinned) ? PalettePanelState::Visible : PalettePanelState::Pinned;
        return "color_pin";
    }
    return "";
}

bool ColorPalettePanel::handleClick(sf::Vector2f mousePos, Canvas& canvas) {
    return !processClick(mousePos, canvas).empty();
}

bool ColorPalettePanel::handlePaletteClick(sf::Vector2f mousePos, sf::Color& outPrimary, sf::Color& outSecondary) {
    return false;
}

void ColorPalettePanel::setColors(sf::Color primary, sf::Color secondary) {
    updateFromRGB(primary);
}

float ColorPalettePanel::getCurrentX() const { return currentX; }
void ColorPalettePanel::forceClose() { targetX = 1920.f; }
bool ColorPalettePanel::isHovered() const { return state == PalettePanelState::Visible; }
bool ColorPalettePanel::isPanelPinned() const { return state == PalettePanelState::Pinned; }
sf::FloatRect ColorPalettePanel::getHandleBounds() const { return sf::FloatRect(0, 0, 0, 0); }
ColorManager& ColorPalettePanel::getColorManager() { return colorManager; }

bool ColorPalettePanel::getIsEyedropperActive() const { return isEyedropperActive; }
void ColorPalettePanel::setEyedropperActive(bool active) { isEyedropperActive = active; }

bool ColorPalettePanel::getIsDetached() const { return isDetached; }
void ColorPalettePanel::setIsDetached(bool detached) { isDetached = detached; }