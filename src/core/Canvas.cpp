#include "Canvas.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <stack>

Layer::Layer(std::string n) : name(n), visible(true), locked(false), opacity(255.0f), blendMode(BlendMode::Normal) {
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
        name = other.name;
        visible = other.visible;
        locked = other.locked;
        opacity = other.opacity;
        blendMode = other.blendMode;
        if (!texture) {
            texture = new sf::RenderTexture();
            texture->create(1920, 1080);
        }
        texture->clear(sf::Color::Transparent);
        if (other.texture) {
            sf::Sprite spr(other.texture->getTexture());
            texture->draw(spr);
            texture->display();
        }
    }
    return *this;
}

Layer::Layer(Layer&& other) noexcept : texture(other.texture), name(std::move(other.name)), visible(other.visible), locked(other.locked), opacity(other.opacity), blendMode(other.blendMode) {
    other.texture = nullptr;
}

Layer& Layer::operator=(Layer&& other) noexcept {
    if (this != &other) {
        delete texture;
        texture = other.texture;
        name = std::move(other.name);
        visible = other.visible;
        locked = other.locked;
        opacity = other.opacity;
        blendMode = other.blendMode;
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

Canvas::Canvas() : isDrawing(false), activeTool(ToolType::Brush), brushSize(5.0f), brushColor(sf::Color::Black), brushSmoothing(true),
activeLayer(1), onionSkinEnabled(false), onionSkinOpacity(85.0f), onionSkinPrevCount(1), onionSkinNextCount(0),
viewScale(1.0f), targetScale(1.0f), canvasLogicalSize(1920, 1080) {
    // Initialize preview buffer strictly to 1920x1080 to match max logic size
    previewTexture.create(1920, 1080);
    previewTexture.clear(sf::Color::Transparent);
}

void Canvas::init() {
    initCustom(1920, 1080);
}

void Canvas::initCustom(int width, int height) {
    canvasLogicalSize = sf::Vector2u(width, height);

    deskTexture.loadFromFile("assets/workbench.png", sf::IntRect(114, 702, 1669, 379));
    deskSprite.setTexture(deskTexture);
    deskSprite.setOrigin(1669.f / 2.f, 379.f / 2.f);
    deskSprite.setPosition(1920.f / 2.f, 850.f);

    canvasTexture.loadFromFile("assets/canvas.png");
    canvasSprite.setTexture(canvasTexture);

    float cScale = 700.f / canvasSprite.getLocalBounds().height;
    canvasSprite.setScale(cScale, cScale);
    canvasSprite.setOrigin(canvasSprite.getLocalBounds().width / 2.f, canvasSprite.getLocalBounds().height / 2.f);
    canvasSprite.setPosition(1920.f / 2.f, deskSprite.getPosition().y - (379.f / 2.f) - (700.f / 2.f) + 120.f);

    sf::FloatRect cBounds = canvasSprite.getGlobalBounds();
    float frameOffsetX = cBounds.width * 0.08f;
    float frameOffsetYTop = cBounds.height * 0.16f;
    float frameOffsetYBot = cBounds.height * 0.16f;

    // Strict drawing bounds
    drawArea = sf::FloatRect(
        cBounds.left + frameOffsetX,
        cBounds.top + frameOffsetYTop,
        cBounds.width - (frameOffsetX * 2.0f),
        cBounds.height - frameOffsetYTop - frameOffsetYBot
    );

    // Resize preview buffer strictly to requested width/height to enforce clipping
    if (width != previewTexture.getSize().x || height != previewTexture.getSize().y) {
        previewTexture.create(width, height);
    }
    previewTexture.clear(sf::Color::Transparent);

    frames.clear();
    frames.emplace_back();
    undoHistory.clear();
    redoHistory.clear();
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
    space.left += pad;
    space.top += pad;
    space.width -= pad * 2;
    space.height -= pad * 2;

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
}

sf::Transform Canvas::getTransform() const {
    sf::Transform t;
    t.translate(viewOffset).scale(viewScale, viewScale);
    return t;
}

sf::Transform Canvas::getInverseTransform() const {
    return getTransform().getInverse();
}

void Canvas::addFrame(int index) {
    saveUndoState();
    frames.insert(frames.begin() + index + 1, Frame());
}

void Canvas::duplicateFrame(int index) {
    saveUndoState();
    if (index >= 0 && index < static_cast<int>(frames.size())) {
        frames.insert(frames.begin() + index + 1, Frame(frames[index]));
    }
}

void Canvas::deleteFrame(int index) {
    if (frames.size() > 1 && index >= 0 && index < static_cast<int>(frames.size())) {
        saveUndoState();
        frames.erase(frames.begin() + index);
    }
}

void Canvas::clearAllFrames() {
    saveUndoState();
    frames.clear();
    frames.emplace_back();
}

void Canvas::addLayerToFrame(int frameIndex, const std::string& name) {
    if (frameIndex >= 0 && frameIndex < static_cast<int>(frames.size())) {
        saveUndoState();
        frames[frameIndex].layers.emplace_back(name);
    }
}

void Canvas::setLayerProperties(int frameIndex, int layerIndex, const std::string& name, bool visible, bool locked, float opacity, BlendMode mode) {
    if (frameIndex >= 0 && frameIndex < static_cast<int>(frames.size()) && layerIndex >= 0 && layerIndex < static_cast<int>(frames[frameIndex].layers.size())) {
        auto& l = frames[frameIndex].layers[layerIndex];
        l.name = name;
        l.visible = visible;
        l.locked = locked;
        l.opacity = opacity;
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
        }
    }
}

void Canvas::setOnionSkin(bool enabled, float opacity, int prevCount, int nextCount) {
    onionSkinEnabled = enabled;
    onionSkinOpacity = std::clamp(opacity, 0.0f, 255.0f);
    onionSkinPrevCount = prevCount;
    onionSkinNextCount = nextCount;
}

bool Canvas::isOnionSkinEnabled() const { return onionSkinEnabled; }
float Canvas::getOnionSkinOpacity() const { return onionSkinOpacity; }

void Canvas::setActiveTool(ToolType tool) {
    activeTool = tool;
    isDrawing = false; // Force reset drawing state when switching tools
}
ToolType Canvas::getActiveTool() const { return activeTool; }

// Robust Flood Fill that respects texture boundaries and connects properly
void Canvas::executeFloodFill(sf::Vector2i startPoint, sf::Color targetColor, sf::Color replacementColor, sf::Image& image) {
    if (targetColor == replacementColor) return;

    int w = image.getSize().x;
    int h = image.getSize().y;

    if (startPoint.x < 0 || startPoint.x >= w || startPoint.y < 0 || startPoint.y >= h) return;
    if (image.getPixel(startPoint.x, startPoint.y) != targetColor) return;

    std::stack<sf::Vector2i> stack;
    stack.push(startPoint);

    while (!stack.empty()) {
        sf::Vector2i pt = stack.top();
        stack.pop();

        if (pt.x < 0 || pt.x >= w || pt.y < 0 || pt.y >= h) continue;
        if (image.getPixel(pt.x, pt.y) != targetColor) continue;

        image.setPixel(pt.x, pt.y, replacementColor);

        stack.push(sf::Vector2i(pt.x + 1, pt.y));
        stack.push(sf::Vector2i(pt.x - 1, pt.y));
        stack.push(sf::Vector2i(pt.x, pt.y + 1));
        stack.push(sf::Vector2i(pt.x, pt.y - 1));
    }
}

void Canvas::handleMousePressed(sf::Vector2f logicalPos, bool middleClick, int currentFrame) {
    if (currentFrame < 0 || currentFrame >= static_cast<int>(frames.size())) return;

    // Convert generic logical coordinate back into local texture coordinates strictly clamped
    sf::Vector2f texScale(1920.f / drawArea.width, 1080.f / drawArea.height);
    sf::Vector2f localPos((logicalPos.x - drawArea.left) * texScale.x, (logicalPos.y - drawArea.top) * texScale.y);

    if (drawArea.contains(logicalPos)) {
        if (!frames[currentFrame].layers[activeLayer].locked && frames[currentFrame].layers[activeLayer].visible) {

            // Eyedropper early return guarantees no tool locking
            if (activeTool == ToolType::Eyedropper) {
                sf::Image img = frames[currentFrame].layers[activeLayer].texture->getTexture().copyToImage();
                sf::Vector2i pixelPos(static_cast<int>(localPos.x), static_cast<int>(localPos.y));
                if (pixelPos.x >= 0 && pixelPos.y >= 0 && pixelPos.x < static_cast<int>(img.getSize().x) && pixelPos.y < static_cast<int>(img.getSize().y)) {
                    brushColor = img.getPixel(pixelPos.x, pixelPos.y);
                }
                return; // Prevent fallthrough
            }

            // Fill respects boundaries and converts correctly
            if (activeTool == ToolType::Fill) {
                saveUndoState();
                sf::Image img = frames[currentFrame].layers[activeLayer].texture->getTexture().copyToImage();
                sf::Vector2i pixelPos(static_cast<int>(localPos.x), static_cast<int>(localPos.y));

                if (pixelPos.x >= 0 && pixelPos.y >= 0 && pixelPos.x < static_cast<int>(img.getSize().x) && pixelPos.y < static_cast<int>(img.getSize().y)) {
                    sf::Color targetCol = img.getPixel(pixelPos.x, pixelPos.y);
                    executeFloodFill(pixelPos, targetCol, brushColor, img);

                    sf::Texture tex;
                    tex.loadFromImage(img);
                    sf::Sprite spr(tex);
                    frames[currentFrame].layers[activeLayer].texture->clear(sf::Color::Transparent);
                    frames[currentFrame].layers[activeLayer].texture->draw(spr);
                    frames[currentFrame].layers[activeLayer].texture->display();
                }
                return;
            }

            if (!middleClick) {
                saveUndoState();
                isDrawing = true;
                startPos = localPos;
                lastPos = localPos;
                previewTexture.clear(sf::Color::Transparent);
            }
        }
    }
}

void Canvas::bakePreviewToLayer(int currentFrame) {
    if (currentFrame < 0 || currentFrame >= static_cast<int>(frames.size())) return;
    previewTexture.display();
    sf::Sprite spr(previewTexture.getTexture());
    frames[currentFrame].layers[activeLayer].texture->draw(spr);
    frames[currentFrame].layers[activeLayer].texture->display();
    previewTexture.clear(sf::Color::Transparent);
}

void Canvas::handleMouseReleased(sf::Vector2f logicalPos, int currentFrame) {
    if (isDrawing && currentFrame >= 0 && currentFrame < static_cast<int>(frames.size())) {
        if (activeTool == ToolType::Line || activeTool == ToolType::Rectangle || activeTool == ToolType::Circle) {
            bakePreviewToLayer(currentFrame);
        }
        isDrawing = false;
    }
}

void Canvas::handleMouseMoved(sf::Vector2f logicalPos, int currentFrame) {
    // Tool safety check
    if (activeTool == ToolType::Eyedropper || activeTool == ToolType::Fill) return;

    if (isDrawing && currentFrame >= 0 && currentFrame < static_cast<int>(frames.size())) {
        if (frames[currentFrame].layers[activeLayer].locked || !frames[currentFrame].layers[activeLayer].visible) {
            isDrawing = false;
            return;
        }

        // Convert generic logical coordinate back into local texture coordinates for clamped drawing
        sf::Vector2f texScale(1920.f / drawArea.width, 1080.f / drawArea.height);
        sf::Vector2f localPos((logicalPos.x - drawArea.left) * texScale.x, (logicalPos.y - drawArea.top) * texScale.y);

        sf::RenderTexture* targetTex = (activeTool == ToolType::Brush || activeTool == ToolType::Pencil || activeTool == ToolType::Eraser) ?
            frames[currentFrame].layers[activeLayer].texture : &previewTexture;

        if (activeTool == ToolType::Line || activeTool == ToolType::Rectangle || activeTool == ToolType::Circle) {
            targetTex->clear(sf::Color::Transparent); // Clear preview buffer every frame
        }

        if (activeTool == ToolType::Brush || activeTool == ToolType::Eraser || activeTool == ToolType::Pencil || activeTool == ToolType::Line) {
            sf::Vector2f d = localPos - lastPos;
            if (activeTool == ToolType::Line) d = localPos - startPos;

            float length = std::sqrt(d.x * d.x + d.y * d.y);
            sf::RectangleShape line(sf::Vector2f(length, brushSize));
            line.setOrigin(0, brushSize / 2.f);
            line.setPosition(activeTool == ToolType::Line ? startPos : lastPos);
            line.setRotation(std::atan2(d.y, d.x) * 180.f / 3.14159265f);

            sf::Color drawCol = (activeTool == ToolType::Eraser) ? sf::Color::Transparent : brushColor;
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
        else if (activeTool == ToolType::Rectangle) {
            sf::Vector2f size(localPos.x - startPos.x, localPos.y - startPos.y);
            sf::RectangleShape rect(sf::Vector2f(std::abs(size.x), std::abs(size.y)));
            rect.setPosition(std::min(startPos.x, localPos.x), std::min(startPos.y, localPos.y));
            rect.setFillColor(sf::Color::Transparent);
            rect.setOutlineThickness(brushSize);
            rect.setOutlineColor(brushColor);
            targetTex->draw(rect);
        }
        else if (activeTool == ToolType::Circle) {
            float radius = std::sqrt(std::pow(localPos.x - startPos.x, 2) + std::pow(localPos.y - startPos.y, 2));
            sf::CircleShape circ(radius);
            circ.setOrigin(radius, radius);
            circ.setPosition(startPos);
            circ.setFillColor(sf::Color::Transparent);
            circ.setOutlineThickness(brushSize);
            circ.setOutlineColor(brushColor);
            targetTex->draw(circ);
        }

        targetTex->display();
        if (activeTool == ToolType::Brush || activeTool == ToolType::Eraser || activeTool == ToolType::Pencil) {
            lastPos = localPos;
        }

        // Auto-release drawing if user drags outside the logical canvas bounds
        if (!drawArea.contains(logicalPos)) {
            if (activeTool == ToolType::Line || activeTool == ToolType::Rectangle || activeTool == ToolType::Circle) {
                bakePreviewToLayer(currentFrame);
            }
            isDrawing = false;
        }

    }
}

void Canvas::setBrushSize(float size) { brushSize = std::clamp(size, 1.0f, 100.0f); }
float Canvas::getBrushSize() const { return brushSize; }
void Canvas::setBrushColor(sf::Color color) { brushColor = color; }
sf::Color Canvas::getBrushColor() const { return brushColor; }
void Canvas::setBrushSmoothing(bool smoothing) { brushSmoothing = smoothing; }

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
    }
}

void Canvas::redo() {
    if (!redoHistory.empty()) {
        undoHistory.push_back(frames);
        frames = redoHistory.back();
        redoHistory.pop_back();
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
    case BlendMode::Normal:
    default: return sf::RenderStates(sf::BlendAlpha);
    }
}

void Canvas::draw(sf::RenderWindow& window, int currentFrame, bool isPlaying, const sf::RenderStates& states) {
    window.draw(deskSprite, states);
    window.draw(canvasSprite, states); // Draw the presentation frame first

    // Setup an explicit scale state mapping the logical 1920x1080 textures directly to the bounds of the presentation frame
    sf::Vector2f texScale(drawArea.width / 1920.f, drawArea.height / 1080.f);

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
                        sf::RenderStates onionStates = innerStates;
                        onionStates.blendMode = getSFMLBlendMode(layer.blendMode).blendMode;
                        window.draw(onionSpr, onionStates);
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
                        sf::RenderStates onionStates = innerStates;
                        onionStates.blendMode = getSFMLBlendMode(layer.blendMode).blendMode;
                        window.draw(onionSpr, onionStates);
                    }
                }
            }
        }
    }

    if (currentFrame >= 0 && currentFrame < static_cast<int>(frames.size())) {
        for (const auto& layer : frames[currentFrame].layers) {
            if (layer.visible) {
                sf::Sprite spr(layer.texture->getTexture());
                spr.setColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(layer.opacity)));
                sf::RenderStates layerStates = innerStates;
                layerStates.blendMode = getSFMLBlendMode(layer.blendMode).blendMode;
                window.draw(spr, layerStates);
            }
        }
    }

    // Draw shape preview strictly using the inner bounds transform so it never spills over the UI
    if (isDrawing && (activeTool == ToolType::Line || activeTool == ToolType::Rectangle || activeTool == ToolType::Circle)) {
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