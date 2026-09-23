#include "ShapeTool.h"
#include "../UI/UITheme.h"
#include <cmath>
#include <algorithm>

ShapeTool::ShapeTool(Canvas& canvas, Timeline& timeline)
    : m_canvas(canvas), m_timeline(timeline), m_isDragging(false), m_isPanning(false),
    m_currentShapeId(ShapeId::Rectangle), m_panelPos(64.f, 78.f), m_panelSize(310.f, 330.f),
    m_isDraggingPanel(false) {}

void ShapeTool::Initialize() {
    m_font.loadFromFile("assets/font.otf");
    m_panelPos = sf::Vector2f(64.f, 78.f);
    m_panelSize = sf::Vector2f(310.f, 330.f);
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
            m_shapeManager.activeShape->strokeWidth = m_canvas.getPixelMode()
                ? static_cast<float>(m_canvas.getPixelBrushSize())
                : m_canvas.getBrushSize();
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

        if (m_canvas.getPixelMode()) {
            auto* target = m_canvas.getActiveRenderTexture(m_timeline.getCurrentFrame());
            if (target) {
                m_canvas.saveUndoState();

                if (m_canvas.getSymmetryManager().enabled) {
                    m_shapeManager.activeShape->applySymmetry(m_canvas.getSymmetryManager().startPoint, m_canvas.getSymmetryManager().endPoint);
                }

                m_shapeManager.rasterizeActive(*target, true);
                target->display();
            }
        }
        else {
            if (m_shapeManager.activeShape) {
                m_canvas.saveUndoState();
                sf::VertexArray mesh = m_shapeManager.activeShape->toVectorMesh();

                if (m_canvas.getSymmetryManager().enabled) {
                    sf::Vector2f symStart = m_canvas.getSymmetryManager().startPoint;
                    sf::Vector2f symEnd = m_canvas.getSymmetryManager().endPoint;
                    sf::Vector2f dir = symEnd - symStart;
                    float lenSq = dir.x * dir.x + dir.y * dir.y;
                    if (lenSq > 0.0001f) {
                        size_t count = mesh.getVertexCount();
                        for (size_t i = 0; i < count; ++i) {
                            sf::Vector2f p = mesh[i].position;
                            sf::Vector2f v = p - symStart;
                            float t = (v.x * dir.x + v.y * dir.y) / lenSq;
                            sf::Vector2f proj = symStart + dir * t;
                            sf::Vector2f reflected = p + 2.0f * (proj - p);
                            mesh.append(sf::Vertex(reflected, mesh[i].color));
                        }
                    }
                }

                m_canvas.addVectorMesh(mesh, m_timeline.getCurrentFrame(), m_canvas.getActiveLayer());
            }
        }

        m_shapeManager.clearActive();
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

    if (m_canvas.getSymmetryManager().enabled && m_shapeManager.activeShape && !m_canvas.getPixelMode()) {
        sf::VertexArray symPreview = m_shapeManager.activeShape->toVectorMesh();
        sf::Vector2f symStart = m_canvas.getSymmetryManager().startPoint;
        sf::Vector2f symEnd = m_canvas.getSymmetryManager().endPoint;
        sf::Vector2f dir = symEnd - symStart;
        float lenSq = dir.x * dir.x + dir.y * dir.y;
        if (lenSq > 0.0001f) {
            for (size_t i = 0; i < symPreview.getVertexCount(); ++i) {
                sf::Vector2f p = symPreview[i].position;
                sf::Vector2f v = p - symStart;
                float t = (v.x * dir.x + v.y * dir.y) / lenSq;
                sf::Vector2f proj = symStart + dir * t;
                symPreview[i].position = p + 2.0f * (proj - p);
            }
            window.draw(symPreview, previewStates);
        }
    }

    drawPropertiesPanel(window);
}

void ShapeTool::drawPropertiesPanel(sf::RenderWindow& window) {
    sf::FloatRect panelBounds(m_panelPos.x, m_panelPos.y, m_panelSize.x, m_panelSize.y);
    WisdomUI::Theme::DrawSunsetPanel(window, panelBounds, 1.0f);

    sf::FloatRect headerGrip(m_panelPos.x + 10.f, m_panelPos.y + 8.f, m_panelSize.x - 20.f, 32.f);
    sf::RectangleShape gripBg(sf::Vector2f(headerGrip.width, headerGrip.height));
    gripBg.setPosition(headerGrip.left, headerGrip.top);
    gripBg.setFillColor(WisdomUI::Theme::SunsetDeepDark);
    gripBg.setOutlineThickness(1.2f);
    gripBg.setOutlineColor(WisdomUI::Theme::SunsetPlum);
    window.draw(gripBg);

    std::string headerTitle = m_canvas.getPixelMode() ? ":: PIXEL SHAPES ::" : ":: VECTOR SHAPES ::";
    WisdomUI::Theme::DrawCrispText(window, m_font, headerTitle, 15, headerGrip.left + headerGrip.width / 2.0f, headerGrip.top + headerGrip.height / 2.0f, WisdomUI::Theme::SunsetGold, sf::Color(14, 6, 20), true, true);

    std::vector<std::pair<std::string, ShapeId>> col1 = {
        { "Line", ShapeId::Line },
        { "Rectangle", ShapeId::Rectangle },
        { "Circle", ShapeId::Circle },
        { "Triangle", ShapeId::Triangle },
        { "Star", ShapeId::Star }
    };

    std::vector<std::pair<std::string, ShapeId>> col2 = {
        { "Arrow", ShapeId::Arrow },
        { "Filled Rect", ShapeId::FilledRectangle },
        { "Filled Circle", ShapeId::FilledCircle },
        { "Filled Tri", ShapeId::FilledTriangle },
        { "Filled Star", ShapeId::FilledStar }
    };

    m_shapeButtons.clear();
    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    float btnW = 135.f;
    float btnH = 42.f;
    float startX1 = m_panelPos.x + 14.f;
    float startX2 = m_panelPos.x + 14.f + btnW + 12.f;
    float startY = m_panelPos.y + 52.f;
    float rowSpacing = 50.f;

    for (size_t i = 0; i < col1.size(); ++i) {
        float y = startY + static_cast<float>(i) * rowSpacing;

        // Left Column
        sf::FloatRect btn1(startX1, y, btnW, btnH);
        m_shapeButtons.push_back({ btn1, col1[i].second });
        bool active1 = (m_currentShapeId == col1[i].second);
        bool hov1 = btn1.contains(mPos);
        WisdomUI::Theme::DrawSunsetButton(window, btn1, col1[i].first, m_font, 14, active1, hov1, active1, 1.0f);

        // Right Column
        sf::FloatRect btn2(startX2, y, btnW, btnH);
        m_shapeButtons.push_back({ btn2, col2[i].second });
        bool active2 = (m_currentShapeId == col2[i].second);
        bool hov2 = btn2.contains(mPos);
        WisdomUI::Theme::DrawSunsetButton(window, btn2, col2[i].first, m_font, 14, active2, hov2, active2, 1.0f);
    }

    WisdomUI::Theme::DrawCrispText(window, m_font, "[Shift] Square/Circle   [Ctrl] From Center", 11, m_panelPos.x + m_panelSize.x * 0.5f, m_panelPos.y + m_panelSize.y - 16.f, WisdomUI::Theme::TextSecondary, sf::Color::Transparent, true, true);
}