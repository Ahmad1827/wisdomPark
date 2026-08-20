#include "TopBar.h"
#include "../UITheme.h"
#include "../UIIcons.h"
#include <algorithm>
#include <cmath>

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
            {"New Project (Ctrl+N)", onNew, 0.0f, {}},
            {"Open Project... (Ctrl+O)", onOpen, 0.0f, {}},
            {"Save Project (Ctrl+S)", onSave, 0.0f, {}},
            {"Export PNG / Sheet...", onExport, 0.0f, {}},
            {"Exit to Menu", onExit, 0.0f, {}}
        }, 0.0f, false, 240.0f });

        m_menus.push_back({ "Edit", {}, {
            {"Undo (Ctrl+Z)", onUndo, 0.0f, {}},
            {"Redo (Ctrl+Y)", onRedo, 0.0f, {}}
        }, 0.0f, false, 200.0f });

        m_menus.push_back({ "View", {}, {
            {"Toggle Fullscreen (F11)", onToggleFullscreen, 0.0f, {}}
        }, 0.0f, false, 220.0f });

        m_quickBtns.clear();
        m_quickBtns.push_back({ "undo", "Undo (Ctrl+Z)", {}, onUndo, 0.0f, 1.0f });
        m_quickBtns.push_back({ "redo", "Redo (Ctrl+Y)", {}, onRedo, 0.0f, 1.0f });
        m_quickBtns.push_back({ "save", "Save Project (Ctrl+S)", {}, onSave, 0.0f, 1.0f });
        m_quickBtns.push_back({ "fullscreen", "Toggle Fullscreen", {}, onToggleFullscreen, 0.0f, 1.0f });
    }

    void TopBar::SetProjectName(const std::string& name, bool isDirty) {
        m_projectName = name;
        m_isDirty = isDirty;
    }

    sf::FloatRect TopBar::getDropdownPanelBounds(int menuIndex) const {
        if (menuIndex < 0 || menuIndex >= static_cast<int>(m_menus.size())) return {};
        const auto& menu = m_menus[menuIndex];
        float itemH = 30.0f;
        float totalH = menu.actions.size() * itemH + 12.0f;
        float currentH = totalH * menu.openProgress;
        return sf::FloatRect(std::floor(menu.bounds.left), std::floor(m_bounds.top + m_bounds.height + 3.0f), menu.dropdownWidth, currentH);
    }

    sf::FloatRect TopBar::getDropdownItemBounds(int menuIndex, int actionIndex) const {
        if (menuIndex < 0 || menuIndex >= static_cast<int>(m_menus.size())) return {};
        const auto& menu = m_menus[menuIndex];
        if (actionIndex < 0 || actionIndex >= static_cast<int>(menu.actions.size())) return {};

        float dropX = std::floor(menu.bounds.left);
        float dropY = std::floor(m_bounds.top + m_bounds.height + 3.0f);
        float itemH = 30.0f;
        float itemY = dropY + 6.0f + actionIndex * itemH;
        float itemW = menu.dropdownWidth - 12.0f;

        return sf::FloatRect(dropX + 6.0f, itemY, itemW, itemH);
    }

    void TopBar::SetBounds(const sf::FloatRect& bounds) {
        m_bounds = bounds;

        float logoRight = bounds.left + 140.0f;

        sf::Text projMeasure(m_projectName + (m_isDirty ? " *" : ""), m_font, 12);
        float projBadgeW = std::clamp(projMeasure.getLocalBounds().width + 24.0f, 90.0f, 220.0f);
        float menuStartX = logoRight + projBadgeW + 20.0f;

        for (auto& menu : m_menus) {
            sf::Text t(menu.title, m_font, 12);
            float w = t.getLocalBounds().width + 24.0f;
            menu.bounds = sf::FloatRect(std::floor(menuStartX), std::floor(bounds.top + 5.0f), std::floor(w), 26.0f);
            menuStartX += w + 8.0f;
        }

        float btnSize = 26.0f;
        float totalQuickW = m_quickBtns.size() * (btnSize + 6.0f) - 6.0f;
        float qX = bounds.left + bounds.width - totalQuickW - 14.0f;

        for (auto& qb : m_quickBtns) {
            qb.bounds = sf::FloatRect(std::floor(qX), std::floor(bounds.top + 5.0f), btnSize, btnSize);
            qX += btnSize + 6.0f;
        }
    }

    void TopBar::Update(float deltaTime, const sf::Vector2f& mousePos) {
        m_globalTime += deltaTime;
        m_shimmerOffset += deltaTime * 120.0f;
        if (m_shimmerOffset > m_bounds.width + 400.0f) {
            m_shimmerOffset = -200.0f;
        }

        for (size_t i = 0; i < m_menus.size(); ++i) {
            auto& menu = m_menus[i];
            bool isTargetOpen = (m_openMenuIndex == static_cast<int>(i));
            float target = isTargetOpen ? 1.0f : 0.0f;
            menu.openProgress += (target - menu.openProgress) * 18.0f * deltaTime;
            menu.openProgress = std::clamp(menu.openProgress, 0.0f, 1.0f);

            if (isTargetOpen && menu.openProgress > 0.1f) {
                for (size_t j = 0; j < menu.actions.size(); ++j) {
                    sf::FloatRect actBounds = getDropdownItemBounds(static_cast<int>(i), static_cast<int>(j));
                    bool actHover = actBounds.contains(mousePos);
                    menu.actions[j].hoverAlpha += ((actHover ? 1.0f : 0.0f) - menu.actions[j].hoverAlpha) * 20.0f * deltaTime;
                }
            }
        }

        for (auto& qb : m_quickBtns) {
            bool hover = qb.bounds.contains(mousePos);
            qb.hoverAlpha += ((hover ? 1.0f : 0.0f) - qb.hoverAlpha) * 14.0f * deltaTime;
            qb.scale += ((hover ? 1.10f : 1.0f) - qb.scale) * 16.0f * deltaTime;
        }
    }

    bool TopBar::HandleEvent(const sf::Event& event, const sf::RenderWindow& window) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f mousePos = window.mapPixelToCoords({ event.mouseButton.x, event.mouseButton.y });

            if (m_openMenuIndex != -1) {
                auto& openMenu = m_menus[m_openMenuIndex];
                for (size_t j = 0; j < openMenu.actions.size(); ++j) {
                    sf::FloatRect itemBounds = getDropdownItemBounds(m_openMenuIndex, static_cast<int>(j));
                    if (itemBounds.contains(mousePos)) {
                        auto cb = openMenu.actions[j].callback;
                        m_openMenuIndex = -1;
                        if (cb) cb();
                        return true;
                    }
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
                    auto cb = qb.onClick;
                    m_openMenuIndex = -1;
                    if (cb) cb();
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
        Theme::DrawSunsetPanel(window, m_bounds, 1.0f);

        sf::RectangleShape crest(sf::Vector2f(10.0f, 10.0f));
        crest.setPosition(m_bounds.left + 14.0f, m_bounds.top + 13.0f);
        crest.setRotation(45.0f);
        crest.setFillColor(Theme::SunsetAmber);
        crest.setOutlineThickness(1.0f);
        crest.setOutlineColor(Theme::SunsetCoralDark);
        window.draw(crest);

        Theme::DrawCrispText(window, m_font, "WISDOM PARK", 13, m_bounds.left + 32.0f, m_bounds.top + 10.0f, Theme::SunsetAmber, sf::Color(14, 6, 20));

        float logoRight = m_bounds.left + 140.0f;
        sf::Text projMeasure(m_projectName + (m_isDirty ? " *" : ""), m_font, 12);
        float projBadgeW = std::clamp(projMeasure.getLocalBounds().width + 24.0f, 90.0f, 220.0f);
        sf::FloatRect badgeBounds(logoRight, m_bounds.top + 5.0f, projBadgeW, 26.0f);

        sf::RectangleShape badge(sf::Vector2f(badgeBounds.width, badgeBounds.height));
        badge.setPosition(badgeBounds.left, badgeBounds.top);
        badge.setFillColor(Theme::SunsetDeepDark);
        badge.setOutlineThickness(1.0f);
        badge.setOutlineColor(m_isDirty ? Theme::SunsetCoral : Theme::SunsetPlum);
        window.draw(badge);

        Theme::DrawCrispText(window, m_font, m_projectName + (m_isDirty ? " *" : ""), 11, badgeBounds.left + badgeBounds.width / 2.0f, badgeBounds.top + badgeBounds.height / 2.0f, m_isDirty ? Theme::SunsetGold : Theme::TextSecondary, sf::Color::Transparent, true, true);

        sf::Vector2i mPosI = sf::Mouse::getPosition(window);
        sf::Vector2f mPos = window.mapPixelToCoords(mPosI);

        for (size_t i = 0; i < m_menus.size(); ++i) {
            const auto& menu = m_menus[i];
            bool isOpen = (m_openMenuIndex == static_cast<int>(i));
            bool hovered = menu.bounds.contains(mPos);

            Theme::DrawSunsetButton(window, menu.bounds, menu.title, m_font, 12, isOpen, hovered, false, 1.0f);
        }

        for (const auto& qb : m_quickBtns) {
            Theme::DrawSunsetButton(window, qb.bounds, "", m_font, 11, false, qb.hoverAlpha > 0.5f, false, qb.scale);

            sf::Vector2f iconPos(qb.bounds.left + 4.0f, qb.bounds.top + 4.0f);
            Icons::Draw(window, qb.id, iconPos, 18.0f, qb.hoverAlpha > 0.5f ? Theme::SunsetAmber : Theme::TextSecondary);
        }

        for (size_t i = 0; i < m_menus.size(); ++i) {
            const auto& menu = m_menus[i];
            if (menu.openProgress > 0.02f) {
                sf::FloatRect dropPanel = getDropdownPanelBounds(static_cast<int>(i));
                Theme::DrawSunsetPanel(window, dropPanel, menu.openProgress);

                if (menu.openProgress > 0.35f) {
                    for (size_t j = 0; j < menu.actions.size(); ++j) {
                        const auto& act = menu.actions[j];
                        sf::FloatRect itemRect = getDropdownItemBounds(static_cast<int>(i), static_cast<int>(j));

                        if (act.hoverAlpha > 0.01f) {
                            sf::RectangleShape hovBg(sf::Vector2f(itemRect.width, itemRect.height));
                            hovBg.setPosition(itemRect.left, itemRect.top);
                            sf::Color hCol = Theme::SunsetPlum;
                            hCol.a = static_cast<sf::Uint8>(240 * act.hoverAlpha * menu.openProgress);
                            hovBg.setFillColor(hCol);
                            hovBg.setOutlineThickness(1.0f);
                            sf::Color hBorder = Theme::SunsetCoral;
                            hBorder.a = static_cast<sf::Uint8>(255 * act.hoverAlpha * menu.openProgress);
                            hovBg.setOutlineColor(hBorder);
                            window.draw(hovBg);
                        }

                        sf::Color txtColor = (act.hoverAlpha > 0.5f) ? Theme::SunsetGold : Theme::TextPrimary;
                        Theme::DrawCrispText(window, m_font, act.label, 12, itemRect.left + 10.0f, itemRect.top + 6.0f, txtColor, sf::Color(14, 6, 20));
                    }
                }
            }
        }
    }

}