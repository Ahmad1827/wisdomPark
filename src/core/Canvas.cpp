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
Frame::Frame(const Frame& other) { for (const auto& l : other.layers) layers.emplace_back(l); }
Frame& Frame::operator=(const Frame& other) { if (this != &other) { layers.clear(); for (const auto& l : other.layers) layers.emplace_back(l); } return *this; }
Frame::Frame(Frame&& other) noexcept = default;
Frame& Frame::operator=(Frame&& other) noexcept = default;

Canvas::Canvas() : isDrawing(false), activeTool(ToolType::Brush), brushSize(5.0f), brushHardness(1.0f),
primaryColor(sf::Color::Black), secondaryColor(sf::Color::White), brushSmoothing(true),
fillTolerance(0.f), fillContiguous(true),
activeLayer(1), onionSkinEnabled(false), onionSkinOpacity(85.0f), onionSkinPrevCount(1), onionSkinNextCount(0),
viewScale(1.0f), targetScale(1.0f), canvasLogicalSize(1920, 1080),
hasSelection(false), isMovingSelection(false), selectionDashOffset(0.f) {
    previewTexture.create(1920, 1080);
    previewTexture.clear(sf::Color::Transparent);
}

void Canvas::init() { initCustom(1920, 1080); }

void Canvas::initCustom(int width, int height) {
    canvasLogicalSize = sf::Vector2u(width, height);

    if (!deskTexture.loadFromFile("assets/workbench.png", sf::IntRect(114, 702, 1669, 379))) {
        std::cerr << "CRITICAL: Missing workbench.png" << std::endl;
    }
    deskSprite.setTexture(deskTexture);
    deskSprite.setOrigin(1669.f / 2.f, 379.f / 2.f);
    deskSprite.setPosition(1920.f / 2.f, 850.f);

    if (!canvasTexture.loadFromFile("assets/canvas.png")) {
        std::cerr << "CRITICAL: Missing canvas.png" << std::endl;
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
        previewTexture.create(width, height);
    }
    previewTexture.clear(sf::Color::Transparent);

    frames.clear();
    frames.emplace_back();
    undoHistory.clear();
    redoHistory.clear();
    hasSelection = false;
}

void Canvas::updateTransform(float dt, sf::FloatRect space) {
    sf::FloatRect dBounds = deskSprite.getGlobalBounds();
    sf::FloatRect cBounds = canvasSprite.getGlobalBounds();

    float left = std::min(dBounds.left, cBounds.left);
    float top = std::min(dBounds.top, cBounds.top);
    float right = std::max(dBounds.left + dBounds.width, cBounds.left + cBounds.width);
    float bottom = std::max(dBounds.top + dBounds.height, cBounds.top + cBounds.height);

    float unionW = right - left;
    float unionH = bottom - top;

    float pad = 60.f;
    space.left += pad; space.top += pad; space.width -= pad * 2; space.height -= pad * 2;

    float sX = space.width / unionW;
    float sY = space.height / unionH;
    targetScale = std::min(sX, sY);

    float spaceCX = space.left + space.width / 2.f;
    float spaceCY = space.top + space.height / 2.f;
    float unionCX = left + unionW / 2.f;
    float unionCY = top + unionH / 2.f;

    targetOffset.x = spaceCX - (unionCX * targetScale);
    targetOffset.y = spaceCY - (unionCY * targetScale);

    viewScale += (targetScale - viewScale) * 12.f * dt;
    viewOffset.x += (targetOffset.x - viewOffset.x) * 12.f * dt;
    viewOffset.y += (targetOffset.y - viewOffset.y) * 12.f * dt;

    if (hasSelection) selectionDashOffset += 20.f * dt;
}

sf::Transform Canvas::getTransform() const {
    sf::Transform t;
    t.translate(viewOffset).scale(viewScale, viewScale);
    return t;
}

sf::Transform Canvas::getInverseTransform() const { return getTransform().getInverse(); }

void Canvas::addFrame(int index) { saveUndoState(); frames.insert(frames.begin() + index + 1, Frame()); }
void Canvas::duplicateFrame(int index) { saveUndoState(); if (index >= 0 && index < static_cast<int>(frames.size())) frames.insert(frames.begin() + index + 1, Frame(frames[index])); }
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
            if (activeLayer >= static_cast<int>(frames[frameIndex].layers.size())) activeLayer = static_cast<int>(frames[frameIndex].layers.size()) - 1;
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
        l.name = name; l.visible = visible; l.locked = locked; l.opacity = std::clamp(opacity, 0.f, 1.f); l.blendMode = mode;
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

void Canvas::setActiveLayer(int index) { activeLayer = std::max(0, index); }
int Canvas::getActiveLayer() const { return activeLayer; }

void Canvas::setOnionSkin(bool enabled, float opacity, int prevCount, int nextCount) {
    onionSkinEnabled = enabled; onionSkinOpacity = std::clamp(opacity, 0.0f, 255.0f);
    onionSkinPrevCount = prevCount; onionSkinNextCount = nextCount;
}

bool Canvas::isOnionSkinEnabled() const { return onionSkinEnabled; }
float Canvas::getOnionSkinOpacity() const { return onionSkinOpacity; }

void Canvas::commitSelection(int currentFrame) {
    if (!hasSelection) return;
    if (currentFrame >= 0 && currentFrame < static_cast<int>(frames.size())) {
        saveUndoState();
        sf::Sprite pasteSpr(selectionTexture);
        pasteSpr.setPosition(selectionBounds.left + selectionOffset.x, selectionBounds.top + selectionOffset.y);
        frames[currentFrame].layers[activeLayer].texture->draw(pasteSpr);
        frames[currentFrame].layers[activeLayer].texture->display();
    }
    hasSelection = false;
    isMovingSelection = false;
}

void Canvas::setActiveTool(ToolType tool) {
    if (activeTool == ToolType::Select && hasSelection && tool != ToolType::Select) {
        // Find a way to pass current frame, or rely on undo state tracking elsewhere.
        // For simplicity, selection commits are typically handled by UI/Event manager before tool switch.
        hasSelection = false;
        isMovingSelection = false;
    }
    activeTool = tool;
    isDrawing = false;
}
ToolType Canvas::getActiveTool() const { return activeTool; }

bool Canvas::colorMatches(const sf::Color& a, const sf::Color& b) const {
    if (fillTolerance <= 0.f) return a == b;
    float dist = std::abs(a.r - b.r) + std::abs(a.g - b.g) + std::abs(a.b - b.b) + std::abs(a.a - b.a);
    return (dist / (4.0f * 255.0f)) <= fillTolerance;
}

void Canvas::executeGlobalFill(sf::Color targetColor, sf::Color replacementColor, sf::Image& image) {
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

    std::queue<sf::Vector2i> q;
    q.push(startPoint);

    while (!q.empty()) {
        sf::Vector2i pt = q.front();
        q.pop();

        int x = pt.x;
        int y = pt.y;

        while (x > 0 && colorMatches(image.getPixel(x - 1, y), targetColor)) x--;

        bool spanAbove = false;
        bool spanBelow = false;

        while (x < w && colorMatches(image.getPixel(x, y), targetColor)) {
            image.setPixel(x, y, replacementColor);

            if (y > 0) {
                bool matchAbove = colorMatches(image.getPixel(x, y - 1), targetColor);
                if (!spanAbove && matchAbove) { q.push(sf::Vector2i(x, y - 1)); spanAbove = true; }
                else if (spanAbove && !matchAbove) spanAbove = false;
            }
            if (y < h - 1) {
                bool matchBelow = colorMatches(image.getPixel(x, y + 1), targetColor);
                if (!spanBelow && matchBelow) { q.push(sf::Vector2i(x, y + 1)); spanBelow = true; }
                else if (spanBelow && !matchBelow) spanBelow = false;
            }
            x++;
        }
    }
}

void Canvas::handleMousePressed(sf::Vector2f logicalPos, bool rightClick, int currentFrame) {
    if (currentFrame < 0 || currentFrame >= static_cast<int>(frames.size())) return;

    sf::Vector2f texScale(canvasLogicalSize.x / drawArea.width, canvasLogicalSize.y / drawArea.height);
    sf::Vector2f localPos((logicalPos.x - drawArea.left) * texScale.x, (logicalPos.y - drawArea.top) * texScale.y);

    if (drawArea.contains(logicalPos)) {
        if (!frames[currentFrame].layers[activeLayer].locked && frames[currentFrame].layers[activeLayer].visible) {

            sf::Color drawCol = rightClick ? secondaryColor : primaryColor;

            if (activeTool == ToolType::Select) {
                if (hasSelection) {
                    sf::FloatRect currentBounds(selectionBounds.left + selectionOffset.x, selectionBounds.top + selectionOffset.y, selectionBounds.width, selectionBounds.height);
                    if (currentBounds.contains(localPos)) {
                        isMovingSelection = true;
                        startPos = localPos;
                        return;
                    }
                    else {
                        commitSelection(currentFrame);
                    }
                }

                hasSelection = false;
                isDrawing = true;
                startPos = localPos;
                lastPos = localPos;
                return;
            }

            if (hasSelection) { commitSelection(currentFrame); }

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
            previewTexture.clear(sf::Color::Transparent);
        }
    }
}

void Canvas::handleMouseReleased(sf::Vector2f logicalPos, int currentFrame) {
    if (activeTool == ToolType::Select && isDrawing) {
        isDrawing = false;

        sf::Vector2f texScale(canvasLogicalSize.x / drawArea.width, canvasLogicalSize.y / drawArea.height);
        sf::Vector2f localPos((logicalPos.x - drawArea.left) * texScale.x, (logicalPos.y - drawArea.top) * texScale.y);

        float left = std::min(startPos.x, localPos.x);
        float top = std::min(startPos.y, localPos.y);
        float width = std::abs(localPos.x - startPos.x);
        float height = std::abs(localPos.y - startPos.y);

        if (width > 5.f && height > 5.f) {
            hasSelection = true;
            selectionBounds = sf::FloatRect(left, top, width, height);
            selectionOffset = sf::Vector2f(0.f, 0.f);

            saveUndoState();

            sf::Image layerImg = frames[currentFrame].layers[activeLayer].texture->getTexture().copyToImage();
            sf::Image extImg;
            extImg.create(static_cast<unsigned int>(width), static_cast<unsigned int>(height), sf::Color::Transparent);
            extImg.copy(layerImg, 0, 0, sf::IntRect(static_cast<int>(left), static_cast<int>(top), static_cast<int>(width), static_cast<int>(height)));
            selectionTexture.loadFromImage(extImg);

            sf::RectangleShape clearRect(sf::Vector2f(width, height));
            clearRect.setPosition(left, top);
            clearRect.setFillColor(sf::Color::Transparent);
            frames[currentFrame].layers[activeLayer].texture->draw(clearRect, sf::RenderStates(sf::BlendNone));
            frames[currentFrame].layers[activeLayer].texture->display();
        }
    }

    if (activeTool == ToolType::Select && isMovingSelection) {
        isMovingSelection = false;
        selectionBounds.left += selectionOffset.x;
        selectionBounds.top += selectionOffset.y;
        selectionOffset = sf::Vector2f(0.f, 0.f);
    }

    if (isDrawing && currentFrame >= 0 && currentFrame < static_cast<int>(frames.size())) {
        isDrawing = false;
    }
}

void Canvas::handleMouseMoved(sf::Vector2f logicalPos, int currentFrame) {
    if (activeTool == ToolType::Fill) return;

    sf::Vector2f texScale(canvasLogicalSize.x / drawArea.width, canvasLogicalSize.y / drawArea.height);
    sf::Vector2f localPos((logicalPos.x - drawArea.left) * texScale.x, (logicalPos.y - drawArea.top) * texScale.y);

    if (activeTool == ToolType::Select && isMovingSelection) {
        selectionOffset = localPos - startPos;
        return;
    }

    if (isDrawing && currentFrame >= 0 && currentFrame < static_cast<int>(frames.size())) {
        if (frames[currentFrame].layers[activeLayer].locked || !frames[currentFrame].layers[activeLayer].visible) {
            isDrawing = false;
            return;
        }

        if (activeTool == ToolType::Select) {
            previewTexture.clear(sf::Color::Transparent);
            sf::RectangleShape rect(sf::Vector2f(std::abs(localPos.x - startPos.x), std::abs(localPos.y - startPos.y)));
            rect.setPosition(std::min(startPos.x, localPos.x), std::min(startPos.y, localPos.y));
            rect.setFillColor(sf::Color(0, 122, 204, 50));
            rect.setOutlineThickness(1.f);
            rect.setOutlineColor(sf::Color::White);
            previewTexture.draw(rect);
            previewTexture.display();
            return;
        }

        sf::RenderTexture* targetTex = frames[currentFrame].layers[activeLayer].texture;

        if (activeTool == ToolType::Brush || activeTool == ToolType::Eraser || activeTool == ToolType::Pencil) {
            sf::Vector2f d = localPos - lastPos;
            float length = std::sqrt(d.x * d.x + d.y * d.y);

            sf::Color drawCol = (activeTool == ToolType::Eraser) ? sf::Color::Transparent : primaryColor;

            sf::Uint8 alpha = static_cast<sf::Uint8>(255 * brushHardness);
            if (activeTool != ToolType::Eraser) drawCol.a = alpha;

            sf::RectangleShape line(sf::Vector2f(length, brushSize));
            line.setOrigin(0, brushSize / 2.f);
            line.setPosition(lastPos);
            line.setRotation(std::atan2(d.y, d.x) * 180.f / 3.14159265f);
            line.setFillColor(drawCol);

            sf::CircleShape circle(brushSize / 2.f);
            circle.setOrigin(brushSize / 2.f, brushSize / 2.f);
            circle.setPosition(localPos);
            circle.setFillColor(drawCol);

            sf::RenderStates rs = (activeTool == ToolType::Eraser) ? sf::RenderStates(sf::BlendNone) : sf::RenderStates::Default;

            targetTex->draw(line, rs);
            if (brushSmoothing || activeTool != ToolType::Pencil) {
                targetTex->draw(circle, rs);
            }
        }

        targetTex->display();
        lastPos = localPos;

        if (!drawArea.contains(logicalPos)) { isDrawing = false; }
    }
}

void Canvas::setBrushSize(float size) { brushSize = std::clamp(size, 1.0f, 100.0f); }
float Canvas::getBrushSize() const { return brushSize; }
void Canvas::setBrushHardness(float hardness) { brushHardness = std::clamp(hardness, 0.1f, 1.0f); }
void Canvas::setPrimaryColor(sf::Color color) { primaryColor = color; }
void Canvas::setSecondaryColor(sf::Color color) { secondaryColor = color; }
sf::Color Canvas::getPrimaryColor() const { return primaryColor; }
void Canvas::setBrushSmoothing(bool smoothing) { brushSmoothing = smoothing; }
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
        hasSelection = false;
    }
}

void Canvas::redo() {
    if (!redoHistory.empty()) {
        undoHistory.push_back(frames);
        frames = redoHistory.back();
        redoHistory.pop_back();
        hasSelection = false;
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

    sf::Vector2f texScale(drawArea.width / canvasLogicalSize.x, drawArea.height / canvasLogicalSize.y);
    sf::Transform innerTransform = states.transform;
    innerTransform.translate(drawArea.left, drawArea.top);
    innerTransform.scale(texScale);
    sf::RenderStates innerStates = states;
    innerStates.transform = innerTransform;

    if (!isPlaying && onionSkinEnabled) {
        for (int i = 1; i <= onionSkinPrevCount; ++i) {
            int prevIdx = currentFrame - i;
            if (prevIdx >= 0 && prevIdx < static_cast<int>(frames.size())) {
                float fadeOpac = onionSkinOpacity * (1.0f - (float)(i - 1) / onionSkinPrevCount);
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
                float fadeOpac = onionSkinOpacity * (1.0f - (float)(i - 1) / onionSkinNextCount);
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
                spr.setColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(255 * layer.opacity)));
                sf::RenderStates layerStates = innerStates;
                layerStates.blendMode = getSFMLBlendMode(layer.blendMode).blendMode;
                window.draw(spr, layerStates);

                if (hasSelection && static_cast<int>(i) == activeLayer) {
                    sf::Sprite selSpr(selectionTexture);
                    selSpr.setPosition(selectionBounds.left + selectionOffset.x, selectionBounds.top + selectionOffset.y);
                    window.draw(selSpr, layerStates);

                    sf::RectangleShape ants(sf::Vector2f(selectionBounds.width, selectionBounds.height));
                    ants.setPosition(selectionBounds.left + selectionOffset.x, selectionBounds.top + selectionOffset.y);
                    ants.setFillColor(sf::Color::Transparent);
                    ants.setOutlineThickness(1.f / targetScale);

                    if (static_cast<int>(selectionDashOffset) % 2 == 0) ants.setOutlineColor(sf::Color::Black);
                    else ants.setOutlineColor(sf::Color::White);

                    window.draw(ants, innerStates);
                }
            }
        }
    }

    if (isDrawing && activeTool == ToolType::Select) {
        sf::Sprite prevSpr(previewTexture.getTexture());
        window.draw(prevSpr, innerStates);
    }
}

void Canvas::drawShadows(sf::RenderWindow& window, sf::Vector2f logicalSunPos, const std::vector<sf::FloatRect>& items, const std::vector<std::string>& categories, const sf::RenderStates& states) {
    for (size_t i = 0; i < items.size(); ++i) {
        bool isClutter = (categories[i] == "healing" || categories[i] == "status-cures" || categories[i] == "vitamins" || categories[i] == "clutter");
        float shadowLen = isClutter ? 30.0f : 150.0f;

        sf::Vector2f baseCenter(items[i].left + items[i].width / 2.0f, items[i].top + items[i].height);
        sf::Vector2f dir = baseCenter - logicalSunPos;
        float dist = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (dist > 0) { dir.x /= dist; dir.y /= dist; }

        sf::ConvexShape shadow;
        shadow.setPointCount(4);
        shadow.setPoint(0, sf::Vector2f(items[i].left, items[i].top + items[i].height));
        shadow.setPoint(1, sf::Vector2f(items[i].left + items[i].width, items[i].top + items[i].height));
        shadow.setPoint(2, sf::Vector2f(items[i].left + items[i].width + dir.x * shadowLen, items[i].top + items[i].height + dir.y * shadowLen));
        shadow.setPoint(3, sf::Vector2f(items[i].left + dir.x * shadowLen, items[i].top + items[i].height + dir.y * shadowLen));

        shadow.setFillColor(sf::Color(0, 0, 0, 100));
        window.draw(shadow, states);
    }

    sf::CircleShape sunShape(15);
    sunShape.setOrigin(15, 15);
    sunShape.setPosition(logicalSunPos);
    sunShape.setFillColor(sf::Color(255, 255, 200, 200));
    sunShape.setOutlineThickness(2);
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