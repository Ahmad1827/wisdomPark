#include "GradientTool.h"
#include <cmath>

GradientTool::GradientTool(Canvas& canvas, Timeline& timeline, GradientConfig& config)
    : m_canvas(canvas), m_timeline(timeline), m_config(config), m_isPanning(false), m_isDragging(false) {}

void GradientTool::Initialize() {}

void GradientTool::SetBounds(const sf::FloatRect& bounds) { m_bounds = bounds; }

sf::Vector2f GradientTool::applyModifiers(sf::Vector2f rawPos) {
    sf::Vector2f diff = rawPos - m_startPos;
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) {
        float angle = std::atan2(diff.y, diff.x);
        float step = 3.14159265f / 4.0f;
        angle = std::round(angle / step) * step;
        float mag = std::sqrt(diff.x * diff.x + diff.y * diff.y);
        rawPos = m_startPos + sf::Vector2f(std::cos(angle) * mag, std::sin(angle) * mag);
    }
    if (m_canvas.getPixelMode() && m_config.snapToGrid) {
        rawPos.x = std::round(rawPos.x);
        rawPos.y = std::round(rawPos.y);
    }
    return rawPos;
}

void GradientTool::HandleEvent(const sf::Event& event, const sf::RenderWindow& window) {
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
        m_canvas.pan(currentPanPos - m_lastPanPos);
        m_lastPanPos = currentPanPos;
        return;
    }

    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape && m_isDragging) {
        m_isDragging = false;
        return;
    }

    sf::Vector2f logicalPos = m_canvas.getInverseTransform().transformPoint(mousePos);
    float scaleX = static_cast<float>(m_canvas.getCanvasSize().x) / m_canvas.getDrawArea().width;
    float scaleY = static_cast<float>(m_canvas.getCanvasSize().y) / m_canvas.getDrawArea().height;
    sf::Vector2f trueCanvasPos;
    trueCanvasPos.x = (logicalPos.x - m_canvas.getDrawArea().left) * scaleX;
    trueCanvasPos.y = (logicalPos.y - m_canvas.getDrawArea().top) * scaleY;

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (m_canvas.getDrawArea().contains(logicalPos)) {
            m_isDragging = true;
            m_startPos = applyModifiers(trueCanvasPos);
            m_currentPos = m_startPos;
            if (m_config.livePreview) updatePreview();
        }
    }
    else if (event.type == sf::Event::MouseMoved && m_isDragging) {
        m_currentPos = applyModifiers(trueCanvasPos);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt)) {
            m_startPos = m_startPos - (m_currentPos - m_startPos);
        }
        if (m_config.livePreview) updatePreview();
    }
    else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left && m_isDragging) {
        m_isDragging = false;
        m_currentPos = applyModifiers(trueCanvasPos);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt)) {
            m_startPos = m_startPos - (m_currentPos - m_startPos);
        }
        applyGradient();
    }
}

void GradientTool::Update(float deltaTime, const sf::RenderWindow& window) {
    m_canvas.updateTransform(deltaTime, m_bounds);
}

void GradientTool::updatePreview() {
    sf::Vector2u fullSize = m_canvas.getCanvasSize();
    sf::Vector2u previewSize = fullSize;

    // Severely limit preview generation size to prevent CPU stall on huge canvases
    unsigned int maxDim = m_canvas.getPixelMode() ? 256 : 256;
    float scaleX = 1.0f;
    float scaleY = 1.0f;

    if (fullSize.x > maxDim || fullSize.y > maxDim) {
        float ratio = std::min(static_cast<float>(maxDim) / fullSize.x, static_cast<float>(maxDim) / fullSize.y);
        previewSize.x = static_cast<unsigned int>(fullSize.x * ratio);
        previewSize.y = static_cast<unsigned int>(fullSize.y * ratio);

        if (previewSize.x == 0) previewSize.x = 1;
        if (previewSize.y == 0) previewSize.y = 1;

        scaleX = static_cast<float>(fullSize.x) / previewSize.x;
        scaleY = static_cast<float>(fullSize.y) / previewSize.y;
    }

    // Adjust origin targeting for preview scale
    sf::Vector2f pStart = m_startPos;
    sf::Vector2f pCurrent = m_currentPos;
    pStart.x /= scaleX; pStart.y /= scaleY;
    pCurrent.x /= scaleX; pCurrent.y /= scaleY;

    sf::Image img = GradientSystem::generate(m_config, pStart, pCurrent, previewSize, m_canvas.getPixelMode(), nullptr);
    m_previewTexture.loadFromImage(img);
    m_previewSprite.setTexture(m_previewTexture, true);

    // Map strictly to the visual canvas draw area to prevent alignment/scaling distortions
    m_previewSprite.setPosition(m_canvas.getDrawArea().left, m_canvas.getDrawArea().top);
    m_previewSprite.setScale(
        m_canvas.getDrawArea().width / static_cast<float>(previewSize.x),
        m_canvas.getDrawArea().height / static_cast<float>(previewSize.y)
    );
}

void GradientTool::applyGradient() {
    m_canvas.saveUndoState();
    sf::Vector2u size = m_canvas.getCanvasSize();
    sf::Image img = GradientSystem::generate(m_config, m_startPos, m_currentPos, size, m_canvas.getPixelMode(), nullptr);

    sf::Texture tex;
    tex.loadFromImage(img);
    sf::Sprite spr(tex);

    sf::RenderTexture* target = m_canvas.getActiveRenderTexture(m_timeline.getCurrentFrame());
    if (target) {
        sf::RenderStates states;
        states.blendMode = m_config.blendMode;
        target->draw(spr, states);
        target->display();
    }
}

void GradientTool::Render(sf::RenderWindow& window) {
    sf::RenderStates canvasStates;
    canvasStates.transform = m_canvas.getTransform();
    m_canvas.draw(window, m_timeline.getCurrentFrame(), m_timeline.isPlaying(), canvasStates);
    if (m_isDragging && m_config.livePreview) {
        window.draw(m_previewSprite, canvasStates);
    }
}