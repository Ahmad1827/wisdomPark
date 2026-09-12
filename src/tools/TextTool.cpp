#include "TextTool.h"
#include "../core/FontManager.h"
#include <iostream>

TextTool::TextTool(Canvas& canvas, Timeline& timeline, TextManager& tm)
    : m_canvas(canvas), m_timeline(timeline), m_tm(tm), m_isPanning(false) {
    m_lastPrimaryColor = m_canvas.getPrimaryColor();
}

TextTool::~TextTool() {
    commitText();
}

void TextTool::commitText() {
    TextObject* editing = m_tm.getEditingText();
    if (editing) {
        if (!editing->text.isEmpty()) {
            m_tm.rasterizeText(m_timeline.getCurrentFrame(), m_canvas.getActiveLayer(), editing->id, m_canvas);
        }
        else {
            m_tm.deleteText(m_timeline.getCurrentFrame(), editing->id);
        }
    }
}

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

    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        if (m_isDraggingText) {
            m_isDraggingText = false;
            return;
        }
    }

    sf::Vector2f logicalPos = m_canvas.getInverseTransform().transformPoint(mousePos);
    float scaleX = static_cast<float>(m_canvas.getCanvasSize().x) / m_canvas.getDrawArea().width;
    float scaleY = static_cast<float>(m_canvas.getCanvasSize().y) / m_canvas.getDrawArea().height;
    sf::Vector2f trueCanvasPos(
        (logicalPos.x - m_canvas.getDrawArea().left) * scaleX,
        (logicalPos.y - m_canvas.getDrawArea().top) * scaleY
    );

    if (event.type == sf::Event::MouseMoved && m_isDraggingText) {
        TextObject* editingText = m_tm.getEditingText();
        if (editingText) {
            editingText->position = m_textDragStartPos + (trueCanvasPos - m_textDragStartMouse);
        }
        return;
    }

    TextObject* editingText = m_tm.getEditingText();

    if (event.type == sf::Event::TextEntered && editingText) {
        if (event.text.unicode == '\b') {
            if (!editingText->text.isEmpty()) {
                m_tm.saveUndoState(m_timeline.getCurrentFrame());
                editingText->text.erase(editingText->text.getSize() - 1, 1);
            }
        }
        else if (event.text.unicode >= 32 && event.text.unicode != 127) {
            m_tm.saveUndoState(m_timeline.getCurrentFrame());
            editingText->text += event.text.unicode;
        }
        return;
    }

    if (event.type == sf::Event::KeyPressed && editingText) {
        if (event.key.code == sf::Keyboard::Enter || event.key.code == sf::Keyboard::Escape) {
            commitText();
            return;
        }
    }


    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {

        float scaleX = static_cast<float>(m_canvas.getCanvasSize().x) / m_canvas.getDrawArea().width;
        float scaleY = static_cast<float>(m_canvas.getCanvasSize().y) / m_canvas.getDrawArea().height;

        sf::Vector2f trueCanvasPos;
        trueCanvasPos.x = (logicalPos.x - m_canvas.getDrawArea().left) * scaleX;
        trueCanvasPos.y = (logicalPos.y - m_canvas.getDrawArea().top) * scaleY;

        if (trueCanvasPos.x < 0 || trueCanvasPos.x > m_canvas.getCanvasSize().x ||
            trueCanvasPos.y < 0 || trueCanvasPos.y > m_canvas.getCanvasSize().y) {
            m_tm.clearEditingState();
            return;
        }

        if (m_canvas.getDrawArea().contains(logicalPos)) {
            std::string hitId = m_tm.hitTest(m_timeline.getCurrentFrame(), m_canvas.getActiveLayer(), trueCanvasPos);

            if (!hitId.empty()) {
                commitText();
                TextObject* hit = m_tm.getText(m_timeline.getCurrentFrame(), hitId);
                if (hit) {
                    hit->isEditing = true;
                    m_canvas.setActiveLayer(hit->layerIndex, m_timeline.getCurrentFrame());
                    m_isDraggingText = true;
                    m_textDragStartMouse = trueCanvasPos;
                    m_textDragStartPos = hit->position;
                }
            }
            else {
                commitText();
                std::string newId = m_tm.createText(m_timeline.getCurrentFrame(), m_canvas.getActiveLayer(), trueCanvasPos);
                TextObject* newObj = m_tm.getText(m_timeline.getCurrentFrame(), newId);
                if (newObj) {
                    newObj->color = m_canvas.getPrimaryColor();
                }
            }
        }
    }
}

void TextTool::Update(float deltaTime, const sf::RenderWindow& window) {
    m_canvas.updateTransform(deltaTime, m_bounds);

    sf::Color currentPrimary = m_canvas.getPrimaryColor();
    if (currentPrimary != m_lastPrimaryColor) {
        TextObject* editingText = m_tm.getEditingText();
        if (editingText) {
            m_tm.saveUndoState(m_timeline.getCurrentFrame());
            editingText->color = currentPrimary;
        }
        m_lastPrimaryColor = currentPrimary;
    }
}

void TextTool::Render(sf::RenderWindow& window) {
    sf::RenderStates canvasStates;
    canvasStates.transform = m_canvas.getTransform();
    m_canvas.draw(window, m_timeline.getCurrentFrame(), m_timeline.isPlaying(), canvasStates);
}