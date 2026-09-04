#include "Canvas.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <stack>
#include <queue>
#include "TextSystem.h"

static const sf::RenderWindow* g_activeWindow = nullptr;

const int DEFAULT_NORMAL_W = 1280;
const int DEFAULT_NORMAL_H = 720;
const int DEFAULT_PIXEL_W = 64;
const int DEFAULT_PIXEL_H = 64;

Layer::Layer(std::string n) : name(n), visible(true), locked(false), opacity(1.0f), blendMode(BlendMode::Normal), persistent(false), colorTag(0), isImageResource(false) {
    texture = std::make_shared<sf::RenderTexture>();
    texture->create(1, 1);
    texture->clear(sf::Color::Transparent);
    texture->setSmooth(false);
}

Layer::Layer(const Layer& other) : name(other.name), visible(other.visible), locked(other.locked), opacity(other.opacity), blendMode(other.blendMode), persistent(other.persistent), colorTag(other.colorTag), isImageResource(other.isImageResource) {
    if (persistent) {
        texture = other.texture;
    }
    else {
        texture = std::make_shared<sf::RenderTexture>();
        if (other.texture) {
            texture->create(other.texture->getSize().x, other.texture->getSize().y);
            texture->clear(sf::Color::Transparent);
            texture->setSmooth(other.texture->isSmooth());
            sf::Sprite spr(other.texture->getTexture());
            texture->draw(spr, sf::RenderStates(sf::BlendNone));
            texture->display();
        }
        else {
            texture->create(1, 1);
        }
    }
    if (other.staticTexture) {
        staticTexture = other.staticTexture;
    }
}

Layer& Layer::operator=(const Layer& other) {
    if (this != &other) {
        name = other.name;
        visible = other.visible;
        locked = other.locked;
        opacity = other.opacity;
        blendMode = other.blendMode;
        persistent = other.persistent;
        colorTag = other.colorTag;
        isImageResource = other.isImageResource;
        if (persistent) {
            texture = other.texture;
        }
        else {
            if (!texture) {
                texture = std::make_shared<sf::RenderTexture>();
            }
            if (other.texture) {
                texture->create(other.texture->getSize().x, other.texture->getSize().y);
                texture->setSmooth(other.texture->isSmooth());
                texture->clear(sf::Color::Transparent);
                sf::Sprite spr(other.texture->getTexture());
                texture->draw(spr, sf::RenderStates(sf::BlendNone));
                texture->display();
            }
            else {
                texture->create(1, 1);
            }
        }
        if (other.staticTexture) {
            staticTexture = other.staticTexture;
        }
    }
    return *this;
}

Layer::Layer(Layer&& other) noexcept : texture(std::move(other.texture)), name(std::move(other.name)), visible(other.visible), locked(other.locked), opacity(other.opacity), blendMode(other.blendMode), persistent(other.persistent), colorTag(other.colorTag), isImageResource(other.isImageResource), staticTexture(std::move(other.staticTexture)) {}

Layer& Layer::operator=(Layer&& other) noexcept {
    if (this != &other) {
        texture = std::move(other.texture);
        name = std::move(other.name);
        visible = other.visible;
        locked = other.locked;
        opacity = other.opacity;
        blendMode = other.blendMode;
        persistent = other.persistent;
        colorTag = other.colorTag;
        isImageResource = other.isImageResource;
        staticTexture = std::move(other.staticTexture);
    }
    return *this;
}

Frame::Frame() {
    layers.emplace_back("Background");
    layers.emplace_back("Artwork");
    layers[0].persistent = true;
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
shiftAnchor(0.f, 0.f), hasShiftAnchor(false),
activeTool(ToolType::Brush), primaryColor(sf::Color::Black), secondaryColor(sf::Color::White),
fillTolerance(0.f), fillContiguous(true),
activeLayer(1), onionSkinEnabled(true), onionSkinPrevOpacity(89.25f), onionSkinNextOpacity(89.25f), onionSkinPrevCount(1), onionSkinNextCount(1),
viewScale(1.0f), targetScale(1.0f), canvasLogicalSize(DEFAULT_NORMAL_W, DEFAULT_NORMAL_H), zoomMultiplier(1.0f), panOffset(0.f, 0.f),
isPixelMode(false), pixelBrushSize(1), pixelGridEnabled(true), pixelSnapEnabled(true), tileModeX(false), tileModeY(false), pixelPerfectEnabled(false), isDirty(false),
transformMode(TransformState::None), pendingTransform(false), currentRotation(0.0f), currentScale(1.0f, 1.0f) {
    brushEngine.initDefaultPresets();
}

void Canvas::init() {
    if (isPixelMode) initCustom(DEFAULT_PIXEL_W, DEFAULT_PIXEL_H);
    else initCustom(DEFAULT_NORMAL_W, DEFAULT_NORMAL_H);
}

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

    float maxViewportHeight = 700.f;
    float maxViewportWidth = 1400.f;

    float aspectCanvas = static_cast<float>(canvasLogicalSize.x) / static_cast<float>(canvasLogicalSize.y);

    float targetHeight = maxViewportHeight;
    float targetWidth = targetHeight * aspectCanvas;

    if (targetWidth > maxViewportWidth) {
        targetWidth = maxViewportWidth;
        targetHeight = targetWidth / aspectCanvas;
    }

    canvasSprite.setScale(targetWidth / canvasSprite.getLocalBounds().width, targetHeight / canvasSprite.getLocalBounds().height);
    canvasSprite.setOrigin(canvasSprite.getLocalBounds().width / 2.f, canvasSprite.getLocalBounds().height / 2.f);
    canvasSprite.setPosition(1920.f / 2.f, deskSprite.getPosition().y - (379.f / 2.f) - (targetHeight / 2.f) + 120.f);

    sf::FloatRect cBounds = canvasSprite.getGlobalBounds();
    drawArea = cBounds;

    frames.clear();
    frames.emplace_back();
    for (auto& l : frames[0].layers) {
        l.texture->create(canvasLogicalSize.x, canvasLogicalSize.y);
        l.texture->clear(sf::Color::Transparent);
        l.texture->setSmooth(false);
    }

    undoHistory.clear();
    redoHistory.clear();
    selection.clearSelection();
    resetView();
    isDirty = false;

    hasFrameAssets = frameTex[0].loadFromFile("assets/textures/frame/top_left.png") &&
        frameTex[1].loadFromFile("assets/textures/frame/top.png") &&
        frameTex[2].loadFromFile("assets/textures/frame/top_right.png") &&
        frameTex[3].loadFromFile("assets/textures/frame/left.png") &&
        frameTex[4].loadFromFile("assets/textures/frame/right.png") &&
        frameTex[5].loadFromFile("assets/textures/frame/bottom_left.png") &&
        frameTex[6].loadFromFile("assets/textures/frame/bottom.png") &&
        frameTex[7].loadFromFile("assets/textures/frame/bottom_right.png");

    if (hasFrameAssets) {
        for (int i = 0; i < 8; ++i) {
            frameTex[i].setSmooth(false);
        }
        frameTex[1].setRepeated(true);
        frameTex[3].setRepeated(true);
        frameTex[4].setRepeated(true);
        frameTex[6].setRepeated(true);
    }
}

void Canvas::zoom(float delta) {
    if (!g_activeWindow) {
        zoomMultiplier *= (1.0f + delta * 0.1f);
        zoomMultiplier = std::max(0.1f, std::min(zoomMultiplier, 50.0f));
        return;
    }

    float oldZoom = zoomMultiplier;

    zoomMultiplier *= (1.0f + delta * 0.1f);
    zoomMultiplier = std::max(0.1f, std::min(zoomMultiplier, 50.0f));

    if (oldZoom == zoomMultiplier) return;

    float ratio = zoomMultiplier / oldZoom;

    sf::Vector2i mousePosI = sf::Mouse::getPosition(*g_activeWindow);
    sf::Vector2f mousePos(static_cast<float>(mousePosI.x), static_cast<float>(mousePosI.y));

    sf::Vector2f screenCenter(
        static_cast<float>(g_activeWindow->getSize().x) / 2.0f,
        static_cast<float>(g_activeWindow->getSize().y) / 2.0f
    );

    panOffset.x = (mousePos.x - screenCenter.x) - ratio * (mousePos.x - screenCenter.x - panOffset.x);
    panOffset.y = (mousePos.y - screenCenter.y) - ratio * (mousePos.y - screenCenter.y - panOffset.y);
}

void Canvas::pan(sf::Vector2f delta) {
    panOffset += delta;
}

void Canvas::resetView() {
    zoomMultiplier = 1.0f;
    panOffset = { 0.f, 0.f };
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
    float baseScale = std::min(static_cast<float>(sX), static_cast<float>(sY));
    targetScale = baseScale * zoomMultiplier;

    float spaceCX = space.left + space.width / 2.f;
    float spaceCY = space.top + space.height / 2.f;
    float unionCX = left + unionW / 2.f;
    float unionCY = top + unionH / 2.f;

    targetOffset.x = spaceCX - (unionCX * targetScale) + panOffset.x;
    targetOffset.y = spaceCY - (unionCY * targetScale) + panOffset.y;

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
    Frame newFrame;
    newFrame.layers.clear();
    int srcIndex = (index >= 0 && index < static_cast<int>(frames.size())) ? index : 0;

    for (const auto& l : frames[srcIndex].layers) {
        Layer newL(l.name);
        newL.visible = l.visible;
        newL.locked = l.locked;
        newL.opacity = l.opacity;
        newL.blendMode = l.blendMode;
        newL.persistent = l.persistent;
        newL.colorTag = l.colorTag;
        if (l.persistent) {
            newL.texture = l.texture;
        }
        else {
            newL.texture->create(canvasLogicalSize.x, canvasLogicalSize.y);
            newL.texture->clear(sf::Color::Transparent);
            if (isPixelMode) newL.texture->setSmooth(false);
            else newL.texture->setSmooth(true);
        }
        newFrame.layers.push_back(newL);
    }
    frames.insert(frames.begin() + (index + 1), newFrame);
}

void Canvas::duplicateFrame(int index) {
    saveUndoState();
    if (index >= 0 && index < static_cast<int>(frames.size())) {
        frames.insert(frames.begin() + (index + 1), Frame(frames[index]));
    }
}

void Canvas::deleteFrame(int index) { if (frames.size() > 1 && index >= 0 && index < static_cast<int>(frames.size())) { saveUndoState(); frames.erase(frames.begin() + index); } }
void Canvas::clearAllFrames() {
    saveUndoState(); frames.clear(); frames.emplace_back();
    for (auto& l : frames[0].layers) { l.texture->create(canvasLogicalSize.x, canvasLogicalSize.y); l.texture->clear(sf::Color::Transparent); }
}

void Canvas::addLayer(int frameIndex, const std::string& name) {
    if (frameIndex >= 0 && frameIndex < static_cast<int>(frames.size())) {
        saveUndoState();
        for (size_t i = 0; i < frames.size(); ++i) {
            Layer newL(name);
            newL.texture->create(canvasLogicalSize.x, canvasLogicalSize.y);
            newL.texture->clear(sf::Color::Transparent);
            if (isPixelMode) newL.texture->setSmooth(false);
            else newL.texture->setSmooth(true);
            frames[i].layers.push_back(newL);
        }
        activeLayer = static_cast<int>(frames[0].layers.size()) - 1;
    }
}

void Canvas::deleteLayer(int frameIndex, int layerIndex) {
    if (frames.size() > 0 && frames[0].layers.size() > 1) {
        if (layerIndex >= 0 && layerIndex < static_cast<int>(frames[0].layers.size())) {
            saveUndoState();
            for (size_t i = 0; i < frames.size(); ++i) {
                frames[i].layers.erase(frames[i].layers.begin() + layerIndex);
            }
            if (activeLayer >= static_cast<int>(frames[0].layers.size())) {
                activeLayer = static_cast<int>(frames[0].layers.size()) - 1;
            }
        }
    }
}

void Canvas::duplicateLayer(int frameIndex, int layerIndex) {
    if (frames.size() > 0 && layerIndex >= 0 && layerIndex < static_cast<int>(frames[0].layers.size())) {
        saveUndoState();
        std::string newName = frames[0].layers[layerIndex].name + " Copy";
        bool vis = frames[0].layers[layerIndex].visible;
        bool lck = frames[0].layers[layerIndex].locked;
        float op = frames[0].layers[layerIndex].opacity;
        BlendMode bm = frames[0].layers[layerIndex].blendMode;
        int ct = frames[0].layers[layerIndex].colorTag;

        for (size_t i = 0; i < frames.size(); ++i) {
            Layer copyL(newName);
            copyL.visible = vis; copyL.locked = lck; copyL.opacity = op; copyL.blendMode = bm; copyL.colorTag = ct;
            copyL.persistent = false;

            copyL.texture->create(canvasLogicalSize.x, canvasLogicalSize.y);
            copyL.texture->clear(sf::Color::Transparent);
            if (isPixelMode) copyL.texture->setSmooth(false);
            else copyL.texture->setSmooth(true);
            sf::Sprite spr(frames[i].layers[layerIndex].texture->getTexture());
            copyL.texture->draw(spr, sf::RenderStates(sf::BlendNone));
            copyL.texture->display();

            frames[i].layers.insert(frames[i].layers.begin() + layerIndex + 1, copyL);
        }
        activeLayer = layerIndex + 1;
    }
}

void Canvas::setLayerProperties(int frameIndex, int layerIndex, const std::string& name, bool visible, bool locked, float opacity, BlendMode mode) {
    if (frames.size() > 0 && layerIndex >= 0 && layerIndex < static_cast<int>(frames[0].layers.size())) {
        for (size_t i = 0; i < frames.size(); ++i) {
            auto& l = frames[i].layers[layerIndex];
            l.name = name;
            l.visible = visible;
            l.locked = locked;
            l.opacity = std::max(0.0f, std::min(static_cast<float>(opacity), 1.0f));
            l.blendMode = mode;
        }
    }
}

void Canvas::toggleLayerPersistence(int frameIndex, int layerIndex) {
    if (frameIndex >= 0 && frameIndex < static_cast<int>(frames.size()) && layerIndex >= 0 && layerIndex < static_cast<int>(frames[frameIndex].layers.size())) {
        saveUndoState();
        bool isPersist = !frames[frameIndex].layers[layerIndex].persistent;
        auto targetTex = frames[frameIndex].layers[layerIndex].texture;

        for (size_t i = 0; i < frames.size(); ++i) {
            frames[i].layers[layerIndex].persistent = isPersist;
            if (isPersist && static_cast<int>(i) != frameIndex) {
                frames[i].layers[layerIndex].texture = targetTex;
            }
            else if (!isPersist && static_cast<int>(i) != frameIndex) {
                auto newTex = std::make_shared<sf::RenderTexture>();
                newTex->create(canvasLogicalSize.x, canvasLogicalSize.y);
                newTex->clear(sf::Color::Transparent);
                if (isPixelMode) newTex->setSmooth(false);
                else newTex->setSmooth(true);
                sf::Sprite spr(targetTex->getTexture());
                newTex->draw(spr, sf::RenderStates(sf::BlendNone));
                newTex->display();
                frames[i].layers[layerIndex].texture = newTex;
            }
        }
    }
}

void Canvas::cycleLayerColorTag(int frameIndex, int layerIndex) {
    if (frames.size() > 0 && layerIndex >= 0 && layerIndex < static_cast<int>(frames[0].layers.size())) {
        saveUndoState();
        int nextTag = (frames[frameIndex].layers[layerIndex].colorTag + 1) % 7;
        for (size_t i = 0; i < frames.size(); ++i) {
            frames[i].layers[layerIndex].colorTag = nextTag;
        }
    }
}

void Canvas::pushLayerToNextFrame(int currentFrame, int layerIndex) {
    if (currentFrame >= 0 && currentFrame < static_cast<int>(frames.size()) - 1 && layerIndex >= 0 && layerIndex < static_cast<int>(frames[0].layers.size())) {
        saveUndoState();
        auto srcTex = frames[currentFrame].layers[layerIndex].texture;
        auto dstTex = frames[currentFrame + 1].layers[layerIndex].texture;
        if (!frames[currentFrame].layers[layerIndex].persistent) {
            dstTex->clear(sf::Color::Transparent);
            sf::Sprite spr(srcTex->getTexture());
            dstTex->draw(spr, sf::RenderStates(sf::BlendNone));
            dstTex->display();
        }
    }
}

void Canvas::mergeDown(int frameIndex) {
    if (frames.size() > 0 && activeLayer > 0 && activeLayer < static_cast<int>(frames[0].layers.size())) {
        saveUndoState();
        for (size_t i = 0; i < frames.size(); ++i) {
            auto& topLayer = frames[i].layers[activeLayer];
            auto& bottomLayer = frames[i].layers[activeLayer - 1];

            sf::Sprite spr(topLayer.texture->getTexture());
            sf::RenderStates states;
            states.blendMode = getSFMLBlendMode(topLayer.blendMode).blendMode;
            sf::Color sprCol(255, 255, 255, static_cast<sf::Uint8>(255.0f * topLayer.opacity));
            spr.setColor(sprCol);

            bottomLayer.texture->draw(spr, states);
            bottomLayer.texture->display();

            frames[i].layers.erase(frames[i].layers.begin() + activeLayer);
        }
        activeLayer--;
    }
}

void Canvas::mergeVisible(int frameIndex) {
    if (frames.size() > 0) {
        saveUndoState();
        for (size_t i = 0; i < frames.size(); ++i) {
            Layer mergedLayer("Merged Visible");
            mergedLayer.texture->create(canvasLogicalSize.x, canvasLogicalSize.y);
            mergedLayer.texture->clear(sf::Color::Transparent);
            if (isPixelMode) mergedLayer.texture->setSmooth(false);
            else mergedLayer.texture->setSmooth(true);

            for (const auto& layer : frames[i].layers) {
                if (layer.visible) {
                    sf::Sprite spr(layer.texture->getTexture());
                    sf::RenderStates states;
                    states.blendMode = getSFMLBlendMode(layer.blendMode).blendMode;
                    spr.setColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(255.0f * layer.opacity)));
                    mergedLayer.texture->draw(spr, states);
                }
            }
            mergedLayer.texture->display();

            for (auto it = frames[i].layers.begin(); it != frames[i].layers.end(); ) {
                if (it->visible) it = frames[i].layers.erase(it);
                else ++it;
            }
            frames[i].layers.push_back(mergedLayer);
        }
        activeLayer = static_cast<int>(frames[0].layers.size()) - 1;
    }
}

void Canvas::moveLayer(int frameIndex, int fromIndex, int toIndex) {
    if (frames.size() > 0 && fromIndex >= 0 && fromIndex < static_cast<int>(frames[0].layers.size()) && toIndex >= 0 && toIndex < static_cast<int>(frames[0].layers.size())) {
        saveUndoState();
        for (size_t i = 0; i < frames.size(); ++i) {
            Layer temp = std::move(frames[i].layers[fromIndex]);
            frames[i].layers.erase(frames[i].layers.begin() + fromIndex);
            frames[i].layers.insert(frames[i].layers.begin() + toIndex, std::move(temp));
        }
        if (activeLayer == fromIndex) activeLayer = toIndex;
        else if (activeLayer == toIndex) activeLayer = fromIndex;
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
        selection.commitToLayer(frames[currentFrame].layers[activeLayer].texture.get());
    }
    transformMode = TransformState::None;
    pendingTransform = false;
}

void Canvas::copySelection(int currentFrame) {
    if (!frames.empty() && currentFrame >= 0 && currentFrame < static_cast<int>(frames.size())) {
        selection.copy(frames[currentFrame].layers[activeLayer].texture.get());
    }
}

void Canvas::pasteSelection(int currentFrame) {
    commitSelection(currentFrame);
    saveUndoState();

    addLayer(currentFrame, "Pasted Object");

    selection.paste(canvasLogicalSize);
    setActiveTool(ToolType::Select);
}

void Canvas::deleteSelection(int currentFrame) {
    if (!frames.empty() && currentFrame >= 0 && currentFrame < static_cast<int>(frames.size()) && selection.isActive()) {
        saveUndoState();
        selection.deleteSelection(frames[currentFrame].layers[activeLayer].texture.get());
    }
    transformMode = TransformState::None;
    pendingTransform = false;
}

void Canvas::fillSelection(sf::Color color, int currentFrame) {
    if (!frames.empty() && currentFrame >= 0 && currentFrame < static_cast<int>(frames.size())) {
        saveUndoState();
        sf::Image img = frames[currentFrame].layers[activeLayer].texture->getTexture().copyToImage();
        unsigned int w = std::min(img.getSize().x, canvasLogicalSize.x);
        unsigned int h = std::min(img.getSize().y, canvasLogicalSize.y);
        for (unsigned int y = 0; y < h; ++y) {
            for (unsigned int x = 0; x < w; ++x) {
                if (!selection.isActive() || selection.isPointInsideSelection(sf::Vector2f(static_cast<float>(x), static_cast<float>(y)))) {
                    img.setPixel(x, y, color);
                }
            }
        }
        sf::Texture newTex; newTex.loadFromImage(img);
        frames[currentFrame].layers[activeLayer].texture->clear(sf::Color::Transparent);
        frames[currentFrame].layers[activeLayer].texture->draw(sf::Sprite(newTex), sf::RenderStates(sf::BlendNone));
        frames[currentFrame].layers[activeLayer].texture->display();
    }
}

void Canvas::flipSelectionHorizontal(int currentFrame) {
    if (!frames.empty() && currentFrame >= 0 && currentFrame < static_cast<int>(frames.size()) && selection.isActive()) {
        if (selection.getState() == SelectionState::Selected) {
            saveUndoState();
            selection.extractFromLayer(frames[currentFrame].layers[activeLayer].texture.get(), true);
        }
        selection.flipHorizontal();
    }
}

void Canvas::flipSelectionVertical(int currentFrame) {
    if (!frames.empty() && currentFrame >= 0 && currentFrame < static_cast<int>(frames.size()) && selection.isActive()) {
        if (selection.getState() == SelectionState::Selected) {
            saveUndoState();
            selection.extractFromLayer(frames[currentFrame].layers[activeLayer].texture.get(), true);
        }
        selection.flipVertical();
    }
}

void Canvas::duplicateSelection(int currentFrame) {
    if (!frames.empty() && currentFrame >= 0 && currentFrame < static_cast<int>(frames.size()) && selection.isActive()) {
        saveUndoState();
        selection.copy(frames[currentFrame].layers[activeLayer].texture.get());
        commitSelection(currentFrame);

        addLayer(currentFrame, frames[currentFrame].layers[activeLayer].name + " Duplicate");
        selection.paste(canvasLogicalSize);

        setActiveTool(ToolType::Select);
    }
}

void Canvas::cropSelection(int currentFrame) {
    if (!frames.empty() && currentFrame >= 0 && currentFrame < static_cast<int>(frames.size()) && selection.isActive()) {
        saveUndoState();
        sf::Image layerImg = frames[currentFrame].layers[activeLayer].texture->getTexture().copyToImage();
        sf::Image croppedImg;
        croppedImg.create(layerImg.getSize().x, layerImg.getSize().y, sf::Color::Transparent);
        for (unsigned int x = 0; x < layerImg.getSize().x; ++x) {
            for (unsigned int y = 0; y < layerImg.getSize().y; ++y) {
                if (selection.isPointInsideSelection(sf::Vector2f(x, y))) {
                    croppedImg.setPixel(x, y, layerImg.getPixel(x, y));
                }
            }
        }
        sf::Texture newTex;
        newTex.loadFromImage(croppedImg);
        frames[currentFrame].layers[activeLayer].texture->clear(sf::Color::Transparent);
        frames[currentFrame].layers[activeLayer].texture->draw(sf::Sprite(newTex), sf::RenderStates(sf::BlendNone));
        frames[currentFrame].layers[activeLayer].texture->display();
        commitSelection(currentFrame);
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

void Canvas::setPrimaryColor(sf::Color color) { primaryColor = color; }
void Canvas::setSecondaryColor(sf::Color color) { secondaryColor = color; }
sf::Color Canvas::getPrimaryColor() const { return primaryColor; }
sf::Color Canvas::getSecondaryColor() const { return secondaryColor; }
void Canvas::setFillSettings(float tolerance, bool contiguous) { fillTolerance = tolerance; fillContiguous = contiguous; }

void Canvas::saveUndoState() {
    isDirty = true;
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
        transformMode = TransformState::None;
        pendingTransform = false;
    }
}

void Canvas::redo() {
    if (!redoHistory.empty()) {
        undoHistory.push_back(frames);
        frames = redoHistory.back();
        redoHistory.pop_back();
        selection.clearSelection();
        transformMode = TransformState::None;
        pendingTransform = false;
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

bool Canvas::colorMatches(const sf::Color& a, const sf::Color& b) const {
    if (fillTolerance <= 0.0f) return a == b;
    float diffR = std::abs(static_cast<float>(a.r) - static_cast<float>(b.r));
    float diffG = std::abs(static_cast<float>(a.g) - static_cast<float>(b.g));
    float diffB = std::abs(static_cast<float>(a.b) - static_cast<float>(b.b));
    float diffA = std::abs(static_cast<float>(a.a) - static_cast<float>(b.a));
    float maxDiff = std::max(std::max(diffR, diffG), std::max(diffB, diffA));
    return (maxDiff / 255.0f) <= fillTolerance;
}

void Canvas::executeGlobalFill(sf::Color targetColor, sf::Color replacementColor, sf::Image& image) {
    if (colorMatches(targetColor, replacementColor)) return;

    unsigned int w = std::min(image.getSize().x, canvasLogicalSize.x);
    unsigned int h = std::min(image.getSize().y, canvasLogicalSize.y);

    sf::Uint8* pixels = const_cast<sf::Uint8*>(image.getPixelsPtr());
    unsigned int fullW = image.getSize().x;

    for (unsigned int y = 0; y < h; ++y) {
        size_t rowStart = (static_cast<size_t>(y) * fullW) * 4;
        for (unsigned int x = 0; x < w; ++x) {
            if (selection.isActive() && !selection.isPointInsideSelection(sf::Vector2f(static_cast<float>(x), static_cast<float>(y)))) continue;

            size_t idx = rowStart + static_cast<size_t>(x) * 4;
            sf::Color px(pixels[idx], pixels[idx + 1], pixels[idx + 2], pixels[idx + 3]);
            if (colorMatches(px, targetColor)) {
                pixels[idx] = replacementColor.r;
                pixels[idx + 1] = replacementColor.g;
                pixels[idx + 2] = replacementColor.b;
                pixels[idx + 3] = replacementColor.a;
            }
        }
    }
}

void Canvas::executeQueueFill(sf::Vector2i startPoint, sf::Color targetColor, sf::Color replacementColor, sf::Image& image) {
    if (colorMatches(targetColor, replacementColor)) return;

    if (selection.isActive() && !selection.isPointInsideSelection(sf::Vector2f(static_cast<float>(startPoint.x), static_cast<float>(startPoint.y)))) return;

    int w = static_cast<int>(std::min(image.getSize().x, canvasLogicalSize.x));
    int h = static_cast<int>(std::min(image.getSize().y, canvasLogicalSize.y));

    if (startPoint.x < 0 || startPoint.x >= w || startPoint.y < 0 || startPoint.y >= h) return;

    sf::Uint8* pixels = const_cast<sf::Uint8*>(image.getPixelsPtr());
    int fullW = static_cast<int>(image.getSize().x);

    auto getPx = [&](int x, int y) -> sf::Color {
        size_t idx = (static_cast<size_t>(y) * fullW + static_cast<size_t>(x)) * 4;
        return sf::Color(pixels[idx], pixels[idx + 1], pixels[idx + 2], pixels[idx + 3]);
        };

    if (!colorMatches(getPx(startPoint.x, startPoint.y), targetColor)) return;

    auto canFill = [&](int x, int y) {
        if (selection.isActive() && !selection.isPointInsideSelection(sf::Vector2f(static_cast<float>(x), static_cast<float>(y)))) return false;
        return colorMatches(getPx(x, y), targetColor);
        };

    auto setPx = [&](int x, int y, sf::Color c) {
        size_t idx = (static_cast<size_t>(y) * fullW + static_cast<size_t>(x)) * 4;
        pixels[idx] = c.r; pixels[idx + 1] = c.g; pixels[idx + 2] = c.b; pixels[idx + 3] = c.a;
        };

    std::vector<sf::Vector2i> stack;
    stack.push_back(startPoint);

    while (!stack.empty()) {
        sf::Vector2i p = stack.back();
        stack.pop_back();

        int x = p.x;
        int y = p.y;

        while (x >= 0 && canFill(x, y)) {
            x--;
        }
        x++;

        bool spanAbove = false;
        bool spanBelow = false;

        while (x < w && canFill(x, y)) {
            setPx(x, y, replacementColor);

            if (y > 0) {
                bool match = canFill(x, y - 1);
                if (!spanAbove && match) {
                    stack.push_back(sf::Vector2i(x, y - 1));
                    spanAbove = true;
                }
                else if (spanAbove && !match) {
                    spanAbove = false;
                }
            }

            if (y < h - 1) {
                bool match = canFill(x, y + 1);
                if (!spanBelow && match) {
                    stack.push_back(sf::Vector2i(x, y + 1));
                    spanBelow = true;
                }
                else if (spanBelow && !match) {
                    spanBelow = false;
                }
            }
            x++;
        }
    }
}

void Canvas::drawPixelExact(int x, int y, sf::Color c, int frameIdx) {
    if (useDithering && !ditherManager.shouldDrawPixel(x, y)) return;

    sf::RenderTexture* target = getActiveRenderTexture(frameIdx);
    if (!target) return;

    sf::RenderStates states;
    if (activeTool == ToolType::Eraser || c == sf::Color::Transparent) states.blendMode = sf::BlendNone;

    auto points = symmetryManager.getSymmetricPoints(sf::Vector2f(static_cast<float>(x), static_cast<float>(y)));

    for (auto pt : points) {
        if (selection.isActive() && !selection.isPointInsideSelection(pt)) continue;

        int ix = static_cast<int>(std::round(pt.x));
        int iy = static_cast<int>(std::round(pt.y));

        if (ix < 0 || iy < 0 || ix >= static_cast<int>(canvasLogicalSize.x) || iy >= static_cast<int>(canvasLogicalSize.y)) continue;

        sf::RectangleShape px(sf::Vector2f(static_cast<float>(pixelBrushSize), static_cast<float>(pixelBrushSize)));
        px.setFillColor(c);

        float tx = static_cast<float>(ix) - std::floor(static_cast<float>(pixelBrushSize) / 2.0f);
        float ty = static_cast<float>(iy) - std::floor(static_cast<float>(pixelBrushSize) / 2.0f);
        px.setPosition(tx, ty);

        target->draw(px, states);

        if (tileModeX) {
            px.setPosition(tx - static_cast<float>(canvasLogicalSize.x), ty); target->draw(px, states);
            px.setPosition(tx + static_cast<float>(canvasLogicalSize.x), ty); target->draw(px, states);
        }
        if (tileModeY) {
            px.setPosition(tx, ty - static_cast<float>(canvasLogicalSize.y)); target->draw(px, states);
            px.setPosition(tx, ty + static_cast<float>(canvasLogicalSize.y)); target->draw(px, states);
        }
        if (tileModeX && tileModeY) {
            px.setPosition(tx - static_cast<float>(canvasLogicalSize.x), ty - static_cast<float>(canvasLogicalSize.y)); target->draw(px, states);
            px.setPosition(tx + static_cast<float>(canvasLogicalSize.x), ty - static_cast<float>(canvasLogicalSize.y)); target->draw(px, states);
            px.setPosition(tx - static_cast<float>(canvasLogicalSize.x), ty + static_cast<float>(canvasLogicalSize.y)); target->draw(px, states);
            px.setPosition(tx + static_cast<float>(canvasLogicalSize.x), ty + static_cast<float>(canvasLogicalSize.y)); target->draw(px, states);
        }
    }
}

std::vector<sf::Vector2i> Canvas::getBresenhamPoints(int x0, int y0, int x1, int y1) {
    std::vector<sf::Vector2i> pts;
    int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    while (true) {
        pts.push_back(sf::Vector2i(x0, y0));
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
    return pts;
}

void Canvas::drawBresenhamLine(int x0, int y0, int x1, int y1, sf::Color c, int frameIdx) {
    auto pts = getBresenhamPoints(x0, y0, x1, y1);
    for (auto p : pts) {
        drawPixelExact(p.x, p.y, c, frameIdx);
    }
}

void Canvas::drawContinuousLine(sf::Vector2f from, sf::Vector2f to, sf::Color col, int currentFrame) {
    if (currentFrame < 0 || currentFrame >= static_cast<int>(frames.size())) return;
    if (frames[currentFrame].layers[activeLayer].locked || !frames[currentFrame].layers[activeLayer].visible) return;

    sf::RenderTexture* targetTex = frames[currentFrame].layers[activeLayer].texture.get();
    if (!targetTex) return;

    if (isPixelMode) {
        drawBresenhamLine(static_cast<int>(from.x), static_cast<int>(from.y),
            static_cast<int>(to.x), static_cast<int>(to.y),
            col, currentFrame);
        targetTex->display();
    }
    else {
        if (activeTool == ToolType::Eraser) {
            sf::RenderStates rs(sf::BlendNone);
            float currentEraserSize = brushEngine.getActivePreset().size;
            float length = std::sqrt((to.x - from.x) * (to.x - from.x) + (to.y - from.y) * (to.y - from.y));
            sf::RectangleShape line(sf::Vector2f(length, currentEraserSize));
            line.setOrigin(0.0f, currentEraserSize / 2.f);
            line.setPosition(from);
            line.setRotation(std::atan2(to.y - from.y, to.x - from.x) * 180.f / 3.14159265f);
            line.setFillColor(col);

            sf::CircleShape circle(currentEraserSize / 2.f);
            circle.setOrigin(currentEraserSize / 2.f, currentEraserSize / 2.f);
            circle.setPosition(to);
            circle.setFillColor(col);

            sf::CircleShape startCircle(currentEraserSize / 2.f);
            startCircle.setOrigin(currentEraserSize / 2.f, currentEraserSize / 2.f);
            startCircle.setPosition(from);
            startCircle.setFillColor(col);

            targetTex->draw(startCircle, rs);
            targetTex->draw(line, rs);
            targetTex->draw(circle, rs);
        }
        else {
            float dist = std::hypot(to.x - from.x, to.y - from.y);
            float spacing = std::max(1.0f, brushEngine.getActivePreset().spacing);
            int steps = std::max(1, static_cast<int>(dist / spacing));
            for (int s = 0; s <= steps; ++s) {
                float t = static_cast<float>(s) / static_cast<float>(steps);
                sf::Vector2f interpPos = from + (to - from) * t;
                brushEngine.paintStroke(targetTex, interpPos, col, 1.0f);
            }
        }
        targetTex->display();
    }
    isDirty = true;
}

float Canvas::computeHandleHitRadius() const {
    float worldPerLogicalPixel = drawArea.width / static_cast<float>(canvasLogicalSize.x);
    float denom = std::max(0.0001f, worldPerLogicalPixel * viewScale);
    return 14.0f / denom;
}

bool Canvas::isImageResourceActive(int currentFrame) const {
    if (currentFrame < 0 || currentFrame >= static_cast<int>(frames.size())) return false;
    if (activeLayer < 0 || activeLayer >= static_cast<int>(frames[currentFrame].layers.size())) return false;
    return frames[currentFrame].layers[activeLayer].isImageResource;
}

void Canvas::handleMousePressed(sf::Vector2f logicalPos, bool rightClick, int currentFrame) {
    if (currentFrame < 0 || currentFrame >= static_cast<int>(frames.size())) return;

    if (g_activeWindow) {
        sf::Vector2i pixelPos = sf::Mouse::getPosition(*g_activeWindow);
        sf::Vector2f mappedPos = g_activeWindow->mapPixelToCoords(pixelPos);
        logicalPos = getInverseTransform().transformPoint(mappedPos);
    }

    if (rightClick) {
        return;
    }

    float scaleX = static_cast<float>(canvasLogicalSize.x) / drawArea.width;
    float scaleY = static_cast<float>(canvasLogicalSize.y) / drawArea.height;
    sf::Vector2f localPos((logicalPos.x - drawArea.left) * scaleX, (logicalPos.y - drawArea.top) * scaleY);

    if (isPixelMode && pixelSnapEnabled) {
        localPos.x = std::floor(localPos.x);
        localPos.y = std::floor(localPos.y);
    }

    if (drawArea.contains(logicalPos)) {
        if (!frames[currentFrame].layers[activeLayer].locked && frames[currentFrame].layers[activeLayer].visible) {

            sf::Color drawCol = primaryColor;

            if (activeTool == ToolType::Symmetry) {
                isDrawing = true;
                startPos = localPos;
                lastPos = localPos;
                symmetryManager.enabled = true;
                symmetryManager.setEndpoints(localPos, localPos);
                return;
            }

            if (activeTool == ToolType::Select) {
                if (pendingTransform && selection.getState() == SelectionState::Floating) {
                    if (selection.startResize(localPos, computeHandleHitRadius())) {
                        return;
                    }
                }

                if (selection.isPointInsideSelection(localPos)) {
                    if (selection.getState() == SelectionState::Selected) {
                        saveUndoState();
                        selection.extractFromLayer(frames[currentFrame].layers[activeLayer].texture.get(), true);
                    }
                    selection.startDrag(localPos);
                    return;
                }

                commitSelection(currentFrame);
                selection.startLasso(localPos, canvasLogicalSize);
                return;
            }

            if (activeTool == ToolType::MagicWand) {
                commitSelection(currentFrame);
                autoSelectObject(localPos, currentFrame);
                return;
            }

            if (selection.isActive() && activeTool != ToolType::Fill && activeTool != ToolType::Shapes) {
            }

            if (activeTool == ToolType::Fill) {
                if (isPixelMode) fillTolerance = 0.0f;
                else fillTolerance = 0.08f;

                saveUndoState();
                sf::Image img = frames[currentFrame].layers[activeLayer].texture->getTexture().copyToImage();
                sf::Vector2i pixelPos(static_cast<int>(localPos.x), static_cast<int>(localPos.y));

                if (pixelPos.x >= 0 && pixelPos.y >= 0 && pixelPos.x < static_cast<int>(img.getSize().x) && pixelPos.y < static_cast<int>(img.getSize().y)) {
                    sf::Color targetCol = img.getPixel(pixelPos.x, pixelPos.y);

                    if (fillContiguous) executeQueueFill(pixelPos, targetCol, drawCol, img);
                    else executeGlobalFill(targetCol, drawCol, img);

                    sf::Texture tex;
                    tex.loadFromImage(img);
                    sf::Sprite spr(tex);
                    frames[currentFrame].layers[activeLayer].texture->clear(sf::Color::Transparent);
                    frames[currentFrame].layers[activeLayer].texture->draw(spr, sf::RenderStates(sf::BlendNone));
                    frames[currentFrame].layers[activeLayer].texture->display();
                }
                return;
            }

            bool isShift = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);
            bool canDrawLine = (activeTool == ToolType::Brush || activeTool == ToolType::Pencil || activeTool == ToolType::Eraser);

            if (isShift && hasShiftAnchor && canDrawLine) {
                saveUndoState();
                sf::Color pC = (activeTool == ToolType::Eraser) ? sf::Color::Transparent : drawCol;
                drawContinuousLine(shiftAnchor, localPos, pC, currentFrame);
                shiftAnchor = localPos;
                lastPos = localPos;
                startPos = localPos;
                isDrawing = false;
                return;
            }

            saveUndoState();
            isDrawing = true;
            startPos = localPos;
            lastPos = localPos;
            shiftAnchor = localPos;
            hasShiftAnchor = true;

            if (isPixelMode) {
                sf::Color pC = (activeTool == ToolType::Eraser) ? sf::Color::Transparent : drawCol;

                if (pixelPerfectEnabled && (activeTool == ToolType::Brush || activeTool == ToolType::Pencil || activeTool == ToolType::Eraser) && pixelBrushSize == 1) {
                    sf::RenderTexture* targetTex = frames[currentFrame].layers[activeLayer].texture.get();
                    layerSnapshot = targetTex->getTexture().copyToImage();
                    activeStroke.clear();
                    activeStroke.push_back(sf::Vector2i(static_cast<int>(localPos.x), static_cast<int>(localPos.y)));
                }

                drawPixelExact(static_cast<int>(localPos.x), static_cast<int>(localPos.y), pC, currentFrame);
                frames[currentFrame].layers[activeLayer].texture->display();
            }
            else {
                brushEngine.resetStroke(localPos);
            }
        }
    }
}

void Canvas::handleMouseReleased(sf::Vector2f logicalPos, int currentFrame) {
    if (g_activeWindow) {
        sf::Vector2i pixelPos = sf::Mouse::getPosition(*g_activeWindow);
        sf::Vector2f mappedPos = g_activeWindow->mapPixelToCoords(pixelPos);
        logicalPos = getInverseTransform().transformPoint(mappedPos);
    }

    float scaleX = static_cast<float>(canvasLogicalSize.x) / drawArea.width;
    float scaleY = static_cast<float>(canvasLogicalSize.y) / drawArea.height;
    sf::Vector2f localPos((logicalPos.x - drawArea.left) * scaleX, (logicalPos.y - drawArea.top) * scaleY);

    if (isPixelMode && pixelSnapEnabled) {
        localPos.x = std::floor(localPos.x);
        localPos.y = std::floor(localPos.y);
    }

    if (activeTool == ToolType::Symmetry) {
        isDrawing = false;
        return;
    }

    if (activeTool == ToolType::Select) {
        if (selection.getState() == SelectionState::Drawing) {
            selection.endLasso();

            if (selection.getState() == SelectionState::Inactive) {
                autoSelectObject(localPos, currentFrame);
            }
        }
        else if (selection.getState() == SelectionState::Floating) {
            if (selection.isResizing()) selection.endResize();
            else selection.endDrag();
        }
    }

    if (isDrawing) {
        shiftAnchor = localPos;
        hasShiftAnchor = true;
    }
    isDrawing = false;
}

void Canvas::handleMouseMoved(sf::Vector2f logicalPos, sf::Vector2f rawPos, int currentFrame) {
    if (g_activeWindow) {
        sf::Vector2i pixelPos = sf::Mouse::getPosition(*g_activeWindow);
        sf::Vector2f mappedPos = g_activeWindow->mapPixelToCoords(pixelPos);
        logicalPos = getInverseTransform().transformPoint(mappedPos);
        rawPos = mappedPos;
    }

    isHoveringCanvas = drawArea.contains(logicalPos);
    rawMousePos = rawPos;

    float scaleX = static_cast<float>(canvasLogicalSize.x) / drawArea.width;
    float scaleY = static_cast<float>(canvasLogicalSize.y) / drawArea.height;
    sf::Vector2f localPos((logicalPos.x - drawArea.left) * scaleX, (logicalPos.y - drawArea.top) * scaleY);

    if (isPixelMode && pixelSnapEnabled) {
        localPos.x = std::floor(localPos.x);
        localPos.y = std::floor(localPos.y);
    }

    lastHoverLocalPos = localPos;

    if (activeTool == ToolType::Fill) return;

    if (activeTool == ToolType::Symmetry) {
        if (isDrawing) {
            sf::Vector2f endPos = localPos;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift)) {
                sf::Vector2f diff = endPos - startPos;
                float angle = std::atan2(diff.y, diff.x);
                float snappedAngle = std::round(angle / (3.14159265f / 4.f)) * (3.14159265f / 4.f);
                float length = std::sqrt(diff.x * diff.x + diff.y * diff.y);
                endPos.x = startPos.x + length * std::cos(snappedAngle);
                endPos.y = startPos.y + length * std::sin(snappedAngle);
            }
            symmetryManager.setEndpoints(startPos, endPos);
        }
        return;
    }

    if (activeTool == ToolType::Select) {
        bool allowOutside = isImageResourceActive(currentFrame);

        if (selection.isResizing()) {
            selection.resize(localPos, canvasLogicalSize, allowOutside);
            return;
        }
        if (selection.getState() == SelectionState::Drawing) selection.addLassoPoint(localPos, canvasLogicalSize);
        else if (selection.getState() == SelectionState::Floating) selection.drag(localPos, canvasLogicalSize, allowOutside);
        return;
    }

    if (isDrawing && currentFrame >= 0 && currentFrame < static_cast<int>(frames.size())) {
        if (frames[currentFrame].layers[activeLayer].locked || !frames[currentFrame].layers[activeLayer].visible) {
            isDrawing = false;
            return;
        }

        sf::Color drawCol = (activeTool == ToolType::Eraser) ? sf::Color::Transparent : primaryColor;

        sf::Vector2f targetPos = localPos;
        if (m_perspectiveManager && m_perspectiveManager->getActiveConfig() && m_perspectiveManager->getActiveConfig()->guideSettings.brushSnap) {
            if (activeTool == ToolType::Brush || activeTool == ToolType::Pencil || activeTool == ToolType::Eraser) {
                int activeVPIndex;
                targetPos = PerspectiveSnapper::snapLine(startPos, localPos, *m_perspectiveManager->getActiveConfig(), activeVPIndex);
            }
        }

        if (isPixelMode) {
            sf::RenderTexture* targetTex = frames[currentFrame].layers[activeLayer].texture.get();

            if (pixelPerfectEnabled && (activeTool == ToolType::Brush || activeTool == ToolType::Pencil || activeTool == ToolType::Eraser) && pixelBrushSize == 1) {
                auto pts = getBresenhamPoints(static_cast<int>(lastPos.x), static_cast<int>(lastPos.y), static_cast<int>(targetPos.x), static_cast<int>(targetPos.y));
                for (size_t i = 1; i < pts.size(); ++i) activeStroke.push_back(pts[i]);

                std::vector<sf::Vector2i> filtered;
                for (auto p : activeStroke) {
                    if (filtered.empty()) { filtered.push_back(p); continue; }
                    if (filtered.back() == p) continue;
                    if (filtered.size() >= 2) {
                        sf::Vector2i a = filtered[filtered.size() - 2];
                        sf::Vector2i b = filtered[filtered.size() - 1];
                        sf::Vector2i c = p;
                        if ((a.x == b.x && b.y == c.y && a.x != c.x && a.y != c.y) ||
                            (a.y == b.y && b.x == c.x && a.x != c.x && a.y != c.y)) {
                            filtered.pop_back();
                        }
                    }
                    filtered.push_back(p);
                }
                activeStroke = filtered;

                targetTex->clear(sf::Color::Transparent);
                sf::Texture temp; temp.loadFromImage(layerSnapshot);
                targetTex->draw(sf::Sprite(temp), sf::RenderStates(sf::BlendNone));

                for (auto p : activeStroke) {
                    drawPixelExact(p.x, p.y, drawCol, currentFrame);
                }
                targetTex->display();
            }
            else {
                drawBresenhamLine(static_cast<int>(lastPos.x), static_cast<int>(lastPos.y), static_cast<int>(targetPos.x), static_cast<int>(targetPos.y), drawCol, currentFrame);
                targetTex->display();
            }
        }
        else {
            sf::RenderTexture* targetTex = frames[currentFrame].layers[activeLayer].texture.get();
            if (activeTool == ToolType::Eraser) {
                sf::RenderStates rs(sf::BlendNone);
                float currentEraserSize = brushEngine.getActivePreset().size;

                float length = std::sqrt((targetPos.x - lastPos.x) * (targetPos.x - lastPos.x) + (targetPos.y - lastPos.y) * (targetPos.y - lastPos.y));
                sf::RectangleShape line(sf::Vector2f(length, currentEraserSize));
                line.setOrigin(0.0f, currentEraserSize / 2.f);
                line.setPosition(lastPos);
                line.setRotation(std::atan2(targetPos.y - lastPos.y, targetPos.x - lastPos.x) * 180.f / 3.14159265f);
                line.setFillColor(drawCol);

                sf::CircleShape circle(currentEraserSize / 2.f);
                circle.setOrigin(currentEraserSize / 2.f, currentEraserSize / 2.f);
                circle.setPosition(targetPos);
                circle.setFillColor(drawCol);

                targetTex->draw(line, rs);
                targetTex->draw(circle, rs);
            }
            else {
                brushEngine.paintStroke(targetTex, targetPos, drawCol, 1.0f);
            }
            targetTex->display();
        }

        lastPos = targetPos;
        shiftAnchor = targetPos;
        hasShiftAnchor = true;

        if (!isHoveringCanvas) {
            isDrawing = false;
        }
    }
}

void Canvas::makeOutline(int currentFrame, sf::Color outlineColor) {
    if (frames.empty() || currentFrame < 0 || currentFrame >= static_cast<int>(frames.size())) return;
    if (activeLayer < 0 || activeLayer >= static_cast<int>(frames[currentFrame].layers.size())) return;
    if (frames[currentFrame].layers[activeLayer].locked || !frames[currentFrame].layers[activeLayer].visible) return;

    sf::RenderTexture* targetTex = frames[currentFrame].layers[activeLayer].texture.get();
    if (!targetTex) return;

    saveUndoState();

    sf::Image img = targetTex->getTexture().copyToImage();
    int w = static_cast<int>(img.getSize().x);
    int h = static_cast<int>(img.getSize().y);

    std::vector<sf::Vector2i> outlinePts;

    const int dx[8] = { -1, 0, 1, -1, 1, -1, 0, 1 };
    const int dy[8] = { -1, -1, -1, 0, 0, 1, 1, 1 };

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (img.getPixel(x, y).a <= 30) {
                if (selection.isActive() && !selection.isPointInsideSelection(sf::Vector2f(static_cast<float>(x), static_cast<float>(y)))) {
                    continue;
                }
                bool neighborSolid = false;
                for (int d = 0; d < 8; ++d) {
                    int nx = x + dx[d];
                    int ny = y + dy[d];
                    if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                        if (img.getPixel(nx, ny).a > 30) {
                            if (!selection.isActive() || selection.isPointInsideSelection(sf::Vector2f(static_cast<float>(nx), static_cast<float>(ny)))) {
                                neighborSolid = true;
                                break;
                            }
                        }
                    }
                }
                if (neighborSolid) {
                    outlinePts.push_back(sf::Vector2i(x, y));
                }
            }
        }
    }

    for (const auto& pt : outlinePts) {
        img.setPixel(pt.x, pt.y, outlineColor);
    }

    sf::Texture newTex;
    newTex.loadFromImage(img);
    targetTex->clear(sf::Color::Transparent);
    targetTex->draw(sf::Sprite(newTex), sf::RenderStates(sf::BlendNone));
    targetTex->display();
    isDirty = true;
}

void Canvas::draw(sf::RenderWindow& window, int currentFrame, bool isPlaying, const sf::RenderStates& states) {
    g_activeWindow = &window;

    window.draw(deskSprite, states);

    sf::Vector2f texScale(drawArea.width / static_cast<float>(canvasLogicalSize.x), drawArea.height / static_cast<float>(canvasLogicalSize.y));
    sf::Transform innerTransform = states.transform;
    innerTransform.translate(std::round(drawArea.left), std::round(drawArea.top));
    innerTransform.scale(texScale);
    sf::RenderStates innerStates = states;
    innerStates.transform = innerTransform;

    sf::Transform frameTransform = states.transform;
    frameTransform.translate(std::round(drawArea.left), std::round(drawArea.top));
    sf::RenderStates frameStates = states;
    frameStates.transform = frameTransform;

    const float frameThickness = 16.f;
    sf::Color frameColor(45, 35, 25);
    sf::Color shadowColor(20, 15, 10);

    if (hasFrameAssets) {
        float cx = 0.f;
        float cy = 0.f;
        float cw = std::round(drawArea.width);
        float ch = std::round(drawArea.height);

        float tlW = static_cast<float>(frameTex[0].getSize().x);
        float tlH = static_cast<float>(frameTex[0].getSize().y);
        float tH = static_cast<float>(frameTex[1].getSize().y);
        float trW = static_cast<float>(frameTex[2].getSize().x);
        float trH = static_cast<float>(frameTex[2].getSize().y);
        float lW = static_cast<float>(frameTex[3].getSize().x);
        float rW = static_cast<float>(frameTex[4].getSize().x);
        float blW = static_cast<float>(frameTex[5].getSize().x);
        float bH = static_cast<float>(frameTex[6].getSize().y);
        float brW = static_cast<float>(frameTex[7].getSize().x);

        sf::Sprite sTopLeft(frameTex[0]);
        sf::Sprite sTop(frameTex[1]);
        sf::Sprite sTopRight(frameTex[2]);
        sf::Sprite sLeft(frameTex[3]);
        sf::Sprite sRight(frameTex[4]);
        sf::Sprite sBotLeft(frameTex[5]);
        sf::Sprite sBottom(frameTex[6]);
        sf::Sprite sBotRight(frameTex[7]);

        sTop.setPosition(cx, cy - tH);
        sTop.setTextureRect(sf::IntRect(0, 0, static_cast<int>(cw), static_cast<int>(tH)));

        sBottom.setPosition(cx, cy + ch);
        sBottom.setTextureRect(sf::IntRect(0, 0, static_cast<int>(cw), static_cast<int>(bH)));

        sLeft.setPosition(cx - lW, cy);
        sLeft.setTextureRect(sf::IntRect(0, 0, static_cast<int>(lW), static_cast<int>(ch)));

        sRight.setPosition(cx + cw, cy);
        sRight.setTextureRect(sf::IntRect(0, 0, static_cast<int>(rW), static_cast<int>(ch)));

        sTopLeft.setPosition(cx - lW, cy - tH);
        sTopRight.setPosition(cx + cw - (trW - rW), cy - tH);
        sBotLeft.setPosition(cx - lW, cy + ch);
        sBotRight.setPosition(cx + cw - (brW - rW), cy + ch);

        window.draw(sTop, frameStates);
        window.draw(sBottom, frameStates);
        window.draw(sLeft, frameStates);
        window.draw(sRight, frameStates);
        window.draw(sTopLeft, frameStates);
        window.draw(sTopRight, frameStates);
        window.draw(sBotLeft, frameStates);
        window.draw(sBotRight, frameStates);
    }
    else {
        float dw = std::round(drawArea.width);
        float dh = std::round(drawArea.height);
        float cx = 0.f;
        float cy = 0.f;

        sf::RectangleShape topEdge({ dw + 2 * frameThickness, frameThickness });
        topEdge.setPosition(cx - frameThickness, cy - frameThickness);
        topEdge.setFillColor(frameColor);

        sf::RectangleShape bottomEdge({ dw + 2 * frameThickness, frameThickness });
        bottomEdge.setPosition(cx - frameThickness, cy + dh);
        bottomEdge.setFillColor(frameColor);

        sf::RectangleShape leftEdge({ frameThickness, dh });
        leftEdge.setPosition(cx - frameThickness, cy);
        leftEdge.setFillColor(frameColor);

        sf::RectangleShape rightEdge({ frameThickness, dh });
        rightEdge.setPosition(cx + dw, cy);
        rightEdge.setFillColor(frameColor);

        sf::RectangleShape innerShadow({ dw, dh });
        innerShadow.setPosition(cx, cy);
        innerShadow.setFillColor(sf::Color::Transparent);
        innerShadow.setOutlineThickness(1.5f);
        innerShadow.setOutlineColor(shadowColor);

        window.draw(topEdge, frameStates);
        window.draw(bottomEdge, frameStates);
        window.draw(leftEdge, frameStates);
        window.draw(rightEdge, frameStates);
        window.draw(innerShadow, frameStates);
    }

    sf::RectangleShape bg(sf::Vector2f(canvasLogicalSize.x, canvasLogicalSize.y));
    bg.setFillColor(sf::Color::White);
    window.draw(bg, innerStates);

    if (isPixelMode) {
        float tileSize = 4.0f;
        int cols = static_cast<int>(std::ceil(canvasLogicalSize.x / tileSize));
        int rows = static_cast<int>(std::ceil(canvasLogicalSize.y / tileSize));
        sf::VertexArray checkerboard(sf::Quads);
        sf::Color c1(190, 190, 190);
        sf::Color c2(240, 240, 240);

        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                sf::Color color = ((r + c) % 2 == 0) ? c1 : c2;
                float x = c * tileSize;
                float y = r * tileSize;
                float w = std::min(tileSize, static_cast<float>(canvasLogicalSize.x) - x);
                float h = std::min(tileSize, static_cast<float>(canvasLogicalSize.y) - y);

                checkerboard.append(sf::Vertex(sf::Vector2f(x, y), color));
                checkerboard.append(sf::Vertex(sf::Vector2f(x + w, y), color));
                checkerboard.append(sf::Vertex(sf::Vector2f(x + w, y + h), color));
                checkerboard.append(sf::Vertex(sf::Vector2f(x, y + h), color));
            }
        }
        window.draw(checkerboard, innerStates);

        if (pixelGridEnabled && viewScale > 0.1f) {
            sf::VertexArray lines(sf::Lines);
            sf::Color gridLine(180, 180, 180, 210);
            unsigned int step = 1;
            for (unsigned int x = 0; x <= canvasLogicalSize.x; x += step) {
                lines.append(sf::Vertex(sf::Vector2f(static_cast<float>(x), 0.f), gridLine));
                lines.append(sf::Vertex(sf::Vector2f(static_cast<float>(x), static_cast<float>(canvasLogicalSize.y)), gridLine));
            }
            for (unsigned int y = 0; y <= canvasLogicalSize.y; y += step) {
                lines.append(sf::Vertex(sf::Vector2f(0.f, static_cast<float>(y)), gridLine));
                lines.append(sf::Vertex(sf::Vector2f(static_cast<float>(canvasLogicalSize.x), static_cast<float>(y)), gridLine));
            }
            window.draw(lines, innerStates);
        }

        symmetryManager.drawGuides(window, innerStates, sf::FloatRect(0, 0, static_cast<float>(canvasLogicalSize.x), static_cast<float>(canvasLogicalSize.y)), viewScale);
    }

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

                if (static_cast<int>(i) == activeLayer) {
                    selection.drawPixels(window, layerStates);
                }

                if (m_textManager) {
                    m_textManager->render(window, currentFrame, static_cast<int>(i), isPixelMode, layerStates, canvasLogicalSize);
                }
            }
        }
    }

    float worldPerLogicalPixel = drawArea.width / static_cast<float>(canvasLogicalSize.x);
    float handleDenom = std::max(0.0001f, worldPerLogicalPixel * viewScale);
    selection.setHandleVisualSize(10.0f / handleDenom);
    selection.setShowHandles(pendingTransform);

    selection.draw(window, innerStates);

    sf::Vector2i mousePosI = sf::Mouse::getPosition(window);
    sf::Vector2f currentRawMousePos = window.mapPixelToCoords(mousePosI);

    sf::Vector2f logicalPos = getInverseTransform().transformPoint(currentRawMousePos);
    bool currentlyHovering = drawArea.contains(logicalPos);

    if (!isPlaying && currentlyHovering && (activeTool == ToolType::Brush || activeTool == ToolType::Pencil || activeTool == ToolType::Eraser)) {
        if (isPixelMode) {
            float scaleX = static_cast<float>(canvasLogicalSize.x) / drawArea.width;
            float scaleY = static_cast<float>(canvasLogicalSize.y) / drawArea.height;

            sf::Vector2f localPos = getInverseTransform().transformPoint(currentRawMousePos);
            localPos.x = (localPos.x - drawArea.left) * scaleX;
            localPos.y = (localPos.y - drawArea.top) * scaleY;

            float tx = std::floor(localPos.x) - std::floor(static_cast<float>(pixelBrushSize) / 2.0f);
            float ty = std::floor(localPos.y) - std::floor(static_cast<float>(pixelBrushSize) / 2.0f);

            sf::RectangleShape pxHover(sf::Vector2f(static_cast<float>(pixelBrushSize), static_cast<float>(pixelBrushSize)));
            pxHover.setFillColor(sf::Color(255, 255, 255, 90));
            pxHover.setOutlineThickness(0.f);
            pxHover.setPosition(tx, ty);

            sf::RenderStates hoverStates = innerStates;
            hoverStates.blendMode = sf::BlendAdd;

            window.draw(pxHover, hoverStates);
        }
        else {
            brushEngine.drawPreviewCursor(window, currentRawMousePos, sf::Color::White, viewScale);
        }
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
        return frames[currentFrame].layers[activeLayer].texture.get();
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

void Canvas::setPixelMode(bool enabled) { isPixelMode = enabled; }
bool Canvas::getPixelMode() const { return isPixelMode; }
void Canvas::setPixelBrushSize(int size) { pixelBrushSize = size; }
int Canvas::getPixelBrushSize() const { return pixelBrushSize; }
bool Canvas::getIsDirty() const { return isDirty; }
void Canvas::clearIsDirty() { isDirty = false; }

void Canvas::cyclePixelBrushSize() {
    if (pixelBrushSize == 1) pixelBrushSize = 2;
    else if (pixelBrushSize == 2) pixelBrushSize = 4;
    else if (pixelBrushSize == 4) pixelBrushSize = 8;
    else if (pixelBrushSize == 8) pixelBrushSize = 16;
    else pixelBrushSize = 1;
}
void Canvas::togglePixelGrid() { pixelGridEnabled = !pixelGridEnabled; }
bool Canvas::isPixelGridEnabled() const { return pixelGridEnabled; }
void Canvas::togglePixelSnap() { pixelSnapEnabled = !pixelSnapEnabled; }
bool Canvas::isPixelSnapEnabled() const { return pixelSnapEnabled; }
void Canvas::toggleTileMode() {
    if (!tileModeX && !tileModeY) { tileModeX = true; tileModeY = false; }
    else if (tileModeX && !tileModeY) { tileModeX = false; tileModeY = true; }
    else if (!tileModeX && tileModeY) { tileModeX = true; tileModeY = true; }
    else { tileModeX = false; tileModeY = false; }
}
void Canvas::togglePixelPerfect() { pixelPerfectEnabled = !pixelPerfectEnabled; }
bool Canvas::isPixelPerfectEnabled() const { return pixelPerfectEnabled; }

void Canvas::importImageToActiveLayer(const std::string& filepath, int currentFrame) {
    if (currentFrame < 0 || currentFrame >= static_cast<int>(frames.size())) return;

    auto tex = std::make_shared<sf::Texture>();
    if (tex->loadFromFile(filepath)) {
        saveUndoState();

        addLayer(currentFrame, "Imported Image");

        auto& targetLayer = frames[currentFrame].layers[activeLayer];
        targetLayer.isImageResource = true;
        targetLayer.staticTexture = tex;

        sf::Vector2u texSize = tex->getSize();

        float maxW = static_cast<float>(canvasLogicalSize.x) * 0.9f;
        float maxH = static_cast<float>(canvasLogicalSize.y) * 0.9f;
        float scale = std::min(maxW / static_cast<float>(texSize.x), maxH / static_cast<float>(texSize.y));
        scale = std::min(scale, 1.0f);

        sf::Sprite importSprite(*tex);
        importSprite.setScale(scale, scale);

        float scaledW = static_cast<float>(texSize.x) * scale;
        float scaledH = static_cast<float>(texSize.y) * scale;
        float centerX = (canvasLogicalSize.x / 2.0f) - (scaledW / 2.0f);
        float centerY = (canvasLogicalSize.y / 2.0f) - (scaledH / 2.0f);
        importSprite.setPosition(centerX, centerY);

        targetLayer.texture->clear(sf::Color::Transparent);
        targetLayer.texture->draw(importSprite, sf::RenderStates(sf::BlendAlpha));
        targetLayer.texture->display();

        isDirty = true;

        commitSelection(currentFrame);
        selection.startLasso(sf::Vector2f(centerX, centerY), canvasLogicalSize);
        selection.addLassoPoint(sf::Vector2f(centerX + scaledW, centerY), canvasLogicalSize);
        selection.addLassoPoint(sf::Vector2f(centerX + scaledW, centerY + scaledH), canvasLogicalSize);
        selection.addLassoPoint(sf::Vector2f(centerX, centerY + scaledH), canvasLogicalSize);
        selection.endLasso();
        selection.extractFromLayer(targetLayer.texture.get(), true);
        setActiveTool(ToolType::Select);
    }
}

void Canvas::enterTransformMode(int currentFrame) {
    if (!selection.isActive() || frames.empty() || currentFrame < 0 || currentFrame >= static_cast<int>(frames.size())) return;

    if (selection.getState() == SelectionState::Selected) {
        saveUndoState();
        selection.extractFromLayer(frames[currentFrame].layers[activeLayer].texture.get(), true);
    }

    if (selection.getState() == SelectionState::Floating) {
        transformMode = TransformState::Scaling;
        pendingTransform = true;
    }
}

void Canvas::applyTransform(int currentFrame) {
    if (pendingTransform) {
        commitSelection(currentFrame);
        transformMode = TransformState::None;
        pendingTransform = false;
    }
}

void Canvas::cancelTransform() {
    if (pendingTransform) {
        undo();
        transformMode = TransformState::None;
        pendingTransform = false;
    }
}

bool Canvas::isTransforming() const {
    return pendingTransform;
}

void Canvas::autoSelectObject(sf::Vector2f pos, int currentFrame) {
    if (frames.empty() || currentFrame < 0 || currentFrame >= static_cast<int>(frames.size())) return;

    int sx = static_cast<int>(pos.x);
    int sy = static_cast<int>(pos.y);

    int targetLayerIndex = -1;
    sf::Image targetImg;

    for (int i = static_cast<int>(frames[currentFrame].layers.size()) - 1; i >= 0; --i) {
        if (!frames[currentFrame].layers[i].visible || frames[currentFrame].layers[i].locked) continue;

        sf::Image tempImg = frames[currentFrame].layers[i].texture->getTexture().copyToImage();
        if (sx >= 0 && sy >= 0 && sx < static_cast<int>(tempImg.getSize().x) && sy < static_cast<int>(tempImg.getSize().y)) {
            if (tempImg.getPixel(sx, sy).a > 0) {
                targetLayerIndex = i;
                targetImg = tempImg;
                break;
            }
        }
    }

    if (targetLayerIndex == -1) {
        selection.clearSelection();
        return;
    }

    activeLayer = targetLayerIndex;

    int minX = sx, maxX = sx, minY = sy, maxY = sy;
    std::vector<sf::Vector2i> stack;
    stack.push_back(sf::Vector2i(sx, sy));

    int w = targetImg.getSize().x;
    int h = targetImg.getSize().y;

    std::vector<bool> visited(w * h, false);
    visited[sy * w + sx] = true;

    while (!stack.empty()) {
        sf::Vector2i p = stack.back();
        stack.pop_back();

        if (p.x < minX) minX = p.x;
        if (p.x > maxX) maxX = p.x;
        if (p.y < minY) minY = p.y;
        if (p.y > maxY) maxY = p.y;

        sf::Vector2i neighbors[4] = {
            {p.x - 1, p.y}, {p.x + 1, p.y},
            {p.x, p.y - 1}, {p.x, p.y + 1}
        };

        for (int i = 0; i < 4; ++i) {
            int nx = neighbors[i].x;
            int ny = neighbors[i].y;
            if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                int idx = ny * w + nx;
                if (!visited[idx]) {
                    visited[idx] = true;
                    if (targetImg.getPixel(nx, ny).a > 0) {
                        stack.push_back(sf::Vector2i(nx, ny));
                    }
                }
            }
        }
    }

    selection.startLasso(sf::Vector2f(static_cast<float>(minX), static_cast<float>(minY)), canvasLogicalSize);
    selection.addLassoPoint(sf::Vector2f(static_cast<float>(maxX + 1), static_cast<float>(minY)), canvasLogicalSize);
    selection.addLassoPoint(sf::Vector2f(static_cast<float>(maxX + 1), static_cast<float>(maxY + 1)), canvasLogicalSize);
    selection.addLassoPoint(sf::Vector2f(static_cast<float>(minX), static_cast<float>(maxY + 1)), canvasLogicalSize);
    selection.endLasso();
}