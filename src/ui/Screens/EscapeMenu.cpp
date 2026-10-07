// In-editor pause menu. Drawing and click handling share the layout helpers below.
#include "../UIManager.h"
#include "../UITheme.h"
#include "../../core/NativeDialogs.h"
#include "MenuWidgets.h"
#include <algorithm>
#include <cmath>
#include <string>

using WisdomUI::Theme;
using WisdomUI::Animation;
using namespace MenuWidgets;

namespace {

    enum class ItemStyle { Primary, Normal, Danger };

    struct EscapeItem {
        const char* id;
        const char* label;
        int group;
        ItemStyle style;
    };

    // Groups are separated by a little extra space: resume, project, display, leave.
    const EscapeItem kItems[] = {
        { "resume",     "Resume",             0, ItemStyle::Primary },
        { "resize",     "Resize Canvas",      1, ItemStyle::Normal },
        { "save",       "Save Project",       1, ItemStyle::Normal },
        { "save_as",    "Save Project As...", 1, ItemStyle::Normal },
        { "export",     "Export",             1, ItemStyle::Normal },
        { "fullscreen", "Fullscreen",         2, ItemStyle::Normal },
        { "main_menu",  "Main Menu",          3, ItemStyle::Danger },
        { "exit",       "Exit",               3, ItemStyle::Danger }
    };
    const int kItemCount = static_cast<int>(sizeof(kItems) / sizeof(kItems[0]));

    const float kPanelWidth = 540.0f;
    const float kItemsTop = 132.0f;
    const float kItemHeight = 54.0f;
    const float kItemGap = 10.0f;
    const float kGroupGap = 10.0f;
    const float kPanelPadding = 36.0f;

    float itemOffset(int index) {
        return kItemsTop + static_cast<float>(index) * (kItemHeight + kItemGap) + static_cast<float>(kItems[index].group) * kGroupGap;
    }

    sf::FloatRect panelBounds() {
        float height = itemOffset(kItemCount - 1) + kItemHeight + kPanelPadding;
        return sf::FloatRect(std::floor((1920.0f - kPanelWidth) * 0.5f), std::floor((1080.0f - height) * 0.5f), kPanelWidth, height);
    }

    sf::FloatRect itemBounds(int index) {
        sf::FloatRect panel = panelBounds();
        return sf::FloatRect(panel.left + kPanelPadding, panel.top + itemOffset(index), panel.width - kPanelPadding * 2.0f, kItemHeight);
    }

    // The menu has no open() call, so its entrance is timed from the first frame it is drawn again.
    unsigned int s_lastDrawnFrame = 0;
    float s_openTime = 0.0f;

}

void UIManager::drawEscapeMenu(sf::RenderWindow& window, Canvas& canvas, Timeline& timeline) {
    if (Theme::s_frame - s_lastDrawnFrame > 1) s_openTime = Theme::s_time;
    s_lastDrawnFrame = Theme::s_frame;

    float elapsed = Theme::s_time - s_openTime;
    float appear = Animation::EaseOutCubic(elapsed / 0.22f);

    sf::View savedView = beginModal(window, appear);

    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    bool mouseDown = sf::Mouse::isButtonPressed(sf::Mouse::Left);

    sf::FloatRect panel = panelBounds();
    float cx = panel.left + panel.width * 0.5f;
    Theme::DrawSunsetPanel(window, panel, appear);

    // ---- Header ----
    Theme::DrawCrispText(window, font, "STUDIO PAUSED", 30, cx, panel.top + 44.0f, Theme::SunsetGold, Theme::SunsetCoralDark, true, true);

    std::string name = activeProjectName;
    std::replace(name.begin(), name.end(), '_', ' ');
    if (name.length() > 26) name = name.substr(0, 24) + "..";

    bool dirty = canvas.getIsDirty();
    const std::string separator = "   |   ";
    const std::string status = dirty ? "UNSAVED CHANGES" : "SAVED";
    float nameW = Theme::MeasureText(font, name, 15);
    float separatorW = Theme::MeasureText(font, separator, 15);
    float lineX = std::floor(cx - (nameW + separatorW + Theme::MeasureText(font, status, 15)) * 0.5f);
    const float lineY = panel.top + 80.0f;
    Theme::DrawCrispText(window, font, name, 15, lineX, lineY, Theme::SunsetPeach, kTextShadow, false, true);
    Theme::DrawCrispText(window, font, separator, 15, lineX + nameW, lineY, Theme::SunsetPlum, sf::Color::Transparent, false, true);
    Theme::DrawCrispText(window, font, status, 15, lineX + nameW + separatorW, lineY, dirty ? Theme::SunsetCoral : Theme::TextMuted, kTextShadow, false, true);

    sf::RectangleShape rule(sf::Vector2f(panel.width - kPanelPadding * 2.0f, 1.0f));
    rule.setPosition(panel.left + kPanelPadding, panel.top + 110.0f);
    rule.setFillColor(Theme::SunsetPlum);
    window.draw(rule);

    float accentW = 180.0f * Animation::EaseOutCubic((elapsed - 0.08f) / 0.4f);
    sf::RectangleShape accent(sf::Vector2f(accentW, 2.0f));
    accent.setPosition(std::floor(cx - accentW * 0.5f), panel.top + 109.0f);
    accent.setFillColor(Theme::SunsetAmber);
    window.draw(accent);

    // ---- Items ----
    for (int i = 0; i < kItemCount; ++i) {
        const EscapeItem& item = kItems[i];
        const std::string id = item.id;

        float a = Animation::EaseOutCubic((elapsed - 0.05f - 0.03f * static_cast<float>(i)) / 0.3f);
        if (a <= 0.0f) continue;

        sf::FloatRect bounds = itemBounds(i);
        bool hovered = bounds.contains(mousePos);
        float t = Theme::AnimateHover(bounds, hovered);
        sf::FloatRect b = shifted(bounds, -14.0f * (1.0f - a), (hovered && mouseDown) ? 1.0f : 0.0f);
        float centerY = b.top + b.height * 0.5f;

        // Right-hand hint: the shortcut, or the current value of whatever the item changes.
        std::string hint;
        if (id == "resume") hint = "ESC";
        else if (id == "save") hint = keybindManager.getActionString("proj_save");
        else if (id == "resize") hint = std::to_string(canvas.getCanvasSize().x) + " x " + std::to_string(canvas.getCanvasSize().y);
        else if (id == "fullscreen") hint = uiFullscreen ? "ON" : "OFF";

        sf::Color fill, outline, glowColor, labelColor, hintColor, barColor;
        float glowAlpha = 48.0f * t;
        if (item.style == ItemStyle::Primary) {
            fill = Theme::Mix(Theme::SunsetAmber, Theme::SunsetGold, t);
            outline = Theme::SunsetGlow;
            glowColor = Theme::SunsetGold;
            glowAlpha = 26.0f + 12.0f * std::sin(Theme::s_time * 3.0f) + 46.0f * t;
            labelColor = Theme::SunsetDeepDark;
            hintColor = sf::Color(92, 48, 18);
            barColor = Theme::SunsetDeepDark;
        }
        else if (item.style == ItemStyle::Danger) {
            fill = Theme::Mix(sf::Color(34, 12, 30, 240), Theme::DangerHover, t);
            outline = Theme::Mix(Theme::SunsetCoralDark, Theme::SunsetCoral, t);
            glowColor = Theme::SunsetCoral;
            labelColor = Theme::Mix(Theme::TextPeach, sf::Color::White, t);
            hintColor = Theme::TextMuted;
            barColor = Theme::SunsetCoral;
        }
        else {
            fill = Theme::Mix(sf::Color(22, 14, 36, 240), sf::Color(52, 32, 76, 248), t);
            outline = Theme::Mix(Theme::Border, Theme::SunsetPeach, t);
            glowColor = Theme::SunsetPeach;
            labelColor = Theme::Mix(Theme::TextPrimary, Theme::SunsetGold, t);
            hintColor = Theme::Mix(Theme::TextMuted, Theme::TextSecondary, t);
            barColor = Theme::SunsetAmber;
            if (id == "fullscreen" && uiFullscreen) hintColor = Theme::SunsetGold;
        }

        if (glowAlpha > 1.0f) {
            sf::ConvexShape glow = Theme::ChamferedRect(b.left - 3.0f, b.top - 3.0f, b.width + 6.0f, b.height + 6.0f, 6.0f);
            glow.setFillColor(Theme::WithAlpha(glowColor, glowAlpha * a));
            window.draw(glow);
        }

        sf::ConvexShape body = Theme::ChamferedRect(b.left, b.top, b.width, b.height, 4.0f);
        body.setFillColor(faded(fill, a));
        body.setOutlineThickness(1.0f);
        body.setOutlineColor(faded(outline, a));
        window.draw(body);

        sf::RectangleShape shine(sf::Vector2f(b.width - 8.0f, 1.0f));
        shine.setPosition(b.left + 4.0f, b.top + 1.0f);
        shine.setFillColor(faded(sf::Color(255, 255, 255, static_cast<sf::Uint8>(item.style == ItemStyle::Primary ? 130.0f : 26.0f + 40.0f * t)), a));
        window.draw(shine);

        // A marker grows out of the left edge as the item is hovered.
        if (t > 0.02f) {
            float barH = (b.height - 16.0f) * t;
            sf::RectangleShape bar(sf::Vector2f(3.0f, barH));
            bar.setPosition(b.left + 1.0f, std::floor(centerY - barH * 0.5f));
            bar.setFillColor(faded(barColor, a));
            window.draw(bar);
        }

        sf::Color labelShadow = (item.style == ItemStyle::Primary) ? sf::Color::Transparent : faded(kTextShadow, a);
        Theme::DrawCrispText(window, font, item.label, 19, b.left + 22.0f + 6.0f * t, centerY, faded(labelColor, a), labelShadow, false, true);

        if (!hint.empty()) {
            float hintW = Theme::MeasureText(font, hint, 14);
            Theme::DrawCrispText(window, font, hint, 14, b.left + b.width - 20.0f - hintW, centerY, faded(hintColor, a), sf::Color::Transparent, false, true);
        }
    }

    window.setView(savedView);
}

bool UIManager::handleEscapeMenuEvent(const sf::Event& event, sf::RenderWindow& window, AppState& currentState, AppSettings& settings, Canvas& canvas, Timeline& timeline) {
    if (!m_showEscapeMenu) return false;

    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
        m_showEscapeMenu = false;
        return true;
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2i pixelPos(event.mouseButton.x, event.mouseButton.y);
        sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);

        for (int i = 0; i < kItemCount; ++i) {
            if (!itemBounds(i).contains(mousePos)) continue;

            const std::string action = kItems[i].id;

            if (action == "resume") {
                m_showEscapeMenu = false;
            }
            else if (action == "resize") {
                m_showEscapeMenu = false;
                m_showResizeModal = true;
                m_resizeWBuf = std::to_string(canvas.getCanvasSize().x);
                m_resizeHBuf = std::to_string(canvas.getCanvasSize().y);
                m_activeResizeField = 0;
            }
            else if (action == "save") {
                if (triggerSave(canvas, timeline)) {
                    showMessage("Project Saved Successfully!", sf::Color::Green);
                }
                else {
                    showMessage("Error Saving Project!", sf::Color::Red);
                }
            }
            else if (action == "save_as") {
                std::string file = NativeDialogs::saveFileDialog("Wisdom Park Projects\0*.wpk\0", "wpk", activeProjectName);
                if (!file.empty() && projManager) {
                    activeProjectPath = file;
                    if (projManager->saveProjectAs(activeProjectPath, activeProjectName, canvas, static_cast<int>(timeline.getFps()), canvas.getPixelMode())) {
                        canvas.clearIsDirty();
                        showMessage("Project Saved As Successfully!", sf::Color::Green);
                    }
                }
            }
            else if (action == "export") {
                m_showEscapeMenu = false;
                exportModal.open(canvas, timeline.getCurrentFrame());
            }
            else if (action == "fullscreen") {
                toggleFullscreen(window, settings);
            }
            else if (action == "main_menu") {
                m_showEscapeMenu = false;
                if (canvas.getIsDirty()) {
                    showUnsavedWarning = true;
                }
                else {
                    currentState = AppState::Welcome;
                    currentMenuState = MenuState::Main;
                }
            }
            else if (action == "exit") {
                window.close();
            }
            return true;
        }
    }
    return true;
}
