#include "ShapeTool.h"
#include "../UI/UITheme.h"
#include <cmath>
#include <algorithm>

ShapeTool::ShapeTool(Canvas& canvas, Timeline& timeline)
    : m_canvas(canvas), m_timeline(timeline), m_isDragging(false), m_isPanning(false),
    m_currentShapeId(ShapeId::Rectangle), m_panelPos(64.f, 78.f), m_panelSize(210.f, 320.f),
    m_isDraggingPanel(false) {}

void ShapeTool::Initialize() {
    m_font.loadFromFile("assets/font.otf");
    m_panelPos = sf::Vector2f(64.f, 78.f);
}

void ShapeTool::SetBounds(const sf::FloatRect& bounds) {
    m_bounds = bounds;
}

void ShapeTool::HandleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    sf::Vector2i mousePosI = sf::Mouse::getPosition(window);
    sf::Vector2f mousePos = window.mapPixelToCoords(mousePosI);

    sf::FloatRect headerGrip(m_panelPos.x, m_panelPos.y, m_panelSize.x, 34.f);

    if (headerGrip.contains(mousePos) && event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        m_isDraggingPanel = true;
        m_panelDragOffset = mousePos - m_panelPos;
        return;
    }

    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        m_isDraggingPanel = false;
    }

    if (event.type == sf::Event::MouseMoved && m_isDraggingPanel) {
        m_panelPos = mousePos - m_panelDragOffset;
        m_panelPos.x = std::clamp(m_panelPos.x, 56.f, 1920.f - m_panelSize.x);
        m_panelPos.y = std::clamp(m_panelPos.y, 40.f, 1080.f - m_panelSize.y);
        return;
    }

    if (sf::FloatRect(m_panelPos.x, m_panelPos.y, m_panelSize.x, m_panelSize.y).contains(mousePos)) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            for (const auto& btn : m_shapeButtons) {
                if (btn.first.contains(mousePos)) {
                    m_currentShapeId = btn.second;
                    return;
                }
            }
        }
        return;
    }

    if (event.type == sf::Event::MouseButtonPressed && (event.mouseButton.button == sf::Mouse::Right || event.mouseButton.button == sf::Mouse::Middle)) {
        m_isPanning = true;
        m_lastPanPos = mousePos;
        return;
    }
    if (event.type == sf::Event::MouseButtonReleased && (event.mouseButton.button == sf::Mouse::Right || event.mouseButton.button == sf::Mouse::Middle)) {
        m_isPanning = false;
        return;
    }
    if (event.type == sf::Event::MouseMoved && m_isPanning) {
        sf::Vector2f delta = mousePos - m_lastPanPos;
        m_canvas.pan(delta);
        m_lastPanPos = mousePos;
        return;
    }

    sf::Vector2f viewPos = m_canvas.getInverseTransform().transformPoint(mousePos);
    float scaleX = static_cast<float>(m_canvas.getCanvasSize().x) / m_canvas.getDrawArea().width;
    float scaleY = static_cast<float>(m_canvas.getCanvasSize().y) / m_canvas.getDrawArea().height;
    sf::Vector2f logicalPos((viewPos.x - m_canvas.getDrawArea().left) * scaleX, (viewPos.y - m_canvas.getDrawArea().top) * scaleY);

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (m_canvas.getPixelMode()) {
            logicalPos.x = std::floor(logicalPos.x);
            logicalPos.y = std::floor(logicalPos.y);
        }
        m_isDragging = true;
        m_startPos = logicalPos;
        m_shapeManager.beginShape(m_currentShapeId, logicalPos);
        if (m_shapeManager.activeShape) {
            m_shapeManager.activeShape->strokeColor = m_canvas.getPrimaryColor();
            m_shapeManager.activeShape->fillColor = m_canvas.getSecondaryColor();
            m_shapeManager.activeShape->strokeWidth = m_canvas.getBrushSize();
        }
    }
    else if (event.type == sf::Event::MouseMoved && m_isDragging) {
        if (m_canvas.getPixelMode()) {
            logicalPos.x = std::floor(logicalPos.x);
            logicalPos.y = std::floor(logicalPos.y);
        }
        bool lock = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);
        bool center = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::RControl);
        m_shapeManager.updateShape(logicalPos, lock, center);
    }
    else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left && m_isDragging) {
        m_isDragging = false;

        auto* target = m_canvas.getActiveRenderTexture(m_timeline.getCurrentFrame());
        if (target) {
            m_canvas.saveUndoState();

            if (m_canvas.getSymmetryManager().enabled) {
                m_shapeManager.activeShape->applySymmetry(m_canvas.getSymmetryManager().startPoint, m_canvas.getSymmetryManager().endPoint);
            }

            m_shapeManager.rasterizeActive(*target, m_canvas.getPixelMode());
            target->display();

            m_shapeManager.clearActive();
        }
    }
}

void ShapeTool::Update(float deltaTime, const sf::RenderWindow& window) {
    m_canvas.updateTransform(deltaTime, m_bounds);
}

void ShapeTool::Render(sf::RenderWindow& window) {
    sf::RenderStates canvasStates;
    canvasStates.transform = m_canvas.getTransform();
    m_canvas.draw(window, m_timeline.getCurrentFrame(), m_timeline.isPlaying(), canvasStates);

    sf::Transform innerTransform;
    innerTransform.translate(std::round(m_canvas.getDrawArea().left), std::round(m_canvas.getDrawArea().top));
    float scaleX = m_canvas.getDrawArea().width / static_cast<float>(m_canvas.getCanvasSize().x);
    float scaleY = m_canvas.getDrawArea().height / static_cast<float>(m_canvas.getCanvasSize().y);
    innerTransform.scale(scaleX, scaleY);

    sf::RenderStates previewStates;
    previewStates.transform = m_canvas.getTransform() * innerTransform;

    m_shapeManager.drawActive(window, m_canvas.getPixelMode(), previewStates);

    drawPropertiesPanel(window);
}

void ShapeTool::drawPropertiesPanel(sf::RenderWindow& window) {
    sf::FloatRect panelBounds(m_panelPos.x, m_panelPos.y, m_panelSize.x, m_panelSize.y);
    WisdomUI::Theme::DrawSunsetPanel(window, panelBounds, 1.0f);

    sf::FloatRect headerGrip(m_panelPos.x + 8.f, m_panelPos.y + 6.f, m_panelSize.x - 16.f, 26.f);
    sf::RectangleShape gripBg(sf::Vector2f(headerGrip.width, headerGrip.height));
    gripBg.setPosition(headerGrip.left, headerGrip.top);
    gripBg.setFillColor(WisdomUI::Theme::SunsetDeepDark);
    gripBg.setOutlineThickness(1.f);
    gripBg.setOutlineColor(WisdomUI::Theme::SunsetPlum);
    window.draw(gripBg);

    WisdomUI::Theme::DrawCrispText(window, m_font, ":: VECTOR SHAPES ::", 12, headerGrip.left + headerGrip.width / 2.0f, headerGrip.top + headerGrip.height / 2.0f, WisdomUI::Theme::SunsetAmber, sf::Color(14, 6, 20), true, true);

    std::vector<std::pair<std::string, ShapeId>> opts = {
        {"Line", ShapeId::Line},
        {"Rectangle", ShapeId::Rectangle},
        {"Filled Rect", ShapeId::FilledRectangle},
        {"Circle", ShapeId::Circle},
        {"Filled Circle", ShapeId::FilledCircle},
        {"Polygon", ShapeId::Polygon},
        {"Star", ShapeId::Star},
        {"Arrow", ShapeId::Arrow}
    };

    m_shapeButtons.clear();
    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    float y = m_panelPos.y + 40.f;

    for (const auto& o : opts) {
        sf::FloatRect btnRect(m_panelPos.x + 12.f, y, m_panelSize.x - 24.f, 28.f);
        m_shapeButtons.push_back({ btnRect, o.second });

        bool isActive = (m_currentShapeId == o.second);
        bool isHovered = btnRect.contains(mPos);

        WisdomUI::Theme::DrawSunsetButton(window, btnRect, o.first, m_font, 11, isActive, isHovered, isActive, 1.0f);
        y += 33.f;
    }
}