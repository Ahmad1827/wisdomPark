#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <functional>

namespace WisdomUI {

    struct TopMenuAction {
        std::string label;
        std::function<void()> callback;
        float hoverAlpha = 0.0f;
        sf::FloatRect bounds;
    };

    struct TopMenu {
        std::string title;
        sf::FloatRect bounds;
        std::vector<TopMenuAction> actions;
        float openProgress = 0.0f;
        bool isHovered = false;
        float dropdownWidth = 230.0f;
    };

    struct TopQuickButton {
        std::string id;
        std::string tooltip;
        sf::FloatRect bounds;
        std::function<void()> onClick;
        float hoverAlpha = 0.0f;
        float scale = 1.0f;
    };

    class TopBar {
    private:
        sf::FloatRect m_bounds;
        sf::Font m_font;
        std::string m_projectName = "Untitled";
        bool m_isDirty = false;

        std::vector<TopMenu> m_menus;
        std::vector<TopQuickButton> m_quickBtns;
        int m_openMenuIndex = -1;

        float m_globalTime = 0.0f;
        float m_shimmerOffset = -200.0f;

        sf::FloatRect getDropdownItemBounds(int menuIndex, int actionIndex) const;
        sf::FloatRect getDropdownPanelBounds(int menuIndex) const;

    public:
        TopBar();

        void Initialize(const sf::Font& font,
            std::function<void()> onNew,
            std::function<void()> onOpen,
            std::function<void()> onSave,
            std::function<void()> onExport,
            std::function<void()> onUndo,
            std::function<void()> onRedo,
            std::function<void()> onToggleFullscreen,
            std::function<void()> onExit);

        void SetProjectName(const std::string& name, bool isDirty);
        void SetBounds(const sf::FloatRect& bounds);
        void Update(float deltaTime, const sf::Vector2f& mousePos);
        bool HandleEvent(const sf::Event& event, const sf::RenderWindow& window);
        void Render(sf::RenderWindow& window);
    };

}