#include "ColorPalettePanel.h"
#include "../UI/UITheme.h"
#include <iomanip>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <iostream>

ColorPalettePanel::ColorPalettePanel()
    : width(290.f), currentX(1920.f), targetX(1920.f), state(PalettePanelState::Hidden),
    hovered(false), isDetached(false), detachedPos(1500.f, 80.f), detachedSize(290.f, 640.f),
    isDraggingWindow(false), windowDragOffset(0.f, 0.f), isResizing(false),
    activeResizeDir(PaletteResizeDir::None), pickerSize(210.f), pickerX(0.f), pickerY(0.f),
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
    headerText.setCharacterSize(13);
    headerText.setFillColor(WisdomUI::Theme::Gold);

    closeBtn.setSize(sf::Vector2f(24.f, 24.f));
    closeBtn.setFillColor(WisdomUI::Theme::PanelInset);
    closeBtn.setOutlineThickness(1.f);
    closeBtn.setOutlineColor(WisdomUI::Theme::Border);
    closeText.setFont(font);
    closeText.setString("X");
    closeText.setCharacterSize(11);
    closeText.setFillColor(WisdomUI::Theme::TextSecondary);

    pinBtn.setSize(sf::Vector2f(44.f, 24.f));
    pinBtn.setFillColor(WisdomUI::Theme::PanelInset);
    pinBtn.setOutlineThickness(1.f);
    pinBtn.setOutlineColor(WisdomUI::Theme::Border);
    pinLabel.setFont(font);
    pinLabel.setString("Pin");
    pinLabel.setCharacterSize(11);
    pinLabel.setFillColor(WisdomUI::Theme::TextSecondary);

    detachBtn.setSize(sf::Vector2f(50.f, 24.f));
    detachBtn.setFillColor(WisdomUI::Theme::PanelInset);
    detachBtn.setOutlineThickness(1.f);
    detachBtn.setOutlineColor(WisdomUI::Theme::Border);
    detachLabel.setFont(font);
    detachLabel.setString("Float");
    detachLabel.setCharacterSize(11);
    detachLabel.setFillColor(WisdomUI::Theme::TextSecondary);

    primaryBox.setSize(sf::Vector2f(38.f, 38.f));
    primaryBox.setOutlineThickness(1.5f);
    primaryBox.setOutlineColor(WisdomUI::Theme::BorderHighlight);

    secondaryBox.setSize(sf::Vector2f(38.f, 38.f));
    secondaryBox.setOutlineThickness(1.f);
    secondaryBox.setOutlineColor(WisdomUI::Theme::Border);

    eyedropperBtn.setSize(sf::Vector2f(96.f, 26.f));
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

    svSelector.setSize(sf::Vector2f(8.f, 8.f));
    svSelector.setOrigin(4.f, 4.f);
    svSelector.setOutlineThickness(1.5f);
    svSelector.setOutlineColor(WisdomUI::Theme::Gold);
    svSelector.setFillColor(sf::Color::Transparent);

    hueSelector.setSize(sf::Vector2f(5.f, 18.f));
    hueSelector.setOrigin(2.5f, 0.f);
    hueSelector.setOutlineThickness(1.f);
    hueSelector.setOutlineColor(sf::Color::Black);
    hueSelector.setFillColor(WisdomUI::Theme::Gold);

    alphaSelector.setSize(sf::Vector2f(5.f, 18.f));
    alphaSelector.setOrigin(2.5f, 0.f);
    alphaSelector.setOutlineThickness(1.f);
    alphaSelector.setOutlineColor(sf::Color::Black);
    alphaSelector.setFillColor(WisdomUI::Theme::Gold);

    updatePickerImages();
    loadAdvicePalettes();
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

    float panelX = std::floor(isDetached ? detachedPos.x : currentX);
    float panelY = std::floor(isDetached ? detachedPos.y : 68.f);
    float panelW = std::floor(isDetached ? detachedSize.x : width);
    float panelH = std::floor(isDetached ? detachedSize.y : (1080.f - 92.f));

    background.setPosition(panelX, panelY);
    background.setSize(sf::Vector2f(panelW, panelH));

    headerBg.setPosition(panelX, panelY);
    headerBg.setSize(sf::Vector2f(panelW, 32.f));
    headerText.setPosition(panelX + 12.f, panelY + 8.f);

    closeBtn.setPosition(panelX + panelW - 30.f, panelY + 4.f);
    closeText.setPosition(std::floor(closeBtn.getPosition().x + 8.f), std::floor(closeBtn.getPosition().y + 4.f));

    detachBtn.setPosition(panelX + panelW - 86.f, panelY + 4.f);
    detachLabel.setString(isDetached ? "Dock" : "Float");
    detachLabel.setPosition(std::floor(detachBtn.getPosition().x + 10.f), std::floor(detachBtn.getPosition().y + 4.f));

    pinBtn.setPosition(panelX + panelW - 136.f, panelY + 4.f);
    pinLabel.setPosition(std::floor(pinBtn.getPosition().x + 12.f), std::floor(pinBtn.getPosition().y + 4.f));

    primaryBox.setPosition(panelX + 16.f, panelY + 38.f);
    secondaryBox.setPosition(panelX + 34.f, panelY + 54.f);

    primaryBox.setFillColor(canvas.getPrimaryColor());
    secondaryBox.setFillColor(canvas.getSecondaryColor());

    eyedropperBtn.setPosition(panelX + 90.f, panelY + 44.f);
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

    float maxSVFromHeight = panelH - 280.f;
    float maxSVFromWidth = panelW - 32.f;
    pickerSize = std::floor(std::clamp(std::min(maxSVFromWidth, maxSVFromHeight), 120.f, 320.f));

    pickerX = std::floor(panelX + (panelW - pickerSize) * 0.5f);
    pickerY = std::floor(panelY + 92.f);

    svSprite.setPosition(pickerX, pickerY);
    svSprite.setScale(pickerSize / 200.f, pickerSize / 200.f);
    svSelector.setPosition(std::floor(pickerX + currentSat * pickerSize), std::floor(pickerY + (1.0f - currentVal) * pickerSize));

    float hueY = std::floor(pickerY + pickerSize + 10.f);
    hueSprite.setPosition(pickerX, hueY);
    hueSprite.setScale(pickerSize / 200.f, 18.f / 15.f);
    hueSelector.setSize(sf::Vector2f(5.f, 18.f));
    hueSelector.setPosition(std::floor(pickerX + (currentHue / 360.f) * pickerSize), hueY);

    float alphaY = std::floor(hueY + 24.f);
    alphaSprite.setPosition(pickerX, alphaY);
    alphaSprite.setScale(pickerSize / 200.f, 18.f / 15.f);
    alphaSelector.setSize(sf::Vector2f(5.f, 18.f));
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

    float inputY = std::floor(alphaSprite.getPosition().y + 26.f);

    auto drawInput = [&](int index, const std::string& label, const std::string& val, float x, float w) {
        sf::Text t(label, font, 11);
        t.setPosition(std::floor(x), std::floor(inputY));
        t.setFillColor(WisdomUI::Theme::TextSecondary);
        window.draw(t);

        sf::RectangleShape box(sf::Vector2f(w, 24.f));
        box.setPosition(std::floor(x), std::floor(inputY + 14.f));
        box.setFillColor(WisdomUI::Theme::PanelInset);
        box.setOutlineThickness(1.f);
        box.setOutlineColor(activeInputIndex == index ? WisdomUI::Theme::BorderHighlight : WisdomUI::Theme::Border);
        window.draw(box);

        sf::Text v(activeInputIndex == index ? inputBuffer + "_" : val, font, 12);
        sf::FloatRect vBounds = v.getLocalBounds();
        v.setPosition(std::floor(x + (w - vBounds.width) / 2.f), std::floor(inputY + 17.f));
        v.setFillColor(WisdomUI::Theme::Gold);
        window.draw(v);
        };

    sf::Color curC = ColorManager::hsvToRgb(currentHue, currentSat, currentVal);
    curC.a = static_cast<sf::Uint8>(currentAlpha * 255.f);

    float boxW = 34.f;
    float hexW = 66.f;
    float boxSpacing = (pickerSize - (4.f * boxW) - hexW) / 4.f;
    boxSpacing = std::max(2.f, boxSpacing);

    float curInputX = pickerX;
    drawInput(0, "R", std::to_string(curC.r), curInputX, boxW); curInputX += boxW + boxSpacing;
    drawInput(1, "G", std::to_string(curC.g), curInputX, boxW); curInputX += boxW + boxSpacing;
    drawInput(2, "B", std::to_string(curC.b), curInputX, boxW); curInputX += boxW + boxSpacing;
    drawInput(3, "A", std::to_string(curC.a), curInputX, boxW); curInputX += boxW + boxSpacing;
    drawInput(4, "Hex", colorToHex(curC), curInputX, hexW);

    float sx = pickerX;
    float sy = std::floor(inputY + 44.f);
    if (sy + 20.f < panelY + panelH) {
        sf::Text rt("Recent", font, 11);
        rt.setPosition(std::floor(sx), std::floor(sy));
        rt.setFillColor(WisdomUI::Theme::Gold);
        window.draw(rt);
        sy += 16.f;

        for (const auto& c : colorManager.getRecentColors()) {
            if (sy + 20.f >= panelY + panelH - 4.f) break;
            sf::RectangleShape s(sf::Vector2f(20.f, 20.f));
            s.setPosition(std::floor(sx), std::floor(sy));
            s.setFillColor(c);
            s.setOutlineThickness(1.f);
            s.setOutlineColor(WisdomUI::Theme::Border);
            window.draw(s);
            sx += 24.f;
            if (sx > pickerX + pickerSize - 20.f) { sx = pickerX; sy += 24.f; }
        }

        sx = pickerX;
        sy += 28.f;
        if (sy + 20.f < panelY + panelH) {
            sf::Text ct("Swatches", font, 11);
            ct.setPosition(std::floor(sx), std::floor(sy));
            ct.setFillColor(WisdomUI::Theme::Gold);
            window.draw(ct);

            sf::RectangleShape addSwatchBtn(sf::Vector2f(18.f, 18.f));
            addSwatchBtn.setPosition(std::floor(pickerX + pickerSize - 18.f), std::floor(sy));
            addSwatchBtn.setFillColor(WisdomUI::Theme::PanelInset);
            addSwatchBtn.setOutlineThickness(1.f);
            addSwatchBtn.setOutlineColor(WisdomUI::Theme::Border);
            window.draw(addSwatchBtn);
            sf::Text plus("+", font, 13);
            plus.setPosition(std::floor(addSwatchBtn.getPosition().x + 4.f), std::floor(addSwatchBtn.getPosition().y - 1.f));
            plus.setFillColor(WisdomUI::Theme::Gold);
            window.draw(plus);

            sy += 20.f;
            for (const auto& c : colorManager.getCustomSwatches()) {
                if (sy + 22.f >= panelY + panelH - 4.f) break;
                sf::RectangleShape s(sf::Vector2f(22.f, 22.f));
                s.setPosition(std::floor(sx), std::floor(sy));
                s.setFillColor(c);
                s.setOutlineThickness(1.f);
                s.setOutlineColor(WisdomUI::Theme::Border);
                window.draw(s);
                sx += 26.f;
                if (sx > pickerX + pickerSize - 22.f) { sx = pickerX; sy += 26.f; }
            }
        }

        m_adviceSwatchBounds.clear();
        m_advicePinBtnBounds.clear();
        m_adviceDeleteBtnBounds.clear();

        sx = pickerX;
        sy += 30.f;

        if (sy + 22.f < panelY + panelH) {
            sf::Text at("Color Advice (Max 5)", font, 11);
            at.setPosition(std::floor(sx), std::floor(sy));
            at.setFillColor(WisdomUI::Theme::Gold);
            window.draw(at);

            m_adviceClearAllBounds = sf::FloatRect(pickerX + pickerSize - 40.f, sy - 1.f, 40.f, 16.f);
            bool hovClear = m_adviceClearAllBounds.contains(mousePos);
            sf::RectangleShape clearBtn(sf::Vector2f(m_adviceClearAllBounds.width, m_adviceClearAllBounds.height));
            clearBtn.setPosition(m_adviceClearAllBounds.left, m_adviceClearAllBounds.top);
            clearBtn.setFillColor(hovClear ? sf::Color(140, 30, 45) : WisdomUI::Theme::PanelInset);
            clearBtn.setOutlineThickness(1.f);
            clearBtn.setOutlineColor(WisdomUI::Theme::Border);
            window.draw(clearBtn);

            sf::Text clrTxt("Clear", font, 9);
            clrTxt.setPosition(m_adviceClearAllBounds.left + 8.f, m_adviceClearAllBounds.top + 1.f);
            clrTxt.setFillColor(hovClear ? sf::Color::White : WisdomUI::Theme::TextSecondary);
            window.draw(clrTxt);

            sy += 20.f;

            for (size_t pIdx = 0; pIdx < m_advicePalettes.size(); ++pIdx) {
                if (sy + 22.f >= panelY + panelH - 4.f) break;

                const auto& pal = m_advicePalettes[pIdx];
                float advX = pickerX;

                for (size_t cIdx = 0; cIdx < pal.colors.size(); ++cIdx) {
                    sf::FloatRect swRect(std::floor(advX), std::floor(sy), 22.f, 22.f);
                    sf::RectangleShape s(sf::Vector2f(swRect.width, swRect.height));
                    s.setPosition(swRect.left, swRect.top);
                    s.setFillColor(pal.colors[cIdx]);
                    s.setOutlineThickness(1.f);
                    s.setOutlineColor(WisdomUI::Theme::BorderHighlight);
                    window.draw(s);

                    m_adviceSwatchBounds.push_back({ swRect, { pal.colors[cIdx], pIdx } });
                    advX += 26.f;
                }

                sf::FloatRect pinRect(pickerX + pickerSize - 42.f, sy + 2.f, 19.f, 18.f);
                bool hovPin = pinRect.contains(mousePos);
                sf::RectangleShape pinBtn(sf::Vector2f(pinRect.width, pinRect.height));
                pinBtn.setPosition(pinRect.left, pinRect.top);
                pinBtn.setFillColor(pal.isPinned ? sf::Color(220, 160, 40) : (hovPin ? WisdomUI::Theme::PanelHover : WisdomUI::Theme::PanelInset));
                pinBtn.setOutlineThickness(1.f);
                pinBtn.setOutlineColor(pal.isPinned ? WisdomUI::Theme::Gold : WisdomUI::Theme::Border);
                window.draw(pinBtn);

                sf::Text pinTxt("P", font, 11);
                pinTxt.setPosition(pinRect.left + 5.f, pinRect.top - 1.f);
                pinTxt.setFillColor(pal.isPinned ? sf::Color(14, 6, 20) : (hovPin ? sf::Color::White : WisdomUI::Theme::TextSecondary));
                window.draw(pinTxt);

                m_advicePinBtnBounds.push_back({ pinRect, pIdx });

                sf::FloatRect delRect(pickerX + pickerSize - 20.f, sy + 2.f, 18.f, 18.f);
                bool hovDel = delRect.contains(mousePos);
                sf::RectangleShape delBtn(sf::Vector2f(delRect.width, delRect.height));
                delBtn.setPosition(delRect.left, delRect.top);
                delBtn.setFillColor(pal.isPinned ? sf::Color(30, 20, 30, 120) : (hovDel ? sf::Color(180, 40, 55) : WisdomUI::Theme::PanelInset));
                delBtn.setOutlineThickness(1.f);
                delBtn.setOutlineColor(WisdomUI::Theme::Border);
                window.draw(delBtn);

                sf::Text xTxt("x", font, 11);
                xTxt.setPosition(delRect.left + 5.f, delRect.top - 1.f);
                xTxt.setFillColor(pal.isPinned ? sf::Color(80, 70, 80) : (hovDel ? sf::Color::White : WisdomUI::Theme::TextSecondary));
                window.draw(xTxt);

                m_adviceDeleteBtnBounds.push_back({ delRect, pIdx });
                sy += 26.f;
            }
        }
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

        float inputY = std::floor(alphaSprite.getPosition().y + 26.f + 14.f);
        float boxW = 34.f;
        float hexW = 66.f;
        float boxSpacing = (pickerSize - (4.f * boxW) - hexW) / 4.f;
        boxSpacing = std::max(2.f, boxSpacing);

        float curInputX = pickerX;
        if (sf::FloatRect(curInputX, inputY, boxW, 24.f).contains(mousePos)) { activeInputIndex = 0; inputBuffer = ""; return true; } curInputX += boxW + boxSpacing;
        if (sf::FloatRect(curInputX, inputY, boxW, 24.f).contains(mousePos)) { activeInputIndex = 1; inputBuffer = ""; return true; } curInputX += boxW + boxSpacing;
        if (sf::FloatRect(curInputX, inputY, boxW, 24.f).contains(mousePos)) { activeInputIndex = 2; inputBuffer = ""; return true; } curInputX += boxW + boxSpacing;
        if (sf::FloatRect(curInputX, inputY, boxW, 24.f).contains(mousePos)) { activeInputIndex = 3; inputBuffer = ""; return true; } curInputX += boxW + boxSpacing;
        if (sf::FloatRect(curInputX, inputY, hexW, 24.f).contains(mousePos)) { activeInputIndex = 4; inputBuffer = ""; return true; }

        activeInputIndex = -1;

        sf::Color curC = ColorManager::hsvToRgb(currentHue, currentSat, currentVal);
        curC.a = static_cast<sf::Uint8>(currentAlpha * 255.f);
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
                }
                updateFromRGB(picked);
                canvas.setPrimaryColor(picked);
                return true;
            }
        }
        float sy = std::floor(inputY + 44.f + 16.f);
        float sx = pickerX;
        for (const auto& c : colorManager.getRecentColors()) {
            if (sf::FloatRect(sx, sy, 20.f, 20.f).contains(mousePos)) {
                updateFromRGB(c);
                canvas.setPrimaryColor(c);
                return true;
            }
            sx += 24.f;
            if (sx > pickerX + pickerSize - 20.f) { sx = pickerX; sy += 24.f; }
        }

        sx = pickerX;
        sy += 28.f;
        if (sf::FloatRect(pickerX + pickerSize - 18.f, sy, 18.f, 18.f).contains(mousePos)) {
            colorManager.addCustomSwatch(curC);
            return true;
        }

        sy += 20.f;
        int removeIdx = -1;
        for (size_t i = 0; i < colorManager.getCustomSwatches().size(); ++i) {
            if (sf::FloatRect(sx, sy, 22.f, 22.f).contains(mousePos)) {
                if (sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt)) removeIdx = static_cast<int>(i);
                else {
                    updateFromRGB(colorManager.getCustomSwatches()[i]);
                    canvas.setPrimaryColor(colorManager.getCustomSwatches()[i]);
                }
                return true;
            }
            sx += 26.f;
            if (sx > pickerX + pickerSize - 22.f) { sx = pickerX; sy += 26.f; }
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

void ColorPalettePanel::saveAdvicePalettes() const {
    std::ofstream out("color_advice.dat");
    if (!out.is_open()) return;

    out << m_advicePalettes.size() << "\n";
    for (const auto& pal : m_advicePalettes) {
        out << (pal.isPinned ? 1 : 0) << " " << pal.useCount << " " << pal.colors.size() << " ";
        for (const auto& c : pal.colors) {
            out << static_cast<int>(c.r) << " " << static_cast<int>(c.g) << " " << static_cast<int>(c.b) << " " << static_cast<int>(c.a) << " ";
        }
        out << "\n";
    }
}

void ColorPalettePanel::loadAdvicePalettes() {
    std::ifstream in("color_advice.dat");
    if (!in.is_open()) return;

    size_t total = 0;
    if (!(in >> total)) return;

    m_advicePalettes.clear();
    for (size_t i = 0; i < total; ++i) {
        int pinned = 0, useCount = 0;
        size_t numCols = 0;
        if (!(in >> pinned >> useCount >> numCols)) break;

        AdvicePalette pal;
        pal.isPinned = (pinned != 0);
        pal.useCount = useCount;

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
            bool pinnedState = m_advicePalettes[i].isPinned;
            int uses = m_advicePalettes[i].useCount + 1;
            m_advicePalettes.erase(m_advicePalettes.begin() + i);
            m_advicePalettes.push_back({ ramp, uses, pinnedState });
            saveAdvicePalettes();
            return true;
        }
    }

    if (m_advicePalettes.size() >= 5) {
        int victimIdx = -1;
        int minUses = 2147483647;

        for (size_t i = 0; i < m_advicePalettes.size(); ++i) {
            if (!m_advicePalettes[i].isPinned) {
                if (victimIdx == -1 || m_advicePalettes[i].useCount < minUses) {
                    minUses = m_advicePalettes[i].useCount;
                    victimIdx = static_cast<int>(i);
                }
            }
        }

        if (victimIdx == -1) {
            return false;
        }

        m_advicePalettes.erase(m_advicePalettes.begin() + victimIdx);
    }

    m_advicePalettes.push_back({ ramp, 0, false });
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