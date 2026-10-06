#include "KeybindSettingsPanel.h"
#include "UITheme.h"
#include <algorithm>

KeybindSettingsPanel::KeybindSettingsPanel() : isOpen(false), scrollY(0.f), kbm(nullptr) {}

void KeybindSettingsPanel::init(KeybindManager* keyManager) {
    kbm = keyManager;
    font.loadFromFile("assets/font.otf");

    overlay.setSize(sf::Vector2f(1920.f, 1080.f));
    overlay.setFillColor(sf::Color(0, 0, 0, 180));

    // These shapes only hold the hit boxes; draw() renders them with the shared theme.
    background.setSize(sf::Vector2f(820.f, 820.f));
    background.setPosition(1920.f / 2.f - 410.f, 1080.f / 2.f - 410.f);

    closeBtn.setSize(sf::Vector2f(104.f, 32.f));
    closeBtn.setPosition(background.getPosition().x + 684.f, background.getPosition().y + 24.f);

    restoreBtn.setSize(sf::Vector2f(160.f, 32.f));
    restoreBtn.setPosition(background.getPosition().x + 510.f, background.getPosition().y + 24.f);
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

    using WisdomUI::Theme;
    const sf::Color textShadow(14, 6, 20);
    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    window.draw(overlay);
    Theme::DrawSunsetPanel(window, background.getGlobalBounds(), 1.0f);

    Theme::DrawCrispText(window, font, "KEYBIND SETTINGS", 22, background.getPosition().x + 32.f, background.getPosition().y + 40.f, Theme::SunsetAmber, textShadow, false, true);

    sf::FloatRect restoreBounds = restoreBtn.getGlobalBounds();
    sf::FloatRect closeBounds = closeBtn.getGlobalBounds();
    Theme::DrawSunsetButton(window, restoreBounds, "Restore Defaults", font, 13, false, restoreBounds.contains(mousePos), false, 1.0f);
    Theme::DrawSunsetButton(window, closeBounds, "Close", font, 13, false, closeBounds.contains(mousePos), true, 1.0f);

    if (!conflictMessage.empty()) {
        Theme::DrawCrispText(window, font, conflictMessage, 14, background.getPosition().x + 32.f, background.getPosition().y + 76.f, Theme::SunsetCoral, textShadow, false, true);
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
                Theme::DrawCrispText(window, font, currentCat, 15, baseX, y + 16.f, Theme::SunsetAmber, textShadow, false, true);
            }
            y += 42.f;
        }

        if (y > clipRect.top - 42.f && y < clipRect.top + clipRect.height) {
            bool listening = (listeningId == id);
            sf::FloatRect bindBounds(baseX + 470.f, y, 220.f, 32.f);
            bool hovered = bindBounds.contains(mousePos);

            Theme::DrawCrispText(window, font, act.name, 14, baseX + 20.f, y + 16.f, Theme::TextPrimary, textShadow, false, true);

            sf::RectangleShape bindBox(sf::Vector2f(bindBounds.width, bindBounds.height));
            bindBox.setPosition(bindBounds.left, bindBounds.top);
            bindBox.setFillColor(listening ? Theme::SunsetCoralDark : Theme::PanelInset);
            bindBox.setOutlineThickness(1.f);
            bindBox.setOutlineColor((listening || hovered) ? Theme::SunsetGold : Theme::SunsetPlum);
            window.draw(bindBox);

            Theme::DrawCrispText(window, font, listening ? "Press key..." : kbm->getActionString(id), 14, bindBounds.left + 10.f, y + 16.f, listening ? Theme::TextPrimary : Theme::TextGold, sf::Color::Transparent, false, true);
        }
        y += 42.f;
    }
}