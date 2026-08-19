#include "LayerPanel.h"
#include "../UI/UITheme.h"
#include <algorithm>
#include <iostream>

LayerPanel::LayerPanel() : scrollOffset(0.f), maxScroll(0.f), renamingLayerIndex(-1), draggedLayerIndex(-1), isDragging(false), currentX(1920.f), targetX(1920.f), width(280.f), state(LayerPanelState::Hidden) {}

void LayerPanel::init() {
    font.loadFromFile("assets/font.otf");

    background.setFillColor(WisdomUI::Theme::Panel);
    background.setOutlineThickness(1.f);
    background.setOutlineColor(WisdomUI::Theme::Border);

    headerBg.setFillColor(WisdomUI::Theme::PanelInset);
    headerText.setFont(font);
    headerText.setString("LAYERS");
    headerText.setCharacterSize(13);
    headerText.setFillColor(WisdomUI::Theme::Gold);

    closeBtn.setSize(sf::Vector2f(22.f, 22.f));
    closeBtn.setFillColor(WisdomUI::Theme::PanelInset);
    closeBtn.setOutlineThickness(1.f);
    closeBtn.setOutlineColor(WisdomUI::Theme::Border);
    closeText.setFont(font);
    closeText.setString("X");
    closeText.setCharacterSize(11);
    closeText.setFillColor(WisdomUI::Theme::TextSecondary);

    pinBtn.setSize(sf::Vector2f(42.f, 22.f));
    pinBtn.setFillColor(WisdomUI::Theme::PanelInset);
    pinBtn.setOutlineThickness(1.f);
    pinBtn.setOutlineColor(WisdomUI::Theme::Border);
    pinText.setFont(font);
    pinText.setString("Pin");
    pinText.setCharacterSize(11);
    pinText.setFillColor(WisdomUI::Theme::TextSecondary);

    auto setupBtn = [&](sf::RectangleShape& r, sf::Text& t, std::string str, float w) {
        r.setSize(sf::Vector2f(w, 22.f));
        r.setFillColor(WisdomUI::Theme::PanelInset);
        r.setOutlineThickness(1.f);
        r.setOutlineColor(WisdomUI::Theme::Border);
        t.setFont(font);
        t.setString(str);
        t.setCharacterSize(11);
        t.setFillColor(WisdomUI::Theme::Gold);
        };

    setupBtn(addBtn, addText, "+", 24.f);
    setupBtn(dupBtn, dupText, "D", 24.f);
    setupBtn(delBtn, delText, "-", 24.f);
    setupBtn(mergeDownBtn, mergeDownText, "Mv", 26.f);
    setupBtn(mergeVisBtn, mergeVisText, "M*", 26.f);
    setupBtn(pushBtn, pushText, ">>", 24.f);

    renameBox.setFillColor(WisdomUI::Theme::PanelInset);
    renameBox.setOutlineThickness(1.f);
    renameBox.setOutlineColor(WisdomUI::Theme::BorderHighlight);
    renameText.setFont(font);
    renameText.setCharacterSize(12);
    renameText.setFillColor(sf::Color::White);
}

void LayerPanel::update(float dt, bool focusMode, bool isOpen) {
    if (focusMode || !isOpen) targetX = 1920.f;
    else targetX = 1920.f - 44.f - width;

    currentX += (targetX - currentX) * 16.f * dt;

    background.setPosition(currentX, 36.f + 32.f);
    background.setSize(sf::Vector2f(width, 1080.f - (36.f + 32.f + 24.f)));

    headerBg.setPosition(currentX, 36.f + 32.f);
    headerBg.setSize(sf::Vector2f(width, 32.f));
    headerText.setPosition(currentX + 12.f, 36.f + 32.f + 7.f);

    closeBtn.setPosition(currentX + width - 30.f, 36.f + 32.f + 5.f);
    closeText.setPosition(currentX + width - 23.f, 36.f + 32.f + 7.f);

    float actionY = 36.f + 32.f + 38.f;
    pinBtn.setPosition(currentX + 8.f, actionY);
    pinText.setPosition(currentX + 18.f, actionY + 3.f);

    pushBtn.setPosition(currentX + width - 170.f, actionY);
    pushText.setPosition(currentX + width - 165.f, actionY + 3.f);

    addBtn.setPosition(currentX + width - 142.f, actionY);
    addText.setPosition(currentX + width - 135.f, actionY + 3.f);

    dupBtn.setPosition(currentX + width - 114.f, actionY);
    dupText.setPosition(currentX + width - 107.f, actionY + 3.f);

    delBtn.setPosition(currentX + width - 86.f, actionY);
    delText.setPosition(currentX + width - 79.f, actionY + 3.f);

    mergeDownBtn.setPosition(currentX + width - 58.f, actionY);
    mergeDownText.setPosition(currentX + width - 53.f, actionY + 3.f);

    mergeVisBtn.setPosition(currentX + width - 28.f, actionY);
    mergeVisText.setPosition(currentX + width - 23.f, actionY + 3.f);
}

void LayerPanel::updateHover(sf::Vector2f mousePos, bool canOpen) {
    bool inPanel = background.getGlobalBounds().contains(mousePos);
    if (state == LayerPanelState::Hidden) {
        if (canOpen && inPanel) state = LayerPanelState::Visible;
    }
    else if (state == LayerPanelState::Visible) {
        if (!inPanel) state = LayerPanelState::Hidden;
    }
}

bool LayerPanel::isPanelPinned() const { return state == LayerPanelState::Pinned; }
void LayerPanel::forceClose() { targetX = 1920.f; }
bool LayerPanel::isHovered() const { return state == LayerPanelState::Visible; }
float LayerPanel::getCurrentX() const { return currentX; }
bool LayerPanel::isOpen() const { return currentX < 1918.f; }
sf::FloatRect LayerPanel::getHandleBounds() const { return sf::FloatRect(0, 0, 0, 0); }

sf::Color LayerPanel::getTagColor(int tagId) const {
    switch (tagId) {
    case 1: return sf::Color(255, 60, 60);
    case 2: return sf::Color(60, 140, 255);
    case 3: return sf::Color(60, 220, 80);
    case 4: return sf::Color(255, 215, 60);
    case 5: return sf::Color(180, 70, 240);
    case 6: return sf::Color(255, 140, 30);
    default: return WisdomUI::Theme::Border;
    }
}

void LayerPanel::draw(sf::RenderWindow& window, Canvas& canvas, int currentFrame) {
    if (currentX >= 1918.f) return;

    WisdomUI::Theme::DrawFiligreePanel(window, background.getGlobalBounds(), 1.0f);

    window.draw(headerBg);
    window.draw(headerText);
    window.draw(closeBtn);
    window.draw(closeText);

    auto styleBtn = [&](sf::RectangleShape& r, sf::Text& t) {
        bool hov = r.getGlobalBounds().contains(window.mapPixelToCoords(sf::Mouse::getPosition(window)));
        r.setFillColor(hov ? WisdomUI::Theme::PanelHover : WisdomUI::Theme::PanelInset);
        r.setOutlineColor(hov ? WisdomUI::Theme::BorderHighlight : WisdomUI::Theme::Border);
        window.draw(r);
        window.draw(t);
        };

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
    float startY = 36.f + 32.f + 68.f;

    if (isDragging && !sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
        isDragging = false;
        float my = static_cast<float>(window.mapPixelToCoords(sf::Mouse::getPosition(window)).y);
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

    for (int i = static_cast<int>(frame->layers.size()) - 1; i >= 0; --i) {
        float curY = startY + (frame->layers.size() - 1 - i) * rowHeight - scrollOffset;
        if (isDragging && draggedLayerIndex == i) curY = window.mapPixelToCoords(sf::Mouse::getPosition(window)).y - 20.f;

        if (curY + rowHeight < startY || curY > 1080.f - 24.f) continue;

        rowCache[i].bounds = sf::FloatRect(currentX + 4.f, curY, width - 8.f, rowHeight - 2.f);
        rowCache[i].colorTagBounds = sf::FloatRect(currentX + 6.f, curY + 2.f, 4.f, rowHeight - 6.f);
        rowCache[i].eyeBounds = sf::FloatRect(currentX + 14.f, curY + 11.f, 18.f, 18.f);
        rowCache[i].lockBounds = sf::FloatRect(currentX + 36.f, curY + 11.f, 18.f, 18.f);
        rowCache[i].persistBounds = sf::FloatRect(currentX + 58.f, curY + 11.f, 18.f, 18.f);
        rowCache[i].nameBounds = sf::FloatRect(currentX + 122.f, curY + 4.f, 145.f, 18.f);
        rowCache[i].opacityBounds = sf::FloatRect(currentX + 122.f, curY + 26.f, 65.f, 8.f);
        rowCache[i].blendBounds = sf::FloatRect(currentX + 195.f, curY + 23.f, 75.f, 14.f);

        sf::RectangleShape rowBg(sf::Vector2f(rowCache[i].bounds.width, rowCache[i].bounds.height));
        rowBg.setPosition(rowCache[i].bounds.left, rowCache[i].bounds.top);
        if (i == canvas.getActiveLayer()) {
            rowBg.setFillColor(WisdomUI::Theme::PanelHover);
            rowBg.setOutlineThickness(1.f);
            rowBg.setOutlineColor(WisdomUI::Theme::BorderHighlight);
        }
        else {
            rowBg.setFillColor(WisdomUI::Theme::PanelInset);
            rowBg.setOutlineThickness(1.f);
            rowBg.setOutlineColor(WisdomUI::Theme::Border);
        }
        window.draw(rowBg);

        sf::RectangleShape colorTag(sf::Vector2f(rowCache[i].colorTagBounds.width, rowCache[i].colorTagBounds.height));
        colorTag.setPosition(rowCache[i].colorTagBounds.left, rowCache[i].colorTagBounds.top);
        colorTag.setFillColor(getTagColor(frame->layers[i].colorTag));
        window.draw(colorTag);

        auto drawToggle = [&](sf::FloatRect r, bool toggleState, std::string label, sf::Color activeCol) {
            sf::RectangleShape toggle(sf::Vector2f(r.width, r.height));
            toggle.setPosition(r.left, r.top);
            toggle.setFillColor(toggleState ? activeCol : WisdomUI::Theme::Background);
            toggle.setOutlineThickness(1.f);
            toggle.setOutlineColor(toggleState ? WisdomUI::Theme::BorderHighlight : WisdomUI::Theme::Border);
            window.draw(toggle);

            sf::Text t(label, font, 9);
            t.setPosition(r.left + 5.f, r.top + 2.f);
            t.setFillColor(toggleState ? sf::Color::White : WisdomUI::Theme::TextMuted);
            window.draw(t);
            };

        drawToggle(rowCache[i].eyeBounds, frame->layers[i].visible, "V", WisdomUI::Theme::Accent);
        drawToggle(rowCache[i].lockBounds, frame->layers[i].locked, "L", sf::Color(190, 50, 50));
        drawToggle(rowCache[i].persistBounds, frame->layers[i].persistent, "P", WisdomUI::Theme::Gold);

        float tX = currentX + 80.f;
        float tY = curY + 4.f;

        sf::RectangleShape thumbBase(sf::Vector2f(34.f, 34.f));
        thumbBase.setPosition(tX, tY);
        thumbBase.setFillColor(sf::Color(180, 180, 180));
        thumbBase.setOutlineThickness(1.f);
        thumbBase.setOutlineColor(WisdomUI::Theme::Border);
        window.draw(thumbBase);

        if (frame->layers[i].texture) {
            sf::Sprite thumb(frame->layers[i].texture->getTexture());
            float canvasW = static_cast<float>(canvas.getCanvasSize().x);
            float canvasH = static_cast<float>(canvas.getCanvasSize().y);
            if (canvasW > 0.f && canvasH > 0.f) {
                float s = std::min(34.f / canvasW, 34.f / canvasH);
                thumb.setScale(s, s);
                thumb.setPosition(tX, tY);
                window.draw(thumb);
            }
        }

        if (renamingLayerIndex == i) {
            renameBox.setPosition(rowCache[i].nameBounds.left, rowCache[i].nameBounds.top);
            renameBox.setSize(sf::Vector2f(rowCache[i].nameBounds.width, rowCache[i].nameBounds.height));
            renameText.setString(renameBuffer + "_");
            renameText.setPosition(renameBox.getPosition().x + 2.f, renameBox.getPosition().y + 2.f);
            window.draw(renameBox);
            window.draw(renameText);
        }
        else {
            sf::Text nText(frame->layers[i].name, font, 11);
            nText.setPosition(rowCache[i].nameBounds.left, rowCache[i].nameBounds.top);
            nText.setFillColor(i == canvas.getActiveLayer() ? WisdomUI::Theme::Gold : WisdomUI::Theme::TextPrimary);
            window.draw(nText);
        }

        sf::RectangleShape opTrack(sf::Vector2f(rowCache[i].opacityBounds.width, rowCache[i].opacityBounds.height));
        opTrack.setPosition(rowCache[i].opacityBounds.left, rowCache[i].opacityBounds.top);
        opTrack.setFillColor(WisdomUI::Theme::PanelInset);
        window.draw(opTrack);

        float opVal = frame->layers[i].opacity > 1.0f ? (frame->layers[i].opacity / 255.f) : frame->layers[i].opacity;
        sf::RectangleShape opFill(sf::Vector2f(rowCache[i].opacityBounds.width * std::clamp(opVal, 0.f, 1.f), rowCache[i].opacityBounds.height));
        opFill.setPosition(rowCache[i].opacityBounds.left, rowCache[i].opacityBounds.top);
        opFill.setFillColor(WisdomUI::Theme::BorderHighlight);
        window.draw(opFill);

        std::string modeStr = "Norm";
        if (frame->layers[i].blendMode == BlendMode::Multiply) modeStr = "Mult";
        else if (frame->layers[i].blendMode == BlendMode::Additive) modeStr = "Add";
        else if (frame->layers[i].blendMode == BlendMode::Screen) modeStr = "Scrn";
        else if (frame->layers[i].blendMode == BlendMode::Overlay) modeStr = "Ovrl";
        sf::Text bText(modeStr, font, 10);
        bText.setPosition(rowCache[i].blendBounds.left, rowCache[i].blendBounds.top);
        bText.setFillColor(WisdomUI::Theme::TextSecondary);
        window.draw(bText);
    }

    maxScroll = std::max(0.f, static_cast<float>(frame->layers.size()) * rowHeight - (1080.f - startY - 24.f));
}

std::string LayerPanel::processClick(sf::Vector2f mousePos, Canvas& canvas, int currentFrame) {
    if (closeBtn.getGlobalBounds().contains(mousePos)) {
        forceClose();
        return "layer_close";
    }

    if (pinBtn.getGlobalBounds().contains(mousePos)) {
        state = (state == LayerPanelState::Pinned) ? LayerPanelState::Visible : LayerPanelState::Pinned;
        return "layer_pin";
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

    if (event.type == sf::Event::MouseWheelScrolled && background.getGlobalBounds().contains(mousePos)) {
        scrollOffset -= event.mouseWheelScroll.delta * 20.f;
        scrollOffset = std::clamp(scrollOffset, 0.f, maxScroll);
        return true;
    }
    return false;
}