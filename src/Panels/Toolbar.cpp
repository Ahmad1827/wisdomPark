#include "Panels/Toolbar.h"
#include "Theme.h"
#include "StudioEngineFacade.h"
#include <iostream>
#include <cmath>

namespace StudioUI {

void Toolbar::Initialize(const std::string& fontPath,
                         std::function<void()> onOpenImage,
                         std::function<void()> onLoadProject,
                         std::function<void()> onSaveProject,
                         std::function<void()> onExport,
                         std::function<void()> onToggleUI,
                         std::function<void()> onOpenWizard,
                         std::function<void()> onDetect) {
    
    std::vector<std::string> fontCandidates = { fontPath, "Resources/font.ttf", "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf" };
    for (const auto& path : fontCandidates) { if (m_font.loadFromFile(path)) break; }

    m_buttons.clear();
    
    // Core Project and File Operations
    m_buttons.push_back({"open_img", u8"Import", {}, [onOpenImage](StudioCore::StudioEngineFacade&){ onOpenImage(); }});
    m_buttons.push_back({"load_proj", u8"Open", {}, [onLoadProject](StudioCore::StudioEngineFacade&){ onLoadProject(); }});
    m_buttons.push_back({"save_proj", u8"Save", {}, [onSaveProject](StudioCore::StudioEngineFacade&){ onSaveProject(); }});
    m_buttons.push_back({"export", u8"Export", {}, [onExport](StudioCore::StudioEngineFacade&){ onExport(); }});
    
    // Engine State Operations
    m_buttons.push_back({"undo", u8"Undo", {}, [](StudioCore::StudioEngineFacade& engine){ engine.Undo(); }});
    m_buttons.push_back({"redo", u8"Redo", {}, [](StudioCore::StudioEngineFacade& engine){ engine.Redo(); }});
    
    // Sprite Detection and Wizards
    m_buttons.push_back({"detect", u8"Detect", {}, [onDetect](StudioCore::StudioEngineFacade&){ onDetect(); }});
    m_buttons.push_back({"wizard", u8"Wizard", {}, [onOpenWizard](StudioCore::StudioEngineFacade&){ onOpenWizard(); }});
    
    // View Options
    m_buttons.push_back({"toggle_ui", u8"View", {}, [onToggleUI](StudioCore::StudioEngineFacade&){ onToggleUI(); }, true, false});

    LayoutButtons(1280.0f);
}

void Toolbar::LayoutButtons(float windowWidth) {
    m_background.setSize(sf::Vector2f(m_bounds.width, Theme::ToolbarHeight));
    m_background.setFillColor(Theme::PanelBackground);

    m_bottomBorder.setSize({windowWidth, Theme::BorderThickness});
    m_bottomBorder.setFillColor(Theme::BorderColor);
    m_bottomBorder.setPosition(0.0f, Theme::ToolbarHeight - Theme::BorderThickness);

    float startX = 12.0f;
    float buttonHeight = Theme::ToolbarHeight - 8.0f;
    float fixedButtonWidth = 85.0f; // Adjusted width to fit all 9 tools perfectly
    float spacing = 4.0f;

    m_dividers.clear();

    for (size_t i = 0; i < m_buttons.size(); ++i) {
        m_buttons[i].bounds = sf::FloatRect(startX, 4.0f, fixedButtonWidth, buttonHeight);
        startX += fixedButtonWidth + spacing;

        // Group Dividers to visually separate functional groups
        if (m_buttons[i].id == "export" || m_buttons[i].id == "redo" || m_buttons[i].id == "wizard") {
            sf::RectangleShape divider({Theme::BorderThickness, buttonHeight - 4.0f});
            divider.setPosition(startX + 2.0f, 6.0f);
            divider.setFillColor(Theme::BorderColor);
            m_dividers.push_back(divider);
            startX += 8.0f; // Extra space for divider
        }
    }
}

void Toolbar::Update(float deltaTime, sf::Vector2f mousePos) {
    for (size_t i = 0; i < m_buttons.size(); ++i) {
        bool isHovered = m_buttons[i].bounds.contains(mousePos);
        float targetAlpha = isHovered ? 1.0f : 0.0f;

        float speed = 10.0f;
        if (m_buttons[i].hoverAlpha < targetAlpha) {
            m_buttons[i].hoverAlpha = std::min(targetAlpha, m_buttons[i].hoverAlpha + deltaTime * speed);
        } else if (m_buttons[i].hoverAlpha > targetAlpha) {
            m_buttons[i].hoverAlpha = std::max(targetAlpha, m_buttons[i].hoverAlpha - deltaTime * speed);
        }
    }
}

bool Toolbar::HandleEvent(const sf::Event& event, const sf::RenderWindow& window, StudioCore::StudioEngineFacade& engine) {
    sf::Vector2i mousePixel = sf::Mouse::getPosition(window);
    sf::Vector2f mousePos(mousePixel.x, mousePixel.y);

    if (mousePos.y > Theme::ToolbarHeight) {
        m_hoveredIndex = -1;
        return false;
    }

    if (event.type == sf::Event::MouseMoved) {
        m_hoveredIndex = -1;
        for (size_t i = 0; i < m_buttons.size(); ++i) {
            if (m_buttons[i].bounds.contains(mousePos)) {
                m_hoveredIndex = static_cast<int>(i);
                break;
            }
        }
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        for (size_t i = 0; i < m_buttons.size(); ++i) {
            if (m_buttons[i].bounds.contains(mousePos)) {
                if (m_buttons[i].isToggle) {
                    m_buttons[i].isToggled = !m_buttons[i].isToggled;
                }
                if (m_buttons[i].onClick) {
                    m_buttons[i].onClick(engine); // Restored callback directly into Engine
                }
                return true;
            }
        }
    }

    return false;
}

void Toolbar::Render(sf::RenderWindow& window) {
    sf::Vector2u winSize = window.getSize();
    LayoutButtons(static_cast<float>(m_bounds.width));

    sf::Vector2i mousePixel = sf::Mouse::getPosition(window);
    sf::Vector2f mousePos(mousePixel.x, mousePixel.y);
    Update(0.016f, mousePos);

    window.draw(m_background);
    window.draw(m_bottomBorder);

    for (const auto& divider : m_dividers) {
        window.draw(divider);
    }

    for (size_t i = 0; i < m_buttons.size(); ++i) {
        const auto& btn = m_buttons[i];

        sf::RectangleShape bg({btn.bounds.width, btn.bounds.height});
        bg.setPosition(btn.bounds.left, btn.bounds.top);

        if (btn.isToggled) {
            bg.setFillColor(Theme::AccentColor);
        } else {
            sf::Color base = Theme::PanelBackground;
            sf::Color hover = Theme::HoverColor;
            float t = btn.hoverAlpha;

            sf::Color blended(
                static_cast<sf::Uint8>(base.r + (hover.r - base.r) * t),
                static_cast<sf::Uint8>(base.g + (hover.g - base.g) * t),
                static_cast<sf::Uint8>(base.b + (hover.b - base.b) * t)
            );
            bg.setFillColor(blended);
        }

        bg.setOutlineThickness(Theme::BorderThickness);
        bg.setOutlineColor(btn.isToggled ? Theme::AccentHoverColor : Theme::BorderColor);
        window.draw(bg);

        sf::Text labelText;
        labelText.setFont(m_font);
        labelText.setString(btn.label);
        labelText.setCharacterSize(13);
        labelText.setFillColor(btn.isToggled ? sf::Color::White : Theme::TextPrimary);

        sf::FloatRect textBounds = labelText.getLocalBounds();
        float textX = std::floor(btn.bounds.left + (btn.bounds.width - textBounds.width) / 2.0f - textBounds.left);
        float textY = std::floor(btn.bounds.top + (btn.bounds.height - textBounds.height) / 2.0f - textBounds.top - 1.0f);
        labelText.setPosition(textX, textY);

        window.draw(labelText);
    }
}

} // namespace StudioUI