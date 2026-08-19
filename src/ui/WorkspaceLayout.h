#pragma once
#include <SFML/Graphics.hpp>
#include "UITheme.h"
#include <algorithm>

namespace WisdomUI {

    class WorkspaceLayout {
    public:
        struct LayoutRegions {
            sf::FloatRect topBar;
            sf::FloatRect optionsBar;
            sf::FloatRect toolDock;
            sf::FloatRect rightDockTabs;
            sf::FloatRect rightDock;
            sf::FloatRect timeline;
            sf::FloatRect statusBar;
            sf::FloatRect canvas;
        };

        WorkspaceLayout() = default;

        static sf::View GetLetterboxView(sf::Vector2u windowSize);
        LayoutRegions Update(bool showRightDock, bool showTimeline);

    private:
        LayoutRegions m_currentLayout;
    };

}