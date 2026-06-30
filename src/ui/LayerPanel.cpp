#include "LayerPanel.h"
#include <algorithm>
#include <iostream>

LayerPanel::LayerPanel() : scrollOffset(0.f), maxScroll(0.f), renamingLayerIndex(-1), draggedLayerIndex(-1), isDragging(false), currentX(1920.f), targetX(1920.f), width(280.f), state(LayerPanelState::Hidden) {}

void LayerPanel::init() {
    font.loadFromFile("assets/font.otf");

    background.setFillColor(sf::Color(25, 25, 30, 240));
    background.setOutlineThickness(1.f);
    background.setOutlineColor(sf::Color(255, 255, 255, 30));

    handleBg.setFillColor(sf::Color(40, 40, 45, 200));
    handleBg.setOutlineThickness(1.f);
    handleBg.setOutlineColor(sf::Color(255, 255, 255, 30));

    handleText.setFont(font);
    handleText.setString("< L");
    handleText.setCharacterSize(14);
    handleText.setFillColor(sf::Color(200, 200, 200));

    headerBg.setFillColor(sf::Color(40, 40, 45, 255));
    headerText.setFont(font);
    headerText.setString("LAYERS");
    headerText.setCharacterSize(14);
    headerText.setFillColor(sf::Color(200, 200, 200));

    pinBtn.setSize(sf::Vector2f(45.f, 24.f));
    pinBtn.setFillColor(sf::Color(50, 50, 60, 255));
    pinText.setFont(font);
    pinText.setString("Pin");
    pinText.setCharacterSize(12);
    pinText.setFillColor(sf::Color::White);

    auto setupBtn = [&](sf::RectangleShape& r, sf::Text& t, std::string str, float w) {
        r.setSize(sf::Vector2f(w, 24.f));
        r.setFillColor(sf::Color(50, 50, 60, 255));
        t.setFont(font);
        t.setString(str);
        t.setCharacterSize(12);
        t.setFillColor(sf::Color::White);
        };
    setupBtn(addBtn, addText, "+", 25.f);
    setupBtn(dupBtn, dupText, "D", 25.f);
    setupBtn(delBtn, delText, "-", 25.f);
    setupBtn(mergeDownBtn, mergeDownText, "Mv", 25.f);
    setupBtn(mergeVisBtn, mergeVisText, "M*", 25.f);
    setupBtn(pushBtn, pushText, ">>", 25.f);

    renameBox.setFillColor(sf::Color(10, 10, 15, 255));
    renameBox.setOutlineThickness(1.f);
    renameBox.setOutlineColor(sf::Color(0, 122, 204));
    renameText.setFont(font);
    renameText.setCharacterSize(12);
    renameText.setFillColor(sf::Color::White);
}

void LayerPanel::update(float dt, bool focusMode) {
    if (focusMode) targetX = 1920.f;
    else targetX = (state == LayerPanelState::Pinned || state == LayerPanelState::Visible) ? 1920.f - width : 1920.f;

    currentX += (targetX - currentX) * 15.f * dt;

    background.setPosition(currentX, 0.f);
    background.setSize(sf::Vector2f(width, 1080.f));

    handleBg.setPosition(currentX - 24.f, 350.f);
    handleBg.setSize(sf::Vector2f(24.f, 80.f));
    handleText.setPosition(currentX - 20.f, 380.f);

    headerBg.setPosition(currentX, 0.f);
    headerBg.setSize(sf::Vector2f(width, 30.f));
    headerText.setPosition(currentX + 10.f, 5.f);

    pinBtn.setPosition(currentX + 5.f, 40.f);
    pinText.setPosition(currentX + 15.f, 43.f);

    if (state == LayerPanelState::Pinned) {
        handleText.setString("x");
        pinText.setString("Unpin");
    }
    else {
        handleText.setString("< L");
        pinText.setString("Pin");
    }

    pushBtn.setPosition(currentX + width - 180.f, 40.f);
    pushText.setPosition(currentX + width - 175.f, 43.f);

    addBtn.setPosition(currentX + width - 150.f, 40.f);
    addText.setPosition(currentX + width - 142.f, 43.f);

    dupBtn.setPosition(currentX + width - 120.f, 40.f);
    dupText.setPosition(currentX + width - 112.f, 43.f);

    delBtn.setPosition(currentX + width - 90.f, 40.f);
    delText.setPosition(currentX + width - 83.f, 43.f);

    mergeDownBtn.setPosition(currentX + width - 60.f, 40.f);
    mergeDownText.setPosition(currentX + width - 55.f, 43.f);

    mergeVisBtn.setPosition(currentX + width - 30.f, 40.f);
    mergeVisText.setPosition(currentX + width - 25.f, 43.f);
}

void LayerPanel::updateHover(sf::Vector2f mousePos, bool canOpen) {
    bool inPanel = background.getGlobalBounds().contains(mousePos);
    bool inHandle = handleBg.getGlobalBounds().contains(mousePos);

    if (state == LayerPanelState::Hidden) {
        if (canOpen && inHandle) state = LayerPanelState::Visible;
    }
    else if (state == LayerPanelState::Visible) {
        if (!inPanel && !inHandle) state = LayerPanelState::Hidden;
    }
}

bool LayerPanel::isPanelPinned() const { return state == LayerPanelState::Pinned; }
void LayerPanel::forceClose() { if (state != LayerPanelState::Pinned) state = LayerPanelState::Hidden; }
bool LayerPanel::isHovered() const { return state == LayerPanelState::Visible; }
float LayerPanel::getCurrentX() const { return currentX; }
bool LayerPanel::isOpen() const { return state != LayerPanelState::Hidden; }
sf::FloatRect LayerPanel::getHandleBounds() const { return handleBg.getGlobalBounds(); }

sf::Color LayerPanel::getTagColor(int tagId) const {
    switch (tagId) {
    case 1: return sf::Color(255, 50, 50);
    case 2: return sf::Color(50, 100, 255);
    case 3: return sf::Color(50, 200, 50);
    case 4: return sf::Color(255, 200, 50);
    case 5: return sf::Color(150, 50, 200);
    case 6: return sf::Color(255, 120, 0);
    default: return sf::Color(100, 100, 110);
    }
}

void LayerPanel::draw(sf::RenderWindow& window, Canvas& canvas, int currentFrame) {
    window.draw(background);
    if (state != LayerPanelState::Pinned) {
        window.draw(handleBg);
        window.draw(handleText);
    }

    window.draw(headerBg);
    window.draw(headerText);

    auto styleBtn = [&](sf::RectangleShape& r, sf::Text& t) {
        r.setFillColor(r.getGlobalBounds().contains(sf::Vector2f(sf::Mouse::getPosition(window))) ? sf::Color(80, 80, 90) : sf::Color(50, 50, 60));
        window.draw(r); window.draw(t);
        };

    if (state == LayerPanelState::Pinned) {
        pinBtn.setOutlineThickness(1.f);
        pinBtn.setOutlineColor(sf::Color(0, 191, 255));
        pinText.setFillColor(sf::Color(0, 191, 255));
    }
    else {
        pinBtn.setOutlineThickness(0.f);
        pinText.setFillColor(sf::Color::White);
    }

    styleBtn(pinBtn, pinText);
    styleBtn(pushBtn, pushText);
    styleBtn(addBtn, addText);
    styleBtn(dupBtn, dupText);
    styleBtn(delBtn, delText);
    styleBtn(mergeDownBtn, mergeDownText);
    styleBtn(mergeVisBtn, mergeVisText);

    const Frame* frame = canvas.getFrameReadOnly(currentFrame);
    if (!frame) return;

    rowCache.clear();
    rowCache.resize(frame->layers.size());

    float rowHeight = 45.f;
    float startY = 80.f;

    if (isDragging && !sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
        isDragging = false;
        float my = static_cast<float>(sf::Mouse::getPosition(window).y);
        if (draggedLayerIndex != -1) {
            for (int i = 0; i < static_cast<int>(rowCache.size()); ++i) {
                float checkY = startY + (rowCache.size() - 1 - i) * rowHeight - scrollOffset;
                if (my >= checkY && my <= checkY + rowHeight) {
                    if (i != draggedLayerIndex) {
                        canvas.moveLayer(currentFrame, draggedLayerIndex, i);
                    }
                    break;
                }
            }
        }
        draggedLayerIndex = -1;
    }

    sf::View oldView = window.getView();
    sf::View panelView(sf::FloatRect(currentX, startY, width, 1080.f - startY));
    panelView.setViewport(sf::FloatRect(currentX / window.getSize().x, startY / window.getSize().y, width / window.getSize().x, (1080.f - startY) / window.getSize().y));
    window.setView(panelView);

    for (int i = static_cast<int>(frame->layers.size()) - 1; i >= 0; --i) {
        float curY = startY + (frame->layers.size() - 1 - i) * rowHeight - scrollOffset;
        if (isDragging && draggedLayerIndex == i) curY = sf::Mouse::getPosition(window).y - 20.f;

        rowCache[i].bounds = sf::FloatRect(currentX, curY, width, rowHeight);
        rowCache[i].colorTagBounds = sf::FloatRect(currentX + 2.f, curY + 1.f, 6.f, rowHeight - 2.f);
        rowCache[i].eyeBounds = sf::FloatRect(currentX + 12.f, curY + 10.f, 20.f, 20.f);
        rowCache[i].lockBounds = sf::FloatRect(currentX + 37.f, curY + 10.f, 20.f, 20.f);
        rowCache[i].persistBounds = sf::FloatRect(currentX + 62.f, curY + 10.f, 20.f, 20.f);
        rowCache[i].nameBounds = sf::FloatRect(currentX + 125.f, curY + 5.f, 150.f, 20.f);
        rowCache[i].opacityBounds = sf::FloatRect(currentX + 125.f, curY + 28.f, 60.f, 10.f);
        rowCache[i].blendBounds = sf::FloatRect(currentX + 195.f, curY + 25.f, 80.f, 15.f);

        sf::RectangleShape rowBg(sf::Vector2f(width - 4.f, rowHeight - 2.f));
        rowBg.setPosition(currentX + 2.f, curY + 1.f);
        if (i == canvas.getActiveLayer()) rowBg.setFillColor(sf::Color(0, 122, 204, 150));
        else rowBg.setFillColor(sf::Color(35, 35, 40, 200));
        window.draw(rowBg);

        sf::RectangleShape colorTag(sf::Vector2f(rowCache[i].colorTagBounds.width, rowCache[i].colorTagBounds.height));
        colorTag.setPosition(rowCache[i].colorTagBounds.left, rowCache[i].colorTagBounds.top);
        colorTag.setFillColor(getTagColor(frame->layers[i].colorTag));
        window.draw(colorTag);

        auto drawToggle = [&](sf::FloatRect r, bool toggleState, std::string label, sf::Color activeCol) {
            sf::RectangleShape toggle(sf::Vector2f(16.f, 16.f));
            toggle.setPosition(r.left + 2.f, r.top + 2.f);
            toggle.setFillColor(toggleState ? activeCol : sf::Color(60, 60, 70));
            sf::Text t(label, font, 10);
            t.setPosition(r.left + 4.f, r.top + 2.f);
            t.setFillColor(sf::Color::White);
            window.draw(toggle); window.draw(t);
            };

        drawToggle(rowCache[i].eyeBounds, frame->layers[i].visible, "O", sf::Color(200, 200, 200));
        drawToggle(rowCache[i].lockBounds, frame->layers[i].locked, "L", sf::Color(200, 50, 50));
        drawToggle(rowCache[i].persistBounds, frame->layers[i].persistent, "P", sf::Color(255, 150, 0));

        sf::RectangleShape check1(sf::Vector2f(17.f, 17.f)); check1.setFillColor(sf::Color(100, 100, 100));
        sf::RectangleShape check2(sf::Vector2f(17.f, 17.f)); check2.setFillColor(sf::Color(150, 150, 150));
        float tX = currentX + 85.f; float tY = curY + 5.f;
        check1.setPosition(tX, tY); window.draw(check1);
        check2.setPosition(tX + 17.f, tY); window.draw(check2);
        check2.setPosition(tX, tY + 17.f); window.draw(check2);
        check1.setPosition(tX + 17.f, tY + 17.f); window.draw(check1);

        sf::RectangleShape thumb(sf::Vector2f(34.f, 34.f));
        thumb.setPosition(tX, tY);
        thumb.setTexture(&frame->layers[i].texture->getTexture());
        thumb.setOutlineThickness(1.f);
        thumb.setOutlineColor(sf::Color(255, 255, 255, 50));
        window.draw(thumb);

        if (renamingLayerIndex == i) {
            renameBox.setPosition(rowCache[i].nameBounds.left, rowCache[i].nameBounds.top);
            renameBox.setSize(sf::Vector2f(rowCache[i].nameBounds.width, rowCache[i].nameBounds.height));
            renameText.setString(renameBuffer + "_");
            renameText.setPosition(renameBox.getPosition().x + 2.f, renameBox.getPosition().y + 2.f);
            window.draw(renameBox);
            window.draw(renameText);
        }
        else {
            sf::Text nText(frame->layers[i].name, font, 12);
            nText.setPosition(rowCache[i].nameBounds.left, rowCache[i].nameBounds.top + 2.f);
            nText.setFillColor(sf::Color::White);
            window.draw(nText);
        }

        sf::RectangleShape opTrack(sf::Vector2f(rowCache[i].opacityBounds.width, rowCache[i].opacityBounds.height));
        opTrack.setPosition(rowCache[i].opacityBounds.left, rowCache[i].opacityBounds.top);
        opTrack.setFillColor(sf::Color(50, 50, 60));
        sf::RectangleShape opFill(sf::Vector2f(rowCache[i].opacityBounds.width * frame->layers[i].opacity, rowCache[i].opacityBounds.height));
        opFill.setPosition(rowCache[i].opacityBounds.left, rowCache[i].opacityBounds.top);
        opFill.setFillColor(sf::Color(100, 150, 255));
        window.draw(opTrack); window.draw(opFill);

        std::string modeStr = "Norm";
        if (frame->layers[i].blendMode == BlendMode::Multiply) modeStr = "Mult";
        else if (frame->layers[i].blendMode == BlendMode::Additive) modeStr = "Add";
        else if (frame->layers[i].blendMode == BlendMode::Screen) modeStr = "Scrn";
        else if (frame->layers[i].blendMode == BlendMode::Overlay) modeStr = "Ovrl";
        sf::Text bText(modeStr, font, 10);
        bText.setPosition(rowCache[i].blendBounds.left, rowCache[i].blendBounds.top);
        bText.setFillColor(sf::Color(150, 150, 150));
        window.draw(bText);
    }

    maxScroll = std::max(0.f, static_cast<float>(frame->layers.size()) * rowHeight - (1080.f - startY));
    window.setView(oldView);
}

std::string LayerPanel::processClick(sf::Vector2f mousePos, Canvas& canvas, int currentFrame) {
    if (pinBtn.getGlobalBounds().contains(mousePos)) {
        state = (state == LayerPanelState::Pinned) ? LayerPanelState::Visible : LayerPanelState::Pinned;
        return "layer_pin";
    }
    if (state == LayerPanelState::Hidden && handleBg.getGlobalBounds().contains(mousePos)) {
        state = LayerPanelState::Pinned;
        return "handle_click";
    }

    if (pushBtn.getGlobalBounds().contains(mousePos)) return "layer_push";
    if (addBtn.getGlobalBounds().contains(mousePos)) { canvas.addLayer(currentFrame); return "layer_add"; }
    if (dupBtn.getGlobalBounds().contains(mousePos)) { canvas.duplicateLayer(currentFrame, canvas.getActiveLayer()); return "layer_dup"; }
    if (delBtn.getGlobalBounds().contains(mousePos)) { canvas.deleteLayer(currentFrame, canvas.getActiveLayer()); return "layer_del"; }
    if (mergeDownBtn.getGlobalBounds().contains(mousePos)) { canvas.mergeDown(currentFrame); return "layer_merge_d"; }
    if (mergeVisBtn.getGlobalBounds().contains(mousePos)) { canvas.mergeVisible(currentFrame); return "layer_merge_v"; }

    sf::Vector2f localMouse(mousePos.x, mousePos.y + scrollOffset);
    for (int i = 0; i < static_cast<int>(rowCache.size()); ++i) {
        const Frame* f = canvas.getFrameReadOnly(currentFrame);
        if (!f) break;

        sf::FloatRect adjustedBounds = rowCache[i].bounds;
        adjustedBounds.top += scrollOffset;

        if (adjustedBounds.contains(localMouse)) {
            if (sf::FloatRect(rowCache[i].colorTagBounds.left, rowCache[i].colorTagBounds.top + scrollOffset, rowCache[i].colorTagBounds.width, rowCache[i].colorTagBounds.height).contains(localMouse)) {
                canvas.cycleLayerColorTag(currentFrame, i);
                return "layer_tag";
            }
            if (sf::FloatRect(rowCache[i].eyeBounds.left, rowCache[i].eyeBounds.top + scrollOffset, rowCache[i].eyeBounds.width, rowCache[i].eyeBounds.height).contains(localMouse)) {
                canvas.setLayerProperties(currentFrame, i, f->layers[i].name, !f->layers[i].visible, f->layers[i].locked, f->layers[i].opacity, f->layers[i].blendMode);
                return "layer_vis";
            }
            if (sf::FloatRect(rowCache[i].lockBounds.left, rowCache[i].lockBounds.top + scrollOffset, rowCache[i].lockBounds.width, rowCache[i].lockBounds.height).contains(localMouse)) {
                canvas.setLayerProperties(currentFrame, i, f->layers[i].name, f->layers[i].visible, !f->layers[i].locked, f->layers[i].opacity, f->layers[i].blendMode);
                return "layer_lock";
            }
            if (sf::FloatRect(rowCache[i].persistBounds.left, rowCache[i].persistBounds.top + scrollOffset, rowCache[i].persistBounds.width, rowCache[i].persistBounds.height).contains(localMouse)) {
                canvas.toggleLayerPersistence(currentFrame, i);
                return "layer_persist";
            }
            if (sf::FloatRect(rowCache[i].opacityBounds.left, rowCache[i].opacityBounds.top + scrollOffset, rowCache[i].opacityBounds.width, rowCache[i].opacityBounds.height).contains(localMouse)) {
                float newOp = std::clamp((localMouse.x - rowCache[i].opacityBounds.left) / rowCache[i].opacityBounds.width, 0.f, 1.f);
                canvas.setLayerProperties(currentFrame, i, f->layers[i].name, f->layers[i].visible, f->layers[i].locked, newOp, f->layers[i].blendMode);
                return "layer_op";
            }
            if (sf::FloatRect(rowCache[i].blendBounds.left, rowCache[i].blendBounds.top + scrollOffset, rowCache[i].blendBounds.width, rowCache[i].blendBounds.height).contains(localMouse)) {
                int b = static_cast<int>(f->layers[i].blendMode) + 1;
                if (b > 4) b = 0;
                canvas.setLayerProperties(currentFrame, i, f->layers[i].name, f->layers[i].visible, f->layers[i].locked, f->layers[i].opacity, static_cast<BlendMode>(b));
                return "layer_blend";
            }
            if (sf::FloatRect(rowCache[i].nameBounds.left, rowCache[i].nameBounds.top + scrollOffset, rowCache[i].nameBounds.width, rowCache[i].nameBounds.height).contains(localMouse)) {
                renamingLayerIndex = i;
                renameBuffer = f->layers[i].name;
                canvas.setActiveLayer(i);
                return "layer_rename";
            }
            canvas.setActiveLayer(i);
            draggedLayerIndex = i;
            dragStartPos = mousePos;
            isDragging = true;
            return "layer_select";
        }
    }
    return "";
}

bool LayerPanel::handleClick(sf::Vector2f mousePos, Canvas& canvas, int currentFrame) {
    return !processClick(mousePos, canvas, currentFrame).empty();
}

bool LayerPanel::handleEvent(const sf::Event& event, sf::Vector2f mousePos, Canvas& canvas, int currentFrame) {
    if (renamingLayerIndex != -1) {
        if (event.type == sf::Event::TextEntered) {
            if (event.text.unicode == '\b' && !renameBuffer.empty()) renameBuffer.pop_back();
            else if (event.text.unicode < 128 && event.text.unicode != '\r' && event.text.unicode != '\n' && event.text.unicode != '\b') renameBuffer += static_cast<char>(event.text.unicode);
            return true;
        }
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter) {
            const Frame* f = canvas.getFrameReadOnly(currentFrame);
            if (f && renamingLayerIndex < static_cast<int>(f->layers.size())) {
                canvas.setLayerProperties(currentFrame, renamingLayerIndex, renameBuffer, f->layers[renamingLayerIndex].visible, f->layers[renamingLayerIndex].locked, f->layers[renamingLayerIndex].opacity, f->layers[renamingLayerIndex].blendMode);
            }
            renamingLayerIndex = -1;
            return true;
        }
    }

    if (event.type == sf::Event::MouseWheelScrolled && (background.getGlobalBounds().contains(mousePos) || handleBg.getGlobalBounds().contains(mousePos))) {
        scrollOffset -= event.mouseWheelScroll.delta * 20.f;
        scrollOffset = std::clamp(scrollOffset, 0.f, maxScroll);
        return true;
    }
    return false;
}