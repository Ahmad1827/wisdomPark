#pragma once
#include <SFML/Graphics.hpp>

namespace WisdomUI {

    class WorkspaceLayout {
    public:
        struct LayoutRegions {
            sf::FloatRect topBar;
            sf::FloatRect optionsBar;
            sf::FloatRect toolDock;
            sf::FloatRect rightDock;
            sf::FloatRect timeline;
            sf::FloatRect statusBar;
            sf::FloatRect canvas; // The most important area
        };

        WorkspaceLayout() = default;

        LayoutRegions Update(const sf::Vector2u& windowSize, bool showRightDock, bool showTimeline);

    private:
        LayoutRegions m_currentLayout;
    };

}