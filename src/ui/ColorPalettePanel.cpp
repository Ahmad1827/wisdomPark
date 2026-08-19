#include "ColorPalettePanel.h"
#include "../UI/UITheme.h"
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <iostream>

ColorPalettePanel::ColorPalettePanel() : width(280.f), currentX(1920.f), targetX(1920.f), state(PalettePanelState::Hidden), hovered(false), currentHue(0.f), currentSat(1.f), currentVal(1.f), currentAlpha(1.f), isDraggingSV(false), isDraggingHue(false), isDraggingAlpha(false), activeInputIndex(-1), isEyedropperActive(false) {}

void ColorPalettePanel::init() {
    colorManager.init();
    font.loadFromFile("assets/font.otf");

    background.setFillColor(WisdomUI::Theme::Panel);
    background.setOutlineThickness(1.f);
    background.setOutlineColor(WisdomUI::Theme::Border);

    headerBg.setFillColor(WisdomUI::Theme::PanelInset);
    headerText.setFont(font);
    headerText.setString("COLORS");
    headerText.setCharacterSize(13);
    headerText.setFillColor(WisdomUI::Theme::Gold);

    closeBtn.setSize(sf::Vector2f(22.f, 22.f));
    closeBtn.setFillColor(WisdomUI::Theme::PanelInset);
    closeBtn.setOutlineThickness(1.f);
    closeBtn.setOutlineColor(WisdomUI::Theme::Border);
    closeText.setFont(font);
    closeText.setString("X");
    closeText.setCharacterSize(11);
    closeText.setFillColor(WisdomUI::Theme::TextSecondary);

    pinBtn.setSize(sf::Vector2f(width - 24.f, 22.f));
    pinBtn.setFillColor(WisdomUI::Theme::PanelInset);
    pinBtn.setOutlineThickness(1.f);
    pinBtn.setOutlineColor(WisdomUI::Theme::Border);

    pinLabel.setFont(font);
    pinLabel.setString("Pin Palette");
    pinLabel.setCharacterSize(11);
    pinLabel.setFillColor(WisdomUI::Theme::TextSecondary);

    primaryBox.setSize(sf::Vector2f(36.f, 36.f));
    primaryBox.setOutlineThickness(1.f);
    primaryBox.setOutlineColor(WisdomUI::Theme::BorderHighlight);

    secondaryBox.setSize(sf::Vector2f(36.f, 36.f));
    secondaryBox.setOutlineThickness(1.f);
    secondaryBox.setOutlineColor(WisdomUI::Theme::Border);

    eyedropperBtn.setSize(sf::Vector2f(90.f, 24.f));
    eyedropperBtn.setFillColor(WisdomUI::Theme::PanelInset);
    eyedropperBtn.setOutlineThickness(1.f);
    eyedropperBtn.setOutlineColor(WisdomUI::Theme::Border);

    eyedropperLabel.setFont(font);
    eyedropperLabel.setString("Eyedropper");
    eyedropperLabel.setCharacterSize(11);
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

    svSelector.setSize(sf::Vector2f(6.f, 6.f));
    svSelector.setOrigin(3.f, 3.f);
    svSelector.setOutlineThickness(1.f);
    svSelector.setOutlineColor(WisdomUI::Theme::Gold);
    svSelector.setFillColor(sf::Color::Transparent);

    hueSelector.setSize(sf::Vector2f(4.f, 15.f));
    hueSelector.setOrigin(2.f, 0.f);
    hueSelector.setOutlineThickness(1.f);
    hueSelector.setOutlineColor(sf::Color::Black);
    hueSelector.setFillColor(WisdomUI::Theme::Gold);

    alphaSelector.setSize(sf::Vector2f(4.f, 15.f));
    alphaSelector.setOrigin(2.f, 0.f);
    alphaSelector.setOutlineThickness(1.f);
    alphaSelector.setOutlineColor(sf::Color::Black);
    alphaSelector.setFillColor(WisdomUI::Theme::Gold);

    updatePickerImages();
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

void ColorPalettePanel::update(float dt, bool focusMode, Canvas& canvas, bool isOpen) {
    if (focusMode || !isOpen) targetX = 1920.f;
    else targetX = 1920.f - 44.f - width;

    currentX += (targetX - currentX) * 16.0f * dt;
    background.setPosition(currentX, 36.f + 32.f);
    background.setSize(sf::Vector2f(width, 1080.f - (36.f + 32.f + 24.f)));

    headerBg.setPosition(currentX, 36.f + 32.f);
    headerBg.setSize(sf::Vector2f(width, 32.f));
    headerText.setPosition(currentX + 12.f, 36.f + 32.f + 7.f);

    closeBtn.setPosition(currentX + width - 30.f, 36.f + 32.f + 5.f);
    closeText.setPosition(currentX + width - 23.f, 36.f + 32.f + 7.f);

    pinBtn.setPosition(currentX + 12.f, 36.f + 32.f + 38.f);
    pinLabel.setPosition(currentX + 22.f, 36.f + 32.f + 41.f);

    primaryBox.setPosition(currentX + 20.f, 36.f + 32.f + 70.f);
    secondaryBox.setPosition(currentX + 40.f, 36.f + 32.f + 90.f);

    primaryBox.setFillColor(canvas.getPrimaryColor());
    secondaryBox.setFillColor(canvas.getSecondaryColor());

    eyedropperBtn.setPosition(currentX + 100.f, 36.f + 32.f + 80.f);
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
    eyedropperLabel.setPosition(currentX + 112.f, 36.f + 32.f + 84.f);

    float pickerY = 36.f + 32.f + 140.f;
    svSprite.setPosition(currentX + 20.f, pickerY);
    svSelector.setPosition(currentX + 20.f + currentSat * 200.f, pickerY + (1.0f - currentVal) * 200.f);

    hueSprite.setPosition(currentX + 20.f, pickerY + 210.f);
    hueSelector.setPosition(currentX + 20.f + (currentHue / 360.f) * 200.f, pickerY + 210.f);

    alphaSprite.setPosition(currentX + 20.f, pickerY + 235.f);
    alphaSelector.setPosition(currentX + 20.f + currentAlpha * 200.f, pickerY + 235.f);
}

void ColorPalettePanel::updateHover(sf::Vector2f mousePos, bool canOpen) {
    bool inPanel = background.getGlobalBounds().contains(mousePos);
    if (state == PalettePanelState::Hidden) {
        if (canOpen && inPanel) state = PalettePanelState::Visible;
    }
    else if (state == PalettePanelState::Visible) {
        if (!inPanel) state = PalettePanelState::Hidden;
    }
}

void ColorPalettePanel::draw(sf::RenderWindow& window) {
    if (currentX >= 1918.f) return;

    WisdomUI::Theme::DrawFiligreePanel(window, background.getGlobalBounds(), 1.0f);

    window.draw(headerBg);
    window.draw(headerText);
    window.draw(closeBtn);
    window.draw(closeText);

    auto styleBtn = [&](sf::RectangleShape& r, sf::Text& t) {
        bool hov = r.getGlobalBounds().contains(window.mapPixelToCoords(sf::Mouse::getPosition(window)));
        r.setFillColor(hov ? WisdomUI::Theme::PanelHover : WisdomUI::Theme::PanelInset);
        r.setOutlineColor(hov ? WisdomUI::Theme::BorderHighlight : WisdomUI::Theme::Border);
        window.draw(r);
        window.draw(t);
        };

    styleBtn(pinBtn, pinLabel);

    window.draw(secondaryBox);
    window.draw(primaryBox);

    window.draw(eyedropperBtn);
    window.draw(eyedropperLabel);

    sf::RectangleShape svOutline(sf::Vector2f(202.f, 202.f));
    svOutline.setPosition(svSprite.getPosition().x - 1.f, svSprite.getPosition().y - 1.f);
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

    float inputY = alphaSprite.getPosition().y + 26.f;
    auto drawInput = [&](int index, std::string label, std::string val, float x, float w) {
        sf::Text t(label, font, 11);
        t.setPosition(x, inputY);
        t.setFillColor(WisdomUI::Theme::TextSecondary);
        window.draw(t);

        sf::RectangleShape box(sf::Vector2f(w, 20.f));
        box.setPosition(x, inputY + 14.f);
        box.setFillColor(WisdomUI::Theme::PanelInset);
        box.setOutlineThickness(1.f);
        box.setOutlineColor(activeInputIndex == index ? WisdomUI::Theme::BorderHighlight : WisdomUI::Theme::Border);
        window.draw(box);

        sf::Text v(activeInputIndex == index ? inputBuffer + "_" : val, font, 11);
        v.setPosition(x + 4.f, inputY + 16.f);
        v.setFillColor(WisdomUI::Theme::Gold);
        window.draw(v);
        };

    sf::Color curC = ColorManager::hsvToRgb(currentHue, currentSat, currentVal);
    curC.a = static_cast<sf::Uint8>(currentAlpha * 255.f);

    drawInput(0, "R", std::to_string(curC.r), currentX + 20.f, 32.f);
    drawInput(1, "G", std::to_string(curC.g), currentX + 58.f, 32.f);
    drawInput(2, "B", std::to_string(curC.b), currentX + 96.f, 32.f);
    drawInput(3, "A", std::to_string(curC.a), currentX + 134.f, 32.f);
    drawInput(4, "Hex", colorToHex(curC), currentX + 175.f, 65.f);

    float sx = currentX + 20.f;
    float sy = inputY + 45.f;
    sf::Text rt("Recent", font, 11);
    rt.setPosition(sx, sy);
    rt.setFillColor(WisdomUI::Theme::Gold);
    window.draw(rt);
    sy += 18.f;

    for (const auto& c : colorManager.getRecentColors()) {
        sf::RectangleShape s(sf::Vector2f(16.f, 16.f));
        s.setPosition(sx, sy);
        s.setFillColor(c);
        s.setOutlineThickness(1.f);
        s.setOutlineColor(WisdomUI::Theme::Border);
        window.draw(s);
        sx += 20.f;
        if (sx > currentX + width - 30.f) { sx = currentX + 20.f; sy += 20.f; }
    }

    sx = currentX + 20.f;
    sy += 25.f;
    sf::Text ct("Swatches", font, 11);
    ct.setPosition(sx, sy);
    ct.setFillColor(WisdomUI::Theme::Gold);
    window.draw(ct);

    sf::RectangleShape addBtn(sf::Vector2f(18.f, 18.f));
    addBtn.setPosition(currentX + width - 35.f, sy - 2.f);
    addBtn.setFillColor(WisdomUI::Theme::PanelInset);
    addBtn.setOutlineThickness(1.f);
    addBtn.setOutlineColor(WisdomUI::Theme::Border);
    window.draw(addBtn);
    sf::Text plus("+", font, 14);
    plus.setPosition(addBtn.getPosition().x + 4.f, addBtn.getPosition().y - 3.f);
    plus.setFillColor(WisdomUI::Theme::Gold);
    window.draw(plus);

    sy += 18.f;
    for (const auto& c : colorManager.getCustomSwatches()) {
        sf::RectangleShape s(sf::Vector2f(18.f, 18.f));
        s.setPosition(sx, sy);
        s.setFillColor(c);
        s.setOutlineThickness(1.f);
        s.setOutlineColor(WisdomUI::Theme::Border);
        window.draw(s);
        sx += 23.f;
        if (sx > currentX + width - 30.f) { sx = currentX + 20.f; sy += 23.f; }
    }
}

std::string ColorPalettePanel::processClick(sf::Vector2f mousePos, Canvas& canvas) {
    if (closeBtn.getGlobalBounds().contains(mousePos)) {
        forceClose();
        return "color_close";
    }

    if (pinBtn.getGlobalBounds().contains(mousePos)) {
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

bool ColorPalettePanel::handleEvent(const sf::Event& event, sf::Vector2f mousePos, Canvas& canvas) {
    if (activeInputIndex != -1) {
        if (event.type == sf::Event::TextEntered) {
            if (event.text.unicode == '\b' && !inputBuffer.empty()) inputBuffer.pop_back();
            else if (event.text.unicode < 128 && event.text.unicode != '\r' && event.text.unicode != '\n' && event.text.unicode != '\b') inputBuffer += static_cast<char>(event.text.unicode);
            return true;
        }
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter) {
            sf::Color curC = ColorManager::hsvToRgb(currentHue, currentSat, currentVal);
            curC.a = static_cast<sf::Uint8>(currentAlpha * 255.f);

            try {
                if (activeInputIndex == 0) curC.r = std::clamp(std::stoi(inputBuffer), 0, 255);
                else if (activeInputIndex == 1) curC.g = std::clamp(std::stoi(inputBuffer), 0, 255);
                else if (activeInputIndex == 2) curC.b = std::clamp(std::stoi(inputBuffer), 0, 255);
                else if (activeInputIndex == 3) curC.a = std::clamp(std::stoi(inputBuffer), 0, 255);
                else if (activeInputIndex == 4) {
                    std::string hex = inputBuffer;
                    if (hex[0] == '#') hex = hex.substr(1);
                    if (hex.length() >= 6) {
                        curC.r = std::stoi(hex.substr(0, 2), nullptr, 16);
                        curC.g = std::stoi(hex.substr(2, 2), nullptr, 16);
                        curC.b = std::stoi(hex.substr(4, 2), nullptr, 16);
                    }
                }
            }
            catch (...) {}

            updateFromRGB(curC);
            canvas.setPrimaryColor(curC);
            activeInputIndex = -1;
            return true;
        }
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (closeBtn.getGlobalBounds().contains(mousePos)) {
            forceClose();
            return true;
        }

        if (pinBtn.getGlobalBounds().contains(mousePos)) {
            state = (state == PalettePanelState::Pinned) ? PalettePanelState::Visible : PalettePanelState::Pinned;
            return true;
        }

        if (eyedropperBtn.getGlobalBounds().contains(mousePos)) {
            isEyedropperActive = !isEyedropperActive;
            return true;
        }

        if (svSprite.getGlobalBounds().contains(mousePos)) isDraggingSV = true;
        else if (hueSprite.getGlobalBounds().contains(mousePos)) isDraggingHue = true;
        else if (alphaSprite.getGlobalBounds().contains(mousePos)) isDraggingAlpha = true;

        float inputY = alphaSprite.getPosition().y + 26.f + 14.f;
        if (sf::FloatRect(currentX + 20.f, inputY, 32.f, 20.f).contains(mousePos)) { activeInputIndex = 0; inputBuffer = ""; return true; }
        if (sf::FloatRect(currentX + 58.f, inputY, 32.f, 20.f).contains(mousePos)) { activeInputIndex = 1; inputBuffer = ""; return true; }
        if (sf::FloatRect(currentX + 96.f, inputY, 32.f, 20.f).contains(mousePos)) { activeInputIndex = 2; inputBuffer = ""; return true; }
        if (sf::FloatRect(currentX + 134.f, inputY, 32.f, 20.f).contains(mousePos)) { activeInputIndex = 3; inputBuffer = ""; return true; }
        if (sf::FloatRect(currentX + 175.f, inputY, 65.f, 20.f).contains(mousePos)) { activeInputIndex = 4; inputBuffer = ""; return true; }

        activeInputIndex = -1;

        sf::Color curC = ColorManager::hsvToRgb(currentHue, currentSat, currentVal);
        curC.a = static_cast<sf::Uint8>(currentAlpha * 255.f);

        float sy = inputY + 25.f + 18.f;
        float sx = currentX + 20.f;
        for (const auto& c : colorManager.getRecentColors()) {
            if (sf::FloatRect(sx, sy, 16.f, 16.f).contains(mousePos)) {
                updateFromRGB(c);
                canvas.setPrimaryColor(c);
                return true;
            }
            sx += 20.f;
            if (sx > currentX + width - 30.f) { sx = currentX + 20.f; sy += 20.f; }
        }

        sx = currentX + 20.f;
        sy += 25.f;
        if (sf::FloatRect(currentX + width - 35.f, sy - 2.f, 18.f, 18.f).contains(mousePos)) {
            colorManager.addCustomSwatch(curC);
            return true;
        }

        sy += 18.f;
        int removeIdx = -1;
        for (size_t i = 0; i < colorManager.getCustomSwatches().size(); ++i) {
            if (sf::FloatRect(sx, sy, 18.f, 18.f).contains(mousePos)) {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt)) removeIdx = static_cast<int>(i);
                else {
                    updateFromRGB(colorManager.getCustomSwatches()[i]);
                    canvas.setPrimaryColor(colorManager.getCustomSwatches()[i]);
                }
                return true;
            }
            sx += 23.f;
            if (sx > currentX + width - 30.f) { sx = currentX + 20.f; sy += 23.f; }
        }
        if (removeIdx != -1) colorManager.removeCustomSwatch(removeIdx);
    }

    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        if (isDraggingSV || isDraggingHue || isDraggingAlpha) {
            sf::Color curC = ColorManager::hsvToRgb(currentHue, currentSat, currentVal);
            curC.a = static_cast<sf::Uint8>(currentAlpha * 255.f);
            canvas.setPrimaryColor(curC);
            colorManager.addRecentColor(curC);
        }
        isDraggingSV = false;
        isDraggingHue = false;
        isDraggingAlpha = false;
    }

    if (event.type == sf::Event::MouseMoved) {
        if (isDraggingSV) {
            currentSat = std::clamp((mousePos.x - svSprite.getPosition().x) / 200.f, 0.f, 1.f);
            currentVal = 1.0f - std::clamp((mousePos.y - svSprite.getPosition().y) / 200.f, 0.f, 1.f);
            updatePickerImages();
            sf::Color curC = ColorManager::hsvToRgb(currentHue, currentSat, currentVal);
            curC.a = static_cast<sf::Uint8>(currentAlpha * 255.f);
            canvas.setPrimaryColor(curC);
            return true;
        }
        else if (isDraggingHue) {
            currentHue = std::clamp((mousePos.x - hueSprite.getPosition().x) / 200.f, 0.f, 1.f) * 360.f;
            updatePickerImages();
            sf::Color curC = ColorManager::hsvToRgb(currentHue, currentSat, currentVal);
            curC.a = static_cast<sf::Uint8>(currentAlpha * 255.f);
            canvas.setPrimaryColor(curC);
            return true;
        }
        else if (isDraggingAlpha) {
            currentAlpha = std::clamp((mousePos.x - alphaSprite.getPosition().x) / 200.f, 0.f, 1.f);
            sf::Color curC = ColorManager::hsvToRgb(currentHue, currentSat, currentVal);
            curC.a = static_cast<sf::Uint8>(currentAlpha * 255.f);
            canvas.setPrimaryColor(curC);
            return true;
        }
    }

    return background.getGlobalBounds().contains(mousePos);
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