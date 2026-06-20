#include "LayerPanel.h"

LayerPanel::LayerPanel() : width(220.f), currentX(1920.f), targetX(1920.f), state(LayerPanelState::Hidden) {}

void LayerPanel::init() {
    font.loadFromFile("assets/font.otf");

    background.setSize(sf::Vector2f(width, 450.f));
    background.setFillColor(sf::Color(15, 15, 18, 220));
    background.setOutlineThickness(1.f);
    background.setOutlineColor(sf::Color(255, 255, 255, 15));

    handleBg.setSize(sf::Vector2f(24.f, 80.f));
    handleBg.setFillColor(sf::Color(30, 30, 35, 200));
    handleBg.setOutlineThickness(1.f);
    handleBg.setOutlineColor(sf::Color(255, 255, 255, 30));

    handleLabel.setFont(font);
    handleLabel.setString("<");
    handleLabel.setCharacterSize(16);
    handleLabel.setFillColor(sf::Color(200, 200, 200));

    pinBtn.setSize(sf::Vector2f(width - 40.f, 24.f));
    pinBtn.setFillColor(sf::Color(255, 255, 255, 10));

    pinLabel.setFont(font);
    pinLabel.setString("Layers");
    pinLabel.setCharacterSize(12);
    pinLabel.setFillColor(sf::Color(180, 180, 180));

    addBtn.setSize(sf::Vector2f(40.f, 24.f)); addBtn.setFillColor(sf::Color(0, 122, 204));
    dupBtn.setSize(sf::Vector2f(40.f, 24.f)); dupBtn.setFillColor(sf::Color(80, 80, 80));
    delBtn.setSize(sf::Vector2f(40.f, 24.f)); delBtn.setFillColor(sf::Color(200, 50, 50));
}

void LayerPanel::update(float dt, bool focusMode) {
    if (focusMode) targetX = 1920.f;
    else targetX = (state == LayerPanelState::Pinned || state == LayerPanelState::Visible) ? 1920.f - width : 1920.f;

    currentX += (targetX - currentX) * 15.0f * dt;
    background.setPosition(currentX, 150.f);

    handleBg.setPosition(currentX - 24.f, 200.f);
    handleLabel.setPosition(currentX - 18.f, 230.f);

    if (state == LayerPanelState::Pinned) {
        handleLabel.setString("x");
        pinLabel.setString("Unpin Layers");
        pinLabel.setFillColor(sf::Color(0, 191, 255));
    }
    else {
        handleLabel.setString("<");
        pinLabel.setString("Pin Layers");
        pinLabel.setFillColor(sf::Color(180, 180, 180));
    }

    pinBtn.setPosition(currentX + 20.f, 160.f);
    pinLabel.setPosition(currentX + (width / 2.f) - 30.f, 164.f);

    addBtn.setPosition(currentX + 20.f, 550.f);
    dupBtn.setPosition(currentX + 70.f, 550.f);
    delBtn.setPosition(currentX + 120.f, 550.f);
}

void LayerPanel::updateHover(sf::Vector2f mousePos) {
    bool inPanel = background.getGlobalBounds().contains(mousePos);
    bool inHandle = handleBg.getGlobalBounds().contains(mousePos);

    if (state == LayerPanelState::Hidden && inHandle) state = LayerPanelState::Visible;
    else if (state == LayerPanelState::Visible && !inPanel && !inHandle) state = LayerPanelState::Hidden;
}

void LayerPanel::draw(sf::RenderWindow& window, Canvas& canvas, int currentFrame) {
    window.draw(background);
    if (state != LayerPanelState::Pinned) {
        window.draw(handleBg);
        window.draw(handleLabel);
    }
    window.draw(pinBtn);
    window.draw(pinLabel);

    window.draw(addBtn);
    window.draw(dupBtn);
    window.draw(delBtn);

    sf::Text btnText;
    btnText.setFont(font);
    btnText.setCharacterSize(12);
    btnText.setFillColor(sf::Color::White);

    btnText.setString("+"); btnText.setPosition(addBtn.getPosition().x + 15.f, addBtn.getPosition().y + 4.f); window.draw(btnText);
    btnText.setString("D"); btnText.setPosition(dupBtn.getPosition().x + 15.f, dupBtn.getPosition().y + 4.f); window.draw(btnText);
    btnText.setString("X"); btnText.setPosition(delBtn.getPosition().x + 15.f, delBtn.getPosition().y + 4.f); window.draw(btnText);

    const Frame* frame = canvas.getFrameReadOnly(currentFrame);
    if (!frame) return;

    float y = 200.f;
    for (int i = static_cast<int>(frame->layers.size()) - 1; i >= 0; --i) {
        const auto& layer = frame->layers[i];

        sf::RectangleShape lBox(sf::Vector2f(width - 40.f, 30.f));
        lBox.setPosition(currentX + 20.f, y);
        lBox.setFillColor(i == canvas.getActiveLayer() ? sf::Color(0, 122, 204, 150) : sf::Color(255, 255, 255, 10));
        window.draw(lBox);

        sf::Text lName; lName.setFont(font); lName.setString(layer.name); lName.setCharacterSize(12); lName.setFillColor(sf::Color::White);
        lName.setPosition(currentX + 30.f, y + 8.f);
        window.draw(lName);

        sf::Text lVis; lVis.setFont(font); lVis.setString(layer.visible ? "[O]" : "[-]"); lVis.setCharacterSize(12); lVis.setFillColor(layer.visible ? sf::Color::Green : sf::Color(100, 100, 100));
        lVis.setPosition(currentX + width - 60.f, y + 8.f);
        window.draw(lVis);

        y += 35.f;
    }
}

bool LayerPanel::handleClick(sf::Vector2f mousePos, Canvas& canvas, int currentFrame) {
    if (pinBtn.getGlobalBounds().contains(mousePos)) {
        state = (state == LayerPanelState::Pinned) ? LayerPanelState::Visible : LayerPanelState::Pinned;
        return true;
    }

    if (state == LayerPanelState::Hidden && handleBg.getGlobalBounds().contains(mousePos)) {
        state = LayerPanelState::Pinned;
        return true;
    }

    if (addBtn.getGlobalBounds().contains(mousePos)) { canvas.addLayer(currentFrame, "New Layer"); return true; }
    if (dupBtn.getGlobalBounds().contains(mousePos)) { canvas.duplicateLayer(currentFrame, canvas.getActiveLayer()); return true; }
    if (delBtn.getGlobalBounds().contains(mousePos)) { canvas.deleteLayer(currentFrame, canvas.getActiveLayer()); return true; }

    const Frame* frame = canvas.getFrameReadOnly(currentFrame);
    if (!frame) return false;

    float y = 200.f;
    for (int i = static_cast<int>(frame->layers.size()) - 1; i >= 0; --i) {
        sf::FloatRect lBox(currentX + 20.f, y, width - 40.f, 30.f);
        sf::FloatRect visBox(currentX + width - 60.f, y, 20.f, 30.f);

        if (visBox.contains(mousePos)) {
            const auto& l = frame->layers[i];
            canvas.setLayerProperties(currentFrame, i, l.name, !l.visible, l.locked, l.opacity, l.blendMode);
            return true;
        }
        else if (lBox.contains(mousePos)) {
            canvas.setActiveLayer(i);
            return true;
        }
        y += 35.f;
    }
    return false;
}

float LayerPanel::getCurrentX() const { return currentX; }
void LayerPanel::forceClose() { if (state != LayerPanelState::Pinned) state = LayerPanelState::Hidden; }
bool LayerPanel::isHovered() const { return state == LayerPanelState::Visible; }
bool LayerPanel::isPanelPinned() const { return state == LayerPanelState::Pinned; }