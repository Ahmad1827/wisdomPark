#include "WorkspaceLayout.h"

namespace WisdomUI {

    sf::View WorkspaceLayout::GetLetterboxView(sf::Vector2u windowSize) {
        sf::View view(sf::FloatRect(0.f, 0.f, 1920.f, 1080.f));
        float windowRatio = static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y);
        float viewRatio = 1920.f / 1080.f;
        float sizeX = 1.0f;
        float sizeY = 1.0f;
        float posX = 0.0f;
        float posY = 0.0f;

        if (windowRatio >= viewRatio) {
            sizeX = viewRatio / windowRatio;
            posX = (1.0f - sizeX) / 2.0f;
        }
        else {
            sizeY = windowRatio / viewRatio;
            posY = (1.0f - sizeY) / 2.0f;
        }

        view.setViewport(sf::FloatRect(posX, posY, sizeX, sizeY));
        return view;
    }

    WorkspaceLayout::LayoutRegions WorkspaceLayout::Update(bool showRightDock, bool showTimeline) {
        const float W = 1920.0f;
        const float H = 1080.0f;

        m_currentLayout.topBar = sf::FloatRect(0, 0, W, Theme::TopBarHeight);
        m_currentLayout.optionsBar = sf::FloatRect(0, Theme::TopBarHeight, W, Theme::OptionsBarHeight);

        float currentY = Theme::TopBarHeight + Theme::OptionsBarHeight;
        float bottomBarY = H - Theme::StatusBarHeight;
        m_currentLayout.statusBar = sf::FloatRect(0, bottomBarY, W, Theme::StatusBarHeight);

        float timelineH = showTimeline ? Theme::TimelineHeight : 0.0f;
        m_currentLayout.timeline = sf::FloatRect(0, bottomBarY - timelineH, W, timelineH);

        float midHeight = bottomBarY - currentY - timelineH;

        m_currentLayout.toolDock = sf::FloatRect(0, currentY, Theme::ToolDockWidth, midHeight);

        const float rightTabsW = 44.0f;
        m_currentLayout.rightDockTabs = sf::FloatRect(W - rightTabsW, currentY, rightTabsW, midHeight);

        float rightDockW = showRightDock ? Theme::RightDockWidth : 0.0f;
        m_currentLayout.rightDock = sf::FloatRect(W - rightTabsW - rightDockW, currentY, rightDockW, midHeight);

        float canvasX = Theme::ToolDockWidth;
        float canvasW = W - Theme::ToolDockWidth - rightTabsW - rightDockW;
        m_currentLayout.canvas = sf::FloatRect(canvasX, currentY, std::max(0.0f, canvasW), std::max(0.0f, midHeight));

        return m_currentLayout;
    }

}