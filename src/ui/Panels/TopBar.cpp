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
            {"New Project (Ctrl+N)", onNew, 0.0f},
            {"Open Project... (Ctrl+O)", onOpen, 0.0f},
            {"Save Project (Ctrl+S)", onSave, 0.0f},
            {"Export PNG / Sheet...", onExport, 0.0f},
            {"Exit to Menu", onExit, 0.0f}
        }, 0.0f, false });

        m_menus.push_back({ "Edit", {}, {
            {"Undo (Ctrl+Z)", onUndo, 0.0f},
            {"Redo (Ctrl+Y)", onRedo, 0.0f}
        }, 0.0f, false });

        m_menus.push_back({ "View", {}, {
            {"Toggle Fullscreen (F11)", onToggleFullscreen, 0.0f}
        }, 0.0f, false });

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

    void TopBar::SetBounds(const sf::FloatRect& bounds) {
        m_bounds = bounds;

        float menuX = 210.0f;
        for (auto& menu : m_menus) {
            sf::Text t(menu.title, m_font, 13);
            float w = t.getLocalBounds().width + 22.0f;
            menu.bounds = sf::FloatRect(menuX, bounds.top, w, bounds.height);
            menuX += w + 4.0f;
        }

        float qX = bounds.left + bounds.width - 150.0f;
        float btnSize = bounds.height - 8.0f;
        for (auto& qb : m_quickBtns) {
            qb.bounds = sf::FloatRect(qX, bounds.top + 4.0f, btnSize, btnSize);
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
            menu.openProgress += (target - menu.openProgress) * 16.0f * deltaTime;
            menu.openProgress = std::clamp(menu.openProgress, 0.0f, 1.0f);

            if (isTargetOpen) {
                float itemY = m_bounds.top + m_bounds.height + 4.0f;
                float itemW = 200.0f;
                float itemH = 26.0f;
                for (auto& act : menu.actions) {
                    sf::FloatRect actBounds(menu.bounds.left, itemY, itemW, itemH);
                    bool actHover = actBounds.contains(mousePos);
                    act.hoverAlpha += ((actHover ? 1.0f : 0.0f) - act.hoverAlpha) * 20.0f * deltaTime;
                    itemY += itemH;
                }
            }
        }

        for (auto& qb : m_quickBtns) {
            bool hover = qb.bounds.contains(mousePos);
            qb.hoverAlpha += ((hover ? 1.0f : 0.0f) - qb.hoverAlpha) * 14.0f * deltaTime;
            qb.scale += ((hover ? 1.12f : 1.0f) - qb.scale) * 16.0f * deltaTime;
        }
    }

    bool TopBar::HandleEvent(const sf::Event& event, const sf::RenderWindow& window) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2f mousePos = window.mapPixelToCoords({ event.mouseButton.x, event.mouseButton.y });

            if (m_openMenuIndex != -1) {
                auto& openMenu = m_menus[m_openMenuIndex];
                float itemY = m_bounds.top + m_bounds.height + 4.0f;
                float itemW = 200.0f;
                float itemH = 26.0f;

                for (auto& act : openMenu.actions) {
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
        bg.setFillColor(Theme::WoodDark);
        window.draw(bg);

        sf::RectangleShape woodBevel(sf::Vector2f(m_bounds.width, 2.0f));
        woodBevel.setPosition(m_bounds.left, m_bounds.top);
        woodBevel.setFillColor(Theme::WoodLight);
        window.draw(woodBevel);

        sf::RectangleShape brassLine(sf::Vector2f(m_bounds.width, 2.0f));
        brassLine.setPosition(m_bounds.left, m_bounds.top + m_bounds.height - 2.0f);
        brassLine.setFillColor(Theme::Brass);
        window.draw(brassLine);

        float glintX = m_bounds.left + m_shimmerOffset;
        if (glintX >= m_bounds.left && glintX <= m_bounds.left + m_bounds.width) {
            sf::RectangleShape glint(sf::Vector2f(45.0f, 2.0f));
            glint.setPosition(glintX, m_bounds.top + m_bounds.height - 2.0f);
            glint.setFillColor(Theme::GoldHighlight);
            window.draw(glint);
        }

        sf::RectangleShape crest(sf::Vector2f(12.0f, 12.0f));
        crest.setPosition(m_bounds.left + 10.0f, m_bounds.top + 12.0f);
        crest.setRotation(45.0f);
        crest.setFillColor(Theme::Gold);
        crest.setOutlineThickness(1.0f);
        crest.setOutlineColor(Theme::BrassDark);
        window.draw(crest);

        sf::Text logo("WISDOM PARK", m_font, 13);
        logo.setFillColor(Theme::Gold);
        logo.setPosition(m_bounds.left + 30.0f, m_bounds.top + 9.0f);
        window.draw(logo);

        std::string projTitle = "|   " + m_projectName + (m_isDirty ? " *" : "");
        sf::Text projText(projTitle, m_font, 12);
        projText.setFillColor(m_isDirty ? Theme::GoldHighlight : Theme::TextSecondary);
        projText.setPosition(m_bounds.left + 140.0f, m_bounds.top + 10.0f);
        window.draw(projText);

        for (size_t i = 0; i < m_menus.size(); ++i) {
            const auto& menu = m_menus[i];
            bool isOpen = (m_openMenuIndex == static_cast<int>(i));

            if (isOpen || menu.openProgress > 0.01f) {
                sf::RectangleShape mBg(sf::Vector2f(menu.bounds.width, menu.bounds.height));
                mBg.setPosition(menu.bounds.left, menu.bounds.top);
                sf::Color mCol = Theme::WoodMedium;
                mCol.a = static_cast<sf::Uint8>(255 * menu.openProgress);
                mBg.setFillColor(mCol);
                mBg.setOutlineThickness(1.0f);
                sf::Color bCol = Theme::Gold;
                bCol.a = static_cast<sf::Uint8>(255 * menu.openProgress);
                mBg.setOutlineColor(bCol);
                window.draw(mBg);
            }

            sf::Text mText(menu.title, m_font, 12);
            mText.setFillColor(isOpen ? Theme::Gold : Theme::TextPrimary);
            sf::FloatRect tb = mText.getLocalBounds();
            mText.setPosition(menu.bounds.left + (menu.bounds.width - tb.width) / 2.0f, menu.bounds.top + 9.0f);
            window.draw(mText);
        }

        for (const auto& qb : m_quickBtns) {
            sf::RectangleShape qBg(sf::Vector2f(qb.bounds.width, qb.bounds.height));
            qBg.setOrigin(qb.bounds.width / 2.0f, qb.bounds.height / 2.0f);
            qBg.setPosition(qb.bounds.left + qb.bounds.width / 2.0f, qb.bounds.top + qb.bounds.height / 2.0f);
            qBg.setScale(qb.scale, qb.scale);

            sf::Color btnCol = Animation::InterpolateColor(Theme::WoodMedium, Theme::RubyAccent, qb.hoverAlpha);
            qBg.setFillColor(btnCol);
            qBg.setOutlineThickness(1.0f);
            qBg.setOutlineColor(Animation::InterpolateColor(Theme::Brass, Theme::Gold, qb.hoverAlpha));
            window.draw(qBg);

            sf::Vector2f iconPos(qb.bounds.left + 5.0f, qb.bounds.top + 5.0f);
            Icons::Draw(window, qb.id, iconPos, 18.0f, Animation::InterpolateColor(Theme::TextSecondary, Theme::Gold, qb.hoverAlpha));
        }

        for (size_t i = 0; i < m_menus.size(); ++i) {
            const auto& menu = m_menus[i];
            if (menu.openProgress > 0.02f) {
                float itemW = 200.0f;
                float itemH = 26.0f;
                float totalH = menu.actions.size() * itemH + 8.0f;
                float currentH = totalH * menu.openProgress;

                sf::FloatRect dropBounds(menu.bounds.left, m_bounds.top + m_bounds.height + 2.0f, itemW, currentH);
                Theme::DrawParchmentPanel(window, dropBounds, menu.openProgress);

                if (menu.openProgress > 0.4f) {
                    float itemY = dropBounds.top + 6.0f;
                    for (const auto& act : menu.actions) {
                        if (act.hoverAlpha > 0.01f) {
                            sf::RectangleShape hovBg(sf::Vector2f(itemW - 14.0f, itemH));
                            hovBg.setPosition(dropBounds.left + 7.0f, itemY);
                            sf::Color hCol = Theme::ParchmentDark;
                            hCol.a = static_cast<sf::Uint8>(255 * act.hoverAlpha * menu.openProgress);
                            hovBg.setFillColor(hCol);
                            window.draw(hovBg);
                        }

                        sf::Text actText(act.label, m_font, 11);
                        actText.setFillColor(Theme::TextParchment);
                        actText.setPosition(dropBounds.left + 14.0f, itemY + 4.0f);
                        window.draw(actText);
                        itemY += itemH;
                    }
                }
            }
        }
    }

}