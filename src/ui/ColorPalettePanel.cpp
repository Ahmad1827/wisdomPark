#include "ColorPalettePanel.h"
#include "../UI/UITheme.h"
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <iostream>

ColorPalettePanel::ColorPalettePanel()
    : width(280.f), currentX(1920.f), targetX(1920.f), state(PalettePanelState::Hidden),
    hovered(false), isDetached(false), detachedPos(1500.f, 80.f), detachedSize(280.f, 620.f),
    isDraggingWindow(false), windowDragOffset(0.f, 0.f), isResizing(false),
    activeResizeDir(PaletteResizeDir::None), currentHue(0.f), currentSat(1.f), currentVal(1.f),
    currentAlpha(1.f), isDraggingSV(false), isDraggingHue(false), isDraggingAlpha(false),
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
    headerText.setCharacterSize(12);
    headerText.setFillColor(WisdomUI::Theme::Gold);

    closeBtn.setSize(sf::Vector2f(20.f, 20.f));
    closeBtn.setFillColor(WisdomUI::Theme::PanelInset);
    closeBtn.setOutlineThickness(1.f);
    closeBtn.setOutlineColor(WisdomUI::Theme::Border);
    closeText.setFont(font);
    closeText.setString("X");
    closeText.setCharacterSize(10);
    closeText.setFillColor(WisdomUI::Theme::TextSecondary);

    pinBtn.setSize(sf::Vector2f(38.f, 20.f));
    pinBtn.setFillColor(WisdomUI::Theme::PanelInset);
    pinBtn.setOutlineThickness(1.f);
    pinBtn.setOutlineColor(WisdomUI::Theme::Border);
    pinLabel.setFont(font);
    pinLabel.setString("Pin");
    pinLabel.setCharacterSize(10);
    pinLabel.setFillColor(WisdomUI::Theme::TextSecondary);

    detachBtn.setSize(sf::Vector2f(44.f, 20.f));
    detachBtn.setFillColor(WisdomUI::Theme::PanelInset);
    detachBtn.setOutlineThickness(1.f);
    detachBtn.setOutlineColor(WisdomUI::Theme::Border);
    detachLabel.setFont(font);
    detachLabel.setString("Float");
    detachLabel.setCharacterSize(10);
    detachLabel.setFillColor(WisdomUI::Theme::TextSecondary);

    primaryBox.setSize(sf::Vector2f(34.f, 34.f));
    primaryBox.setOutlineThickness(1.5f);
    primaryBox.setOutlineColor(WisdomUI::Theme::BorderHighlight);

    secondaryBox.setSize(sf::Vector2f(34.f, 34.f));
    secondaryBox.setOutlineThickness(1.f);
    secondaryBox.setOutlineColor(WisdomUI::Theme::Border);

    eyedropperBtn.setSize(sf::Vector2f(84.f, 22.f));
    eyedropperBtn.setFillColor(WisdomUI::Theme::PanelInset);
    eyedropperBtn.setOutlineThickness(1.f);
    eyedropperBtn.setOutlineColor(WisdomUI::Theme::Border);

    eyedropperLabel.setFont(font);
    eyedropperLabel.setString("Eyedropper");
    eyedropperLabel.setCharacterSize(10);
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
        if (focusMode || !isOpen) targetX = 1920.f;
        else targetX = 1920.f - 44.f - width;

        currentX += (targetX - currentX) * 16.0f * dt;
    }

    float panelX = isDetached ? detachedPos.x : currentX;
    float panelY = isDetached ? detachedPos.y : 68.f;
    float panelW = isDetached ? detachedSize.x : width;
    float panelH = isDetached ? detachedSize.y : (1080.f - 92.f);

    background.setPosition(panelX, panelY);
    background.setSize(sf::Vector2f(panelW, panelH));

    headerBg.setPosition(panelX, panelY);
    headerBg.setSize(sf::Vector2f(panelW, 28.f));
    headerText.setPosition(panelX + 10.f, panelY + 6.f);

    closeBtn.setPosition(panelX + panelW - 25.f, panelY + 4.f);
    closeText.setPosition(panelX + panelW - 19.f, panelY + 7.f);

    detachBtn.setPosition(panelX + panelW - 74.f, panelY + 4.f);
    detachLabel.setString(isDetached ? "Dock" : "Float");
    detachLabel.setPosition(panelX + panelW - 66.f, panelY + 7.f);

    pinBtn.setPosition(panelX + panelW - 116.f, panelY + 4.f);
    pinLabel.setPosition(panelX + panelW - 108.f, panelY + 7.f);

    primaryBox.setPosition(panelX + 16.f, panelY + 36.f);
    secondaryBox.setPosition(panelX + 32.f, panelY + 52.f);

    primaryBox.setFillColor(canvas.getPrimaryColor());
    secondaryBox.setFillColor(canvas.getSecondaryColor());

    eyedropperBtn.setPosition(panelX + 80.f, panelY + 42.f);
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
    eyedropperLabel.setPosition(panelX + 92.f, panelY + 46.f);

    float pickerW = std::clamp(panelW - 32.f, 160.f, 400.f);
    float pickerY = panelY + 86.f;
    float pickerX = panelX + 16.f;

    svSprite.setPosition(pickerX, pickerY);
    svSprite.setScale(pickerW / 200.f, pickerW / 200.f);
    svSelector.setPosition(pickerX + currentSat * pickerW, pickerY + (1.0f - currentVal) * pickerW);

    float hueY = pickerY + pickerW + 8.f;
    hueSprite.setPosition(pickerX, hueY);
    hueSprite.setScale(pickerW / 200.f, 1.f);
    hueSelector.setPosition(pickerX + (currentHue / 360.f) * pickerW, hueY);

    float alphaY = hueY + 22.f;
    alphaSprite.setPosition(pickerX, alphaY);
    alphaSprite.setScale(pickerW / 200.f, 1.f);
    alphaSelector.setPosition(pickerX + currentAlpha * pickerW, alphaY);
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

void ColorPalettePanel::draw(sf::RenderWindow& window) {
    if (!isDetached && currentX >= 1918.f) return;

    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    WisdomUI::Theme::DrawFiligreePanel(window, background.getGlobalBounds(), 1.0f);

    window.draw(headerBg);
    window.draw(headerText);

    auto styleBtn = [&](sf::RectangleShape& r, sf::Text& t) {
        bool hov = r.getGlobalBounds().contains(mousePos);
        r.setFillColor(hov ? WisdomUI::Theme::PanelHover : WisdomUI::Theme::PanelInset);
        r.setOutlineColor(hov ? WisdomUI::Theme::BorderHighlight : WisdomUI::Theme::Border);
        window.draw(r);
        window.draw(t);
        };

    styleBtn(closeBtn, closeText);
    styleBtn(detachBtn, detachLabel);
    if (!isDetached) {
        styleBtn(pinBtn, pinLabel);
    }

    window.draw(secondaryBox);
    window.draw(primaryBox);

    window.draw(eyedropperBtn);
    window.draw(eyedropperLabel);

    float pickerW = std::clamp(background.getSize().x - 32.f, 160.f, 400.f);

    sf::RectangleShape svOutline(sf::Vector2f(pickerW + 2.f, pickerW + 2.f));
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

    float panelX = background.getPosition().x;
    float inputY = alphaSprite.getPosition().y + 22.f;

    auto drawInput = [&](int index, const std::string& label, const std::string& val, float x, float w) {
        sf::Text t(label, font, 10);
        t.setPosition(x, inputY);
        t.setFillColor(WisdomUI::Theme::TextSecondary);
        window.draw(t);

        sf::RectangleShape box(sf::Vector2f(w, 18.f));
        box.setPosition(x, inputY + 12.f);
        box.setFillColor(WisdomUI::Theme::PanelInset);
        box.setOutlineThickness(1.f);
        box.setOutlineColor(activeInputIndex == index ? WisdomUI::Theme::BorderHighlight : WisdomUI::Theme::Border);
        window.draw(box);

        sf::Text v(activeInputIndex == index ? inputBuffer + "_" : val, font, 10);
        v.setPosition(x + 4.f, inputY + 14.f);
        v.setFillColor(WisdomUI::Theme::Gold);
        window.draw(v);
        };

    sf::Color curC = ColorManager::hsvToRgb(currentHue, currentSat, currentVal);
    curC.a = static_cast<sf::Uint8>(currentAlpha * 255.f);

    drawInput(0, "R", std::to_string(curC.r), panelX + 16.f, 32.f);
    drawInput(1, "G", std::to_string(curC.g), panelX + 52.f, 32.f);
    drawInput(2, "B", std::to_string(curC.b), panelX + 88.f, 32.f);
    drawInput(3, "A", std::to_string(curC.a), panelX + 124.f, 32.f);
    drawInput(4, "Hex", colorToHex(curC), panelX + 162.f, 54.f);

    float sx = panelX + 16.f;
    float sy = inputY + 36.f;
    sf::Text rt("Recent", font, 10);
    rt.setPosition(sx, sy);
    rt.setFillColor(WisdomUI::Theme::Gold);
    window.draw(rt);
    sy += 16.f;

    for (const auto& c : colorManager.getRecentColors()) {
        sf::RectangleShape s(sf::Vector2f(14.f, 14.f));
        s.setPosition(sx, sy);
        s.setFillColor(c);
        s.setOutlineThickness(1.f);
        s.setOutlineColor(WisdomUI::Theme::Border);
        window.draw(s);
        sx += 18.f;
        if (sx > panelX + background.getSize().x - 24.f) { sx = panelX + 16.f; sy += 18.f; }
    }

    sx = panelX + 16.f;
    sy += 20.f;
    sf::Text ct("Swatches", font, 10);
    ct.setPosition(sx, sy);
    ct.setFillColor(WisdomUI::Theme::Gold);
    window.draw(ct);

    sf::RectangleShape addSwatchBtn(sf::Vector2f(16.f, 16.f));
    addSwatchBtn.setPosition(panelX + background.getSize().x - 26.f, sy - 1.f);
    addSwatchBtn.setFillColor(WisdomUI::Theme::PanelInset);
    addSwatchBtn.setOutlineThickness(1.f);
    addSwatchBtn.setOutlineColor(WisdomUI::Theme::Border);
    window.draw(addSwatchBtn);
    sf::Text plus("+", font, 12);
    plus.setPosition(addSwatchBtn.getPosition().x + 4.f, addSwatchBtn.getPosition().y - 2.f);
    plus.setFillColor(WisdomUI::Theme::Gold);
    window.draw(plus);

    sy += 16.f;
    for (const auto& c : colorManager.getCustomSwatches()) {
        sf::RectangleShape s(sf::Vector2f(16.f, 16.f));
        s.setPosition(sx, sy);
        s.setFillColor(c);
        s.setOutlineThickness(1.f);
        s.setOutlineColor(WisdomUI::Theme::Border);
        window.draw(s);
        sx += 20.f;
        if (sx > panelX + background.getSize().x - 24.f) { sx = panelX + 16.f; sy += 20.f; }
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
}

std::string ColorPalettePanel::processClick(sf::Vector2f mousePos, Canvas& canvas) {
    if (closeBtn.getGlobalBounds().contains(mousePos)) {
        forceClose();
        return "color_close";
    }

    if (detachBtn.getGlobalBounds().contains(mousePos)) {
        isDetached = !isDetached;
        if (isDetached) {
            state = PalettePanelState::Visible;
        }
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

bool ColorPalettePanel::handleEvent(const sf::Event& event, sf::Vector2f mousePos, Canvas& canvas) {
    if (activeInputIndex != -1) {
        if (event.type == sf::Event::TextEntered) {
            if (event.text.unicode == '\b' && !inputBuffer.empty()) inputBuffer.pop_back();
            else if (event.text.unicode < 128 && event.text.unicode != '\r' && event.text.unicode != '\n' && event.text.unicode != '\b') {
                inputBuffer += static_cast<char>(event.text.unicode);
            }
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
                    if (!hex.empty() && hex[0] == '#') hex = hex.substr(1);
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
                detachedSize = sf::Vector2f(width, 620.f);
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

        float panelX = background.getPosition().x;
        float inputY = alphaSprite.getPosition().y + 22.f + 12.f;

        if (sf::FloatRect(panelX + 16.f, inputY, 32.f, 18.f).contains(mousePos)) { activeInputIndex = 0; inputBuffer = ""; return true; }
        if (sf::FloatRect(panelX + 52.f, inputY, 32.f, 18.f).contains(mousePos)) { activeInputIndex = 1; inputBuffer = ""; return true; }
        if (sf::FloatRect(panelX + 88.f, inputY, 32.f, 18.f).contains(mousePos)) { activeInputIndex = 2; inputBuffer = ""; return true; }
        if (sf::FloatRect(panelX + 124.f, inputY, 32.f, 18.f).contains(mousePos)) { activeInputIndex = 3; inputBuffer = ""; return true; }
        if (sf::FloatRect(panelX + 162.f, inputY, 54.f, 18.f).contains(mousePos)) { activeInputIndex = 4; inputBuffer = ""; return true; }

        activeInputIndex = -1;

        sf::Color curC = ColorManager::hsvToRgb(currentHue, currentSat, currentVal);
        curC.a = static_cast<sf::Uint8>(currentAlpha * 255.f);

        float sy = inputY + 36.f + 16.f;
        float sx = panelX + 16.f;
        for (const auto& c : colorManager.getRecentColors()) {
            if (sf::FloatRect(sx, sy, 14.f, 14.f).contains(mousePos)) {
                updateFromRGB(c);
                canvas.setPrimaryColor(c);
                return true;
            }
            sx += 18.f;
            if (sx > panelX + background.getSize().x - 24.f) { sx = panelX + 16.f; sy += 18.f; }
        }

        sx = panelX + 16.f;
        sy += 20.f;
        if (sf::FloatRect(panelX + background.getSize().x - 26.f, sy - 1.f, 16.f, 16.f).contains(mousePos)) {
            colorManager.addCustomSwatch(curC);
            return true;
        }

        sy += 16.f;
        int removeIdx = -1;
        for (size_t i = 0; i < colorManager.getCustomSwatches().size(); ++i) {
            if (sf::FloatRect(sx, sy, 16.f, 16.f).contains(mousePos)) {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt)) removeIdx = static_cast<int>(i);
                else {
                    updateFromRGB(colorManager.getCustomSwatches()[i]);
                    canvas.setPrimaryColor(colorManager.getCustomSwatches()[i]);
                }
                return true;
            }
            sx += 20.f;
            if (sx > panelX + background.getSize().x - 24.f) { sx = panelX + 16.f; sy += 20.f; }
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
        isDraggingWindow = false;
        isResizing = false;
        activeResizeDir = PaletteResizeDir::None;
    }

    if (event.type == sf::Event::MouseMoved) {
        if (isResizing && isDetached) {
            float dx = mousePos.x - resizeStartMouse.x;
            float dy = mousePos.y - resizeStartMouse.y;

            float minW = 240.f;
            float minH = 420.f;

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

        float pickerW = std::clamp(background.getSize().x - 32.f, 160.f, 400.f);

        if (isDraggingSV) {
            currentSat = std::clamp((mousePos.x - svSprite.getPosition().x) / pickerW, 0.f, 1.f);
            currentVal = 1.0f - std::clamp((mousePos.y - svSprite.getPosition().y) / pickerW, 0.f, 1.f);
            updatePickerImages();
            sf::Color curC = ColorManager::hsvToRgb(currentHue, currentSat, currentVal);
            curC.a = static_cast<sf::Uint8>(currentAlpha * 255.f);
            canvas.setPrimaryColor(curC);
            return true;
        }
        else if (isDraggingHue) {
            currentHue = std::clamp((mousePos.x - hueSprite.getPosition().x) / pickerW, 0.f, 1.f) * 360.f;
            updatePickerImages();
            sf::Color curC = ColorManager::hsvToRgb(currentHue, currentSat, currentVal);
            curC.a = static_cast<sf::Uint8>(currentAlpha * 255.f);
            canvas.setPrimaryColor(curC);
            return true;
        }
        else if (isDraggingAlpha) {
            currentAlpha = std::clamp((mousePos.x - alphaSprite.getPosition().x) / pickerW, 0.f, 1.f);
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

bool ColorPalettePanel::getIsDetached() const { return isDetached; }
void ColorPalettePanel::setIsDetached(bool detached) { isDetached = detached; }