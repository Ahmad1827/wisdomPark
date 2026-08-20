#include "AIPanel.h"
#include "../ai/AIManager.h"
#include "../UI/UITheme.h"
#include <algorithm>
#include <cmath>

AIPanel::AIPanel() : position(64.f, 78.f), size(320.f, 540.f), isVisible(false), isTypingPrompt(false), isTypingNegative(false), currentOp(AIOperation::Generate) {}

void AIPanel::init() {
    font.loadFromFile("assets/font.otf");
    position = sf::Vector2f(64.f, 78.f);

    opNames = { "Generate", "Edit", "Variation", "Remove BG", "Upscale", "Colorize", "Inpaint", "Outpaint", "Gen Frame" };
    opValues = { AIOperation::Generate, AIOperation::Edit, AIOperation::Variation, AIOperation::RemoveBackground, AIOperation::Upscale, AIOperation::Colorize, AIOperation::Inpaint, AIOperation::Outpaint, AIOperation::GenerateFrame };
}

void AIPanel::toggle() {
    isVisible = !isVisible;
}

bool AIPanel::getIsVisible() const {
    return isVisible;
}

void AIPanel::update(float dt) {
    if (!isVisible) return;

    float bx = position.x;
    float by = position.y;

    opButtonBounds.clear();
    float opY = by + 40.f;
    float btnW = (size.x - 32.f) / 2.f;

    for (size_t i = 0; i < opNames.size(); ++i) {
        float x = bx + 12.f + (i % 2) * (btnW + 8.f);
        float y = opY + (i / 2) * 28.f;
        opButtonBounds.push_back(sf::FloatRect(x, y, btnW, 24.f));
    }

    float nextY = opY + ((opNames.size() + 1) / 2) * 28.f + 8.f;
    promptBoxBounds = sf::FloatRect(bx + 12.f, nextY, size.x - 24.f, 70.f);

    nextY += 82.f;
    negativePromptBoxBounds = sf::FloatRect(bx + 12.f, nextY, size.x - 24.f, 55.f);

    nextY += 68.f;
    generateBtnBounds = sf::FloatRect(bx + 12.f, nextY, size.x - 24.f, 32.f);

    nextY += 38.f;
    backBtnBounds = sf::FloatRect(bx + 12.f, nextY, size.x - 24.f, 26.f);
}

void AIPanel::draw(sf::RenderWindow& window) {
    if (!isVisible) return;

    sf::FloatRect panelBounds(position.x, position.y, size.x, size.y);
    WisdomUI::Theme::DrawSunsetPanel(window, panelBounds, 1.0f);

    sf::FloatRect headerGrip(position.x + 8.f, position.y + 6.f, size.x - 16.f, 26.f);
    sf::RectangleShape gripBg(sf::Vector2f(headerGrip.width, headerGrip.height));
    gripBg.setPosition(headerGrip.left, headerGrip.top);
    gripBg.setFillColor(WisdomUI::Theme::SunsetDeepDark);
    gripBg.setOutlineThickness(1.f);
    gripBg.setOutlineColor(WisdomUI::Theme::SunsetPlum);
    window.draw(gripBg);

    WisdomUI::Theme::DrawCrispText(window, font, ":: AI ASSISTANT ::", 12, headerGrip.left + headerGrip.width / 2.0f, headerGrip.top + headerGrip.height / 2.0f, WisdomUI::Theme::SunsetAmber, sf::Color(14, 6, 20), true, true);

    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    for (size_t i = 0; i < opButtonBounds.size(); ++i) {
        bool isActive = (opValues[i] == currentOp);
        bool isHov = opButtonBounds[i].contains(mPos);
        WisdomUI::Theme::DrawSunsetButton(window, opButtonBounds[i], opNames[i], font, 11, isActive, isHov, isActive, 1.0f);
    }

    auto drawInputBox = [&](sf::FloatRect b, const std::string& label, const std::string& val, bool active, const std::string& placeholder) {
        sf::RectangleShape box(sf::Vector2f(b.width, b.height));
        box.setPosition(b.left, b.top);
        box.setFillColor(WisdomUI::Theme::SunsetDeepDark);
        box.setOutlineThickness(1.5f);
        box.setOutlineColor(active ? WisdomUI::Theme::SunsetGold : WisdomUI::Theme::SunsetPlum);
        window.draw(box);

        WisdomUI::Theme::DrawCrispText(window, font, label, 10, b.left + 6.f, b.top + 4.f, WisdomUI::Theme::TextSecondary);

        std::string display = val;
        if (active) display += "_";
        else if (display.empty()) display = placeholder;

        sf::Color textColor = (val.empty() && !active) ? WisdomUI::Theme::SunsetPlum : WisdomUI::Theme::TextPrimary;
        WisdomUI::Theme::DrawCrispText(window, font, display, 11, b.left + 6.f, b.top + 18.f, textColor);
        };

    drawInputBox(promptBoxBounds, "PROMPT:", currentPrompt, isTypingPrompt, "Enter generation prompt...");
    drawInputBox(negativePromptBoxBounds, "NEGATIVE PROMPT:", currentNegativePrompt, isTypingNegative, "Optional negative tokens...");

    WisdomUI::Theme::DrawSunsetButton(window, generateBtnBounds, "EXECUTE REQUEST", font, 12, false, generateBtnBounds.contains(mPos), true, 1.0f);
    WisdomUI::Theme::DrawSunsetButton(window, backBtnBounds, "< CLOSE PANEL", font, 11, false, backBtnBounds.contains(mPos), false, 1.0f);
}

bool AIPanel::handleEvent(const sf::Event& event, sf::Vector2f mousePos) {
    if (!isVisible) return false;

    sf::FloatRect headerGrip(position.x, position.y, size.x, 34.f);

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (headerGrip.contains(mousePos)) {
            isDraggingPanel = true;
            dragOffset = mousePos - position;
            return true;
        }

        if (!sf::FloatRect(position.x, position.y, size.x, size.y).contains(mousePos)) return false;

        isTypingPrompt = promptBoxBounds.contains(mousePos);
        isTypingNegative = negativePromptBoxBounds.contains(mousePos);

        for (size_t i = 0; i < opButtonBounds.size(); ++i) {
            if (opButtonBounds[i].contains(mousePos)) {
                currentOp = opValues[i];
                return true;
            }
        }
        return true;
    }
    else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        isDraggingPanel = false;
    }
    else if (event.type == sf::Event::MouseMoved && isDraggingPanel) {
        position = mousePos - dragOffset;
        position.x = std::clamp(position.x, 56.f, 1920.f - size.x);
        position.y = std::clamp(position.y, 40.f, 1080.f - size.y);
        return true;
    }

    if (event.type == sf::Event::TextEntered) {
        if (isTypingPrompt) {
            if (event.text.unicode == '\b' && !currentPrompt.empty()) currentPrompt.pop_back();
            else if (event.text.unicode >= 32 && event.text.unicode < 127) currentPrompt += static_cast<char>(event.text.unicode);
            return true;
        }
        if (isTypingNegative) {
            if (event.text.unicode == '\b' && !currentNegativePrompt.empty()) currentNegativePrompt.pop_back();
            else if (event.text.unicode >= 32 && event.text.unicode < 127) currentNegativePrompt += static_cast<char>(event.text.unicode);
            return true;
        }
    }

    return sf::FloatRect(position.x, position.y, size.x, size.y).contains(mousePos);
}

std::string AIPanel::handleClick(sf::Vector2f mousePos) {
    if (!isVisible) return "";
    if (generateBtnBounds.contains(mousePos)) return "execute";
    if (backBtnBounds.contains(mousePos)) {
        toggle();
        return "back";
    }
    return "";
}

AIRequest AIPanel::buildRequestFromCanvasContext(int w, int h, bool pixelMode, float transparency, bool hasSelection) {
    AIRequest req;
    req.prompt = currentPrompt;
    req.negativePrompt = currentNegativePrompt;
    req.operation = currentOp;
    req.width = w;
    req.height = h;
    req.isPixelMode = pixelMode;
    req.transparency = transparency;
    req.hasMask = hasSelection;
    return req;
}