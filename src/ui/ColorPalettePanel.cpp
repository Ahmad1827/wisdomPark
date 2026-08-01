#include "ColorPalettePanel.h"
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <iostream>

ColorPalettePanel::ColorPalettePanel() : width(280.f), currentX(1920.f), targetX(1920.f), state(PalettePanelState::Hidden), hovered(false), currentHue(0.f), currentSat(1.f), currentVal(1.f), currentAlpha(1.f), isDraggingSV(false), isDraggingHue(false), isDraggingAlpha(false), activeInputIndex(-1), isEyedropperActive(false) {}

void ColorPalettePanel::init() {
    colorManager.init();
    font.loadFromFile("assets/font.otf");

    background.setSize(sf::Vector2f(width, 1080.f));
    background.setFillColor(sf::Color(15, 15, 18, 240));
    background.setOutlineThickness(1.f);
    background.setOutlineColor(sf::Color(255, 255, 255, 30));

    handleBg.setSize(sf::Vector2f(24.f, 80.f));
    handleBg.setFillColor(sf::Color(30, 30, 35, 200));
    handleBg.setOutlineThickness(1.f);
    handleBg.setOutlineColor(sf::Color(255, 255, 255, 30));

    handleLabel.setFont(font);
    handleLabel.setString("< C");
    handleLabel.setCharacterSize(14);
    handleLabel.setFillColor(sf::Color(200, 200, 200));

    headerBg.setFillColor(sf::Color(40, 40, 45, 255));
    headerBg.setSize(sf::Vector2f(width, 30.f));

    headerText.setFont(font);
    headerText.setString("COLORS");
    headerText.setCharacterSize(14);
    headerText.setFillColor(sf::Color(200, 200, 200));

    pinBtn.setSize(sf::Vector2f(width - 20.f, 24.f));
    pinBtn.setFillColor(sf::Color(50, 50, 60, 255));

    pinLabel.setFont(font);
    pinLabel.setString("Pin Palette");
    pinLabel.setCharacterSize(12);
    pinLabel.setFillColor(sf::Color::White);

    primaryBox.setSize(sf::Vector2f(40.f, 40.f));
    primaryBox.setOutlineThickness(2.f);
    primaryBox.setOutlineColor(sf::Color::White);

    secondaryBox.setSize(sf::Vector2f(40.f, 40.f));
    secondaryBox.setOutlineThickness(1.f);
    secondaryBox.setOutlineColor(sf::Color(100, 100, 100));

    // Initialize Eyedropper Button
    eyedropperBtn.setSize(sf::Vector2f(90.f, 25.f));
    eyedropperBtn.setOutlineThickness(1.f);

    eyedropperLabel.setFont(font);
    eyedropperLabel.setString("Eyedropper");
    eyedropperLabel.setCharacterSize(11);
    eyedropperLabel.setFillColor(sf::Color::White);

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
    svSelector.setOutlineColor(sf::Color::White);
    svSelector.setFillColor(sf::Color::Transparent);

    hueSelector.setSize(sf::Vector2f(4.f, 15.f));
    hueSelector.setOrigin(2.f, 0.f);
    hueSelector.setOutlineThickness(1.f);
    hueSelector.setOutlineColor(sf::Color::Black);
    hueSelector.setFillColor(sf::Color::White);

    alphaSelector.setSize(sf::Vector2f(4.f, 15.f));
    alphaSelector.setOrigin(2.f, 0.f);
    alphaSelector.setOutlineThickness(1.f);
    alphaSelector.setOutlineColor(sf::Color::Black);
    alphaSelector.setFillColor(sf::Color::White);

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

void ColorPalettePanel::update(float dt, bool focusMode, Canvas& canvas) {
    if (focusMode) targetX = 1920.f;
    else targetX = (state == PalettePanelState::Pinned || state == PalettePanelState::Visible) ? 1920.f - width : 1920.f;

    currentX += (targetX - currentX) * 15.0f * dt;
    background.setPosition(currentX, 0.f);

    handleBg.setPosition(currentX - 24.f, 800.f);
    handleLabel.setPosition(currentX - 20.f, 830.f);

    headerBg.setPosition(currentX, 0.f);
    headerText.setPosition(currentX + 10.f, 5.f);

    if (state == PalettePanelState::Pinned) {
        handleLabel.setString("x");
        pinLabel.setFillColor(sf::Color(0, 191, 255));
        pinBtn.setOutlineThickness(1.f);
        pinBtn.setOutlineColor(sf::Color(0, 191, 255));
    }
    else {
        handleLabel.setString("< C");
        pinLabel.setFillColor(sf::Color(180, 180, 180));
        pinBtn.setOutlineThickness(0.f);
    }

    pinBtn.setPosition(currentX + 10.f, 40.f);
    pinLabel.setPosition(currentX + 20.f, 43.f);

    primaryBox.setPosition(currentX + 20.f, 80.f);
    secondaryBox.setPosition(currentX + 40.f, 100.f);

    primaryBox.setFillColor(canvas.getPrimaryColor());
    secondaryBox.setFillColor(canvas.getSecondaryColor());

    // Update Eyedropper Position and Color
    eyedropperBtn.setPosition(currentX + 100.f, 95.f);
    if (isEyedropperActive) {
        eyedropperBtn.setFillColor(sf::Color(0, 191, 255, 100));
        eyedropperBtn.setOutlineColor(sf::Color(0, 191, 255));
    }
    else {
        eyedropperBtn.setFillColor(sf::Color(30, 30, 35));
        eyedropperBtn.setOutlineColor(sf::Color(100, 100, 110));
    }
    eyedropperLabel.setPosition(currentX + 112.f, 99.f);

    float pickerY = 150.f;
    svSprite.setPosition(currentX + 20.f, pickerY);
    svSelector.setPosition(currentX + 20.f + currentSat * 200.f, pickerY + (1.0f - currentVal) * 200.f);

    hueSprite.setPosition(currentX + 20.f, pickerY + 210.f);
    hueSelector.setPosition(currentX + 20.f + (currentHue / 360.f) * 200.f, pickerY + 210.f);

    alphaSprite.setPosition(currentX + 20.f, pickerY + 235.f);
    alphaSelector.setPosition(currentX + 20.f + currentAlpha * 200.f, pickerY + 235.f);
}

void ColorPalettePanel::updateHover(sf::Vector2f mousePos, bool canOpen) {
    bool inPanel = background.getGlobalBounds().contains(mousePos);
    bool inHandle = handleBg.getGlobalBounds().contains(mousePos);

    if (state == PalettePanelState::Hidden) {
        if (canOpen && inHandle) state = PalettePanelState::Visible;
        hovered = inHandle;
    }
    else if (state == PalettePanelState::Visible) {
        if (!inPanel && !inHandle) state = PalettePanelState::Hidden;
        hovered = inPanel || inHandle;
    }
    else {
        hovered = inPanel || inHandle;
    }
}

void ColorPalettePanel::draw(sf::RenderWindow& window) {
    window.draw(background);
    if (state != PalettePanelState::Pinned) {
        window.draw(handleBg);
        window.draw(handleLabel);
    }
    window.draw(headerBg);
    window.draw(headerText);
    window.draw(pinBtn);
    window.draw(pinLabel);

    window.draw(secondaryBox);
    window.draw(primaryBox);

    // Draw eyedropper
    window.draw(eyedropperBtn);
    window.draw(eyedropperLabel);

    window.draw(svSprite);
    window.draw(svSelector);
    window.draw(hueSprite);
    window.draw(hueSelector);

    sf::RectangleShape check1(sf::Vector2f(5.f, 5.f)); check1.setFillColor(sf::Color(100, 100, 100));
    sf::RectangleShape check2(sf::Vector2f(5.f, 5.f)); check2.setFillColor(sf::Color(150, 150, 150));
    for (float x = 0; x < 200.f; x += 5.f) {
        for (float y = 0; y < 15.f; y += 5.f) {
            bool alt = (static_cast<int>(x / 5.f) + static_cast<int>(y / 5.f)) % 2 == 0;
            sf::RectangleShape& r = alt ? check1 : check2;
            r.setPosition(alphaSprite.getPosition().x + x, alphaSprite.getPosition().y + y);
            window.draw(r);
        }
    }

    window.draw(alphaSprite);
    window.draw(alphaSelector);

    float inputY = alphaSprite.getPosition().y + 30.f;
    auto drawInput = [&](int index, std::string label, std::string val, float x, float w) {
        sf::Text t(label, font, 12);
        t.setPosition(x, inputY);
        t.setFillColor(sf::Color(150, 150, 150));
        window.draw(t);

        sf::RectangleShape box(sf::Vector2f(w, 20.f));
        box.setPosition(x, inputY + 15.f);
        box.setFillColor(activeInputIndex == index ? sf::Color(30, 30, 40) : sf::Color(15, 15, 20));
        box.setOutlineThickness(1.f);
        box.setOutlineColor(activeInputIndex == index ? sf::Color(0, 122, 204) : sf::Color(100, 100, 110));
        window.draw(box);

        sf::Text v(activeInputIndex == index ? inputBuffer + "_" : val, font, 12);
        v.setPosition(x + 5.f, inputY + 17.f);
        v.setFillColor(sf::Color::White);
        window.draw(v);
        };

    sf::Color curC = ColorManager::hsvToRgb(currentHue, currentSat, currentVal);
    curC.a = static_cast<sf::Uint8>(currentAlpha * 255.f);

    drawInput(0, "R", std::to_string(curC.r), currentX + 20.f, 35.f);
    drawInput(1, "G", std::to_string(curC.g), currentX + 60.f, 35.f);
    drawInput(2, "B", std::to_string(curC.b), currentX + 100.f, 35.f);
    drawInput(3, "A", std::to_string(curC.a), currentX + 140.f, 35.f);
    drawInput(4, "Hex", colorToHex(curC), currentX + 185.f, 65.f);

    float sx = currentX + 20.f;
    float sy = inputY + 50.f;
    sf::Text rt("Recent", font, 12); rt.setPosition(sx, sy); rt.setFillColor(sf::Color(150, 150, 150)); window.draw(rt);
    sy += 20.f;

    for (const auto& c : colorManager.getRecentColors()) {
        sf::RectangleShape s(sf::Vector2f(15.f, 15.f));
        s.setPosition(sx, sy);
        s.setFillColor(c);
        window.draw(s);
        sx += 20.f;
        if (sx > currentX + width - 30.f) { sx = currentX + 20.f; sy += 20.f; }
    }

    sx = currentX + 20.f;
    sy += 30.f;
    sf::Text ct("Swatches", font, 12); ct.setPosition(sx, sy); ct.setFillColor(sf::Color(150, 150, 150)); window.draw(ct);

    sf::RectangleShape addBtn(sf::Vector2f(20.f, 20.f));
    addBtn.setPosition(currentX + width - 40.f, sy - 5.f);
    addBtn.setFillColor(sf::Color(50, 50, 60));
    window.draw(addBtn);
    sf::Text plus("+", font, 16); plus.setPosition(addBtn.getPosition().x + 5.f, addBtn.getPosition().y - 2.f); window.draw(plus);

    sy += 20.f;
    for (const auto& c : colorManager.getCustomSwatches()) {
        sf::RectangleShape s(sf::Vector2f(20.f, 20.f));
        s.setPosition(sx, sy);
        s.setFillColor(c);
        window.draw(s);
        sx += 25.f;
        if (sx > currentX + width - 30.f) { sx = currentX + 20.f; sy += 25.f; }
    }

    sy += 40.f;
    sf::Text ht("Harmony", font, 12); ht.setPosition(currentX + 20.f, sy); ht.setFillColor(sf::Color(150, 150, 150)); window.draw(ht);
    sy += 20.f;

    auto drawHarmonyRow = [&](std::string lbl, std::vector<sf::Color> cols, float y) {
        sf::Text t(lbl, font, 10); t.setPosition(currentX + 20.f, y + 2.f); t.setFillColor(sf::Color(200, 200, 200)); window.draw(t);
        float hx = currentX + 100.f;
        for (auto c : cols) {
            sf::RectangleShape s(sf::Vector2f(15.f, 15.f)); s.setPosition(hx, y); s.setFillColor(c); window.draw(s);
            hx += 20.f;
        }
        };

    auto comp = colorManager.getComplementary(curC); drawHarmonyRow("Comp", { comp[0] }, sy);
    auto anal = colorManager.getAnalogous(curC); drawHarmonyRow("Analog", { anal[0], anal[1] }, sy += 20.f);
    auto tri = colorManager.getTriadic(curC); drawHarmonyRow("Triad", { tri[0], tri[1] }, sy += 20.f);
}

std::string ColorPalettePanel::processClick(sf::Vector2f mousePos, Canvas& canvas) {
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

        float inputY = alphaSprite.getPosition().y + 45.f;
        if (sf::FloatRect(currentX + 20.f, inputY, 35.f, 20.f).contains(mousePos)) { activeInputIndex = 0; inputBuffer = ""; return true; }
        if (sf::FloatRect(currentX + 60.f, inputY, 35.f, 20.f).contains(mousePos)) { activeInputIndex = 1; inputBuffer = ""; return true; }
        if (sf::FloatRect(currentX + 100.f, inputY, 35.f, 20.f).contains(mousePos)) { activeInputIndex = 2; inputBuffer = ""; return true; }
        if (sf::FloatRect(currentX + 140.f, inputY, 35.f, 20.f).contains(mousePos)) { activeInputIndex = 3; inputBuffer = ""; return true; }
        if (sf::FloatRect(currentX + 185.f, inputY, 65.f, 20.f).contains(mousePos)) { activeInputIndex = 4; inputBuffer = ""; return true; }

        activeInputIndex = -1;

        sf::Color curC = ColorManager::hsvToRgb(currentHue, currentSat, currentVal);
        curC.a = static_cast<sf::Uint8>(currentAlpha * 255.f);

        float sy = inputY + 35.f + 20.f;
        float sx = currentX + 20.f;
        for (const auto& c : colorManager.getRecentColors()) {
            if (sf::FloatRect(sx, sy, 15.f, 15.f).contains(mousePos)) {
                updateFromRGB(c);
                canvas.setPrimaryColor(c);
                return true;
            }
            sx += 20.f;
            if (sx > currentX + width - 30.f) { sx = currentX + 20.f; sy += 20.f; }
        }

        sx = currentX + 20.f;
        sy += 30.f;
        if (sf::FloatRect(currentX + width - 40.f, sy - 5.f, 20.f, 20.f).contains(mousePos)) {
            colorManager.addCustomSwatch(curC);
            return true;
        }

        sy += 20.f;
        int removeIdx = -1;
        for (size_t i = 0; i < colorManager.getCustomSwatches().size(); ++i) {
            if (sf::FloatRect(sx, sy, 20.f, 20.f).contains(mousePos)) {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt)) removeIdx = static_cast<int>(i);
                else {
                    updateFromRGB(colorManager.getCustomSwatches()[i]);
                    canvas.setPrimaryColor(colorManager.getCustomSwatches()[i]);
                }
                return true;
            }
            sx += 25.f;
            if (sx > currentX + width - 30.f) { sx = currentX + 20.f; sy += 25.f; }
        }
        if (removeIdx != -1) colorManager.removeCustomSwatch(removeIdx);

        sy += 40.f + 20.f;
        auto checkHarmony = [&](std::vector<sf::Color> cols, float y) {
            float hx = currentX + 100.f;
            for (auto c : cols) {
                if (sf::FloatRect(hx, y, 15.f, 15.f).contains(mousePos)) {
                    updateFromRGB(c); canvas.setPrimaryColor(c); return true;
                }
                hx += 20.f;
            }
            return false;
            };

        auto comp = colorManager.getComplementary(curC); if (checkHarmony({ comp[0] }, sy)) return true;
        auto anal = colorManager.getAnalogous(curC); if (checkHarmony({ anal[0], anal[1] }, sy += 20.f)) return true;
        auto tri = colorManager.getTriadic(curC); if (checkHarmony({ tri[0], tri[1] }, sy += 20.f)) return true;
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
void ColorPalettePanel::forceClose() { if (state != PalettePanelState::Pinned) state = PalettePanelState::Hidden; hovered = false; }
bool ColorPalettePanel::isHovered() const { return hovered; }
bool ColorPalettePanel::isPanelPinned() const { return state == PalettePanelState::Pinned; }
sf::FloatRect ColorPalettePanel::getHandleBounds() const { return handleBg.getGlobalBounds(); }
ColorManager& ColorPalettePanel::getColorManager() { return colorManager; }

bool ColorPalettePanel::getIsEyedropperActive() const { return isEyedropperActive; }
void ColorPalettePanel::setEyedropperActive(bool active) { isEyedropperActive = active; }