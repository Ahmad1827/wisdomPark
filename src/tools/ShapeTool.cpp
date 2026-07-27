#include "ShapeTool.h"

ShapeTool::ShapeTool(Canvas& canvas, Timeline& timeline)
    : m_canvas(canvas), m_timeline(timeline), m_isDragging(false), m_isPanning(false), m_currentShapeId(ShapeId::Rectangle) {
}

void ShapeTool::Initialize() {
    m_font.loadFromFile("assets/font.otf");

    m_panelBg.setSize(sf::Vector2f(180.f, 290.f));
    m_panelBg.setFillColor(sf::Color(25, 25, 30, 240));
    m_panelBg.setOutlineThickness(1.f);
    m_panelBg.setOutlineColor(sf::Color(100, 100, 110));
}

void ShapeTool::SetBounds(const sf::FloatRect& bounds) {
    m_bounds = bounds;
    m_panelBg.setPosition(bounds.left + 110.f, bounds.top + 60.f);
}

void ShapeTool::HandleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    sf::Vector2i mousePosI = sf::Mouse::getPosition(window);
    sf::Vector2f mousePos = window.mapPixelToCoords(mousePosI);

    sf::Vector2f viewPos = m_canvas.getInverseTransform().transformPoint(mousePos);
    float scaleX = static_cast<float>(m_canvas.getCanvasSize().x) / m_canvas.getDrawArea().width;
    float scaleY = static_cast<float>(m_canvas.getCanvasSize().y) / m_canvas.getDrawArea().height;
    sf::Vector2f logicalPos((viewPos.x - m_canvas.getDrawArea().left) * scaleX, (viewPos.y - m_canvas.getDrawArea().top) * scaleY);

    if (m_panelBg.getGlobalBounds().contains(mousePos)) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            float y = mousePos.y - m_panelBg.getPosition().y;
            if (y > 40 && y < 70) m_currentShapeId = ShapeId::Line;
            else if (y > 70 && y < 100) m_currentShapeId = ShapeId::Rectangle;
            else if (y > 100 && y < 130) m_currentShapeId = ShapeId::FilledRectangle;
            else if (y > 130 && y < 160) m_currentShapeId = ShapeId::Circle;
            else if (y > 160 && y < 190) m_currentShapeId = ShapeId::FilledCircle;
            else if (y > 190 && y < 220) m_currentShapeId = ShapeId::Polygon;
            else if (y > 220 && y < 250) m_currentShapeId = ShapeId::Star;
            else if (y > 250 && y < 280) m_currentShapeId = ShapeId::Arrow;
        }
        return;
    }

    // Panning Support
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

    // Shape Drawing Logic
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
            target->display(); // This fully locks the drawn pixels and fixes the invisible bug!

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
    window.draw(m_panelBg);

    sf::Text title("SHAPES MENU", m_font, 14);
    title.setPosition(m_panelBg.getPosition().x + 10.f, m_panelBg.getPosition().y + 10.f);
    title.setFillColor(sf::Color(255, 200, 100));
    window.draw(title);

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

    float y = m_panelBg.getPosition().y + 40.f;
    for (const auto& o : opts) {
        sf::Text t(o.first, m_font, 12);
        t.setPosition(m_panelBg.getPosition().x + 20.f, y + 5.f);

        if (m_currentShapeId == o.second) {
            sf::RectangleShape activeBg(sf::Vector2f(160.f, 24.f));
            activeBg.setPosition(m_panelBg.getPosition().x + 10.f, y);
            activeBg.setFillColor(sf::Color(0, 122, 204, 180));
            window.draw(activeBg);
            t.setFillColor(sf::Color::White);
        }
        else {
            t.setFillColor(sf::Color(200, 200, 200));
        }

        window.draw(t);
        y += 30.f;
    }
}