#include "TopBar.h"
#include "../UITheme.h"
#include "../UIIcons.h"

namespace WisdomUI {

    TopBar::TopBar() = default;

    void TopBar::Initialize(const sf::Font& font,
        std::function<void()> onNew,
        std::function<void()> onOpen,
        std::function<void()> onSave,
        std::function<void()> onExport,
        std::function<void()> onUndo,
        std::function<void()> onRedo,
        std::function<void()> onToggleFullscreen,
        std::function<void()> onExit) {
        m_font = font;

        m_menus.clear();
        m_menus.push_back({ "File", {}, {
            {"New Project", onNew},
            {"Open Project...", onOpen},
            {"Save Project", onSave},
            {"Export PNG / Sheet...", onExport},
            {"Exit to Menu", onExit}
        } });
        m_menus.push_back({ "Edit", {}, {
            {"Undo", onUndo},
            {"Redo", onRedo}
        } });
        m_menus.push_back({ "View", {}, {
            {"Toggle Fullscreen", onToggleFullscreen}
        } });

        m_quickBtns.clear();
        m_quickBtns.push_back({ "undo", "Undo (Ctrl+Z)", {}, onUndo });
        m_quickBtns.push_back({ "redo", "Redo (Ctrl+Y)", {}, onRedo });
        m_quickBtns.push_back({ "save", "Save Project (Ctrl+S)", {}, onSave });
        m_quickBtns.push_back({ "fullscreen", "Toggle Fullscreen", {}, onToggleFullscreen });
    }

    void TopBar::SetProjectName(const std::string& name, bool isDirty) {
        m_projectName = name;
        m_isDirty = isDirty;
    }

    void TopBar::SetBounds(const sf::FloatRect& bounds) {
        m_bounds = bounds;

        float menuX = 180.0f;
        for (auto& menu : m_menus) {
            sf::Text t(menu.title, m_font, 13);
            float w = t.getLocalBounds().width + 16.0f;
            menu.bounds = sf::FloatRect(menuX, bounds.top, w, bounds.height);
            menuX += w + 2.0f;
        }

        float qX = bounds.left + bounds.width - 140.0f;
        float btnSize = bounds.height - 8.0f;
        for (auto& qb : m_quickBtns) {
            qb.bounds = sf::FloatRect(qX, bounds.top + 4.0f, btnSize, btnSize);
            qX += btnSize + 4.0f;
        }
    }

    void TopBar::Update(float deltaTime, const sf::Vector2f& mousePos) {
        for (auto& qb : m_quickBtns) {
            qb.isHovered = qb.bounds.contains(mousePos);
        }
    }

    bool TopBar::HandleEvent(const sf::Event& event, const sf::RenderWindow& window) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f mousePos = window.mapPixelToCoords({ event.mouseButton.x, event.mouseButton.y });

            if (m_openMenuIndex != -1) {
                auto& openMenu = m_menus[m_openMenuIndex];
                float itemY = m_bounds.top + m_bounds.height;
                float itemW = 180.0f;
                float itemH = 26.0f;

                for (const auto& act : openMenu.actions) {
                    sf::FloatRect itemBounds(openMenu.bounds.left, itemY, itemW, itemH);
                    if (itemBounds.contains(mousePos)) {
                        if (act.callback) act.callback();
                        m_openMenuIndex = -1;
                        return true;
                    }
                    itemY += itemH;
                }
            }

            for (size_t i = 0; i < m_menus.size(); ++i) {
                if (m_menus[i].bounds.contains(mousePos)) {
                    m_openMenuIndex = (m_openMenuIndex == static_cast<int>(i)) ? -1 : static_cast<int>(i);
                    return true;
                }
            }

            for (auto& qb : m_quickBtns) {
                if (qb.bounds.contains(mousePos)) {
                    if (qb.onClick) qb.onClick();
                    m_openMenuIndex = -1;
                    return true;
                }
            }

            if (m_bounds.contains(mousePos)) {
                m_openMenuIndex = -1;
                return true;
            }

            m_openMenuIndex = -1;
        }
        return false;
    }

    void TopBar::Render(sf::RenderWindow& window) {
        sf::RectangleShape bg(sf::Vector2f(m_bounds.width, m_bounds.height));
        bg.setPosition(m_bounds.left, m_bounds.top);
        bg.setFillColor(Theme::Background);
        window.draw(bg);

        sf::RectangleShape border(sf::Vector2f(m_bounds.width, Theme::BorderThickness));
        border.setPosition(m_bounds.left, m_bounds.top + m_bounds.height - Theme::BorderThickness);
        border.setFillColor(Theme::Border);
        window.draw(border);

        sf::Text logo("WISDOM PARK", m_font, 13);
        logo.setFillColor(sf::Color(255, 200, 90));
        logo.setPosition(m_bounds.left + 12.0f, m_bounds.top + 9.0f);
        window.draw(logo);

        std::string projTitle = "|  " + m_projectName + (m_isDirty ? " *" : "");
        sf::Text projText(projTitle, m_font, 12);
        projText.setFillColor(Theme::TextSecondary);
        projText.setPosition(m_bounds.left + 105.0f, m_bounds.top + 10.0f);
        window.draw(projText);

        for (size_t i = 0; i < m_menus.size(); ++i) {
            const auto& menu = m_menus[i];
            bool isOpen = (m_openMenuIndex == static_cast<int>(i));

            if (isOpen) {
                sf::RectangleShape mBg(sf::Vector2f(menu.bounds.width, menu.bounds.height));
                mBg.setPosition(menu.bounds.left, menu.bounds.top);
                mBg.setFillColor(Theme::Panel);
                window.draw(mBg);
            }

            sf::Text mText(menu.title, m_font, 12);
            mText.setFillColor(isOpen ? sf::Color::White : Theme::TextPrimary);
            sf::FloatRect tb = mText.getLocalBounds();
            mText.setPosition(menu.bounds.left + (menu.bounds.width - tb.width) / 2.0f, menu.bounds.top + 9.0f);
            window.draw(mText);
        }

        for (const auto& qb : m_quickBtns) {
            sf::RectangleShape qBg(sf::Vector2f(qb.bounds.width, qb.bounds.height));
            qBg.setPosition(qb.bounds.left, qb.bounds.top);
            qBg.setFillColor(qb.isHovered ? Theme::AccentHover : Theme::Panel);
            qBg.setOutlineThickness(1.0f);
            qBg.setOutlineColor(Theme::Border);
            window.draw(qBg);

            Icons::Draw(window, qb.id, sf::Vector2f(qb.bounds.left + 5.0f, qb.bounds.top + 5.0f), 18.0f, Theme::TextPrimary);
        }

        if (m_openMenuIndex != -1) {
            const auto& openMenu = m_menus[m_openMenuIndex];
            float itemY = m_bounds.top + m_bounds.height;
            float itemW = 180.0f;
            float itemH = 26.0f;

            sf::RectangleShape dropBg(sf::Vector2f(itemW, openMenu.actions.size() * itemH + 4.0f));
            dropBg.setPosition(openMenu.bounds.left, itemY);
            dropBg.setFillColor(Theme::Panel);
            dropBg.setOutlineThickness(1.0f);
            dropBg.setOutlineColor(Theme::Border);
            window.draw(dropBg);

            for (const auto& act : openMenu.actions) {
                sf::Text actText(act.label, m_font, 12);
                actText.setFillColor(Theme::TextPrimary);
                actText.setPosition(openMenu.bounds.left + 12.0f, itemY + 5.0f);
                window.draw(actText);
                itemY += itemH;
            }
        }
    }

}