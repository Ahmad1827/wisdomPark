#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <functional>

namespace WisdomUI {

    struct MenuAction {
        std::string label;
        std::function<void()> callback;
    };

    struct MenuCategory {
        std::string title;
        sf::FloatRect bounds;
        std::vector<MenuAction> actions;
        bool isOpen{ false };
    };

    struct QuickBtn {
        std::string id;
        std::string tooltip;
        sf::FloatRect bounds;
        std::function<void()> onClick;
        bool isHovered{ false };
    };

    class TopBar {
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
        bool HandleEvent(const sf::Event& event, const sf::RenderWindow& window);
        void Update(float deltaTime, const sf::Vector2f& mousePos);
        void Render(sf::RenderWindow& window);

    private:
        sf::FloatRect m_bounds;
        sf::Font m_font;
        std::string m_projectName{ "Untitled_Project" };
        bool m_isDirty{ false };

        std::vector<MenuCategory> m_menus;
        std::vector<QuickBtn> m_quickBtns;
        int m_openMenuIndex{ -1 };
    };

}