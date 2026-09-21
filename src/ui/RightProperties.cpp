#include "RightProperties.h"
#include "../UI/UITheme.h"
#include "../core/Canvas.h"
#include <sstream>
#include <iomanip>

RightProperties::RightProperties() : width(290.f), currentX(1920.f), targetX(1920.f), state(RightPanelState::Hidden), hovered(false), pinned(false) {}

void RightProperties::init() {
    font.loadFromFile("assets/font.otf");

    background.setFillColor(WisdomUI::Theme::Panel);
    background.setOutlineThickness(1.f);
    background.setOutlineColor(WisdomUI::Theme::Border);

    headerBg.setFillColor(WisdomUI::Theme::PanelInset);
    headerText.setFont(font);
    headerText.setString("PROPERTIES");
    headerText.setCharacterSize(16);
    headerText.setFillColor(WisdomUI::Theme::Gold);

    closeBtn.setSize(sf::Vector2f(28.f, 28.f));
    closeBtn.setFillColor(WisdomUI::Theme::PanelInset);
    closeBtn.setOutlineThickness(1.f);
    closeBtn.setOutlineColor(WisdomUI::Theme::Border);
    closeText.setFont(font);
    closeText.setString("X");
    closeText.setCharacterSize(14);
    closeText.setFillColor(WisdomUI::Theme::TextSecondary);

    pinBtn.setSize(sf::Vector2f(width - 24.f, 28.f));
    pinBtn.setFillColor(WisdomUI::Theme::PanelInset);
    pinBtn.setOutlineThickness(1.f);
    pinBtn.setOutlineColor(WisdomUI::Theme::Border);

    pinLabel.setFont(font);
    pinLabel.setString("Pin Panel");
    pinLabel.setCharacterSize(13);
    pinLabel.setFillColor(WisdomUI::Theme::TextSecondary);

    auto createSection = [&](std::string id, std::string title, std::vector<std::pair<std::string, std::string>> btns) {
        PropSection sec;
        sec.id = id;
        sec.isOpen = true;

        sec.headerRect.setSize(sf::Vector2f(width - 24.f, 32.f));
        sec.headerRect.setFillColor(WisdomUI::Theme::PanelInset);
        sec.headerRect.setOutlineThickness(1.f);
        sec.headerRect.setOutlineColor(WisdomUI::Theme::Border);

        sec.headerLabel.setFont(font);
        sec.headerLabel.setString(title);
        sec.headerLabel.setCharacterSize(14);
        sec.headerLabel.setFillColor(WisdomUI::Theme::Gold);

        for (const auto& pair : btns) {
            PropItem item;
            item.id = pair.first;
            item.rect.setSize(sf::Vector2f(width - 32.f, 28.f));
            item.rect.setFillColor(WisdomUI::Theme::PanelInset);
            item.rect.setOutlineThickness(1.f);
            item.rect.setOutlineColor(WisdomUI::Theme::Border);

            item.label.setFont(font);
            item.label.setString(pair.second);
            item.label.setCharacterSize(13);
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
    width = 290.f;
    float topBarsH = WisdomUI::Theme::TopBarHeight + WisdomUI::Theme::OptionsBarHeight;
    float statusBarH = WisdomUI::Theme::StatusBarHeight;
    float tabDockW = 52.f;

    if (focusMode || !isOpen) targetX = 1920.f;
    else targetX = 1920.f - tabDockW - width;

    currentX += (targetX - currentX) * 16.0f * dt;

    background.setPosition(currentX, topBarsH);
    background.setSize(sf::Vector2f(width, 1080.f - topBarsH - statusBarH));

    headerBg.setPosition(currentX, topBarsH);
    headerBg.setSize(sf::Vector2f(width, 38.f));
    headerText.setPosition(currentX + 14.f, topBarsH + 8.f);

    closeBtn.setPosition(currentX + width - 36.f, topBarsH + 5.f);
    closeText.setPosition(currentX + width - 27.f, topBarsH + 8.f);

    pinBtn.setPosition(currentX + 12.f, topBarsH + 46.f);
    pinLabel.setPosition(currentX + 24.f, topBarsH + 50.f);

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
    float topBarsH = WisdomUI::Theme::TopBarHeight + WisdomUI::Theme::OptionsBarHeight;
    float startY = topBarsH + 84.f;

    for (auto& sec : sections) {
        sec.headerRect.setPosition(currentX + 12.f, startY);
        sec.headerLabel.setPosition(currentX + 20.f, startY + 6.f);
        startY += 36.f;

        if (sec.isOpen) {
            for (auto& item : sec.items) {
                item.rect.setPosition(currentX + 16.f, startY);
                item.label.setPosition(currentX + 26.f, startY + 5.f);
                startY += 32.f;
            }
            startY += 8.f;
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

void RightProperties::drawTooltip(sf::RenderWindow& window, const std::string& text, sf::Vector2f pos) {
    sf::Text tipText(text, font, 13);
    sf::FloatRect bounds = tipText.getLocalBounds();

    float padX = 10.f;
    float padY = 6.f;
    float w = bounds.width + padX * 2.f;
    float h = bounds.height + padY * 2.f + 4.f;

    float x = pos.x - w - 12.f;
    float y = pos.y + 14.f;

    if (x < 10.f) x = pos.x + 16.f;
    if (y + h > 1070.f) y = pos.y - h - 6.f;

    sf::RectangleShape bg(sf::Vector2f(w, h));
    bg.setPosition(x, y);
    bg.setFillColor(sf::Color(18, 12, 24, 250));
    bg.setOutlineThickness(1.5f);
    bg.setOutlineColor(WisdomUI::Theme::Gold);

    tipText.setPosition(x + padX, y + padY - 2.f);
    tipText.setFillColor(sf::Color::White);

    window.draw(bg);
    window.draw(tipText);
}

void RightProperties::draw(sf::RenderWindow& window) {
    if (currentX >= 1918.f) return;

    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    std::string hoveredTooltip = "";

    WisdomUI::Theme::DrawFiligreePanel(window, background.getGlobalBounds(), 1.0f);

    window.draw(headerBg);
    window.draw(headerText);

    bool hovClose = closeBtn.getGlobalBounds().contains(mousePos);
    if (hovClose) hoveredTooltip = "Close Properties Panel";
    closeBtn.setFillColor(hovClose ? WisdomUI::Theme::PanelHover : WisdomUI::Theme::PanelInset);
    closeBtn.setOutlineColor(hovClose ? WisdomUI::Theme::BorderHighlight : WisdomUI::Theme::Border);
    closeText.setFillColor(hovClose ? sf::Color::White : WisdomUI::Theme::TextSecondary);
    window.draw(closeBtn);
    window.draw(closeText);

    bool hovPin = pinBtn.getGlobalBounds().contains(mousePos);
    if (hovPin) hoveredTooltip = (state == RightPanelState::Pinned) ? "Unpin properties panel" : "Pin properties panel open";
    pinBtn.setFillColor(hovPin ? WisdomUI::Theme::PanelHover : WisdomUI::Theme::PanelInset);
    pinBtn.setOutlineColor(hovPin ? WisdomUI::Theme::BorderHighlight : (state == RightPanelState::Pinned ? WisdomUI::Theme::BorderHighlight : WisdomUI::Theme::Border));
    window.draw(pinBtn);
    window.draw(pinLabel);

    for (auto& sec : sections) {
        bool hovHeader = sec.headerRect.getGlobalBounds().contains(mousePos);
        if (hovHeader) hoveredTooltip = "Click to " + std::string(sec.isOpen ? "collapse " : "expand ") + sec.headerLabel.getString().toAnsiString();
        sec.headerRect.setFillColor(hovHeader ? WisdomUI::Theme::PanelHover : WisdomUI::Theme::PanelInset);
        sec.headerRect.setOutlineColor(hovHeader ? WisdomUI::Theme::BorderHighlight : WisdomUI::Theme::Border);
        window.draw(sec.headerRect);

        sf::Text arrow(sec.isOpen ? "v" : ">", font, 13);
        arrow.setFillColor(WisdomUI::Theme::Gold);
        arrow.setPosition(currentX + width - 30.f, sec.headerRect.getPosition().y + 6.f);
        window.draw(arrow);
        window.draw(sec.headerLabel);

        if (sec.isOpen) {
            for (auto& item : sec.items) {
                bool hov = item.rect.getGlobalBounds().contains(mousePos);

                if (hov) {
                    if (item.id == "fps_up") hoveredTooltip = "Increase animation playback rate by 1 FPS";
                    else if (item.id == "fps_down") hoveredTooltip = "Decrease animation playback rate by 1 FPS";
                    else if (item.id == "fps_display") hoveredTooltip = "Current timeline playback rate";
                    else if (item.id == "onion_toggle") hoveredTooltip = "Toggle adjacent frames visibility overlay";
                    else if (item.id == "onion_op_up") hoveredTooltip = "Increase onion skin overlay opacity";
                    else if (item.id == "onion_op_down") hoveredTooltip = "Decrease onion skin overlay opacity";
                    else if (item.id == "theme_all") hoveredTooltip = "Show all object themes and assets";
                    else if (item.id == "theme_struct") hoveredTooltip = "Filter AI generation to structures only";
                    else if (item.id == "theme_clutter") hoveredTooltip = "Filter AI generation to clutter and props";
                    else if (item.id == "theme_custom") hoveredTooltip = "Filter AI generation to custom artwork";
                    else if (item.id == "theme_wfc") hoveredTooltip = "Toggle procedural Wave Function Collapse synthesis";
                    else if (item.id == "toggle_terrain") hoveredTooltip = "Toggle terrain surface rendering layer";
                    else if (item.id == "toggle_light") hoveredTooltip = "Toggle directional sun lighting & drop shadows";
                }

                if (item.isActive) {
                    item.rect.setFillColor(WisdomUI::Theme::Accent);
                    item.rect.setOutlineColor(WisdomUI::Theme::BorderHighlight);
                    item.label.setFillColor(sf::Color::White);
                }
                else {
                    item.rect.setFillColor(hov ? WisdomUI::Theme::PanelHover : WisdomUI::Theme::PanelInset);
                    item.rect.setOutlineColor(hov ? WisdomUI::Theme::BorderHighlight : WisdomUI::Theme::Border);
                    item.label.setFillColor(hov ? WisdomUI::Theme::Gold : WisdomUI::Theme::TextPrimary);
                }
                window.draw(item.rect);
                window.draw(item.label);
            }
        }
    }

    if (!hoveredTooltip.empty()) {
        drawTooltip(window, hoveredTooltip, mousePos);
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