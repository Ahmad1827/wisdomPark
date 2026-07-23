#include "Rendering/OverlayRenderer.h"
#include "Theme.h"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <vector>

namespace StudioUI {

OverlayRenderer::OverlayRenderer() = default;

bool OverlayRenderer::InitializeFont(const std::string& customPath) {
    std::vector<std::string> fontPaths = {
        customPath,
        "Resources/font.ttf",
        "../Resources/font.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf"
    };

    for (const auto& path : fontPaths) {
        if (m_font.loadFromFile(path)) {
            m_hasFont = true;
            std::cout << "[Overlay] Successfully loaded font: " << path << std::endl;
            return true;
        }
    }
    
    std::cerr << "[Overlay] FATAL: Could not load any system fonts for UI text rendering!" << std::endl;
    m_hasFont = false;
    return false;
}

void OverlayRenderer::DrawTextShadowed(sf::RenderWindow& window, const std::string& str, const sf::Vector2f& pos) {
    if (!m_hasFont) return;
    sf::Text text(str, m_font, 12);
    
    text.setPosition(pos.x + 1, pos.y + 1);
    text.setFillColor(sf::Color(0, 0, 0, 180));
    window.draw(text);

    text.setPosition(pos);
    text.setFillColor(Theme::TextPrimary);
    window.draw(text);
}

void OverlayRenderer::RenderDebug(sf::RenderWindow& window, const DebugInfo& info) {
    if (!m_hasFont) return;

    float panelWidth = 240.0f;
    float panelHeight = 160.0f;
    float posX = 12.0f;
    float posY = Theme::ToolbarHeight + 12.0f;

    sf::RectangleShape bg({panelWidth, panelHeight});
    bg.setPosition(posX, posY);
    bg.setFillColor(Theme::InspectorBackground);
    bg.setOutlineThickness(Theme::BorderThickness);
    bg.setOutlineColor(Theme::BorderColor);
    window.draw(bg);

    sf::RectangleShape header({panelWidth, Theme::HeaderHeight});
    header.setPosition(posX, posY);
    header.setFillColor(Theme::PanelBackground);
    window.draw(header);

    sf::Text headerTitle;
    headerTitle.setFont(m_font);
    headerTitle.setString("DEBUG METRICS (F3)");
    headerTitle.setCharacterSize(11);
    headerTitle.setFillColor(Theme::TextSecondary);
    headerTitle.setPosition(posX + 8.0f, posY + 6.0f);
    window.draw(headerTitle);

    std::ostringstream oss;
    oss << "FPS: " << static_cast<int>(info.fps) << " (" << std::fixed << std::setprecision(1) << info.frameTimeMs << " ms)\n"
        << "Zoom: " << std::fixed << std::setprecision(2) << info.zoomLevel << "x\n"
        << "Pan: [" << static_cast<int>(info.panOffset.x) << ", " << static_cast<int>(info.panOffset.y) << "]\n"
        << "Canvas: " << info.imageWidth << "x" << info.imageHeight << "\n"
        << "Project: " << (info.projectLoaded ? "Active" : "None") << "\n"
        << "Texture: " << (info.textureLoaded ? "Loaded" : "Empty");

    sf::Text debugText;
    debugText.setFont(m_font);
    debugText.setString(oss.str());
    debugText.setCharacterSize(11);
    debugText.setFillColor(Theme::TextPrimary);
    debugText.setPosition(posX + 8.0f, posY + Theme::HeaderHeight + 6.0f);
    window.draw(debugText);
}

void OverlayRenderer::RenderInspector(sf::RenderWindow& window, const InspectorInfo& info) {
    if (!m_hasFont) return;

    float panelWidth = 220.0f;
    float panelHeight = 110.0f;
    float posX = 12.0f;
    float posY = window.getSize().y - Theme::StatusBarHeight - panelHeight - 12.0f;

    sf::RectangleShape bg({panelWidth, panelHeight});
    bg.setPosition(posX, posY);
    bg.setFillColor(Theme::InspectorBackground);
    bg.setOutlineThickness(Theme::BorderThickness);
    bg.setOutlineColor(Theme::BorderColor);
    window.draw(bg);

    sf::RectangleShape header({panelWidth, Theme::HeaderHeight});
    header.setPosition(posX, posY);
    header.setFillColor(Theme::PanelBackground);
    window.draw(header);

    sf::Text headerTitle;
    headerTitle.setFont(m_font);
    headerTitle.setString("CANVAS INSPECTOR");
    headerTitle.setCharacterSize(11);
    headerTitle.setFillColor(Theme::TextSecondary);
    headerTitle.setPosition(posX + 8.0f, posY + 6.0f);
    window.draw(headerTitle);

    std::ostringstream oss;
    if (info.isHoveringImage) {
        oss << "Coords: [" << info.mouseImage.x << ", " << info.mouseImage.y << "]\n"
            << "RGBA: (" << (int)info.pixelColor.r << ", " << (int)info.pixelColor.g << ", " 
                        << (int)info.pixelColor.b << ", " << (int)info.pixelColor.a << ")\n"
            << "Zoom: " << std::fixed << std::setprecision(2) << info.currentZoom << "x";
    } else {
        oss << "Coords: [Out of bounds]\nRGBA: N/A\nZoom: " 
            << std::fixed << std::setprecision(2) << info.currentZoom << "x";
    }

    sf::Text contentText;
    contentText.setFont(m_font);
    contentText.setString(oss.str());
    contentText.setCharacterSize(11);
    contentText.setFillColor(Theme::TextPrimary);
    contentText.setPosition(posX + 8.0f, posY + Theme::HeaderHeight + 6.0f);
    window.draw(contentText);
}

void OverlayRenderer::RenderProgress(sf::RenderWindow& window, const JobProgressInfo& info) {
    if (!info.isRunning || !m_hasFont) return;
    
    sf::Vector2f center(window.getSize().x / 2.0f, window.getSize().y / 2.0f);
    
    float boxWidth = 340.0f;
    float boxHeight = 80.0f;

    sf::RectangleShape panel({boxWidth, boxHeight});
    panel.setOrigin(boxWidth / 2.0f, boxHeight / 2.0f);
    panel.setPosition(center);
    panel.setFillColor(Theme::InspectorBackground);
    panel.setOutlineThickness(Theme::BorderThickness);
    panel.setOutlineColor(Theme::BorderColor);
    window.draw(panel);

    std::ostringstream oss;
    oss << "Detecting Sprites... " << static_cast<int>(info.progress * 100) << "%";
    
    sf::Text titleText(oss.str(), m_font, 12);
    titleText.setFillColor(Theme::TextPrimary);
    sf::FloatRect tBounds = titleText.getLocalBounds();
    titleText.setPosition(center.x - (tBounds.width / 2.0f), center.y - 24.0f);
    window.draw(titleText);

    // Progress bar container
    float barWidth = 300.0f;
    float barHeight = 8.0f;

    sf::RectangleShape bgBar({barWidth, barHeight});
    bgBar.setOrigin(barWidth / 2.0f, barHeight / 2.0f);
    bgBar.setPosition(center.x, center.y + 10.0f);
    bgBar.setFillColor(Theme::PanelBackground);
    window.draw(bgBar);
    
    sf::RectangleShape fgBar({barWidth * info.progress, barHeight});
    fgBar.setPosition(center.x - (barWidth / 2.0f), center.y + 10.0f - (barHeight / 2.0f));
    fgBar.setFillColor(Theme::AccentColor);
    window.draw(fgBar);
}

void OverlayRenderer::RenderSpriteInspector(sf::RenderWindow& window, const SpriteInspectorInfo& info) {
    if (!info.isActive || !m_hasFont) return;
    
    float panelWidth = 230.0f;
    float panelHeight = 120.0f;
    float posX = window.getSize().x - panelWidth - 12.0f;
    float posY = Theme::ToolbarHeight + 12.0f;

    sf::RectangleShape bg({panelWidth, panelHeight});
    bg.setPosition(posX, posY);
    bg.setFillColor(Theme::InspectorBackground);
    bg.setOutlineThickness(Theme::BorderThickness);
    bg.setOutlineColor(Theme::BorderColor);
    window.draw(bg);

    sf::RectangleShape header({panelWidth, Theme::HeaderHeight});
    header.setPosition(posX, posY);
    header.setFillColor(Theme::PanelBackground);
    window.draw(header);

    sf::Text headerTitle;
    headerTitle.setFont(m_font);
    headerTitle.setString("SELECTED SPRITE");
    headerTitle.setCharacterSize(11);
    headerTitle.setFillColor(Theme::TextSecondary);
    headerTitle.setPosition(posX + 8.0f, posY + 6.0f);
    window.draw(headerTitle);

    std::ostringstream oss;
    oss << "ID: " << info.id << "\n"
        << "Rect: " << static_cast<int>(info.x) << ", " << static_cast<int>(info.y) 
        << " [" << static_cast<int>(info.w) << "x" << static_cast<int>(info.h) << "]\n"
        << "Pixels: " << info.pixelCount << "\n"
        << "Center: " << std::fixed << std::setprecision(1) << info.cx << ", " << info.cy;

    sf::Text contentText;
    contentText.setFont(m_font);
    contentText.setString(oss.str());
    contentText.setCharacterSize(11);
    contentText.setFillColor(Theme::TextPrimary);
    contentText.setPosition(posX + 8.0f, posY + Theme::HeaderHeight + 6.0f);
    window.draw(contentText);
}

void OverlayRenderer::RenderEmptyState(sf::RenderWindow& window) {
    if (!m_hasFont) return;
    
    sf::Vector2f center(window.getSize().x / 2.0f, (window.getSize().y - Theme::StatusBarHeight) / 2.0f);
    
    sf::Text title("Wisdom Park Asset Tools", m_font, 20);
    title.setFillColor(Theme::TextSecondary);
    title.setStyle(sf::Text::Bold);
    
    sf::Text subtitle("Sprite Sheet Studio", m_font, 14);
    subtitle.setFillColor(Theme::AccentColor);
    
    sf::Text instructions("Drag & Drop PNG\n\nor\n\nClick Import Image", m_font, 14);
    instructions.setFillColor(Theme::TextSecondary);
    
    // Center alignment math
    title.setPosition(center.x - title.getLocalBounds().width / 2.0f, center.y - 80.0f);
    subtitle.setPosition(center.x - subtitle.getLocalBounds().width / 2.0f, center.y - 50.0f);
    instructions.setPosition(center.x - instructions.getLocalBounds().width / 2.0f, center.y + 10.0f);
    
    window.draw(title);
    window.draw(subtitle);
    window.draw(instructions);
}

} // namespace StudioUI
