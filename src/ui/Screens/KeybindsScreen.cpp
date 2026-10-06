// Start-menu keybinds screen. Drawing and click handling share the layout helpers below.
#include "../UIManager.h"
#include "../UITheme.h"
#include "MenuLayouts.h"
#include "MenuWidgets.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <string>
#include <vector>

using WisdomUI::Theme;
using WisdomUI::Animation;
using namespace MenuWidgets;

namespace {

    struct KeybindTab {
        std::string name;
        sf::FloatRect bounds;
    };

    const float kRowHeight = 52.0f;
    const float kRowGap = 8.0f;

    sf::FloatRect searchBox() {
        return sf::FloatRect(MenuLayout::kContentLeft, MenuLayout::kContentTop, 320.0f, 44.0f);
    }

    sf::FloatRect resetButton() {
        return sf::FloatRect(MenuLayout::kContentLeft + MenuLayout::kContentWidth - 210.0f, 54.0f, 210.0f, 46.0f);
    }

    sf::FloatRect listArea() {
        const float top = MenuLayout::kContentTop + 62.0f;
        return sf::FloatRect(MenuLayout::kContentLeft - 6.0f, top, MenuLayout::kContentWidth + 12.0f, MenuLayout::kContentBottom - top);
    }

    // Rows run in two columns; visibleIndex counts only the rows passing the current filter.
    sf::FloatRect rowBounds(int visibleIndex, float scroll) {
        const float columnGap = 20.0f;
        const float scrollbarSpace = 18.0f;
        const float width = (MenuLayout::kContentWidth - columnGap - scrollbarSpace) * 0.5f;
        float col = static_cast<float>(visibleIndex % 2);
        float row = static_cast<float>(visibleIndex / 2);
        return sf::FloatRect(MenuLayout::kContentLeft + col * (width + columnGap),
            listArea().top + 6.0f + row * (kRowHeight + kRowGap) - scroll, width, kRowHeight);
    }

    sf::FloatRect keyCap(const sf::FloatRect& row) {
        return sf::FloatRect(row.left + row.width - 232.0f, row.top + 7.0f, 220.0f, 38.0f);
    }

    // "All" followed by every category that actually has actions, in registration order.
    std::vector<KeybindTab> categoryTabs(const KeybindManager& manager, const sf::Font& font) {
        std::vector<std::string> names = { "All" };
        for (const auto& id : manager.getActionOrder()) {
            const std::string& category = manager.getAction(id).category;
            if (std::find(names.begin(), names.end(), category) == names.end()) names.push_back(category);
        }

        std::vector<KeybindTab> tabs;
        sf::FloatRect search = searchBox();
        float x = search.left + search.width + 20.0f;
        for (const auto& name : names) {
            float w = Theme::MeasureText(font, name, 15) + 28.0f;
            tabs.push_back({ name, sf::FloatRect(std::floor(x), search.top, std::floor(w), search.height) });
            x += w + 6.0f;
        }
        return tabs;
    }

    std::string lowered(std::string text) {
        std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return text;
    }

    std::vector<std::string> visibleIds(const KeybindManager& manager, const std::string& category, const std::string& query) {
        std::string q = lowered(query);
        std::vector<std::string> ids;
        for (const auto& id : manager.getActionOrder()) {
            const auto& action = manager.getAction(id);
            if (category != "All" && action.category != category) continue;
            if (!q.empty() && lowered(action.name).find(q) == std::string::npos) continue;
            ids.push_back(id);
        }
        return ids;
    }

}

void UIManager::handleKeybindModalEvent(const sf::Event& event, sf::RenderWindow& window) {
    if (!m_showKeybinds) return;

    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    if (!m_listeningKeyActionId.empty()) {
        if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Escape) {
                m_listeningKeyActionId = "";
                return;
            }
            bool ctrl = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::RControl);
            bool shift = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);
            bool alt = sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt) || sf::Keyboard::isKeyPressed(sf::Keyboard::RAlt);

            if (event.key.code != sf::Keyboard::LControl && event.key.code != sf::Keyboard::RControl &&
                event.key.code != sf::Keyboard::LShift && event.key.code != sf::Keyboard::RShift &&
                event.key.code != sf::Keyboard::LAlt && event.key.code != sf::Keyboard::RAlt) {

                Keybind newKb{ event.key.code, ctrl, shift, alt };
                keybindManager.setKeybind(m_listeningKeyActionId, newKb);
                m_listeningKeyActionId = "";
            }
            return;
        }
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (MenuLayout::BackButton().contains(mousePos)) {
            m_showKeybinds = false;
            m_listeningKeyActionId = "";
            m_isTypingKeybindSearch = false;
            return;
        }
        if (resetButton().contains(mousePos)) {
            keybindManager.restoreDefaults();
            m_listeningKeyActionId = "";
            return;
        }

        m_isTypingKeybindSearch = searchBox().contains(mousePos);
        if (m_isTypingKeybindSearch) return;

        for (const auto& tab : categoryTabs(keybindManager, font)) {
            if (tab.bounds.contains(mousePos)) {
                m_selectedKeybindCategory = tab.name;
                m_keybindScrollOffset = 0.0f;
                m_listeningKeyActionId = "";
                return;
            }
        }

        m_listeningKeyActionId = "";
        if (listArea().contains(mousePos)) {
            std::vector<std::string> ids = visibleIds(keybindManager, m_selectedKeybindCategory, m_keybindSearchQuery);
            for (size_t i = 0; i < ids.size(); ++i) {
                if (keyCap(rowBounds(static_cast<int>(i), m_keybindScrollOffset)).contains(mousePos)) {
                    m_listeningKeyActionId = ids[i];
                    return;
                }
            }
        }
    }

    if (event.type == sf::Event::MouseWheelScrolled) {
        m_keybindScrollOffset = std::clamp(m_keybindScrollOffset - event.mouseWheelScroll.delta * 60.0f, 0.0f, m_keybindMaxScroll);
    }

    if (event.type == sf::Event::TextEntered && m_isTypingKeybindSearch) {
        if (event.text.unicode == '\b') {
            if (!m_keybindSearchQuery.empty()) m_keybindSearchQuery.pop_back();
        }
        else if (event.text.unicode >= 32 && event.text.unicode < 127 && m_keybindSearchQuery.length() < 24) {
            m_keybindSearchQuery += static_cast<char>(event.text.unicode);
        }
        m_keybindScrollOffset = 0.0f;
    }
}

void UIManager::drawKeybindModal(sf::RenderWindow& window) {
    if (!m_showKeybinds) return;

    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    drawSubScreenHeader(window, "KEYBINDS", "CLICK A SHORTCUT TO REBIND IT   |   ESC CANCELS", m_keybindTime);

    sf::FloatRect reset = resetButton();
    Theme::DrawSunsetButton(window, reset, "RESET DEFAULTS", font, 15, false, reset.contains(mousePos), false, 1.0f);

    // ---- Search and category tabs ----
    sf::FloatRect search = searchBox();
    float searchT = Theme::AnimateHover(search, search.contains(mousePos));
    sf::RectangleShape searchBg(sf::Vector2f(search.width, search.height));
    searchBg.setPosition(search.left, search.top);
    searchBg.setFillColor(Theme::WithAlpha(Theme::PanelInset, 235.0f));
    searchBg.setOutlineThickness(m_isTypingKeybindSearch ? 1.5f : 1.0f);
    searchBg.setOutlineColor(m_isTypingKeybindSearch ? Theme::SunsetGold : Theme::Mix(Theme::SunsetPlum, Theme::SunsetPeach, searchT));
    window.draw(searchBg);

    bool searchEmpty = m_keybindSearchQuery.empty();
    std::string searchDisplay = searchEmpty ? (m_isTypingKeybindSearch ? "_" : "Search shortcuts...") : (m_keybindSearchQuery + (m_isTypingKeybindSearch ? "_" : ""));
    Theme::DrawCrispText(window, font, searchDisplay, 17, search.left + 16.0f, search.top + search.height * 0.5f,
        (searchEmpty && !m_isTypingKeybindSearch) ? Theme::TextMuted : Theme::TextPrimary, sf::Color::Transparent, false, true);

    for (const auto& tab : categoryTabs(keybindManager, font)) {
        bool selected = (m_selectedKeybindCategory == tab.name);
        Theme::DrawSunsetButton(window, tab.bounds, tab.name, font, 15, selected, tab.bounds.contains(mousePos), false, 1.0f);
    }

    // ---- Shortcut list ----
    sf::FloatRect list = listArea();
    std::vector<std::string> ids = visibleIds(keybindManager, m_selectedKeybindCategory, m_keybindSearchQuery);

    float rowCount = std::ceil(static_cast<float>(ids.size()) / 2.0f);
    float contentH = rowCount * (kRowHeight + kRowGap) + 12.0f;
    m_keybindMaxScroll = std::max(0.0f, contentH - list.height);
    m_keybindScrollOffset = std::clamp(m_keybindScrollOffset, 0.0f, m_keybindMaxScroll);

    if (ids.empty()) {
        Theme::DrawCrispText(window, font, "No shortcuts match that search.", 20, list.left + list.width * 0.5f, list.top + 120.0f, Theme::TextSecondary, kTextShadow, true, true);
        return;
    }

    bool mouseInList = list.contains(mousePos);
    sf::View savedView = window.getView();
    window.setView(clipView(window, list));

    for (size_t i = 0; i < ids.size(); ++i) {
        sf::FloatRect row = rowBounds(static_cast<int>(i), m_keybindScrollOffset);
        if (row.top + row.height < list.top || row.top > list.top + list.height) continue;

        float a = Animation::EaseOutCubic((m_keybindTime - 0.06f - 0.015f * static_cast<float>(std::min<size_t>(i, 24))) / 0.35f);
        if (a <= 0.0f) continue;

        const auto& action = keybindManager.getAction(ids[i]);
        bool listening = (m_listeningKeyActionId == ids[i]);
        float t = Theme::AnimateHover(row, mouseInList && row.contains(mousePos));
        float lit = std::max(t, listening ? 1.0f : 0.0f);
        float centerY = row.top + row.height * 0.5f;

        sf::RectangleShape bg(sf::Vector2f(row.width, row.height));
        bg.setPosition(row.left, row.top);
        bg.setFillColor(faded(Theme::Mix(sf::Color(22, 14, 36, 232), sf::Color(48, 30, 70, 244), lit), a));
        bg.setOutlineThickness(1.0f);
        bg.setOutlineColor(faded(Theme::Mix(Theme::Border, listening ? Theme::SunsetGold : Theme::SunsetPeach, lit), a));
        window.draw(bg);

        if (listening) {
            sf::RectangleShape bar(sf::Vector2f(4.0f, row.height));
            bar.setPosition(row.left, row.top);
            bar.setFillColor(Theme::SunsetGold);
            window.draw(bar);
        }

        Theme::DrawCrispText(window, font, action.name, 18, row.left + 20.0f, centerY, faded(Theme::Mix(Theme::TextPrimary, Theme::SunsetGold, lit), a), faded(kTextShadow, a), false, true);
        Theme::DrawCrispText(window, font, action.category, 13, row.left + row.width * 0.44f, centerY, faded(Theme::TextMuted, a), sf::Color::Transparent, false, true);

        std::string keyStr = keybindManager.getActionString(ids[i]);
        if (listening) keyStr = "PRESS A KEY...";
        else if (keyStr.empty()) keyStr = "UNBOUND";

        if (a > 0.6f) {
            sf::FloatRect cap = keyCap(row);
            Theme::DrawSunsetButton(window, cap, keyStr, font, 15, listening, mouseInList && cap.contains(mousePos), false, 1.0f);
        }
    }

    window.setView(savedView);

    drawScrollbar(window, sf::FloatRect(MenuLayout::kContentLeft + MenuLayout::kContentWidth - 6.0f, list.top + 6.0f, 6.0f, list.height - 12.0f),
        m_keybindScrollOffset, m_keybindMaxScroll, list.height / contentH);
}
