#include "Panels/ExportPreviewPanel.h"
#include "StudioEngineFacade.h"
#include <iostream>
#include "Utils/NativeFileDialog.h"

namespace StudioUI {

ExportPreviewPanel::ExportPreviewPanel() {
    m_view.setSize(1280.f, 720.f);
    m_view.setCenter(640.f, 360.f);
}

bool ExportPreviewPanel::InitializeFont(const std::string& customPath) {
    std::vector<std::string> fontPaths = {
        customPath,
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf"
    };
    for (const auto& path : fontPaths) {
        if (m_font.loadFromFile(path)) {
            m_hasFont = true;
            return true;
        }
    }
    return false;
}

void ExportPreviewPanel::Activate(const StudioCore::StudioEngineFacade& engine) {
    m_isActive = true;
    sf::Image img = engine.GenerateExportPreview(8);
    if (img.getSize().x > 0) {
        m_exportTexture.loadFromImage(img);
        m_exportSprite.setTexture(m_exportTexture, true);
        
        sf::FloatRect bounds = m_exportSprite.getLocalBounds();
        m_exportSprite.setOrigin(bounds.width / 2.0f, bounds.height / 2.0f);
        m_exportSprite.setPosition(0.0f, 0.0f);
        m_view.setCenter(0.0f, 0.0f);
        m_currentZoom = 1.0f;
    }
}

bool ExportPreviewPanel::HandleEvent(const sf::Event& event, const sf::RenderWindow& window, const StudioCore::StudioEngineFacade& engine) {
    if (!m_isActive) return false;

    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Escape) {
            m_isActive = false;
        } else if (event.key.code == sf::Keyboard::Enter) {
            std::string savePath = NativeFileDialog::SaveFileDialog("aligned_spritesheet.png");
            
            if (!savePath.empty()) {
                if (engine.ExportPNG(savePath, 8)) {
                    std::cout << "[✓] Exported successfully to: " << savePath << std::endl;
                    m_isActive = false; // Only exit preview when export actually succeeds
                } else {
                    std::cerr << "[X] Failed to save exported image." << std::endl;
                }
            }
        }
    } else if (event.type == sf::Event::MouseWheelScrolled) {
        float zoomFactor = (event.mouseWheelScroll.delta > 0) ? 0.8f : 1.25f;
        m_view.zoom(zoomFactor);
        m_currentZoom *= zoomFactor;
    } else if (event.type == sf::Event::MouseButtonPressed) {
        if (event.mouseButton.button == sf::Mouse::Middle || event.mouseButton.button == sf::Mouse::Left) {
            m_isPanning = true;
            m_lastMousePos = sf::Mouse::getPosition(window);
        }
    } else if (event.type == sf::Event::MouseButtonReleased) {
        if (event.mouseButton.button == sf::Mouse::Middle || event.mouseButton.button == sf::Mouse::Left) {
            m_isPanning = false;
        }
    } else if (event.type == sf::Event::MouseMoved) {
        if (m_isPanning) {
            sf::Vector2i currentMousePos = sf::Mouse::getPosition(window);
            sf::Vector2f delta(
                static_cast<float>(m_lastMousePos.x - currentMousePos.x) * m_currentZoom,
                static_cast<float>(m_lastMousePos.y - currentMousePos.y) * m_currentZoom
            );
            m_view.move(delta);
            m_lastMousePos = currentMousePos;
        }
    } else if (event.type == sf::Event::Resized) {
        sf::Vector2f center = m_view.getCenter();
        m_view.setSize(static_cast<float>(event.size.width) * m_currentZoom, 
                       static_cast<float>(event.size.height) * m_currentZoom);
        m_view.setCenter(center);
    }
    
    return true; 
}

void ExportPreviewPanel::Render(sf::RenderWindow& window) {
    if (!m_isActive) return;

    window.setView(m_view);
    
    sf::RectangleShape gridBg;
    sf::FloatRect bounds = m_exportSprite.getLocalBounds();
    gridBg.setSize(sf::Vector2f(bounds.width, bounds.height));
    gridBg.setOrigin(bounds.width / 2.0f, bounds.height / 2.0f);
    gridBg.setPosition(0.0f, 0.0f);
    gridBg.setFillColor(sf::Color(20, 20, 20, 200));
    window.draw(gridBg);
    
    window.draw(m_exportSprite);

    window.setView(window.getDefaultView());
    
    if (m_hasFont) {
        sf::Text helpTxt("EXPORT PREVIEW MODE\nPress [ENTER] to open Save Dialog folder browser\nPress [ESC] to cancel", m_font, 18);
        helpTxt.setPosition(20.f, 20.f);
        helpTxt.setFillColor(sf::Color::Cyan);
        window.draw(helpTxt);
    }
}

}