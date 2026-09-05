#include "LayerPanel.h"
#include "../UI/UITheme.h"
#include <algorithm>
#include <cmath>

LayerPanel::LayerPanel()
    : scrollOffset(0.f), maxScroll(0.f), isDraggingScrollbar(false), scrollDragStartY(0.f), scrollDragStartOffset(0.f),
    renamingLayerIndex(-1), draggedLayerIndex(-1), dropVisualSlot(-1), isDragging(false),
    activeOpacityIndex(-1), isDraggingOpacity(false), lastClickedLayerIndex(-1),
    currentX(1920.f), targetX(1920.f), width(300.f), state(LayerPanelState::Hidden) {}

void LayerPanel::init() {
    font.loadFromFile("assets/font.otf");

    background.setFillColor(WisdomUI::Theme::Panel);
    background.setOutlineThickness(1.f);
    background.setOutlineColor(WisdomUI::Theme::Border);

    headerBg.setFillColor(WisdomUI::Theme::PanelInset);
    headerText.setFont(font);
    headerText.setString("LAYERS");
    headerText.setCharacterSize(12);
    headerText.setFillColor(WisdomUI::Theme::Gold);

    closeBtn.setSize(sf::Vector2f(20.f, 20.f));
    closeBtn.setFillColor(WisdomUI::Theme::PanelInset);
    closeBtn.setOutlineThickness(1.f);
    closeBtn.setOutlineColor(WisdomUI::Theme::Border);
    closeText.setFont(font);
    closeText.setString("X");
    closeText.setCharacterSize(10);
    closeText.setFillColor(WisdomUI::Theme::TextSecondary);

    pinBtn.setSize(sf::Vector2f(36.f, 20.f));
    pinBtn.setFillColor(WisdomUI::Theme::PanelInset);
    pinBtn.setOutlineThickness(1.f);
    pinBtn.setOutlineColor(WisdomUI::Theme::Border);
    pinText.setFont(font);
    pinText.setString("Pin");
    pinText.setCharacterSize(10);
    pinText.setFillColor(WisdomUI::Theme::TextSecondary);

    auto setupBtn = [&](sf::RectangleShape& r, sf::Text& t, const std::string& str, float w) {
        r.setSize(sf::Vector2f(w, 22.f));
        r.setFillColor(WisdomUI::Theme::PanelInset);
        r.setOutlineThickness(1.f);
        r.setOutlineColor(WisdomUI::Theme::Border);
        t.setFont(font);
        t.setString(str);
        t.setCharacterSize(10);
        t.setFillColor(WisdomUI::Theme::Gold);
        };

    setupBtn(addBtn, addText, "+ New", 44.f);
    setupBtn(dupBtn, dupText, "Dup", 34.f);
    setupBtn(delBtn, delText, "Del", 32.f);
    setupBtn(mergeDownBtn, mergeDownText, "Merge", 42.f);
    setupBtn(mergeVisBtn, mergeVisText, "Flat", 34.f);
    setupBtn(pushBtn, pushText, "Ext", 36.f);

    renameBox.setFillColor(WisdomUI::Theme::Background);
    renameBox.setOutlineThickness(1.f);
    renameBox.setOutlineColor(WisdomUI::Theme::Gold);
    renameText.setFont(font);
    renameText.setCharacterSize(11);
    renameText.setFillColor(sf::Color::White);
}

void LayerPanel::update(float dt, bool focusMode, bool isOpen) {
    if (focusMode || !isOpen) targetX = 1920.f;
    else targetX = 1920.f - 44.f - width;

    currentX += (targetX - currentX) * 18.f * dt;

    background.setPosition(currentX, 68.f);
    background.setSize(sf::Vector2f(width, 1080.f - 92.f));

    headerBg.setPosition(currentX, 68.f);
    headerBg.setSize(sf::Vector2f(width, 30.f));
    headerText.setPosition(currentX + 12.f, 75.f);

    closeBtn.setPosition(currentX + width - 26.f, 73.f);
    closeText.setPosition(currentX + width - 20.f, 76.f);

    pinBtn.setPosition(currentX + width - 66.f, 73.f);
    pinText.setPosition(currentX + width - 57.f, 76.f);

    float actionY = 104.f;
    float btnX = currentX + 8.f;
    float gap = 4.f;

    auto placeBtn = [&](sf::RectangleShape& r, sf::Text& t) {
        r.setPosition(btnX, actionY);
        t.setPosition(btnX + (r.getSize().x - t.getLocalBounds().width) / 2.f - 1.f, actionY + 4.f);
        btnX += r.getSize().x + gap;
        };

    placeBtn(addBtn, addText);
    placeBtn(dupBtn, dupText);
    placeBtn(delBtn, delText);
    placeBtn(mergeDownBtn, mergeDownText);
    placeBtn(mergeVisBtn, mergeVisText);
    placeBtn(pushBtn, pushText);
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
    case 1: return sf::Color(240, 70, 70);
    case 2: return sf::Color(65, 145, 255);
    case 3: return sf::Color(55, 215, 95);
    case 4: return sf::Color(255, 205, 50);
    case 5: return sf::Color(175, 75, 245);
    case 6: return sf::Color(255, 130, 35);
    default: return sf::Color(60, 50, 70, 120);
    }
}

void LayerPanel::renderEyeIcon(sf::RenderWindow& window, sf::FloatRect bounds, bool visible) {
    float cx = bounds.left + bounds.width * 0.5f;
    float cy = bounds.top + bounds.height * 0.5f;

    if (visible) {
        sf::CircleShape pupil(2.5f);
        pupil.setOrigin(2.5f, 2.5f);
        pupil.setPosition(cx, cy);
        pupil.setFillColor(WisdomUI::Theme::Gold);

        sf::RectangleShape hLine(sf::Vector2f(10.f, 1.5f));
        hLine.setOrigin(5.f, 0.75f);
        hLine.setPosition(cx, cy);
        hLine.setFillColor(WisdomUI::Theme::Gold);

        window.draw(hLine);
        window.draw(pupil);
    }
    else {
        sf::RectangleShape slash(sf::Vector2f(12.f, 1.5f));
        slash.setOrigin(6.f, 0.75f);
        slash.setPosition(cx, cy);
        slash.setRotation(45.f);
        slash.setFillColor(WisdomUI::Theme::TextMuted);
        window.draw(slash);
    }
}

void LayerPanel::renderLockIcon(sf::RenderWindow& window, sf::FloatRect bounds, bool locked) {
    float cx = bounds.left + bounds.width * 0.5f;
    float cy = bounds.top + bounds.height * 0.5f;

    sf::RectangleShape body(sf::Vector2f(8.f, 6.f));
    body.setOrigin(4.f, 2.f);
    body.setPosition(cx, cy);
    body.setFillColor(locked ? sf::Color(220, 75, 75) : WisdomUI::Theme::TextMuted);
    window.draw(body);

    sf::CircleShape loop(3.f);
    loop.setOrigin(3.f, 3.f);
    loop.setPosition(cx, cy - 3.f);
    loop.setFillColor(sf::Color::Transparent);
    loop.setOutlineThickness(1.2f);
    loop.setOutlineColor(locked ? sf::Color(220, 75, 75) : WisdomUI::Theme::TextMuted);
    window.draw(loop);
}

void LayerPanel::renderPersistIcon(sf::RenderWindow& window, sf::FloatRect bounds, bool persistent) {
    float cx = bounds.left + bounds.width * 0.5f;
    float cy = bounds.top + bounds.height * 0.5f;

    sf::CircleShape c1(3.f);
    c1.setOrigin(3.f, 3.f);
    c1.setPosition(cx - 2.5f, cy);
    c1.setFillColor(sf::Color::Transparent);
    c1.setOutlineThickness(1.2f);
    c1.setOutlineColor(persistent ? WisdomUI::Theme::Gold : WisdomUI::Theme::TextMuted);
    window.draw(c1);

    sf::CircleShape c2(3.f);
    c2.setOrigin(3.f, 3.f);
    c2.setPosition(cx + 2.5f, cy);
    c2.setFillColor(sf::Color::Transparent);
    c2.setOutlineThickness(1.2f);
    c2.setOutlineColor(persistent ? WisdomUI::Theme::Gold : WisdomUI::Theme::TextMuted);
    window.draw(c2);
}

void LayerPanel::draw(sf::RenderWindow& window, Canvas& canvas, int currentFrame) {
    if (currentX >= 1918.f) return;

    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    WisdomUI::Theme::DrawFiligreePanel(window, background.getGlobalBounds(), 1.0f);

    window.draw(headerBg);
    window.draw(headerText);

    auto drawHeaderBtn = [&](sf::RectangleShape& r, sf::Text& t, bool active) {
        bool hov = r.getGlobalBounds().contains(mousePos);
        r.setFillColor(active ? WisdomUI::Theme::Gold : (hov ? WisdomUI::Theme::PanelHover : WisdomUI::Theme::PanelInset));
        r.setOutlineColor(hov || active ? WisdomUI::Theme::BorderHighlight : WisdomUI::Theme::Border);
        t.setFillColor(active ? sf::Color::Black : (hov ? sf::Color::White : WisdomUI::Theme::TextSecondary));
        window.draw(r);
        window.draw(t);
        };

    drawHeaderBtn(closeBtn, closeText, false);
    drawHeaderBtn(pinBtn, pinText, state == LayerPanelState::Pinned);

    auto drawActionBtn = [&](sf::RectangleShape& r, sf::Text& t) {
        bool hov = r.getGlobalBounds().contains(mousePos);
        r.setFillColor(hov ? WisdomUI::Theme::PanelHover : WisdomUI::Theme::PanelInset);
        r.setOutlineColor(hov ? WisdomUI::Theme::BorderHighlight : WisdomUI::Theme::Border);
        t.setFillColor(hov ? sf::Color::White : WisdomUI::Theme::Gold);
        window.draw(r);
        window.draw(t);
        };

    drawActionBtn(addBtn, addText);
    drawActionBtn(dupBtn, dupText);
    drawActionBtn(delBtn, delText);
    drawActionBtn(mergeDownBtn, mergeDownText);
    drawActionBtn(mergeVisBtn, mergeVisText);
    drawActionBtn(pushBtn, pushText);

    const Frame* frame = canvas.getFrameReadOnly(currentFrame);
    if (!frame) return;

    size_t layerCount = frame->layers.size();
    rowCache.clear();
    rowCache.resize(layerCount);

    float rowHeight = 48.f;
    float startY = 134.f;
    float bottomLimit = 1080.f - 24.f;
    float viewHeight = bottomLimit - startY;

    maxScroll = std::max(0.f, static_cast<float>(layerCount) * rowHeight - viewHeight);
    scrollOffset = std::clamp(scrollOffset, 0.f, maxScroll);

    dropVisualSlot = -1;
    if (isDragging && draggedLayerIndex != -1) {
        float relY = (mousePos.y + scrollOffset - startY) / rowHeight;
        dropVisualSlot = std::clamp(static_cast<int>(std::round(relY)), 0, static_cast<int>(layerCount));
    }

    for (int i = static_cast<int>(layerCount) - 1; i >= 0; --i) {
        float curY = startY + (layerCount - 1 - i) * rowHeight - scrollOffset;
        if (isDragging && draggedLayerIndex == i) {
            curY = mousePos.y - 24.f;
        }

        rowCache[i].bounds = sf::FloatRect(currentX + 6.f, curY, width - 20.f, rowHeight - 4.f);
        rowCache[i].colorTagBounds = sf::FloatRect(currentX + 8.f, curY + 2.f, 3.f, rowHeight - 8.f);
        rowCache[i].eyeBounds = sf::FloatRect(currentX + 16.f, curY + 14.f, 18.f, 18.f);
        rowCache[i].lockBounds = sf::FloatRect(currentX + 37.f, curY + 14.f, 18.f, 18.f);
        rowCache[i].persistBounds = sf::FloatRect(currentX + 58.f, curY + 14.f, 18.f, 18.f);
        rowCache[i].thumbBounds = sf::FloatRect(currentX + 80.f, curY + 5.f, 34.f, 34.f);
        rowCache[i].nameBounds = sf::FloatRect(currentX + 120.f, curY + 6.f, 96.f, 16.f);
        rowCache[i].opacityBounds = sf::FloatRect(currentX + 120.f, curY + 27.f, 74.f, 10.f);
        rowCache[i].blendBounds = sf::FloatRect(currentX + 224.f, curY + 6.f, 56.f, 16.f);

        if (curY + rowHeight < startY || curY > bottomLimit) continue;

        sf::RectangleShape rowBg(sf::Vector2f(rowCache[i].bounds.width, rowCache[i].bounds.height));
        rowBg.setPosition(rowCache[i].bounds.left, rowCache[i].bounds.top);

        bool isSelected = (i == canvas.getActiveLayer());
        bool isHoveredRow = rowCache[i].bounds.contains(mousePos);

        if (isSelected) {
            rowBg.setFillColor(WisdomUI::Theme::PanelHover);
            rowBg.setOutlineThickness(1.5f);
            rowBg.setOutlineColor(WisdomUI::Theme::BorderHighlight);
        }
        else {
            rowBg.setFillColor(isHoveredRow ? sf::Color(32, 24, 40, 200) : WisdomUI::Theme::PanelInset);
            rowBg.setOutlineThickness(1.f);
            rowBg.setOutlineColor(WisdomUI::Theme::Border);
        }
        window.draw(rowBg);

        sf::RectangleShape colorTag(sf::Vector2f(rowCache[i].colorTagBounds.width, rowCache[i].colorTagBounds.height));
        colorTag.setPosition(rowCache[i].colorTagBounds.left, rowCache[i].colorTagBounds.top);
        colorTag.setFillColor(getTagColor(frame->layers[i].colorTag));
        window.draw(colorTag);

        auto drawIconToggle = [&](sf::FloatRect bounds, bool active, auto renderIcon) {
            bool hov = bounds.contains(mousePos);
            sf::RectangleShape box(sf::Vector2f(bounds.width, bounds.height));
            box.setPosition(bounds.left, bounds.top);
            box.setFillColor(hov ? sf::Color(255, 255, 255, 20) : sf::Color(0, 0, 0, 40));
            box.setOutlineThickness(1.f);
            box.setOutlineColor(hov ? WisdomUI::Theme::Gold : WisdomUI::Theme::Border);
            window.draw(box);
            renderIcon(window, bounds, active);
            };

        drawIconToggle(rowCache[i].eyeBounds, frame->layers[i].visible,
            [this](sf::RenderWindow& w, sf::FloatRect b, bool v) { renderEyeIcon(w, b, v); });
        drawIconToggle(rowCache[i].lockBounds, frame->layers[i].locked,
            [this](sf::RenderWindow& w, sf::FloatRect b, bool l) { renderLockIcon(w, b, l); });
        drawIconToggle(rowCache[i].persistBounds, frame->layers[i].persistent,
            [this](sf::RenderWindow& w, sf::FloatRect b, bool p) { renderPersistIcon(w, b, p); });

        sf::RectangleShape thumbBase(sf::Vector2f(rowCache[i].thumbBounds.width, rowCache[i].thumbBounds.height));
        thumbBase.setPosition(rowCache[i].thumbBounds.left, rowCache[i].thumbBounds.top);
        thumbBase.setFillColor(sf::Color(140, 140, 140));
        thumbBase.setOutlineThickness(1.f);
        thumbBase.setOutlineColor(WisdomUI::Theme::Border);
        window.draw(thumbBase);

        if (frame->layers[i].texture) {
            sf::Sprite thumb(frame->layers[i].texture->getTexture());
            float cw = static_cast<float>(canvas.getCanvasSize().x);
            float ch = static_cast<float>(canvas.getCanvasSize().y);
            if (cw > 0.f && ch > 0.f) {
                float s = std::min(rowCache[i].thumbBounds.width / cw, rowCache[i].thumbBounds.height / ch);
                thumb.setScale(s, s);
                thumb.setPosition(rowCache[i].thumbBounds.left, rowCache[i].thumbBounds.top);
                window.draw(thumb);
            }
        }

        if (renamingLayerIndex == i) {
            renameBox.setPosition(rowCache[i].nameBounds.left, rowCache[i].nameBounds.top);
            renameBox.setSize(sf::Vector2f(rowCache[i].nameBounds.width + 50.f, rowCache[i].nameBounds.height + 2.f));
            renameText.setString(renameBuffer + "_");
            renameText.setPosition(renameBox.getPosition().x + 3.f, renameBox.getPosition().y + 1.f);
            window.draw(renameBox);
            window.draw(renameText);
        }
        else {
            sf::Text nText(frame->layers[i].name, font, 11);
            nText.setPosition(rowCache[i].nameBounds.left, rowCache[i].nameBounds.top);
            nText.setFillColor(isSelected ? WisdomUI::Theme::Gold : WisdomUI::Theme::TextPrimary);
            window.draw(nText);
        }

        sf::RectangleShape opTrack(sf::Vector2f(rowCache[i].opacityBounds.width, rowCache[i].opacityBounds.height));
        opTrack.setPosition(rowCache[i].opacityBounds.left, rowCache[i].opacityBounds.top);
        opTrack.setFillColor(sf::Color(20, 14, 25));
        opTrack.setOutlineThickness(1.f);
        opTrack.setOutlineColor(WisdomUI::Theme::Border);
        window.draw(opTrack);

        float opVal = frame->layers[i].opacity > 1.0f ? (frame->layers[i].opacity / 255.f) : frame->layers[i].opacity;
        sf::RectangleShape opFill(sf::Vector2f(rowCache[i].opacityBounds.width * std::clamp(opVal, 0.f, 1.f), rowCache[i].opacityBounds.height));
        opFill.setPosition(rowCache[i].opacityBounds.left, rowCache[i].opacityBounds.top);
        opFill.setFillColor(WisdomUI::Theme::SunsetCoral);
        window.draw(opFill);

        sf::Text opPercent(std::to_string(static_cast<int>(std::round(std::clamp(opVal, 0.f, 1.f) * 100.f))) + "%", font, 8);
        opPercent.setPosition(rowCache[i].opacityBounds.left + rowCache[i].opacityBounds.width + 4.f, rowCache[i].opacityBounds.top - 1.f);
        opPercent.setFillColor(WisdomUI::Theme::TextSecondary);
        window.draw(opPercent);

        sf::RectangleShape blendBox(sf::Vector2f(rowCache[i].blendBounds.width, rowCache[i].blendBounds.height));
        blendBox.setPosition(rowCache[i].blendBounds.left, rowCache[i].blendBounds.top);
        bool hovBlend = rowCache[i].blendBounds.contains(mousePos);
        blendBox.setFillColor(hovBlend ? WisdomUI::Theme::PanelHover : sf::Color(20, 14, 25));
        blendBox.setOutlineThickness(1.f);
        blendBox.setOutlineColor(hovBlend ? WisdomUI::Theme::Gold : WisdomUI::Theme::Border);
        window.draw(blendBox);

        std::string modeStr = "Normal";
        if (frame->layers[i].blendMode == BlendMode::Multiply) modeStr = "Multiply";
        else if (frame->layers[i].blendMode == BlendMode::Additive) modeStr = "Add";
        else if (frame->layers[i].blendMode == BlendMode::Screen) modeStr = "Screen";
        else if (frame->layers[i].blendMode == BlendMode::Overlay) modeStr = "Overlay";

        sf::Text bText(modeStr, font, 9);
        bText.setPosition(rowCache[i].blendBounds.left + 4.f, rowCache[i].blendBounds.top + 2.f);
        bText.setFillColor(WisdomUI::Theme::TextSecondary);
        window.draw(bText);
    }

    if (isDragging && dropVisualSlot != -1) {
        float lineY = startY + dropVisualSlot * rowHeight - scrollOffset;
        if (lineY >= startY - 2.f && lineY <= bottomLimit + 2.f) {
            sf::RectangleShape indicator(sf::Vector2f(width - 20.f, 3.f));
            indicator.setPosition(currentX + 6.f, lineY - 1.5f);
            indicator.setFillColor(WisdomUI::Theme::Gold);
            window.draw(indicator);
        }
    }

    if (maxScroll > 0.f) {
        float scrollTrackX = currentX + width - 10.f;
        sf::RectangleShape scrollTrack(sf::Vector2f(4.f, viewHeight));
        scrollTrack.setPosition(scrollTrackX, startY);
        scrollTrack.setFillColor(sf::Color(15, 10, 20));
        window.draw(scrollTrack);

        float thumbH = std::max(24.f, viewHeight * (viewHeight / (viewHeight + maxScroll)));
        float thumbY = startY + (scrollOffset / maxScroll) * (viewHeight - thumbH);
        sf::RectangleShape scrollThumb(sf::Vector2f(4.f, thumbH));
        scrollThumb.setPosition(scrollTrackX, thumbY);
        scrollThumb.setFillColor(isDraggingScrollbar ? WisdomUI::Theme::Gold : WisdomUI::Theme::BorderHighlight);
        window.draw(scrollThumb);
    }
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

    if (addBtn.getGlobalBounds().contains(mousePos)) { canvas.addLayer(currentFrame); return "layer_add"; }
    if (dupBtn.getGlobalBounds().contains(mousePos)) { canvas.duplicateLayer(currentFrame, canvas.getActiveLayer()); return "layer_dup"; }
    if (delBtn.getGlobalBounds().contains(mousePos)) { canvas.deleteLayer(currentFrame, canvas.getActiveLayer()); return "layer_del"; }
    if (mergeDownBtn.getGlobalBounds().contains(mousePos)) { canvas.mergeDown(currentFrame); return "layer_merge_d"; }
    if (mergeVisBtn.getGlobalBounds().contains(mousePos)) { canvas.mergeVisible(currentFrame); return "layer_merge_v"; }
    if (pushBtn.getGlobalBounds().contains(mousePos)) { canvas.extendLayerToNextFrame(currentFrame, canvas.getActiveLayer()); return "layer_push"; }

    float startY = 134.f;
    float bottomLimit = 1080.f - 24.f;
    float viewHeight = bottomLimit - startY;

    if (maxScroll > 0.f) {
        sf::FloatRect trackBounds(currentX + width - 14.f, startY, 12.f, viewHeight);
        if (trackBounds.contains(mousePos)) {
            isDraggingScrollbar = true;
            scrollDragStartY = mousePos.y;
            scrollDragStartOffset = scrollOffset;
            return "layer_scroll";
        }
    }

    for (int i = 0; i < static_cast<int>(rowCache.size()); ++i) {
        const Frame* f = canvas.getFrameReadOnly(currentFrame);
        if (!f || i >= static_cast<int>(f->layers.size())) break;

        if (rowCache[i].bounds.contains(mousePos)) {
            if (rowCache[i].colorTagBounds.contains(mousePos)) {
                canvas.cycleLayerColorTag(currentFrame, i);
                return "layer_tag";
            }
            if (rowCache[i].eyeBounds.contains(mousePos)) {
                canvas.setLayerProperties(currentFrame, i, f->layers[i].name, !f->layers[i].visible, f->layers[i].locked, f->layers[i].opacity, f->layers[i].blendMode, true);
                return "layer_vis";
            }
            if (rowCache[i].lockBounds.contains(mousePos)) {
                canvas.setLayerProperties(currentFrame, i, f->layers[i].name, f->layers[i].visible, !f->layers[i].locked, f->layers[i].opacity, f->layers[i].blendMode, true);
                return "layer_lock";
            }
            if (rowCache[i].persistBounds.contains(mousePos)) {
                canvas.toggleLayerPersistence(currentFrame, i);
                return "layer_persist";
            }
            if (rowCache[i].opacityBounds.contains(mousePos)) {
                canvas.saveUndoState();
                isDraggingOpacity = true;
                activeOpacityIndex = i;
                float newOp = std::clamp((mousePos.x - rowCache[i].opacityBounds.left) / rowCache[i].opacityBounds.width, 0.f, 1.f);
                canvas.setLayerProperties(currentFrame, i, f->layers[i].name, f->layers[i].visible, f->layers[i].locked, newOp, f->layers[i].blendMode, false);
                return "layer_op";
            }
            if (rowCache[i].blendBounds.contains(mousePos)) {
                int b = static_cast<int>(f->layers[i].blendMode) + 1;
                if (b > 4) b = 0;
                canvas.setLayerProperties(currentFrame, i, f->layers[i].name, f->layers[i].visible, f->layers[i].locked, f->layers[i].opacity, static_cast<BlendMode>(b), true);
                return "layer_blend";
            }

            if (rowCache[i].nameBounds.contains(mousePos)) {
                if (lastClickedLayerIndex == i && clickTimer.getElapsedTime().asMilliseconds() < 300) {
                    renamingLayerIndex = i;
                    renameBuffer = f->layers[i].name;
                    return "layer_rename";
                }
                clickTimer.restart();
                lastClickedLayerIndex = i;
            }

            canvas.setActiveLayer(i, currentFrame);
            draggedLayerIndex = i;
            dragCurrentPos = mousePos;
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
            if (event.text.unicode == '\b' && !renameBuffer.empty()) {
                renameBuffer.pop_back();
            }
            else if (event.text.unicode >= 32 && event.text.unicode < 127) {
                renameBuffer += static_cast<char>(event.text.unicode);
            }
            return true;
        }
        if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Enter) {
                const Frame* f = canvas.getFrameReadOnly(currentFrame);
                if (f && renamingLayerIndex < static_cast<int>(f->layers.size())) {
                    canvas.setLayerProperties(currentFrame, renamingLayerIndex, renameBuffer, f->layers[renamingLayerIndex].visible, f->layers[renamingLayerIndex].locked, f->layers[renamingLayerIndex].opacity, f->layers[renamingLayerIndex].blendMode, true);
                }
                renamingLayerIndex = -1;
                return true;
            }
            if (event.key.code == sf::Keyboard::Escape) {
                renamingLayerIndex = -1;
                return true;
            }
        }
    }

    if (event.type == sf::Event::MouseMoved) {
        if (isDraggingOpacity && activeOpacityIndex != -1) {
            const Frame* f = canvas.getFrameReadOnly(currentFrame);
            if (f && activeOpacityIndex < static_cast<int>(f->layers.size()) && activeOpacityIndex < static_cast<int>(rowCache.size())) {
                float newOp = std::clamp((mousePos.x - rowCache[activeOpacityIndex].opacityBounds.left) / rowCache[activeOpacityIndex].opacityBounds.width, 0.f, 1.f);
                canvas.setLayerProperties(currentFrame, activeOpacityIndex, f->layers[activeOpacityIndex].name, f->layers[activeOpacityIndex].visible, f->layers[activeOpacityIndex].locked, newOp, f->layers[activeOpacityIndex].blendMode, false);
            }
            return true;
        }

        if (isDraggingScrollbar && maxScroll > 0.f) {
            float startY = 134.f;
            float bottomLimit = 1080.f - 24.f;
            float viewHeight = bottomLimit - startY;
            float deltaY = mousePos.y - scrollDragStartY;
            scrollOffset = std::clamp(scrollDragStartOffset + (deltaY / viewHeight) * maxScroll, 0.f, maxScroll);
            return true;
        }
    }

    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        if (isDraggingOpacity) {
            isDraggingOpacity = false;
            activeOpacityIndex = -1;
            return true;
        }
        if (isDraggingScrollbar) {
            isDraggingScrollbar = false;
            return true;
        }
        if (isDragging) {
            const Frame* f = canvas.getFrameReadOnly(currentFrame);
            if (f && draggedLayerIndex != -1 && dropVisualSlot != -1) {
                size_t layerCount = f->layers.size();
                int targetDataIndex = static_cast<int>(layerCount) - dropVisualSlot;
                if (draggedLayerIndex < targetDataIndex) {
                    targetDataIndex--;
                }
                targetDataIndex = std::clamp(targetDataIndex, 0, static_cast<int>(layerCount) - 1);

                if (targetDataIndex != draggedLayerIndex) {
                    canvas.moveLayer(currentFrame, draggedLayerIndex, targetDataIndex);
                }
            }
            isDragging = false;
            draggedLayerIndex = -1;
            dropVisualSlot = -1;
            return true;
        }
    }

    if (event.type == sf::Event::MouseWheelScrolled && background.getGlobalBounds().contains(mousePos)) {
        scrollOffset -= event.mouseWheelScroll.delta * 24.f;
        scrollOffset = std::clamp(scrollOffset, 0.f, maxScroll);
        return true;
    }

    return false;
}