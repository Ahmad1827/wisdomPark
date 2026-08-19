#include "WorkspaceLayout.h"
#include "UITheme.h"
#include <algorithm>

namespace WisdomUI {

    WorkspaceLayout::LayoutRegions WorkspaceLayout::Update(const sf::Vector2u& windowSize, bool showRightDock, bool showTimeline) {
        float width = static_cast<float>(windowSize.x);
        float height = static_cast<float>(windowSize.y);

        // Top bars span the full width
        m_currentLayout.topBar = sf::FloatRect(0, 0, width, Theme::TopBarHeight);
        m_currentLayout.optionsBar = sf::FloatRect(0, Theme::TopBarHeight, width, Theme::OptionsBarHeight);

        float currentY = Theme::TopBarHeight + Theme::OptionsBarHeight;
        float remainingHeight = height - currentY - Theme::StatusBarHeight;

        // Status bar is pinned to the bottom
        m_currentLayout.statusBar = sf::FloatRect(0, height - Theme::StatusBarHeight, width, Theme::StatusBarHeight);

        // Timeline is pinned above the status bar
        float timelineH = showTimeline ? Theme::TimelineHeight : 0.0f;
        m_currentLayout.timeline = sf::FloatRect(0, height - Theme::StatusBarHeight - timelineH, width, timelineH);

        remainingHeight -= timelineH;

        // Left Tool Dock
        m_currentLayout.toolDock = sf::FloatRect(0, currentY, Theme::ToolDockWidth, remainingHeight);

        // Right Dock
        float rightDockW = showRightDock ? Theme::RightDockWidth : 0.0f;
        m_currentLayout.rightDock = sf::FloatRect(width - rightDockW, currentY, rightDockW, remainingHeight);

        // Canvas gets the exact remaining internal space
        float canvasX = Theme::ToolDockWidth;
        float canvasY = currentY;
        float canvasW = width - Theme::ToolDockWidth - rightDockW;
        float canvasH = remainingHeight;

        m_currentLayout.canvas = sf::FloatRect(canvasX, canvasY, std::max(0.0f, canvasW), std::max(0.0f, canvasH));

        return m_currentLayout;
    }

}