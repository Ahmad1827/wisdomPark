#include "PerspectiveTool.h"

PerspectiveTool::PerspectiveTool(Canvas& canvas, Timeline& timeline, PerspectiveManager& pm)
    : m_canvas(canvas), m_timeline(timeline), m_pm(pm), m_isPanning(false), m_hoveredVPIndex(-1), m_draggedVPIndex(-1) {}

void PerspectiveTool::Initialize() {}

void PerspectiveTool::SetBounds(const sf::FloatRect& bounds) { m_bounds = bounds; }

void PerspectiveTool::HandleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    sf::Vector2i mousePosI = sf::Mouse::getPosition(window);
    sf::Vector2f mousePos = window.mapPixelToCoords(mousePosI);

    if (event.type == sf::Event::MouseButtonPressed && (event.mouseButton.button == sf::Mouse::Right || event.mouseButton.button == sf::Mouse::Middle)) {
        m_isPanning = true;
        m_lastPanPos = sf::Vector2f(static_cast<float>(mousePosI.x), static_cast<float>(mousePosI.y));
        return;
    }
    if (event.type == sf::Event::MouseButtonReleased && (event.mouseButton.button == sf::Mouse::Right || event.mouseButton.button == sf::Mouse::Middle)) {
        m_isPanning = false;
        return;
    }
    if (event.type == sf::Event::MouseMoved && m_isPanning) {
        sf::Vector2f currentPanPos(static_cast<float>(mousePosI.x), static_cast<float>(mousePosI.y));
        sf::Vector2f delta = currentPanPos - m_lastPanPos;
        m_canvas.pan(delta);
        m_lastPanPos = currentPanPos;
        return;
    }

    PerspectiveConfig* activeCfg = m_pm.getActiveConfig();
    if (!activeCfg || activeCfg->guideSettings.locked) return;

    sf::Vector2f viewPos = m_canvas.getInverseTransform().transformPoint(mousePos);
    float scaleX = static_cast<float>(m_canvas.getCanvasSize().x) / m_canvas.getDrawArea().width;
    float scaleY = static_cast<float>(m_canvas.getCanvasSize().y) / m_canvas.getDrawArea().height;
    sf::Vector2f logicalPos((viewPos.x - m_canvas.getDrawArea().left) * scaleX, (viewPos.y - m_canvas.getDrawArea().top) * scaleY);

    if (event.type == sf::Event::MouseMoved) {
        if (m_draggedVPIndex != -1) {
            activeCfg->vps[m_draggedVPIndex].position = logicalPos;
        }
        else {
            m_hoveredVPIndex = -1;
            float hitRadius = 15.0f * (1.0f / std::max(0.001f, m_canvas.getTransform().getMatrix()[0]));
            for (size_t i = 0; i < activeCfg->vps.size(); ++i) {
                sf::Vector2f diff = activeCfg->vps[i].position - logicalPos;
                if (std::sqrt(diff.x * diff.x + diff.y * diff.y) <= hitRadius) {
                    m_hoveredVPIndex = static_cast<int>(i);
                    break;
                }
            }
        }
    }
    else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (m_hoveredVPIndex != -1) {
            m_pm.saveUndoState();
            m_draggedVPIndex = m_hoveredVPIndex;
        }
        else if (activeCfg->mode == PerspectiveMode::Custom && m_canvas.getDrawArea().contains(viewPos)) {
            m_pm.saveUndoState();
            activeCfg->vps.push_back(VanishingPoint(logicalPos, "Custom VP"));
            m_draggedVPIndex = static_cast<int>(activeCfg->vps.size()) - 1;
        }
    }
    else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        m_draggedVPIndex = -1;
    }
}

void PerspectiveTool::Update(float deltaTime, const sf::RenderWindow& window) {
    m_canvas.updateTransform(deltaTime, m_bounds);
}

void PerspectiveTool::Render(sf::RenderWindow& window) {
    sf::RenderStates canvasStates;
    canvasStates.transform = m_canvas.getTransform();
    m_canvas.draw(window, m_timeline.getCurrentFrame(), m_timeline.isPlaying(), canvasStates);

    const PerspectiveConfig* activeCfg = m_pm.getActiveConfigReadOnly();
    if (activeCfg) {
        sf::Transform innerTransform;
        innerTransform.translate(m_canvas.getDrawArea().left, m_canvas.getDrawArea().top);
        float scaleX = m_canvas.getDrawArea().width / static_cast<float>(m_canvas.getCanvasSize().x);
        float scaleY = m_canvas.getDrawArea().height / static_cast<float>(m_canvas.getCanvasSize().y);
        innerTransform.scale(scaleX, scaleY);
        sf::Transform finalTransform = m_canvas.getTransform() * innerTransform;

        PerspectiveRenderer::render(window, *activeCfg, finalTransform, m_canvas.getCanvasSize(), m_hoveredVPIndex, -1);
    }
}