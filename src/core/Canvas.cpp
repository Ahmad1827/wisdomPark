#include "Canvas.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <stack>
#include <queue>

Layer::Layer(std::string n) : name(n), visible(true), locked(false), opacity(1.0f), blendMode(BlendMode::Normal) {
    texture = new sf::RenderTexture();
    texture->create(1920, 1080);
    texture->clear(sf::Color::Transparent);
    texture->display();
}

Layer::~Layer() {
    delete texture;
}

Layer::Layer(const Layer& other) : name(other.name), visible(other.visible), locked(other.locked), opacity(other.opacity), blendMode(other.blendMode) {
    texture = new sf::RenderTexture();
    texture->create(1920, 1080);
    texture->clear(sf::Color::Transparent);
    if (other.texture) {
        sf::Sprite spr(other.texture->getTexture());
        texture->draw(spr);
        texture->display();
    }
}

Layer& Layer::operator=(const Layer& other) {
    if (this != &other) {
        name = other.name; visible = other.visible; locked = other.locked; opacity = other.opacity; blendMode = other.blendMode;
        if (!texture) { texture = new sf::RenderTexture(); texture->create(1920, 1080); }
        texture->clear(sf::Color::Transparent);
        if (other.texture) { sf::Sprite spr(other.texture->getTexture()); texture->draw(spr); texture->display(); }
    }
    return *this;
}

Layer::Layer(Layer&& other) noexcept : texture(other.texture), name(std::move(other.name)), visible(other.visible), locked(other.locked), opacity(other.opacity), blendMode(other.blendMode) {
    other.texture = nullptr;
}

Layer& Layer::operator=(Layer&& other) noexcept {
    if (this != &other) {
        delete texture; texture = other.texture; name = std::move(other.name);
        visible = other.visible; locked = other.locked; opacity = other.opacity; blendMode = other.blendMode;
        other.texture = nullptr;
    }
    return *this;
}

Frame::Frame() {
    layers.emplace_back("Background");
    layers.emplace_back("Artwork");
}

Frame::~Frame() = default;

Frame::Frame(const Frame& other) {
    for (const auto& l : other.layers) {
        layers.emplace_back(l);
    }
}

Frame& Frame::operator=(const Frame& other) {
    if (this != &other) {
        layers.clear();
        for (const auto& l : other.layers) {
            layers.emplace_back(l);
        }
    }
    return *this;
}

Frame::Frame(Frame&& other) noexcept = default;
Frame& Frame::operator=(Frame&& other) noexcept = default;

Canvas::Canvas() : isDrawing(false), startPos(0.f, 0.f), lastPos(0.f, 0.f), lastHoverLocalPos(0.f, 0.f), rawMousePos(0.f, 0.f), isHoveringCanvas(false),
activeTool(ToolType::Brush), primaryColor(sf::Color::Black), secondaryColor(sf::Color::White),
fillTolerance(0.f), fillContiguous(true),
activeLayer(1), onionSkinEnabled(true), onionSkinPrevOpacity(89.25f), onionSkinNextOpacity(89.25f), onionSkinPrevCount(1), onionSkinNextCount(1),
viewScale(1.0f), targetScale(1.0f), canvasLogicalSize(1920, 1080) {
    previewTexture.create(1920, 1080);
    previewTexture.clear(sf::Color::Transparent);
    brushEngine.initDefaultPresets();
}

void Canvas::init() { initCustom(1920, 1080); }

void Canvas::initCustom(int width, int height) {
    canvasLogicalSize = sf::Vector2u(static_cast<unsigned int>(width), static_cast<unsigned int>(height));

    if (!deskTexture.loadFromFile("assets/workbench.png", sf::IntRect(114, 702, 1669, 379))) {
    }
    deskSprite.setTexture(deskTexture);
    deskSprite.setOrigin(1669.f / 2.f, 379.f / 2.f);
    deskSprite.setPosition(1920.f / 2.f, 850.f);

    if (!canvasTexture.loadFromFile("assets/canvas.png")) {
    }
    canvasSprite.setTexture(canvasTexture);

    float cScale = 700.f / canvasSprite.getLocalBounds().height;
    canvasSprite.setScale(cScale, cScale);
    canvasSprite.setOrigin(canvasSprite.getLocalBounds().width / 2.f, canvasSprite.getLocalBounds().height / 2.f);
    canvasSprite.setPosition(1920.f / 2.f, deskSprite.getPosition().y - (379.f / 2.f) - (700.f / 2.f) + 120.f);

    sf::FloatRect cBounds = canvasSprite.getGlobalBounds();
    float frameOffsetX = cBounds.width * 0.08f;
    float frameOffsetYTop = cBounds.height * 0.16f;
    float frameOffsetYBot = cBounds.height * 0.16f;

    drawArea = sf::FloatRect(
        cBounds.left + frameOffsetX,
        cBounds.top + frameOffsetYTop,
        cBounds.width - (frameOffsetX * 2.0f),
        cBounds.height - frameOffsetYTop - frameOffsetYBot
    );

    if (static_cast<unsigned int>(width) != previewTexture.getSize().x || static_cast<unsigned int>(height) != previewTexture.getSize().y) {
        previewTexture.create(static_cast<unsigned int>(width), static_cast<unsigned int>(height));
    }
    previewTexture.clear(sf::Color::Transparent);

    frames.clear();
    frames.emplace_back();
    undoHistory.clear();
    redoHistory.clear();
    selection.clearSelection();
}

void Canvas::updateTransform(float dt, sf::FloatRect space) {
    sf::FloatRect dBounds = deskSprite.getGlobalBounds();
    sf::FloatRect cBounds = canvasSprite.getGlobalBounds();

    float left = std::min(static_cast<float>(dBounds.left), static_cast<float>(cBounds.left));
    float top = std::min(static_cast<float>(dBounds.top), static_cast<float>(cBounds.top));
    float right = std::max(static_cast<float>(dBounds.left + dBounds.width), static_cast<float>(cBounds.left + cBounds.width));
    float bottom = std::max(static_cast<float>(dBounds.top + dBounds.height), static_cast<float>(cBounds.top + cBounds.height));

    float unionW = right - left;
    float unionH = bottom - top;

    float pad = 60.f;
    space.left += pad; space.top += pad; space.width -= pad * 2.0f; space.height -= pad * 2.0f;

    float sX = space.width / unionW;
    float sY = space.height / unionH;
    targetScale = std::min(static_cast<float>(sX), static_cast<float>(sY));

    float spaceCX = space.left + space.width / 2.f;
    float spaceCY = space.top + space.height / 2.f;
    float unionCX = left + unionW / 2.f;
    float unionCY = top + unionH / 2.f;

    targetOffset.x = spaceCX - (unionCX * targetScale);
    targetOffset.y = spaceCY - (unionCY * targetScale);

    viewScale += (targetScale - viewScale) * 12.f * dt;
    viewOffset.x += (targetOffset.x - viewOffset.x) * 12.f * dt;
    viewOffset.y += (targetOffset.y - viewOffset.y) * 12.f * dt;

    selection.update(dt);
}

sf::Transform Canvas::getTransform() const {
    sf::Transform t;
    t.translate(viewOffset).scale(viewScale, viewScale);
    return t;
}

sf::Transform Canvas::getInverseTransform() const { return getTransform().getInverse(); }

void Canvas::addFrame(int index) {
    saveUndoState();
    frames.insert(frames.begin() + (index + 1), Frame());
}

void Canvas::duplicateFrame(int index) {
    saveUndoState();
    if (index >= 0 && index < static_cast<int>(frames.size())) {
        frames.insert(frames.begin() + (index + 1), Frame(frames[index]));
    }
}

void Canvas::deleteFrame(int index) { if (frames.size() > 1 && index >= 0 && index < static_cast<int>(frames.size())) { saveUndoState(); frames.erase(frames.begin() + index); } }
void Canvas::clearAllFrames() { saveUndoState(); frames.clear(); frames.emplace_back(); }

void Canvas::addLayer(int frameIndex, const std::string& name) {
    if (frameIndex >= 0 && frameIndex < static_cast<int>(frames.size())) {
        saveUndoState();
        frames[frameIndex].layers.emplace_back(name);
        activeLayer = static_cast<int>(frames[frameIndex].layers.size()) - 1;
    }
}

void Canvas::deleteLayer(int frameIndex, int layerIndex) {
    if (frameIndex >= 0 && frameIndex < static_cast<int>(frames.size()) && frames[frameIndex].layers.size() > 1) {
        if (layerIndex >= 0 && layerIndex < static_cast<int>(frames[frameIndex].layers.size())) {
            saveUndoState();
            frames[frameIndex].layers.erase(frames[frameIndex].layers.begin() + layerIndex);
            if (activeLayer >= static_cast<int>(frames[frameIndex].layers.size())) {
                activeLayer = static_cast<int>(frames[frameIndex].layers.size()) - 1;
            }
        }
    }
}

void Canvas::duplicateLayer(int frameIndex, int layerIndex) {
    if (frameIndex >= 0 && frameIndex < static_cast<int>(frames.size())) {
        if (layerIndex >= 0 && layerIndex < static_cast<int>(frames[frameIndex].layers.size())) {
            saveUndoState();
            frames[frameIndex].layers.insert(frames[frameIndex].layers.begin() + layerIndex + 1, Layer(frames[frameIndex].layers[layerIndex]));
            frames[frameIndex].layers[layerIndex + 1].name += " Copy";
        }
    }
}

void Canvas::setLayerProperties(int frameIndex, int layerIndex, const std::string& name, bool visible, bool locked, float opacity, BlendMode mode) {
    if (frameIndex >= 0 && frameIndex < static_cast<int>(frames.size()) && layerIndex >= 0 && layerIndex < static_cast<int>(frames[frameIndex].layers.size())) {
        auto& l = frames[frameIndex].layers[layerIndex];
        l.name = name;
        l.visible = visible;
        l.locked = locked;
        l.opacity = std::max(0.0f, std::min(static_cast<float>(opacity), 1.0f));
        l.blendMode = mode;
    }
}

void Canvas::moveLayer(int frameIndex, int fromIndex, int toIndex) {
    if (frameIndex >= 0 && frameIndex < static_cast<int>(frames.size())) {
        auto& flayers = frames[frameIndex].layers;
        if (fromIndex >= 0 && fromIndex < static_cast<int>(flayers.size()) && toIndex >= 0 && toIndex < static_cast<int>(flayers.size())) {
            saveUndoState();
            Layer temp = std::move(flayers[fromIndex]);
            flayers.erase(flayers.begin() + fromIndex);
            flayers.insert(flayers.begin() + toIndex, std::move(temp));
            if (activeLayer == fromIndex) activeLayer = toIndex;
        }
    }
}

void Canvas::setActiveLayer(int index) {
    activeLayer = std::max(0, static_cast<int>(index));
}

int Canvas::getActiveLayer() const {
    return activeLayer;
}

void Canvas::setOnionSkin(bool enabled, float prevOpac, float nextOpac) {
    onionSkinEnabled = enabled;
    onionSkinPrevOpacity = std::max(0.0f, std::min(prevOpac, 255.0f));
    onionSkinNextOpacity = std::max(0.0f, std::min(nextOpac, 255.0f));
}

void Canvas::setOnionSkinCounts(int prevCount, int nextCount) {
    onionSkinPrevCount = std::max(0, prevCount);
    onionSkinNextCount = std::max(0, nextCount);
}

bool Canvas::isOnionSkinEnabled() const { return onionSkinEnabled; }
float Canvas::getOnionSkinPrevOpacity() const { return onionSkinPrevOpacity; }
float Canvas::getOnionSkinNextOpacity() const { return onionSkinNextOpacity; }
int Canvas::getOnionSkinPrevCount() const { return onionSkinPrevCount; }
int Canvas::getOnionSkinNextCount() const { return onionSkinNextCount; }

void Canvas::commitSelection(int currentFrame) {
    if (frames.empty() || currentFrame < 0 || currentFrame >= static_cast<int>(frames.size())) return;
    if (selection.isActive()) {
        saveUndoState();
        selection.commitToLayer(frames[currentFrame].layers[activeLayer].texture);
    }
}

void Canvas::copySelection() {
    selection.copy();
}

void Canvas::pasteSelection(int currentFrame) {
    commitSelection(currentFrame);
    saveUndoState();
    selection.paste(canvasLogicalSize);
    setActiveTool(ToolType::Select);
}

void Canvas::deleteSelection(int currentFrame) {
    if (!frames.empty() && currentFrame >= 0 && currentFrame < static_cast<int>(frames.size()) && selection.isActive()) {
        saveUndoState();
        selection.deleteSelection(frames[currentFrame].layers[activeLayer].texture);
    }
}

void Canvas::flipSelectionHorizontal(int currentFrame) {
    if (!frames.empty() && currentFrame >= 0 && currentFrame < static_cast<int>(frames.size()) && selection.isActive()) {
        if (selection.getState() == SelectionState::Selected) {
            saveUndoState();
            selection.extractFromLayer(frames[currentFrame].layers[activeLayer].texture, true);
        }
        selection.flipHorizontal();
    }
}

void Canvas::flipSelectionVertical(int currentFrame) {
    if (!frames.empty() && currentFrame >= 0 && currentFrame < static_cast<int>(frames.size()) && selection.isActive()) {
        if (selection.getState() == SelectionState::Selected) {
            saveUndoState();
            selection.extractFromLayer(frames[currentFrame].layers[activeLayer].texture, true);
        }
        selection.flipVertical();
    }
}

void Canvas::duplicateSelection(int currentFrame) {
    if (!frames.empty() && currentFrame >= 0 && currentFrame < static_cast<int>(frames.size()) && selection.isActive()) {
        saveUndoState();
        selection.duplicate(frames[currentFrame].layers[activeLayer].texture, canvasLogicalSize);
    }
}

void Canvas::setActiveTool(ToolType tool) {
    activeTool = tool;
    isDrawing = false;
    if (tool == ToolType::Pencil) {
        brushEngine.selectPreset("Pencil");
    }
    else if (tool == ToolType::Brush) {
        brushEngine.selectPreset("Paint");
    }
    else if (tool == ToolType::Eraser) {
        brushEngine.selectPreset("Eraser");
    }
}

ToolType Canvas::getActiveTool() const { return activeTool; }
BrushManager& Canvas::getBrushEngine() { return brushEngine; }

void Canvas::setBrushSize(float size) { brushEngine.setBrushSize(size); }
float Canvas::getBrushSize() const { return brushEngine.getActivePreset().size; }

bool Canvas::colorMatches(const sf::Color& a, const sf::Color& b) const {
    if (fillTolerance <= 0.0f) return a == b;
    float diffR = std::abs(static_cast<float>(a.r) - static_cast<float>(b.r));
    float diffG = std::abs(static_cast<float>(a.g) - static_cast<float>(b.g));
    float diffB = std::abs(static_cast<float>(a.b) - static_cast<float>(b.b));
    float diffA = std::abs(static_cast<float>(a.a) - static_cast<float>(b.a));
    return ((diffR + diffG + diffB + diffA) / (4.0f * 255.0f)) <= fillTolerance;
}

void Canvas::executeGlobalFill(sf::Color targetColor, sf::Color replacementColor, sf::Image& image) {
    if (colorMatches(targetColor, replacementColor)) return;
    int w = static_cast<int>(image.getSize().x);
    int h = static_cast<int>(image.getSize().y);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (colorMatches(image.getPixel(x, y), targetColor)) {
                image.setPixel(x, y, replacementColor);
            }
        }
    }
}

void Canvas::executeScanlineFill(sf::Vector2i startPoint, sf::Color targetColor, sf::Color replacementColor, sf::Image& image) {
    if (colorMatches(targetColor, replacementColor)) return;

    int w = static_cast<int>(image.getSize().x);
    int h = static_cast<int>(image.getSize().y);
    if (startPoint.x < 0 || startPoint.x >= w || startPoint.y < 0 || startPoint.y >= h) return;
    if (!colorMatches(image.getPixel(startPoint.x, startPoint.y), targetColor)) return;

    std::vector<bool> visited(static_cast<size_t>(w * h), false);
    std::stack<sf::Vector2i> stack;
    stack.push(startPoint);

    while (!stack.empty()) {
        sf::Vector2i pt = stack.top();
        stack.pop();

        int cx = pt.x;
        int cy = pt.y;

        int linearIndex = cy * w + cx;
        if (visited[static_cast<size_t>(linearIndex)]) continue;

        while (cx > 0 && colorMatches(image.getPixel(cx - 1, cy), targetColor)) {
            cx--;
        }

        bool spanAbove = false;
        bool spanBelow = false;

        while (cx < w && colorMatches(image.getPixel(cx, cy), targetColor)) {
            int currentLinearIndex = cy * w + cx;
            if (!visited[static_cast<size_t>(currentLinearIndex)]) {
                image.setPixel(cx, cy, replacementColor);
                visited[static_cast<size_t>(currentLinearIndex)] = true;
            }

            if (cy > 0) {
                bool matchAbove = colorMatches(image.getPixel(cx, cy - 1), targetColor);
                int aboveIndex = (cy - 1) * w + cx;
                if (!spanAbove && matchAbove && !visited[static_cast<size_t>(aboveIndex)]) {
                    stack.push(sf::Vector2i(cx, cy - 1));
                    spanAbove = true;
                }
                else if (spanAbove && !matchAbove) {
                    spanAbove = false;
                }
            }

            if (cy < h - 1) {
                bool matchBelow = colorMatches(image.getPixel(cx, cy + 1), targetColor);
                int belowIndex = (cy + 1) * w + cx;
                if (!spanBelow && matchBelow && !visited[static_cast<size_t>(belowIndex)]) {
                    stack.push(sf::Vector2i(cx, cy + 1));
                    spanBelow = true;
                }
                else if (spanBelow && !matchBelow) {
                    spanBelow = false;
                }
            }
            cx++;
        }
    }
}

void Canvas::handleMousePressed(sf::Vector2f logicalPos, bool rightClick, int currentFrame) {
    if (currentFrame < 0 || currentFrame >= static_cast<int>(frames.size())) return;

    sf::Vector2f texScale(static_cast<float>(canvasLogicalSize.x) / drawArea.width, static_cast<float>(canvasLogicalSize.y) / drawArea.height);
    sf::Vector2f localPos((logicalPos.x - drawArea.left) * texScale.x, (logicalPos.y - drawArea.top) * texScale.y);

    if (drawArea.contains(logicalPos)) {
        if (!frames[currentFrame].layers[activeLayer].locked && frames[currentFrame].layers[activeLayer].visible) {

            sf::Color drawCol = rightClick ? secondaryColor : primaryColor;

            if (activeTool == ToolType::Select) {
                if (selection.isPointInsideSelection(localPos)) {
                    if (selection.getState() == SelectionState::Selected) {
                        saveUndoState();
                        selection.extractFromLayer(frames[currentFrame].layers[activeLayer].texture, true);
                    }
                    selection.startDrag(localPos);
                    return;
                }

                commitSelection(currentFrame);
                selection.startLasso(localPos, canvasLogicalSize);
                return;
            }

            if (selection.isActive()) {
                commitSelection(currentFrame);
            }

            if (activeTool == ToolType::Fill) {
                saveUndoState();
                sf::Image img = frames[currentFrame].layers[activeLayer].texture->getTexture().copyToImage();
                sf::Vector2i pixelPos(static_cast<int>(localPos.x), static_cast<int>(localPos.y));

                if (pixelPos.x >= 0 && pixelPos.y >= 0 && pixelPos.x < static_cast<int>(img.getSize().x) && pixelPos.y < static_cast<int>(img.getSize().y)) {
                    sf::Color targetCol = img.getPixel(pixelPos.x, pixelPos.y);

                    if (fillContiguous) executeScanlineFill(pixelPos, targetCol, drawCol, img);
                    else executeGlobalFill(targetCol, drawCol, img);

                    sf::Texture tex;
                    tex.loadFromImage(img);
                    sf::Sprite spr(tex);
                    frames[currentFrame].layers[activeLayer].texture->clear(sf::Color::Transparent);
                    frames[currentFrame].layers[activeLayer].texture->draw(spr);
                    frames[currentFrame].layers[activeLayer].texture->display();
                }
                return;
            }

            saveUndoState();
            isDrawing = true;
            startPos = localPos;
            lastPos = localPos;
            brushEngine.resetStroke(localPos);
        }
    }
}

void Canvas::handleMouseReleased(sf::Vector2f logicalPos, int currentFrame) {
    if (activeTool == ToolType::Select) {
        if (selection.getState() == SelectionState::Drawing) {
            selection.endLasso();
        }
        else if (selection.getState() == SelectionState::Floating) {
            selection.endDrag();
        }
    }
    isDrawing = false;
}

void Canvas::handleMouseMoved(sf::Vector2f logicalPos, sf::Vector2f rawPos, int currentFrame) {
    isHoveringCanvas = drawArea.contains(logicalPos);
    rawMousePos = rawPos;

    sf::Vector2f texScale(static_cast<float>(canvasLogicalSize.x) / drawArea.width, static_cast<float>(canvasLogicalSize.y) / drawArea.height);
    sf::Vector2f localPos((logicalPos.x - drawArea.left) * texScale.x, (logicalPos.y - drawArea.top) * texScale.y);
    lastHoverLocalPos = localPos;

    if (activeTool == ToolType::Fill) return;

    if (activeTool == ToolType::Select) {
        if (selection.getState() == SelectionState::Drawing) selection.addLassoPoint(localPos, canvasLogicalSize);
        else if (selection.getState() == SelectionState::Floating) selection.drag(localPos, canvasLogicalSize);
        return;
    }

    if (isDrawing && currentFrame >= 0 && currentFrame < static_cast<int>(frames.size())) {
        if (frames[currentFrame].layers[activeLayer].locked || !frames[currentFrame].layers[activeLayer].visible) {
            isDrawing = false;
            return;
        }

        sf::RenderTexture* targetTex = frames[currentFrame].layers[activeLayer].texture;
        sf::Color drawCol = (activeTool == ToolType::Eraser) ? sf::Color::Transparent : primaryColor;

        if (activeTool == ToolType::Eraser) {
            sf::RenderStates rs(sf::BlendNone);
            float currentEraserSize = brushEngine.getActivePreset().size;

            float length = std::sqrt((localPos.x - lastPos.x) * (localPos.x - lastPos.x) + (localPos.y - lastPos.y) * (localPos.y - lastPos.y));
            sf::RectangleShape line(sf::Vector2f(length, currentEraserSize));
            line.setOrigin(0.0f, currentEraserSize / 2.f);
            line.setPosition(lastPos);
            line.setRotation(std::atan2(localPos.y - lastPos.y, localPos.x - lastPos.x) * 180.f / 3.14159265f);
            line.setFillColor(drawCol);

            sf::CircleShape circle(currentEraserSize / 2.f);
            circle.setOrigin(currentEraserSize / 2.f, currentEraserSize / 2.f);
            circle.setPosition(localPos);
            circle.setFillColor(drawCol);

            targetTex->draw(line, rs);
            targetTex->draw(circle, rs);
        }
        else {
            brushEngine.paintStroke(targetTex, localPos, drawCol, 1.0f);
        }

        targetTex->display();
        lastPos = localPos;

        if (!isHoveringCanvas) {
            isDrawing = false;
        }
    }
}

void Canvas::setPrimaryColor(sf::Color color) { primaryColor = color; }
void Canvas::setSecondaryColor(sf::Color color) { secondaryColor = color; }
sf::Color Canvas::getPrimaryColor() const { return primaryColor; }
void Canvas::setFillSettings(float tolerance, bool contiguous) { fillTolerance = tolerance; fillContiguous = contiguous; }

void Canvas::saveUndoState() {
    undoHistory.push_back(frames);
    if (undoHistory.size() > 15) undoHistory.erase(undoHistory.begin());
    redoHistory.clear();
}

void Canvas::undo() {
    if (!undoHistory.empty()) {
        redoHistory.push_back(frames);
        frames = undoHistory.back();
        undoHistory.pop_back();
        selection.clearSelection();
    }
}

void Canvas::redo() {
    if (!redoHistory.empty()) {
        undoHistory.push_back(frames);
        frames = redoHistory.back();
        redoHistory.pop_back();
        selection.clearSelection();
    }
}

sf::RenderStates Canvas::getSFMLBlendMode(BlendMode mode) const {
    switch (mode) {
    case BlendMode::Multiply: return sf::RenderStates(sf::BlendMultiply);
    case BlendMode::Additive: return sf::RenderStates(sf::BlendAdd);
    case BlendMode::Screen: {
        sf::BlendMode screenBlend(sf::BlendMode::One, sf::BlendMode::OneMinusSrcColor, sf::BlendMode::Add);
        return sf::RenderStates(screenBlend);
    }
    case BlendMode::Overlay: {
        sf::BlendMode overlayBlend(sf::BlendMode::DstColor, sf::BlendMode::SrcColor, sf::BlendMode::Add);
        return sf::RenderStates(overlayBlend);
    }
    case BlendMode::Normal:
    default: return sf::RenderStates(sf::BlendAlpha);
    }
}

void Canvas::draw(sf::RenderWindow& window, int currentFrame, bool isPlaying, const sf::RenderStates& states) {
    window.draw(deskSprite, states);
    window.draw(canvasSprite, states);

    sf::Vector2f texScale(drawArea.width / static_cast<float>(canvasLogicalSize.x), drawArea.height / static_cast<float>(canvasLogicalSize.y));
    sf::Transform innerTransform = states.transform;
    innerTransform.translate(drawArea.left, drawArea.top);
    innerTransform.scale(texScale);
    sf::RenderStates innerStates = states;
    innerStates.transform = innerTransform;

    if (!isPlaying && onionSkinEnabled) {
        for (int i = 1; i <= onionSkinPrevCount; ++i) {
            int prevIdx = currentFrame - i;
            if (prevIdx >= 0 && prevIdx < static_cast<int>(frames.size())) {
                float fadeOpac = onionSkinPrevOpacity * (1.0f - (static_cast<float>(i) - 1.0f) / static_cast<float>(onionSkinPrevCount));
                for (const auto& layer : frames[prevIdx].layers) {
                    if (layer.visible) {
                        sf::Sprite onionSpr(layer.texture->getTexture());
                        onionSpr.setColor(sf::Color(255, 100, 100, static_cast<sf::Uint8>(fadeOpac)));
                        sf::RenderStates oStates = innerStates;
                        oStates.blendMode = getSFMLBlendMode(layer.blendMode).blendMode;
                        window.draw(onionSpr, oStates);
                    }
                }
            }
        }
        for (int i = 1; i <= onionSkinNextCount; ++i) {
            int nextIdx = currentFrame + i;
            if (nextIdx >= 0 && nextIdx < static_cast<int>(frames.size())) {
                float fadeOpac = onionSkinNextOpacity * (1.0f - (static_cast<float>(i) - 1.0f) / static_cast<float>(onionSkinNextCount));
                for (const auto& layer : frames[nextIdx].layers) {
                    if (layer.visible) {
                        sf::Sprite onionSpr(layer.texture->getTexture());
                        onionSpr.setColor(sf::Color(100, 255, 100, static_cast<sf::Uint8>(fadeOpac)));
                        sf::RenderStates oStates = innerStates;
                        oStates.blendMode = getSFMLBlendMode(layer.blendMode).blendMode;
                        window.draw(onionSpr, oStates);
                    }
                }
            }
        }
    }

    if (currentFrame >= 0 && currentFrame < static_cast<int>(frames.size())) {
        for (size_t i = 0; i < frames[currentFrame].layers.size(); ++i) {
            const auto& layer = frames[currentFrame].layers[i];
            if (layer.visible) {
                sf::Sprite spr(layer.texture->getTexture());
                spr.setColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(255.0f * layer.opacity)));
                sf::RenderStates layerStates = innerStates;
                layerStates.blendMode = getSFMLBlendMode(layer.blendMode).blendMode;
                window.draw(spr, layerStates);
            }
        }
    }

    selection.draw(window, innerStates);

    if (!isPlaying && isHoveringCanvas && (activeTool == ToolType::Brush || activeTool == ToolType::Pencil || activeTool == ToolType::Eraser)) {
        brushEngine.drawPreviewCursor(window, rawMousePos, sf::Color::White, viewScale);
    }
}

void Canvas::drawShadows(sf::RenderWindow& window, sf::Vector2f logicalSunPos, const std::vector<sf::FloatRect>& items, const std::vector<std::string>& categories, const sf::RenderStates& states) {
    for (size_t i = 0; i < items.size(); ++i) {
        bool isClutter = (categories[i] == "healing" || categories[i] == "status-cures" || categories[i] == "vitamins" || categories[i] == "clutter");
        float shadowLen = isClutter ? 30.0f : 150.0f;

        sf::Vector2f baseCenter(items[i].left + items[i].width / 2.0f, items[i].top + items[i].height);
        sf::Vector2f dir = baseCenter - logicalSunPos;
        float dist = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (dist > 0.0f) { dir.x /= dist; dir.y /= dist; }

        sf::ConvexShape shadow;
        shadow.setPointCount(4);
        shadow.setPoint(0, sf::Vector2f(items[i].left, items[i].top + items[i].height));
        shadow.setPoint(1, sf::Vector2f(items[i].left + items[i].width, items[i].top + items[i].height));
        shadow.setPoint(2, sf::Vector2f(items[i].left + items[i].width + dir.x * shadowLen, items[i].top + items[i].height + dir.y * shadowLen));
        shadow.setPoint(3, sf::Vector2f(items[i].left + dir.x * shadowLen, items[i].top + items[i].height + dir.y * shadowLen));

        shadow.setFillColor(sf::Color(0, 0, 0, 100));
        window.draw(shadow, states);
    }

    sf::CircleShape sunShape(15.0f);
    sunShape.setOrigin(15.0f, 15.0f);
    sunShape.setPosition(logicalSunPos);
    sunShape.setFillColor(sf::Color(255, 255, 200, 200));
    sunShape.setOutlineThickness(2.0f);
    sunShape.setOutlineColor(sf::Color::Yellow);
    window.draw(sunShape, states);
}

sf::FloatRect Canvas::getDrawArea() const { return drawArea; }
sf::Vector2u Canvas::getCanvasSize() const { return canvasLogicalSize; }

sf::RenderTexture* Canvas::getActiveRenderTexture(int currentFrame) {
    if (currentFrame >= 0 && currentFrame < static_cast<int>(frames.size()) && activeLayer < static_cast<int>(frames[currentFrame].layers.size())) {
        return frames[currentFrame].layers[activeLayer].texture;
    }
    return nullptr;
}

Frame* Canvas::getFrame(int index) {
    if (index >= 0 && index < static_cast<int>(frames.size())) return &frames[index];
    return nullptr;
}

const Frame* Canvas::getFrameReadOnly(int index) const {
    if (index >= 0 && index < static_cast<int>(frames.size())) return &frames[index];
    return nullptr;
}

size_t Canvas::getFrameCount() const { return frames.size(); }