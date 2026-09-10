#include "KeybindSettingsPanel.h"
#include <algorithm>

KeybindSettingsPanel::KeybindSettingsPanel() : isOpen(false), scrollY(0.f), kbm(nullptr) {}

void KeybindSettingsPanel::init(KeybindManager* keyManager) {
    kbm = keyManager;
    font.loadFromFile("assets/font.otf");

    overlay.setSize(sf::Vector2f(1920.f, 1080.f));
    overlay.setFillColor(sf::Color(0, 0, 0, 180));

    background.setSize(sf::Vector2f(820.f, 820.f));
    background.setPosition(1920.f / 2.f - 410.f, 1080.f / 2.f - 410.f);
    background.setFillColor(sf::Color(20, 20, 24, 255));
    background.setOutlineThickness(1.f);
    background.setOutlineColor(sf::Color(100, 100, 110, 100));

    titleText.setFont(font);
    titleText.setString("Keybind Settings");
    titleText.setCharacterSize(22);
    titleText.setFillColor(sf::Color::White);
    titleText.setPosition(background.getPosition().x + 32.f, background.getPosition().y + 26.f);

    closeBtn.setSize(sf::Vector2f(104.f, 32.f));
    closeBtn.setFillColor(sf::Color(200, 50, 50));
    closeBtn.setPosition(background.getPosition().x + 684.f, background.getPosition().y + 24.f);

    closeLabel.setFont(font);
    closeLabel.setString("Close");
    closeLabel.setCharacterSize(13);
    closeLabel.setFillColor(sf::Color::White);
    closeLabel.setPosition(closeBtn.getPosition().x + 32.f, closeBtn.getPosition().y + 7.f);

    restoreBtn.setSize(sf::Vector2f(160.f, 32.f));
    restoreBtn.setFillColor(sf::Color(100, 100, 100));
    restoreBtn.setPosition(background.getPosition().x + 510.f, background.getPosition().y + 24.f);

    restoreLabel.setFont(font);
    restoreLabel.setString("Restore Defaults");
    restoreLabel.setCharacterSize(13);
    restoreLabel.setFillColor(sf::Color::White);
    restoreLabel.setPosition(restoreBtn.getPosition().x + 18.f, restoreBtn.getPosition().y + 7.f);
}

void KeybindSettingsPanel::toggle() {
    isOpen = !isOpen;
    listeningId = "";
    conflictMessage = "";
    scrollY = 0.f;
}

void KeybindSettingsPanel::close() {
    isOpen = false;
    listeningId = "";
    conflictMessage = "";
}

bool KeybindSettingsPanel::isVisible() const {
    return isOpen;
}

void KeybindSettingsPanel::handleEvent(const sf::Event& event) {
    if (!isOpen) return;

    if (!listeningId.empty() && event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Escape) {
            listeningId = "";
            return;
        }

        if (event.key.code == sf::Keyboard::LControl || event.key.code == sf::Keyboard::RControl ||
            event.key.code == sf::Keyboard::LShift || event.key.code == sf::Keyboard::RShift ||
            event.key.code == sf::Keyboard::LAlt || event.key.code == sf::Keyboard::RAlt) {
            return;
        }

        Keybind nb;
        nb.key = event.key.code;
        nb.ctrl = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::RControl);
        nb.shift = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);
        nb.alt = sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt) || sf::Keyboard::isKeyPressed(sf::Keyboard::RAlt);

        std::string conflict = kbm->getActionConflict(nb, listeningId);
        if (!conflict.empty()) {
            conflictMessage = "Conflict with: " + conflict + ". Try another.";
        }
        else {
            kbm->setKeybind(listeningId, nb);
            listeningId = "";
            conflictMessage = "";
        }
        return;
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f mPos(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));

        if (closeBtn.getGlobalBounds().contains(mPos)) {
            close();
            return;
        }
        if (restoreBtn.getGlobalBounds().contains(mPos)) {
            kbm->restoreDefaults();
            return;
        }

        if (listeningId.empty()) {
            float y = background.getPosition().y + 100.f + scrollY;
            const auto& ids = kbm->getActionOrder();
            std::string currentCat = "";

            for (const auto& id : ids) {
                const auto& act = kbm->getAction(id);
                if (act.category != currentCat) {
                    currentCat = act.category;
                    y += 42.f;
                }

                sf::FloatRect btnBounds(background.getPosition().x + 510.f, y, 220.f, 32.f);
                if (btnBounds.contains(mPos)) {
                    listeningId = id;
                    conflictMessage = "";
                    return;
                }
                y += 42.f;
            }
        }
        else {
            listeningId = "";
        }
    }

    if (event.type == sf::Event::MouseWheelScrolled && event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
        scrollY += event.mouseWheelScroll.delta * 24.f;
        if (scrollY > 0.f) scrollY = 0.f;
    }
}

void KeybindSettingsPanel::updateHover(sf::Vector2f mousePos) {}

void KeybindSettingsPanel::draw(sf::RenderWindow& window) {
    if (!isOpen) return;

    window.draw(overlay);
    window.draw(background);
    window.draw(titleText);
    window.draw(closeBtn);
    window.draw(closeLabel);
    window.draw(restoreBtn);
    window.draw(restoreLabel);

    if (!conflictMessage.empty()) {
        sf::Text errText;
        errText.setFont(font);
        errText.setString(conflictMessage);
        errText.setCharacterSize(13);
        errText.setFillColor(sf::Color::Red);
        errText.setPosition(background.getPosition().x + 32.f, background.getPosition().y + 68.f);
        window.draw(errText);
    }

    float baseX = background.getPosition().x + 40.f;
    float y = background.getPosition().y + 100.f + scrollY;
    std::string currentCat = "";

    sf::FloatRect clipRect(background.getPosition().x, background.getPosition().y + 90.f, 820.f, 710.f);

    for (const auto& id : kbm->getActionOrder()) {
        const auto& act = kbm->getAction(id);

        if (act.category != currentCat) {
            currentCat = act.category;
            if (y > clipRect.top - 42.f && y < clipRect.top + clipRect.height) {
                sf::Text catText;
                catText.setFont(font);
                catText.setString(currentCat);
                catText.setCharacterSize(15);
                catText.setFillColor(sf::Color(150, 150, 200));
                catText.setPosition(baseX, y + 2.f);
                window.draw(catText);
            }
            y += 42.f;
        }

        if (y > clipRect.top - 42.f && y < clipRect.top + clipRect.height) {
            sf::Text nText;
            nText.setFont(font);
            nText.setString(act.name);
            nText.setCharacterSize(13);
            nText.setFillColor(sf::Color::White);
            nText.setPosition(baseX + 20.f, y + 7.f);
            window.draw(nText);

            sf::RectangleShape bindBox(sf::Vector2f(220.f, 32.f));
            bindBox.setPosition(baseX + 470.f, y);
            bindBox.setFillColor(listeningId == id ? sf::Color(0, 122, 204) : sf::Color(40, 40, 45));
            bindBox.setOutlineThickness(1.f);
            bindBox.setOutlineColor(sf::Color(100, 100, 100));
            window.draw(bindBox);

            sf::Text bText;
            bText.setFont(font);
            bText.setCharacterSize(13);
            bText.setFillColor(sf::Color::White);
            bText.setString(listeningId == id ? "Press key..." : kbm->getActionString(id));
            bText.setPosition(baseX + 480.f, y + 7.f);
            window.draw(bText);
        }
        y += 42.f;
    }
}