#include "TextTool.h"

TextTool::TextTool(Canvas& canvas, Timeline& timeline, TextManager& tm)
    : m_canvas(canvas), m_timeline(timeline), m_tm(tm), m_isPanning(false) {}

void TextTool::Initialize() {
    FontManager::getInstance().loadDefaultFonts();
}

void TextTool::SetBounds(const sf::FloatRect& bounds) { m_bounds = bounds; }

void TextTool::HandleEvent(const sf::Event& event, const sf::RenderWindow& window) {
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

    TextObject* editingText = m_tm.getEditingText();

    if (event.type == sf::Event::TextEntered && editingText) {
        if (event.text.unicode == '\b' && !editingText->text.empty()) {
            m_tm.saveUndoState(m_timeline.getCurrentFrame());
            editingText->text.pop_back();
        }
        else if (event.text.unicode >= 32 && event.text.unicode != 127) {
            m_tm.saveUndoState(m_timeline.getCurrentFrame());
            editingText->text += static_cast<char>(event.text.unicode);
        }
        return;
    }

    if (event.type == sf::Event::KeyPressed && editingText) {
        if (event.key.code == sf::Keyboard::Enter && !sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) {
            m_tm.saveUndoState(m_timeline.getCurrentFrame());
            editingText->text += '\n';
            return;
        }
        if (event.key.code == sf::Keyboard::Enter && sf::Keyboard::isKeyPressed(sf::Keyboard::LControl)) {
            m_tm.clearEditingState();
            return;
        }
        if (event.key.code == sf::Keyboard::Escape) {
            m_tm.clearEditingState();
            return;
        }
    }

    sf::Vector2f viewPos = m_canvas.getInverseTransform().transformPoint(mousePos);
    float scaleX = static_cast<float>(m_canvas.getCanvasSize().x) / m_canvas.getDrawArea().width;
    float scaleY = static_cast<float>(m_canvas.getCanvasSize().y) / m_canvas.getDrawArea().height;
    sf::Vector2f logicalPos((viewPos.x - m_canvas.getDrawArea().left) * scaleX, (viewPos.y - m_canvas.getDrawArea().top) * scaleY);

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (m_canvas.getDrawArea().contains(viewPos)) {
            std::string hitId = m_tm.hitTest(m_timeline.getCurrentFrame(), m_canvas.getActiveLayer(), logicalPos);
            if (!hitId.empty()) {
                m_tm.clearEditingState();
                TextObject* hit = m_tm.getText(m_timeline.getCurrentFrame(), hitId);
                if (hit) hit->isEditing = true;
            }
            else {
                m_tm.clearEditingState();
                m_tm.createText(m_timeline.getCurrentFrame(), m_canvas.getActiveLayer(), logicalPos);
            }
        }
    }
}

void TextTool::Update(float deltaTime, const sf::RenderWindow& window) {
    m_canvas.updateTransform(deltaTime, m_bounds);
}

void TextTool::Render(sf::RenderWindow& window) {
    sf::RenderStates canvasStates;
    canvasStates.transform = m_canvas.getTransform();
    m_canvas.draw(window, m_timeline.getCurrentFrame(), m_timeline.isPlaying(), canvasStates);
}