#pragma once
#include <SFML/Graphics.hpp>
#include <string>

namespace StudioCore {
    class StudioEngineFacade;
}

namespace StudioUI {

class ExportPreviewPanel {
public:
    ExportPreviewPanel();
    bool InitializeFont(const std::string& customPath);
    void Activate(const StudioCore::StudioEngineFacade& engine);
    void Deactivate() { m_isActive = false; }
    bool HandleEvent(const sf::Event& event, const sf::RenderWindow& window, const StudioCore::StudioEngineFacade& engine);
    void Render(sf::RenderWindow& window);

    bool IsActive() const { return m_isActive; }

private:
    sf::Font m_font;
    bool m_hasFont{false};
    
    sf::Texture m_exportTexture;
    sf::Sprite m_exportSprite;
    sf::View m_view;

    bool m_isPanning{false};
    sf::Vector2i m_lastMousePos;
    float m_currentZoom{1.0f};
    bool m_isActive{false};
};

}