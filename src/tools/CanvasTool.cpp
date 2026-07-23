#include "CanvasTool.h"

CanvasTool::CanvasTool(Canvas& canvas, Timeline& timeline, bool& isLightingMode)
    : m_canvas(canvas), m_timeline(timeline), m_isLightingMode(isLightingMode), m_isPanning(false) {
}

void CanvasTool::Initialize() {}

void CanvasTool::SetBounds(const sf::FloatRect& bounds) {
    m_bounds = bounds;
}

void CanvasTool::Update(float deltaTime, const sf::RenderWindow& window) {
    m_canvas.updateTransform(deltaTime, m_bounds);
}

void CanvasTool::Render(sf::RenderWindow& window) {
    sf::RenderStates canvasStates;
    canvasStates.transform = m_canvas.getTransform();
    m_canvas.draw(window, m_timeline.getCurrentFrame(), m_timeline.isPlaying(), canvasStates);
}

void CanvasTool::RenderShadows(sf::RenderWindow& window, AIHelper& aiHelper) {
    if (!m_isLightingMode) return;

    sf::RenderStates canvasStates;
    canvasStates.transform = m_canvas.getTransform();

    sf::Vector2f mousePos(static_cast<float>(sf::Mouse::getPosition(window).x), static_cast<float>(sf::Mouse::getPosition(window).y));
    sf::Vector2f logicalSunPos = m_canvas.getInverseTransform().transformPoint(mousePos);

    std::vector<sf::FloatRect> boundsList;
    std::vector<std::string> cats;
    for (const auto& item : aiHelper.getHistory()) {
        boundsList.push_back(item.bounds);
        cats.push_back(item.category);
    }
    m_canvas.drawShadows(window, logicalSunPos, boundsList, cats, canvasStates);
}

void CanvasTool::HandleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    sf::Vector2f mousePos(static_cast<float>(sf::Mouse::getPosition(window).x), static_cast<float>(sf::Mouse::getPosition(window).y));
    sf::Vector2f logicalPos = m_canvas.getInverseTransform().transformPoint(mousePos);

    if (event.type == sf::Event::MouseWheelScrolled && event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
        m_canvas.zoom(event.mouseWheelScroll.delta);
    }

    if (event.type == sf::Event::MouseButtonPressed) {
        if (event.mouseButton.button == sf::Mouse::Middle || event.mouseButton.button == sf::Mouse::Right) {
            m_isPanning = true;
            m_lastPanMousePos = mousePos;
            return;
        }
        if (event.mouseButton.button == sf::Mouse::Left && !m_isPanning) {
            m_canvas.handleMousePressed(logicalPos, false, m_timeline.getCurrentFrame());
        }
    }

    if (event.type == sf::Event::MouseButtonReleased) {
        if (event.mouseButton.button == sf::Mouse::Right || event.mouseButton.button == sf::Mouse::Middle) {
            m_isPanning = false;
            return;
        }
        if (event.mouseButton.button == sf::Mouse::Left && !m_isPanning) {
            m_canvas.handleMouseReleased(logicalPos, m_timeline.getCurrentFrame());

            if (m_canvas.getActiveTool() != ToolType::Select && m_canvas.getDrawArea().contains(logicalPos)) {
                sf::RenderTexture* rt = m_canvas.getActiveRenderTexture(m_timeline.getCurrentFrame());
                if (rt) {
                    sf::Image img = rt->getTexture().copyToImage();
                    m_timeline.getFrameData(m_timeline.getCurrentFrame()).thumbnail = img;
                }
            }
        }
    }

    if (event.type == sf::Event::MouseMoved) {
        if (m_isPanning) {
            m_canvas.pan(mousePos - m_lastPanMousePos);
            m_lastPanMousePos = mousePos;
        }
        else {
            m_canvas.handleMouseMoved(logicalPos, mousePos, m_timeline.getCurrentFrame());
        }
    }
}