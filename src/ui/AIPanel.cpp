#include "AIPanel.h"
#include "../ai/AIManager.h"
#include <cmath>

AIPanel::AIPanel() : currentX(-400.f), targetX(-400.f), width(350.f), isVisible(false), isTypingPrompt(false), isTypingNegative(false), currentOp(AIOperation::Generate) {}

void AIPanel::init() {
    font.loadFromFile("assets/font.otf");

    background.setSize(sf::Vector2f(width, 1080.f));
    background.setFillColor(sf::Color(25, 25, 30, 245));
    background.setOutlineThickness(1.f);
    background.setOutlineColor(sf::Color(100, 150, 255, 100));

    header.setSize(sf::Vector2f(width, 40.f));
    header.setFillColor(sf::Color(35, 35, 45, 255));

    titleText.setFont(font);
    titleText.setString("AI Assistant");
    titleText.setCharacterSize(18);
    titleText.setFillColor(sf::Color::White);

    promptBox.setSize(sf::Vector2f(width - 40.f, 80.f));
    promptBox.setFillColor(sf::Color(15, 15, 18));
    promptBox.setOutlineThickness(1.f);

    promptText.setFont(font);
    promptText.setCharacterSize(14);
    promptText.setFillColor(sf::Color::White);

    negativePromptBox.setSize(sf::Vector2f(width - 40.f, 60.f));
    negativePromptBox.setFillColor(sf::Color(15, 15, 18));
    negativePromptBox.setOutlineThickness(1.f);

    negativePromptText.setFont(font);
    negativePromptText.setCharacterSize(14);
    negativePromptText.setFillColor(sf::Color::White);

    generateBtn.setSize(sf::Vector2f(width - 40.f, 45.f));
    generateBtn.setFillColor(sf::Color(50, 150, 255));

    generateBtnText.setFont(font);
    generateBtnText.setString("Execute");
    generateBtnText.setCharacterSize(16);
    generateBtnText.setFillColor(sf::Color::White);

    backBtn.setSize(sf::Vector2f(width - 40.f, 35.f));
    backBtn.setFillColor(sf::Color(150, 50, 50));

    backBtnText.setFont(font);
    backBtnText.setString("< Close Panel");
    backBtnText.setCharacterSize(14);
    backBtnText.setFillColor(sf::Color::White);

    std::vector<std::string> opNames = { "Generate", "Edit", "Variation", "Remove BG", "Upscale", "Colorize", "Inpaint", "Outpaint", "Gen Frame" };
    opValues = { AIOperation::Generate, AIOperation::Edit, AIOperation::Variation, AIOperation::RemoveBackground, AIOperation::Upscale, AIOperation::Colorize, AIOperation::Inpaint, AIOperation::Outpaint, AIOperation::GenerateFrame };

    for (size_t i = 0; i < opNames.size(); ++i) {
        sf::RectangleShape btn(sf::Vector2f((width - 50.f) / 2.f, 30.f));
        btn.setFillColor(sf::Color(40, 40, 50));
        opButtons.push_back(btn);

        sf::Text txt(opNames[i], font, 12);
        txt.setFillColor(sf::Color::White);
        opTexts.push_back(txt);
    }
}

void AIPanel::toggle() {
    isVisible = !isVisible;
    if (isVisible) targetX = 0.f;
    else targetX = -width;
}

bool AIPanel::getIsVisible() const {
    return isVisible;
}

void AIPanel::update(float dt) {
    if (!isVisible && std::abs(currentX - targetX) < 1.0f) {
        currentX = targetX;
        return;
    }

    currentX += (targetX - currentX) * 15.0f * dt;

    background.setPosition(currentX, 0.f);
    header.setPosition(currentX, 0.f);
    titleText.setPosition(currentX + 20.f, 10.f);

    float y = 60.f;

    for (size_t i = 0; i < opButtons.size(); ++i) {
        float bx = currentX + 20.f + (i % 2) * ((width - 30.f) / 2.f);
        float by = y + (i / 2) * 40.f;
        opButtons[i].setPosition(bx, by);
        if (opValues[i] == currentOp) opButtons[i].setFillColor(sf::Color(0, 120, 200));
        else opButtons[i].setFillColor(sf::Color(40, 40, 50));

        sf::FloatRect bounds = opTexts[i].getLocalBounds();
        opTexts[i].setPosition(bx + opButtons[i].getSize().x / 2.f - bounds.width / 2.f, by + 5.f);
    }

    y += ((opButtons.size() + 1) / 2) * 40.f + 20.f;

    promptBox.setPosition(currentX + 20.f, y);
    promptBox.setOutlineColor(isTypingPrompt ? sf::Color(0, 191, 255) : sf::Color(60, 60, 70));
    promptText.setPosition(currentX + 25.f, y + 5.f);

    y += 100.f;
    negativePromptBox.setPosition(currentX + 20.f, y);
    negativePromptBox.setOutlineColor(isTypingNegative ? sf::Color(255, 100, 100) : sf::Color(60, 60, 70));
    negativePromptText.setPosition(currentX + 25.f, y + 5.f);

    y += 80.f;
    generateBtn.setPosition(currentX + 20.f, y);
    sf::FloatRect gb = generateBtnText.getLocalBounds();
    generateBtnText.setPosition(currentX + 20.f + generateBtn.getSize().x / 2.f - gb.width / 2.f, y + 12.f);

    y += 60.f;
    backBtn.setPosition(currentX + 20.f, y);
    sf::FloatRect bb = backBtnText.getLocalBounds();
    backBtnText.setPosition(currentX + 20.f + backBtn.getSize().x / 2.f - bb.width / 2.f, y + 8.f);

    updateTextDisplays();
}

void AIPanel::updateTextDisplays() {
    std::string p = currentPrompt;
    if (isTypingPrompt) p += "_";
    else if (p.empty()) p = "Enter prompt...";
    promptText.setString(p);

    std::string n = currentNegativePrompt;
    if (isTypingNegative) n += "_";
    else if (n.empty()) n = "Enter negative prompt...";
    negativePromptText.setString(n);
}

void AIPanel::draw(sf::RenderWindow& window) {
    if (currentX <= -width + 1.f) return;

    window.draw(background);
    window.draw(header);
    window.draw(titleText);

    for (size_t i = 0; i < opButtons.size(); ++i) {
        window.draw(opButtons[i]);
        window.draw(opTexts[i]);
    }

    window.draw(promptBox);
    window.draw(promptText);
    window.draw(negativePromptBox);
    window.draw(negativePromptText);
    window.draw(generateBtn);
    window.draw(generateBtnText);
    window.draw(backBtn);
    window.draw(backBtnText);
}

bool AIPanel::handleEvent(const sf::Event& event, sf::Vector2f mousePos) {
    if (!isVisible) return false;

    // Check if the mouse cursor is hovering over the panel slide-out footprint
    bool mouseOverPanel = (mousePos.x >= currentX && mousePos.x <= currentX + width);

    if (event.type == sf::Event::MouseButtonPressed) {
        if (!mouseOverPanel) return false;

        isTypingPrompt = promptBox.getGlobalBounds().contains(mousePos);
        isTypingNegative = negativePromptBox.getGlobalBounds().contains(mousePos);

        for (size_t i = 0; i < opButtons.size(); ++i) {
            if (opButtons[i].getGlobalBounds().contains(mousePos)) {
                currentOp = opValues[i];
            }
        }
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

    // Capture all other event types (like MouseMoved) over the panel area to absorb the focus
    return mouseOverPanel;
}

std::string AIPanel::handleClick(sf::Vector2f mousePos) {
    if (!isVisible) return "";
    if (generateBtn.getGlobalBounds().contains(mousePos)) return "execute";
    if (backBtn.getGlobalBounds().contains(mousePos)) {
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