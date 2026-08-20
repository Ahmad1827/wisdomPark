#include "RightProperties.h"
#include "../UI/UITheme.h"
#include "../core/Canvas.h"
#include <sstream>
#include <iomanip>

RightProperties::RightProperties() : width(260.f), currentX(1920.f), targetX(1920.f), state(RightPanelState::Hidden), hovered(false), pinned(false) {}

void RightProperties::init() {
    font.loadFromFile("assets/font.otf");

    background.setFillColor(WisdomUI::Theme::Panel);
    background.setOutlineThickness(1.f);
    background.setOutlineColor(WisdomUI::Theme::Border);

    headerBg.setFillColor(WisdomUI::Theme::PanelInset);
    headerText.setFont(font);
    headerText.setString("PROPERTIES");
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
    pinLabel.setString("Pin Panel");
    pinLabel.setCharacterSize(11);
    pinLabel.setFillColor(WisdomUI::Theme::TextSecondary);

    auto createSection = [&](std::string id, std::string title, std::vector<std::pair<std::string, std::string>> btns) {
        PropSection sec;
        sec.id = id;
        sec.isOpen = true;

        sec.headerRect.setSize(sf::Vector2f(width - 24.f, 26.f));
        sec.headerRect.setFillColor(WisdomUI::Theme::PanelInset);
        sec.headerRect.setOutlineThickness(1.f);
        sec.headerRect.setOutlineColor(WisdomUI::Theme::Border);

        sec.headerLabel.setFont(font);
        sec.headerLabel.setString(title);
        sec.headerLabel.setCharacterSize(12);
        sec.headerLabel.setFillColor(WisdomUI::Theme::Gold);

        for (const auto& pair : btns) {
            PropItem item;
            item.id = pair.first;
            item.rect.setSize(sf::Vector2f(width - 32.f, 24.f));
            item.rect.setFillColor(WisdomUI::Theme::PanelInset);
            item.rect.setOutlineThickness(1.f);
            item.rect.setOutlineColor(WisdomUI::Theme::Border);

            item.label.setFont(font);
            item.label.setString(pair.second);
            item.label.setCharacterSize(11);
            item.label.setFillColor(WisdomUI::Theme::TextPrimary);
            sec.items.push_back(item);
        }
        sections.push_back(sec);
        };

    createSection("anim", "ANIMATION SPEED", { {"fps_up", "+ FPS Speed"}, {"fps_down", "- FPS Speed"}, {"fps_display", "Speed: 12 FPS"} });
    createSection("onion", "ONION REFERENCE", { {"onion_toggle", "Toggle Reference"}, {"onion_op_up", "Opacity +"}, {"onion_op_down", "Opacity -"} });
    createSection("themes", "THEMES", { {"theme_all", "All Objects"}, {"theme_struct", "Structures Only"}, {"theme_clutter", "Clutter Only"}, {"theme_custom", "Custom Art"} });
    createSection("gen", "GENERATION", { {"theme_wfc", "Procedural WFC"}, {"toggle_terrain", "Toggle Terrain"} });
    createSection("fx", "EFFECTS", { {"toggle_light", "Toggle Lighting"} });

    updateLayout();
}

void RightProperties::update(float dt, bool focusMode, bool isOpen) {
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

    if (state == RightPanelState::Pinned) {
        pinLabel.setString("Unpin Panel");
        pinLabel.setFillColor(WisdomUI::Theme::Gold);
        pinBtn.setOutlineColor(WisdomUI::Theme::BorderHighlight);
    }
    else {
        pinLabel.setString("Pin Panel");
        pinLabel.setFillColor(WisdomUI::Theme::TextSecondary);
        pinBtn.setOutlineColor(WisdomUI::Theme::Border);
    }

    updateLayout();
}

void RightProperties::updateLayout() {
    float startY = 36.f + 32.f + 68.f;
    for (auto& sec : sections) {
        sec.headerRect.setPosition(currentX + 12.f, startY);
        sec.headerLabel.setPosition(currentX + 20.f, startY + 5.f);
        startY += 30.f;

        if (sec.isOpen) {
            for (auto& item : sec.items) {
                item.rect.setPosition(currentX + 16.f, startY);
                item.label.setPosition(currentX + 24.f, startY + 4.f);
                startY += 28.f;
            }
            startY += 6.f;
        }
    }
}

void RightProperties::updateHover(sf::Vector2f mousePos, bool canOpen) {
    bool inPanel = background.getGlobalBounds().contains(mousePos);
    if (state == RightPanelState::Hidden) {
        if (canOpen && inPanel) state = RightPanelState::Visible;
    }
    else if (state == RightPanelState::Visible) {
        if (!inPanel) state = RightPanelState::Hidden;
    }
}

void RightProperties::draw(sf::RenderWindow& window) {
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

    for (auto& sec : sections) {
        window.draw(sec.headerRect);

        sf::Text arrow(sec.isOpen ? "v" : ">", font, 11);
        arrow.setFillColor(WisdomUI::Theme::Gold);
        arrow.setPosition(currentX + width - 26.f, sec.headerRect.getPosition().y + 5.f);
        window.draw(arrow);
        window.draw(sec.headerLabel);

        if (sec.isOpen) {
            for (auto& item : sec.items) {
                if (item.isActive) {
                    item.rect.setFillColor(WisdomUI::Theme::Accent);
                    item.rect.setOutlineColor(WisdomUI::Theme::BorderHighlight);
                    item.label.setFillColor(sf::Color::White);
                }
                else {
                    bool hov = item.rect.getGlobalBounds().contains(window.mapPixelToCoords(sf::Mouse::getPosition(window)));
                    item.rect.setFillColor(hov ? WisdomUI::Theme::PanelHover : WisdomUI::Theme::PanelInset);
                    item.rect.setOutlineColor(hov ? WisdomUI::Theme::BorderHighlight : WisdomUI::Theme::Border);
                    item.label.setFillColor(hov ? WisdomUI::Theme::Gold : WisdomUI::Theme::TextPrimary);
                }
                window.draw(item.rect);
                window.draw(item.label);
            }
        }
    }
}

std::string RightProperties::handleClick(sf::Vector2f mousePos) {
    if (closeBtn.getGlobalBounds().contains(mousePos)) {
        forceClose();
        return "prop_close";
    }

    if (pinBtn.getGlobalBounds().contains(mousePos)) {
        state = (state == RightPanelState::Pinned) ? RightPanelState::Visible : RightPanelState::Pinned;
        pinned = (state == RightPanelState::Pinned);
        return "pin_toggle";
    }

    for (auto& sec : sections) {
        if (sec.headerRect.getGlobalBounds().contains(mousePos)) {
            sec.isOpen = !sec.isOpen;
            updateLayout();
            return "section_toggle";
        }
        if (sec.isOpen) {
            for (auto& item : sec.items) {
                if (item.rect.getGlobalBounds().contains(mousePos)) {
                    if (item.id == "fps_display") return "";
                    return item.id;
                }
            }
        }
    }
    return "";
}

bool RightProperties::handleEvent(const sf::Event& event, sf::Vector2f mousePos, Canvas& canvas, int currentFrame) {
    return false;
}

void RightProperties::syncState(const std::string& theme, bool lighting, bool terrain, bool onion, float onionOpacity, float currentFps) {
    for (auto& sec : sections) {
        for (auto& item : sec.items) {
            item.isActive = false;
            if (item.id == "theme_all" && theme == "all") item.isActive = true;
            if (item.id == "theme_struct" && theme == "structure") item.isActive = true;
            if (item.id == "theme_clutter" && theme == "clutter") item.isActive = true;
            if (item.id == "theme_custom" && theme == "custom") item.isActive = true;
            if (item.id == "theme_wfc" && theme == "wfc") item.isActive = true;
            if (item.id == "toggle_light" && lighting) item.isActive = true;
            if (item.id == "toggle_terrain" && terrain) item.isActive = true;
            if (item.id == "onion_toggle" && onion) item.isActive = true;

            if (item.id == "onion_toggle") {
                std::stringstream ss;
                ss << "Reference (" << static_cast<int>((onionOpacity / 255.f) * 100) << "%)";
                item.label.setString(ss.str());
            }

            if (item.id == "fps_display") {
                std::stringstream ss;
                ss << "Speed: " << static_cast<int>(currentFps) << " FPS";
                item.label.setString(ss.str());
            }
        }
    }
}

float RightProperties::getCurrentX() const { return currentX; }
void RightProperties::forceClose() { targetX = 1920.f; }
bool RightProperties::isHovered() const { return state == RightPanelState::Visible; }
bool RightProperties::isPanelPinned() const { return state == RightPanelState::Pinned; }
sf::FloatRect RightProperties::getHandleBounds() const { return sf::FloatRect(0, 0, 0, 0); }