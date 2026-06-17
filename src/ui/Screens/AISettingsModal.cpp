#include "AISettingsModal.h"

AISettingsModal::AISettingsModal() : isOpen(false), selectedProvider("openai") {}

void AISettingsModal::init() {
    font.loadFromFile("assets/font.otf");

    overlay.setSize(sf::Vector2f(1920.f, 1080.f));
    overlay.setFillColor(sf::Color(0, 0, 0, 180));

    modalBg.setSize(sf::Vector2f(700.f, 500.f));
    modalBg.setPosition(1920.f / 2.f - 350.f, 1080.f / 2.f - 250.f);
    modalBg.setFillColor(sf::Color(35, 35, 40, 240));
    modalBg.setOutlineThickness(2.f);
    modalBg.setOutlineColor(sf::Color(100, 100, 110));

    title.setFont(font);
    title.setString("Configure AI Providers");
    title.setCharacterSize(28);
    title.setFillColor(sf::Color::White);
    title.setPosition(modalBg.getPosition().x + 30.f, modalBg.getPosition().y + 30.f);

    statusText.setFont(font);
    statusText.setCharacterSize(16);
    statusText.setFillColor(sf::Color(200, 200, 200));
    statusText.setPosition(modalBg.getPosition().x + 30.f, modalBg.getPosition().y + 80.f);

    std::vector<std::string> provs = { "openai", "gemini", "anthropic", "openrouter" };
    float px = modalBg.getPosition().x + 30.f;
    for (const auto& p : provs) {
        ModalButton btn;
        btn.id = p;
        btn.rect.setSize(sf::Vector2f(140.f, 40.f));
        btn.rect.setPosition(px, modalBg.getPosition().y + 130.f);
        btn.rect.setFillColor(sf::Color(50, 50, 55));

        btn.text.setFont(font);
        btn.text.setString(p);
        btn.text.setCharacterSize(16);
        btn.text.setFillColor(sf::Color::White);
        sf::FloatRect tRect = btn.text.getLocalBounds();
        btn.text.setOrigin(tRect.left + tRect.width / 2.0f, tRect.top + tRect.height / 2.0f);
        btn.text.setPosition(btn.rect.getPosition().x + 70.f, btn.rect.getPosition().y + 20.f);

        providerButtons.push_back(btn);
        px += 160.f;
    }

    inputLabel.setFont(font);
    inputLabel.setString("API Key:");
    inputLabel.setCharacterSize(18);
    inputLabel.setFillColor(sf::Color::White);
    inputLabel.setPosition(modalBg.getPosition().x + 30.f, modalBg.getPosition().y + 220.f);

    inputBox.setSize(sf::Vector2f(640.f, 50.f));
    inputBox.setPosition(modalBg.getPosition().x + 30.f, modalBg.getPosition().y + 260.f);
    inputBox.setFillColor(sf::Color(20, 20, 25));
    inputBox.setOutlineThickness(1.f);
    inputBox.setOutlineColor(sf::Color(80, 80, 90));

    inputText.setFont(font);
    inputText.setCharacterSize(20);
    inputText.setFillColor(sf::Color::White);
    inputText.setPosition(inputBox.getPosition().x + 15.f, inputBox.getPosition().y + 12.f);

    saveButton.id = "save";
    saveButton.rect.setSize(sf::Vector2f(150.f, 45.f));
    saveButton.rect.setPosition(modalBg.getPosition().x + 520.f, modalBg.getPosition().y + 420.f);
    saveButton.rect.setFillColor(sf::Color(0, 122, 204));
    saveButton.text.setFont(font);
    saveButton.text.setString("Save");
    saveButton.text.setCharacterSize(18);
    saveButton.text.setFillColor(sf::Color::White);
    saveButton.text.setPosition(saveButton.rect.getPosition().x + 55.f, saveButton.rect.getPosition().y + 12.f);

    closeButton.id = "close";
    closeButton.rect.setSize(sf::Vector2f(150.f, 45.f));
    closeButton.rect.setPosition(modalBg.getPosition().x + 350.f, modalBg.getPosition().y + 420.f);
    closeButton.rect.setFillColor(sf::Color(80, 80, 85));
    closeButton.text.setFont(font);
    closeButton.text.setString("Cancel");
    closeButton.text.setCharacterSize(18);
    closeButton.text.setFillColor(sf::Color::White);
    closeButton.text.setPosition(closeButton.rect.getPosition().x + 45.f, closeButton.rect.getPosition().y + 12.f);
}

void AISettingsModal::open(const AppSettings& currentSettings) {
    isOpen = true;
    selectedProvider = currentSettings.activeProvider != "none" ? currentSettings.activeProvider : "openai";
    if (currentSettings.apiKeys.count(selectedProvider)) {
        inputBuffer = currentSettings.apiKeys.at(selectedProvider);
    }
    else {
        inputBuffer = "";
    }
    updateProviderButtons();
}

void AISettingsModal::close() {
    isOpen = false;
}

bool AISettingsModal::getIsOpen() const {
    return isOpen;
}

void AISettingsModal::updateProviderButtons() {
    for (auto& btn : providerButtons) {
        if (btn.id == selectedProvider) {
            btn.rect.setFillColor(sf::Color(0, 122, 204));
        }
        else {
            btn.rect.setFillColor(btn.isHovered ? sf::Color(70, 70, 75) : sf::Color(50, 50, 55));
        }
    }
    inputText.setString(inputBuffer + "_");
    statusText.setString("Editing settings for: " + selectedProvider);
}

void AISettingsModal::updateHover(sf::Vector2f mousePos) {
    if (!isOpen) return;
    for (auto& btn : providerButtons) {
        btn.isHovered = btn.rect.getGlobalBounds().contains(mousePos);
    }
    saveButton.isHovered = saveButton.rect.getGlobalBounds().contains(mousePos);
    saveButton.rect.setFillColor(saveButton.isHovered ? sf::Color(0, 142, 224) : sf::Color(0, 122, 204));

    closeButton.isHovered = closeButton.rect.getGlobalBounds().contains(mousePos);
    closeButton.rect.setFillColor(closeButton.isHovered ? sf::Color(100, 100, 105) : sf::Color(80, 80, 85));

    updateProviderButtons();
}

void AISettingsModal::handleTextEntered(sf::Uint32 unicode) {
    if (unicode == '\b' && !inputBuffer.empty()) {
        inputBuffer.pop_back();
    }
    else if (unicode < 128 && unicode != '\r' && unicode != '\n' && unicode != '\b') {
        inputBuffer += static_cast<char>(unicode);
    }
    inputText.setString(inputBuffer + "_");
}

void AISettingsModal::handleKeyPress(sf::Keyboard::Key key, AppSettings& settings) {
    if (key == sf::Keyboard::Escape) {
        close();
    }
}

bool AISettingsModal::handleClick(sf::Vector2f mousePos, AppSettings& settings) {
    if (!isOpen) return false;

    for (auto& btn : providerButtons) {
        if (btn.rect.getGlobalBounds().contains(mousePos)) {
            settings.apiKeys[selectedProvider] = inputBuffer; // save current before switching
            selectedProvider = btn.id;
            if (settings.apiKeys.count(selectedProvider)) inputBuffer = settings.apiKeys[selectedProvider];
            else inputBuffer = "";
            updateProviderButtons();
            return true;
        }
    }

    if (saveButton.rect.getGlobalBounds().contains(mousePos)) {
        settings.activeProvider = selectedProvider;
        settings.apiKeys[selectedProvider] = inputBuffer;
        SettingsManager::saveSettings(settings);
        close();
        return true;
    }

    if (closeButton.rect.getGlobalBounds().contains(mousePos)) {
        close();
        return true;
    }

    return true; // Block clicks passing through
}

void AISettingsModal::draw(sf::RenderWindow& window) {
    if (!isOpen) return;
    window.draw(overlay);
    window.draw(modalBg);
    window.draw(title);
    window.draw(statusText);
    for (const auto& btn : providerButtons) {
        window.draw(btn.rect);
        window.draw(btn.text);
    }
    window.draw(inputLabel);
    window.draw(inputBox);
    window.draw(inputText);
    window.draw(saveButton.rect);
    window.draw(saveButton.text);
    window.draw(closeButton.rect);
    window.draw(closeButton.text);
}