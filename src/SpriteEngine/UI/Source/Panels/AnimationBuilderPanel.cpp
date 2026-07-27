#include "Panels/AnimationBuilderPanel.h"
#include "StudioEngineFacade.h"

namespace StudioUI {

AnimationBuilderPanel::AnimationBuilderPanel() {
    m_background.setFillColor(sf::Color(0, 0, 0, 200)); // Dim background
    m_modalBackground.setFillColor(sf::Color(40, 40, 45));
    m_modalBackground.setOutlineColor(sf::Color(100, 100, 110));
    m_modalBackground.setOutlineThickness(2.f);
}

void AnimationBuilderPanel::InitializeFont(const std::string& fontPath) {
    if (!m_font.loadFromFile(fontPath)) {
        m_font.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
    }
}

void AnimationBuilderPanel::Activate(StudioCore::StudioEngineFacade& engine) {
    m_proposals = engine.BuildAnimationsByRow();
    m_isActive = true;
}

void AnimationBuilderPanel::HandleEvent(const sf::Event& event, const sf::RenderWindow& window, StudioCore::StudioEngineFacade& engine, bool& exitWizard) {
    if (!m_isActive) return;

    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Escape) {
            m_isActive = false;
            exitWizard = true;
        } else if (event.key.code == sf::Keyboard::Enter) {
            engine.CommitProposedAnimations(m_proposals);
            m_isActive = false;
            exitWizard = true;
        }
    }
    
    // Basic click interactions for toggling settings
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f pos(event.mouseButton.x, event.mouseButton.y);
        float startY = 150.f;
        for (size_t i = 0; i < m_proposals.size(); ++i) {
            sf::FloatRect rowBounds(320.f, startY + (i * 40.f), 600.f, 30.f);
            if (rowBounds.contains(pos)) {
                // Click anywhere on the row increments FPS or toggles loop (simplified UI for wizard without text boxes)
                if (pos.x > 700.f) m_proposals[i].reverseOrder = !m_proposals[i].reverseOrder;
                else if (pos.x > 600.f) m_proposals[i].isLooping = !m_proposals[i].isLooping;
                else if (pos.x > 500.f) m_proposals[i].fps = (m_proposals[i].fps >= 24) ? 4 : m_proposals[i].fps + 2;
            }
        }
    }
}

void AnimationBuilderPanel::Render(sf::RenderWindow& window) {
    if (!m_isActive) return;

    m_background.setPosition(m_bounds.left, m_bounds.top);
    m_background.setSize(sf::Vector2f(m_bounds.width, m_bounds.height));
    
    m_modalBackground.setSize(sf::Vector2f(800.0f, 600.0f));
    m_modalBackground.setPosition(
        m_bounds.left + (m_bounds.width - 800.0f) / 2.0f, 
        m_bounds.top + (m_bounds.height - 600.0f) / 2.0f
    );

    window.draw(m_background);
    window.draw(m_modalBackground);

    sf::Text title("Animation Builder Wizard (Press Enter to Commit, Esc to Cancel)", m_font, 20);
    title.setPosition(m_modalBackground.getPosition().x + 20.f, m_modalBackground.getPosition().y + 20.f);
    window.draw(title);

    float currentY = m_modalBackground.getPosition().y + 80.f;
    for (const auto& prop : m_proposals) {
        sf::Text info(prop.name + " (" + std::to_string(prop.spriteIds.size()) + " frames)", m_font, 16);
        info.setPosition(m_modalBackground.getPosition().x + 20.f, currentY);
        
        sf::Text settings("FPS: " + std::to_string(prop.fps) + " | Loop: " + (prop.isLooping ? "ON" : "OFF") + " | Reverse: " + (prop.reverseOrder ? "ON" : "OFF"), m_font, 16);
        settings.setPosition(m_modalBackground.getPosition().x + 300.f, currentY);
        settings.setFillColor(sf::Color::Cyan);

        window.draw(info);
        window.draw(settings);
        currentY += 40.f;
    }
}

}