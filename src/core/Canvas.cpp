#include "Canvas.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <stack>
#include <queue>
#include "TextSystem.h"

static const sf::RenderWindow* g_activeWindow = nullptr;

static sf::BlendMode eraseBlendMode() {
    return sf::BlendMode(
        sf::BlendMode::Zero, sf::BlendMode::OneMinusSrcAlpha, sf::BlendMode::Add,
        sf::BlendMode::Zero, sf::BlendMode::OneMinusSrcAlpha, sf::BlendMode::Add);
}

static void appendVectorSegment(sf::VertexArray& va, sf::Vector2f p1, sf::Vector2f p2, float radius, sf::Color color) {
    sf::Vector2f dir = p2 - p1;
    float len = std::hypot(dir.x, dir.y);
    if (len < 0.001f) return;

    sf::Vector2f normal(-dir.y / len * radius, dir.x / len * radius);

    sf::Vector2f a = p1 + normal;
    sf::Vector2f b = p1 - normal;
    sf::Vector2f c = p2 + normal;
    sf::Vector2f d = p2 - normal;

    va.append(sf::Vertex(a, color));
    va.append(sf::Vertex(b, color));
    va.append(sf::Vertex(c, color));

    va.append(sf::Vertex(b, color));
    va.append(sf::Vertex(d, color));
    va.append(sf::Vertex(c, color));

    const int steps = 12;
    for (int i = 0; i < steps; ++i) {
        float a1 = static_cast<float>(i) / static_cast<float>(steps) * 6.2831853f;
        float a2 = static_cast<float>(i + 1) / static_cast<float>(steps) * 6.2831853f;
        sf::Vector2f j1 = p2 + sf::Vector2f(std::cos(a1) * radius, std::sin(a1) * radius);
        sf::Vector2f j2 = p2 + sf::Vector2f(std::cos(a2) * radius, std::sin(a2) * radius);
        va.append(sf::Vertex(p2, color));
        va.append(sf::Vertex(j1, color));
        va.append(sf::Vertex(j2, color));
    }
}

static void appendVectorCap(sf::VertexArray& va, sf::Vector2f center, float radius, sf::Color color) {
    const int steps = 16;
    for (int i = 0; i < steps; ++i) {
        float a1 = static_cast<float>(i) / static_cast<float>(steps) * 6.2831853f;
        float a2 = static_cast<float>(i + 1) / static_cast<float>(steps) * 6.2831853f;
        sf::Vector2f p1 = center + sf::Vector2f(std::cos(a1) * radius, std::sin(a1) * radius);
        sf::Vector2f p2 = center + sf::Vector2f(std::cos(a2) * radius, std::sin(a2) * radius);
        va.append(sf::Vertex(center, color));
        va.append(sf::Vertex(p1, color));
        va.append(sf::Vertex(p2, color));
    }
}

static sf::VertexArray meshWithOpacity(const sf::VertexArray& src, float opacity) {
    sf::VertexArray out(src.getPrimitiveType(), src.getVertexCount());
    for (std::size_t i = 0; i < src.getVertexCount(); ++i) {
        out[i] = src[i];
        out[i].color.a = static_cast<sf::Uint8>(
            std::clamp(static_cast<float>(src[i].color.a) * opacity, 0.f, 255.f));
    }
    return out;
}

void Canvas::eraseVectorStrokesAt(sf::Vector2f p1, sf::Vector2f p2, float radius, int currentFrame) {}

const int DEFAULT_NORMAL_W = 1280;
const int DEFAULT_NORMAL_H = 720;
const int DEFAULT_PIXEL_W = 64;
const int DEFAULT_PIXEL_H = 64;

Layer::Layer(std::string n) : name(n), visible(true), locked(false), opacity(1.0f), blendMode(BlendMode::Normal), persistent(false), colorTag(0), isImageResource(false) {
    texture = std::make_shared<sf::RenderTexture>();
    texture->create(1, 1);
    texture->clear(sf::Color::Transparent);
    texture->setSmooth(true);
}

Layer::Layer(const Layer& other) : name(other.name), visible(other.visible), locked(other.locked), opacity(other.opacity), blendMode(other.blendMode), persistent(other.persistent), colorTag(other.colorTag), isImageResource(other.isImageResource) {
    if (persistent) {
        texture = other.texture;
    }
    else {
        texture = std::make_shared<sf::RenderTexture>();
        if (other.texture) {
            sf::ContextSettings ctx;
            ctx.antialiasingLevel = other.texture->isSmooth() ? 8 : 0;
            if (!texture->create(other.texture->getSize().x, other.texture->getSize().y, ctx)) {
                texture->create(other.texture->getSize().x, other.texture->getSize().y);
            }
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
                sf::ContextSettings ctx;
                ctx.antialiasingLevel = other.texture->isSmooth() ? 8 : 0;
                if (!texture->create(other.texture->getSize().x, other.texture->getSize().y, ctx)) {
                    texture->create(other.texture->getSize().x, other.texture->getSize().y);
                }
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
transformMode(TransformState::None), pendingTransform(false), currentRotation(0.0f), currentScale(1.0f, 1.0f), hasFrameAssets(false) {
    brushEngine.initDefaultPresets();
}

bool Canvas::layerHasErase(int frameIndex, int layerIndex) const {
    for (const auto& vs : m_vectorStrokes) {
        if (vs.frame == frameIndex && vs.layer == layerIndex && vs.isErase) return true;
    }
    return false;
}

void Canvas::drawLayerContent(sf::RenderTarget& target, int frameIndex, int layerIndex,
    const sf::RenderStates& layerStates, bool isActiveLayerForPreview) {
    if (frameIndex < 0 || frameIndex >= static_cast<int>(frames.size())) return;
    if (layerIndex < 0 || layerIndex >= static_cast<int>(frames[frameIndex].layers.size())) return;

    const Layer& layer = frames[frameIndex].layers[layerIndex];
    float op = layer.opacity;

    bool activeErase = isActiveLayerForPreview && m_isVectorStrokeActive &&
        m_activeStrokeIsErase && m_activeVectorMesh.getVertexCount() > 0;
    bool floatingHasErase = false;
    if (isActiveLayerForPreview && selection.getState() == SelectionState::Floating) {
        for (const auto& fvs : m_floatingVectorStrokes) {
            if (fvs.isErase) { floatingHasErase = true; break; }
        }
    }

    bool needsComposite = !isPixelMode && (layerHasErase(frameIndex, layerIndex) || activeErase || floatingHasErase);

    if (!needsComposite) {
        if (layer.texture) {
            sf::Sprite spr(layer.texture->getTexture());
            spr.setColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(255.0f * op)));
            target.draw(spr, layerStates);
        }

        if (!isPixelMode) {
            for (const auto& vs : m_vectorStrokes) {
                if (vs.frame != frameIndex || vs.layer != layerIndex) continue;
                if (op < 0.999f) target.draw(meshWithOpacity(vs.mesh, op), layerStates);
                else target.draw(vs.mesh, layerStates);
            }
            if (isActiveLayerForPreview && m_isVectorStrokeActive && m_activeVectorMesh.getVertexCount() > 0) {
                target.draw(m_activeVectorMesh, layerStates);
            }
        }
        return;
    }

    sf::Vector2u targetSize = target.getSize();
    if (m_layerCache.getSize() != targetSize) {
        sf::ContextSettings ctx;
        ctx.antialiasingLevel = 8;
        if (!m_layerCache.create(targetSize.x, targetSize.y, ctx)) {
            m_layerCache.create(targetSize.x, targetSize.y);
        }
        m_layerCache.setSmooth(false);
    }

    m_layerCache.setView(target.getView());
    m_layerCache.clear(sf::Color::Transparent);

    if (layer.texture) {
        sf::Sprite base(layer.texture->getTexture());
        m_layerCache.draw(base, layerStates);
    }

    for (const auto& vs : m_vectorStrokes) {
        if (vs.frame != frameIndex || vs.layer != layerIndex) continue;
        sf::RenderStates st = layerStates;
        st.blendMode = vs.isErase ? eraseBlendMode() : sf::BlendAlpha;
        m_layerCache.draw(vs.mesh, st);
    }

    if (isActiveLayerForPreview && !m_floatingVectorStrokes.empty() && selection.getState() == SelectionState::Floating) {
        sf::RenderStates st = layerStates;
        st.transform *= selection.getFloatingTransform();
        for (const auto& vs : m_floatingVectorStrokes) {
            st.blendMode = vs.isErase ? eraseBlendMode() : sf::BlendAlpha;
            m_layerCache.draw(vs.mesh, st);
        }
    }

    if (isActiveLayerForPreview && m_isVectorStrokeActive && m_activeVectorMesh.getVertexCount() > 0) {
        sf::RenderStates st = layerStates;
        st.blendMode = m_activeStrokeIsErase ? eraseBlendMode() : sf::BlendAlpha;
        m_layerCache.draw(m_activeVectorMesh, st);
    }

    m_layerCache.display();

    sf::Sprite out(m_layerCache.getTexture());
    out.setColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(255.0f * op)));

    sf::View savedView = target.getView();
    target.setView(target.getDefaultView());
    sf::RenderStates finalStates;
    finalStates.blendMode = getSFMLBlendMode(layer.blendMode).blendMode;
    target.draw(out, finalStates);
    target.setView(savedView);
}

bool Canvas::renderLayerToTexture(int frameIndex, int layerIndex, sf::RenderTexture& out) {
    if (frameIndex < 0 || frameIndex >= static_cast<int>(frames.size())) return false;
    if (layerIndex < 0 || layerIndex >= static_cast<int>(frames[frameIndex].layers.size())) return false;

    if (out.getSize() != canvasLogicalSize) {
        sf::ContextSettings ctx;
        ctx.antialiasingLevel = isPixelMode ? 0 : 8;
        if (!out.create(canvasLogicalSize.x, canvasLogicalSize.y, ctx)) {
            if (!out.create(canvasLogicalSize.x, canvasLogicalSize.y)) return false;
        }
        out.setSmooth(!isPixelMode);
    }

    out.clear(sf::Color::Transparent);
    drawLayerContent(out, frameIndex, layerIndex, sf::RenderStates::Default, false);

    if (m_textManager) {
        m_textManager->render(out, frameIndex, layerIndex, isPixelMode, sf::RenderStates::Default, canvasLogicalSize);
    }

    out.display();
    return true;
}

void Canvas::bakeLayerStrokes(int frameIndex, int layerIndex) {
    if (isPixelMode) return;
    if (frameIndex < 0 || frameIndex >= static_cast<int>(frames.size())) return;
    if (layerIndex < 0 || layerIndex >= static_cast<int>(frames[frameIndex].layers.size())) return;

    bool any = false;
    for (const auto& vs : m_vectorStrokes) {
        if (vs.frame == frameIndex && vs.layer == layerIndex) { any = true; break; }
    }
    if (!any) return;

    sf::RenderTexture* targetTex = frames[frameIndex].layers[layerIndex].texture.get();
    if (!targetTex) return;

    renderLayerToTexture(frameIndex, layerIndex, *targetTex);

    for (auto it = m_vectorStrokes.begin(); it != m_vectorStrokes.end(); ) {
        if (it->frame == frameIndex && it->layer == layerIndex) it = m_vectorStrokes.erase(it);
        else ++it;
    }

    isDirty = true;
}

void Canvas::extractFloatingStrokes(int currentFrame) {
    m_floatingVectorStrokes.clear();
    if (isPixelMode) return;
    if (selection.getState() != SelectionState::Selected) return;

    sf::FloatRect bb = selection.getBoundingBox();
    m_floatingLocalSize = sf::Vector2f(bb.width, bb.height);
    sf::Vector2f origin(bb.left, bb.top);

    for (auto it = m_vectorStrokes.begin(); it != m_vectorStrokes.end(); ) {
        if (it->frame != currentFrame || it->layer != activeLayer) {
            ++it;
            continue;
        }

        sf::VertexArray kept(sf::Triangles);
        sf::VertexArray moved(sf::Triangles);

        for (size_t v = 0; v + 2 < it->mesh.getVertexCount(); v += 3) {
            sf::Vector2f centroid =
                (it->mesh[v].position + it->mesh[v + 1].position + it->mesh[v + 2].position) / 3.0f;

            if (selection.isPointInsideSelection(centroid)) {
                for (int k = 0; k < 3; ++k) {
                    sf::Vertex vert = it->mesh[v + k];
                    vert.position -= origin;
                    moved.append(vert);
                }
            }
            else {
                kept.append(it->mesh[v]);
                kept.append(it->mesh[v + 1]);
                kept.append(it->mesh[v + 2]);
            }
        }

        if (moved.getVertexCount() > 0) {
            VectorStroke fs;
            fs.mesh = moved;
            fs.layer = activeLayer;
            fs.frame = currentFrame;
            fs.isErase = it->isErase;
            m_floatingVectorStrokes.push_back(std::move(fs));
        }

        if (kept.getVertexCount() == 0) {
            it = m_vectorStrokes.erase(it);
        }
        else {
            it->mesh = kept;
            ++it;
        }
    }
}

void Canvas::flipFloatingStrokes(bool horizontal) {
    for (auto& vs : m_floatingVectorStrokes) {
        for (size_t v = 0; v < vs.mesh.getVertexCount(); ++v) {
            if (horizontal) vs.mesh[v].position.x = m_floatingLocalSize.x - vs.mesh[v].position.x;
            else vs.mesh[v].position.y = m_floatingLocalSize.y - vs.mesh[v].position.y;
        }
    }
}

void Canvas::init() {
    if (isPixelMode) initCustom(DEFAULT_PIXEL_W, DEFAULT_PIXEL_H);
    else initCustom(DEFAULT_NORMAL_W, DEFAULT_NORMAL_H);
}

void Canvas::initCustom(int width, int height) {
    canvasLogicalSize = sf::Vector2u(static_cast<unsigned int>(width), static_cast<unsigned int>(height));

    if (!deskTexture.loadFromFile("assets/workbench.png", sf::IntRect(114, 702, 1669, 379))) {}
    deskSprite.setTexture(deskTexture);
    deskSprite.setOrigin(1669.f / 2.f, 379.f / 2.f);
    deskSprite.setPosition(1920.f / 2.f, 850.f);

    if (!canvasTexture.loadFromFile("assets/canvas.png")) {}
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
    sf::ContextSettings ctx;
    ctx.antialiasingLevel = isPixelMode ? 0 : 8;
    for (auto& l : frames[0].layers) {
        if (!l.texture->create(canvasLogicalSize.x, canvasLogicalSize.y, ctx)) {
            l.texture->create(canvasLogicalSize.x, canvasLogicalSize.y);
        }
        l.texture->clear(sf::Color::Transparent);
        l.texture->setSmooth(!isPixelMode);
    }

    m_vectorStrokes.clear();
    m_floatingVectorStrokes.clear();
    m_activeVectorMesh.clear();
    m_isVectorStrokeActive = false;
    m_activeStrokeIsErase = false;

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
    sf::Vector2f mousePos = g_activeWindow->mapPixelToCoords(mousePosI);

    sf::Vector2f screenCenter(1920.0f / 2.0f, 1080.0f / 2.0f);

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
            sf::ContextSettings ctx;
            ctx.antialiasingLevel = isPixelMode ? 0 : 8;
            if (!newL.texture->create(canvasLogicalSize.x, canvasLogicalSize.y, ctx)) {
                newL.texture->create(canvasLogicalSize.x, canvasLogicalSize.y);
            }
            newL.texture->clear(sf::Color::Transparent);
            newL.texture->setSmooth(!isPixelMode);
        }
        newFrame.layers.push_back(newL);
    }

    if (!isPixelMode) {
        for (auto& vs : m_vectorStrokes) {
            if (vs.frame > index) vs.frame++;
        }
    }

    frames.insert(frames.begin() + (index + 1), newFrame);
}

void Canvas::duplicateFrame(int index) {
    saveUndoState();
    if (index >= 0 && index < static_cast<int>(frames.size())) {
        if (!isPixelMode) {
            std::vector<VectorStroke> copies;
            for (auto& vs : m_vectorStrokes) {
                if (vs.frame == index) {
                    VectorStroke c;
                    c.mesh = vs.mesh; c.layer = vs.layer; c.frame = index + 1; c.isErase = vs.isErase;
                    copies.push_back(std::move(c));
                }
            }
            for (auto& vs : m_vectorStrokes) {
                if (vs.frame > index) vs.frame++;
            }
            for (auto& c : copies) m_vectorStrokes.push_back(std::move(c));
        }
        frames.insert(frames.begin() + (index + 1), Frame(frames[index]));
    }
}

void Canvas::deleteFrame(int index) {
    if (frames.size() > 1 && index >= 0 && index < static_cast<int>(frames.size())) {
        saveUndoState();

        if (!isPixelMode) {
            for (auto it = m_vectorStrokes.begin(); it != m_vectorStrokes.end(); ) {
                if (it->frame == index) {
                    it = m_vectorStrokes.erase(it);
                }
                else {
                    if (it->frame > index) it->frame--;
                    ++it;
                }
            }
        }

        frames.erase(frames.begin() + index);
    }
}

void Canvas::clearAllFrames() {
    saveUndoState();
    frames.clear();
    frames.emplace_back();
    for (auto& l : frames[0].layers) {
        l.texture->create(canvasLogicalSize.x, canvasLogicalSize.y);
        l.texture->clear(sf::Color::Transparent);
        l.texture->setSmooth(!isPixelMode);
    }
    m_vectorStrokes.clear();
    m_floatingVectorStrokes.clear();
    m_activeVectorMesh.clear();
    m_isVectorStrokeActive = false;
    m_activeStrokeIsErase = false;
}

void Canvas::addLayer(int frameIndex, const std::string& name) {
    if (frameIndex >= 0 && frameIndex < static_cast<int>(frames.size())) {
        saveUndoState();
        sf::ContextSettings ctx;
        ctx.antialiasingLevel = isPixelMode ? 0 : 8;
        for (size_t i = 0; i < frames.size(); ++i) {
            Layer newL(name);
            if (!newL.texture->create(canvasLogicalSize.x, canvasLogicalSize.y, ctx)) {
                newL.texture->create(canvasLogicalSize.x, canvasLogicalSize.y);
            }
            newL.texture->clear(sf::Color::Transparent);
            newL.texture->setSmooth(!isPixelMode);
            frames[i].layers.push_back(newL);
        }
        activeLayer = static_cast<int>(frames[0].layers.size()) - 1;
    }
}

void Canvas::deleteLayer(int frameIndex, int layerIndex) {
    if (frames.empty() || frames[0].layers.size() <= 1) return;
    if (layerIndex < 0 || layerIndex >= static_cast<int>(frames[0].layers.size())) return;

    saveUndoState();

    for (size_t i = 0; i < frames.size(); ++i) {
        frames[i].layers.erase(frames[i].layers.begin() + layerIndex);
    }

    if (!isPixelMode) {
        for (auto it = m_vectorStrokes.begin(); it != m_vectorStrokes.end(); ) {
            if (it->layer == layerIndex) {
                it = m_vectorStrokes.erase(it);
            }
            else {
                if (it->layer > layerIndex) it->layer--;
                ++it;
            }
        }
    }

    if (activeLayer >= static_cast<int>(frames[0].layers.size())) {
        activeLayer = static_cast<int>(frames[0].layers.size()) - 1;
    }

    isDirty = true;
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

            sf::ContextSettings ctx;
            ctx.antialiasingLevel = isPixelMode ? 0 : 8;
            if (!copyL.texture->create(canvasLogicalSize.x, canvasLogicalSize.y, ctx)) {
                copyL.texture->create(canvasLogicalSize.x, canvasLogicalSize.y);
            }
            copyL.texture->clear(sf::Color::Transparent);
            copyL.texture->setSmooth(!isPixelMode);
            sf::Sprite spr(frames[i].layers[layerIndex].texture->getTexture());
            copyL.texture->draw(spr, sf::RenderStates(sf::BlendNone));
            copyL.texture->display();

            frames[i].layers.insert(frames[i].layers.begin() + layerIndex + 1, copyL);
        }
        if (!isPixelMode) {
            std::vector<VectorStroke> duplicates;
            for (auto& vs : m_vectorStrokes) {
                if (vs.layer > layerIndex) {
                    vs.layer++;
                }
                else if (vs.layer == layerIndex) {
                    VectorStroke d;
                    d.mesh = vs.mesh; d.layer = layerIndex + 1; d.frame = vs.frame; d.isErase = vs.isErase;
                    duplicates.push_back(std::move(d));
                }
            }
            for (auto& d : duplicates) m_vectorStrokes.push_back(std::move(d));
        }
        activeLayer = layerIndex + 1;
    }
}

void Canvas::setLayerProperties(int frameIndex, int layerIndex, const std::string& name, bool visible, bool locked, float opacity, BlendMode mode, bool recordUndo) {
    if (frames.empty() || layerIndex < 0 || layerIndex >= static_cast<int>(frames[0].layers.size())) return;
    if (recordUndo) {
        saveUndoState();
    }
    for (size_t i = 0; i < frames.size(); ++i) {
        auto& l = frames[i].layers[layerIndex];
        l.name = name;
        l.visible = visible;
        l.locked = locked;
        l.opacity = std::clamp(opacity, 0.0f, 1.0f);
        l.blendMode = mode;
    }
    isDirty = true;
}

void Canvas::toggleLayerPersistence(int frameIndex, int layerIndex) {
    if (frameIndex >= 0 && frameIndex < static_cast<int>(frames.size())) {
        if (layerIndex >= 0 && layerIndex < static_cast<int>(frames[frameIndex].layers.size())) {
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
                    newTex->setSmooth(!isPixelMode);
                    sf::Sprite spr(targetTex->getTexture());
                    newTex->draw(spr, sf::RenderStates(sf::BlendNone));
                    newTex->display();
                    frames[i].layers[layerIndex].texture = newTex;
                }
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
    if (currentFrame >= 0 && currentFrame < static_cast<int>(frames.size())) {
        if (currentFrame < static_cast<int>(frames.size()) - 1 && layerIndex >= 0 && layerIndex < static_cast<int>(frames[0].layers.size())) {
            saveUndoState();

            auto srcTex = frames[currentFrame].layers[layerIndex].texture;
            auto dstTex = frames[currentFrame + 1].layers[layerIndex].texture;
            if (!frames[currentFrame].layers[layerIndex].persistent) {
                dstTex->clear(sf::Color::Transparent);
                sf::Sprite spr(srcTex->getTexture());
                dstTex->draw(spr, sf::RenderStates(sf::BlendNone));
                dstTex->display();
            }

            if (!isPixelMode) {
                std::vector<VectorStroke> copies;
                for (const auto& vs : m_vectorStrokes) {
                    if (vs.frame == currentFrame && vs.layer == layerIndex) {
                        VectorStroke c;
                        c.mesh = vs.mesh;
                        c.layer = layerIndex;
                        c.frame = currentFrame + 1;
                        c.isErase = vs.isErase;
                        copies.push_back(std::move(c));
                    }
                }
                for (auto it = m_vectorStrokes.begin(); it != m_vectorStrokes.end(); ) {
                    if (it->frame == currentFrame + 1 && it->layer == layerIndex) it = m_vectorStrokes.erase(it);
                    else ++it;
                }
                for (auto& c : copies) m_vectorStrokes.push_back(std::move(c));
            }
        }
    }
}

void Canvas::extendLayerToNextFrame(int currentFrame, int layerIndex) {
    if (currentFrame < 0 || currentFrame >= static_cast<int>(frames.size())) return;
    if (layerIndex < 0 || layerIndex >= static_cast<int>(frames[currentFrame].layers.size())) return;

    saveUndoState();
    if (currentFrame == static_cast<int>(frames.size()) - 1) {
        addFrame(currentFrame);
    }

    auto srcTex = frames[currentFrame].layers[layerIndex].texture;
    auto dstTex = frames[currentFrame + 1].layers[layerIndex].texture;

    if (!frames[currentFrame].layers[layerIndex].persistent && srcTex && dstTex) {
        dstTex->clear(sf::Color::Transparent);
        sf::Sprite spr(srcTex->getTexture());
        dstTex->draw(spr, sf::RenderStates(sf::BlendNone));
        dstTex->display();
    }

    if (!isPixelMode) {
        std::vector<VectorStroke> copies;
        for (const auto& vs : m_vectorStrokes) {
            if (vs.frame == currentFrame && vs.layer == layerIndex) {
                VectorStroke c;
                c.mesh = vs.mesh;
                c.layer = layerIndex;
                c.frame = currentFrame + 1;
                c.isErase = vs.isErase;
                copies.push_back(std::move(c));
            }
        }
        for (auto it = m_vectorStrokes.begin(); it != m_vectorStrokes.end(); ) {
            if (it->frame == currentFrame + 1 && it->layer == layerIndex) it = m_vectorStrokes.erase(it);
            else ++it;
        }
        for (auto& c : copies) m_vectorStrokes.push_back(std::move(c));
    }
    isDirty = true;
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

        if (!isPixelMode) {
            for (auto& vs : m_vectorStrokes) {
                if (vs.layer == activeLayer) {
                    vs.layer = activeLayer - 1;
                }
                else if (vs.layer > activeLayer) {
                    vs.layer--;
                }
            }
        }

        activeLayer--;
    }
}

void Canvas::mergeVisible(int frameIndex) {
    if (frames.size() > 0) {
        saveUndoState();
        for (size_t i = 0; i < frames.size(); ++i) {
            for (size_t l = 0; l < frames[i].layers.size(); ++l) {
                if (frames[i].layers[l].visible) {
                    bakeLayerStrokes(static_cast<int>(i), static_cast<int>(l));
                }
            }

            Layer mergedLayer("Merged Visible");
            mergedLayer.texture->create(canvasLogicalSize.x, canvasLogicalSize.y);
            mergedLayer.texture->clear(sf::Color::Transparent);
            mergedLayer.texture->setSmooth(!isPixelMode);

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

        m_vectorStrokes.clear();
        activeLayer = static_cast<int>(frames[0].layers.size()) - 1;
    }
}

void Canvas::moveLayer(int frameIndex, int fromIndex, int toIndex) {
    if (frames.empty() || fromIndex < 0 || fromIndex >= static_cast<int>(frames[0].layers.size()) ||
        toIndex < 0 || toIndex >= static_cast<int>(frames[0].layers.size()) || fromIndex == toIndex) return;

    saveUndoState();
    for (size_t i = 0; i < frames.size(); ++i) {
        Layer temp = std::move(frames[i].layers[fromIndex]);
        frames[i].layers.erase(frames[i].layers.begin() + fromIndex);
        frames[i].layers.insert(frames[i].layers.begin() + toIndex, std::move(temp));
    }

    if (!isPixelMode) {
        for (auto& vs : m_vectorStrokes) {
            if (vs.layer == fromIndex) {
                vs.layer = toIndex;
            }
            else if (fromIndex < toIndex && vs.layer > fromIndex && vs.layer <= toIndex) {
                vs.layer--;
            }
            else if (fromIndex > toIndex && vs.layer >= toIndex && vs.layer < fromIndex) {
                vs.layer++;
            }
        }
    }

    if (activeLayer == fromIndex) {
        activeLayer = toIndex;
    }
    else if (fromIndex < toIndex && activeLayer > fromIndex && activeLayer <= toIndex) {
        activeLayer--;
    }
    else if (fromIndex > toIndex && activeLayer >= toIndex && activeLayer < fromIndex) {
        activeLayer++;
    }
    isDirty = true;
}

void Canvas::setActiveLayer(int index, int currentFrame) {
    int target = std::max(0, index);
    if (target == activeLayer) return;

    if (currentFrame >= 0 && currentFrame < static_cast<int>(frames.size())) {
        if (selection.isActive() && selection.getState() == SelectionState::Floating) {
            commitSelection(currentFrame);
        }
    }
    activeLayer = target;
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

        if (!isPixelMode) {
            if (!m_floatingVectorStrokes.empty()) {
                sf::Transform t = selection.getFloatingTransform();
                for (auto& vs : m_floatingVectorStrokes) {
                    for (size_t v = 0; v < vs.mesh.getVertexCount(); ++v) {
                        vs.mesh[v].position = t.transformPoint(vs.mesh[v].position);
                    }
                    vs.layer = activeLayer;
                    vs.frame = currentFrame;
                    m_vectorStrokes.push_back(std::move(vs));
                }
                m_floatingVectorStrokes.clear();
            }

            selection.commitToLayer(frames[currentFrame].layers[activeLayer].texture.get());
        }
        else {
            selection.commitToLayer(frames[currentFrame].layers[activeLayer].texture.get());
        }
    }
    transformMode = TransformState::None;
    pendingTransform = false;
}

void Canvas::copySelection(int currentFrame) {
    if (!frames.empty() && currentFrame >= 0 && currentFrame < static_cast<int>(frames.size())) {
        if (!isPixelMode) {
            sf::RenderTexture tmp;
            if (renderLayerToTexture(currentFrame, activeLayer, tmp)) {
                selection.copy(&tmp);
                return;
            }
        }
        selection.copy(frames[currentFrame].layers[activeLayer].texture.get());
    }
}

void Canvas::pasteImage(const sf::Image& img, int currentFrame) {
    if (currentFrame < 0 || currentFrame >= static_cast<int>(frames.size())) return;

    saveUndoState();

    addLayer(currentFrame, "Pasted Clipboard");

    auto& targetLayer = frames[currentFrame].layers[activeLayer];
    targetLayer.isImageResource = true;
    auto tex = std::make_shared<sf::Texture>();
    tex->loadFromImage(img);
    targetLayer.staticTexture = tex;

    sf::Vector2u texSize = img.getSize();

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

void Canvas::pasteSelection(int currentFrame) {
    commitSelection(currentFrame);
    saveUndoState();

    addLayer(currentFrame, "Pasted Object");

    selection.paste(canvasLogicalSize);
    setActiveTool(ToolType::Select);
}

void Canvas::deleteSelection(int currentFrame) {
    if (!frames.empty() && currentFrame >= 0 && currentFrame < static_cast<int>(frames.size())) {
        saveUndoState();
        m_floatingVectorStrokes.clear();
        selection.deleteSelection(frames[currentFrame].layers[activeLayer].texture.get());
    }
    transformMode = TransformState::None;
    pendingTransform = false;
}

void Canvas::fillSelection(sf::Color color, int currentFrame) {
    if (frames.empty() || currentFrame < 0 || currentFrame >= static_cast<int>(frames.size())) return;
    saveUndoState();

    if (!isPixelMode) {
        bool isErase = (color == sf::Color::Transparent || color.a == 0);
        bool recoloredAny = false;

        if (!m_floatingVectorStrokes.empty()) {
            for (auto& vs : m_floatingVectorStrokes) {
                for (size_t v = 0; v < vs.mesh.getVertexCount(); ++v) {
                    vs.mesh[v].color = isErase ? sf::Color::White : color;
                }
                if (isErase) vs.isErase = true;
            }
            recoloredAny = true;
        }

        for (auto& vs : m_vectorStrokes) {
            if (vs.frame == currentFrame && vs.layer == activeLayer && !vs.isErase) {
                for (size_t v = 0; v + 2 < vs.mesh.getVertexCount(); v += 3) {
                    sf::Vector2f centroid = (vs.mesh[v].position + vs.mesh[v + 1].position + vs.mesh[v + 2].position) / 3.0f;
                    if (selection.isPointInsideSelection(centroid) ||
                        selection.isPointInsideSelection(vs.mesh[v].position) ||
                        selection.isPointInsideSelection(vs.mesh[v + 1].position) ||
                        selection.isPointInsideSelection(vs.mesh[v + 2].position)) {

                        vs.mesh[v].color = isErase ? sf::Color::White : color;
                        vs.mesh[v + 1].color = isErase ? sf::Color::White : color;
                        vs.mesh[v + 2].color = isErase ? sf::Color::White : color;
                        recoloredAny = true;
                    }
                }
            }
        }

        if (!recoloredAny && !isErase && selection.isActive()) {
            sf::FloatRect bb = selection.getBoundingBox();
            int x0 = std::max(0, static_cast<int>(std::floor(bb.left)));
            int y0 = std::max(0, static_cast<int>(std::floor(bb.top)));
            int x1 = std::min(static_cast<int>(canvasLogicalSize.x), static_cast<int>(std::ceil(bb.left + bb.width)));
            int y1 = std::min(static_cast<int>(canvasLogicalSize.y), static_cast<int>(std::ceil(bb.top + bb.height)));

            VectorStroke vs;
            vs.mesh.setPrimitiveType(sf::Triangles);
            vs.layer = activeLayer;
            vs.frame = currentFrame;
            vs.isErase = false;

            for (int y = y0; y < y1; ++y) {
                int x = x0;
                while (x < x1) {
                    if (selection.isPointInsideSelection(sf::Vector2f(static_cast<float>(x), static_cast<float>(y)))) {
                        int xStart = x;
                        while (x < x1 && selection.isPointInsideSelection(sf::Vector2f(static_cast<float>(x), static_cast<float>(y)))) {
                            x++;
                        }
                        int xEnd = x;

                        float fx0 = static_cast<float>(xStart);
                        float fx1 = static_cast<float>(xEnd);
                        float fy0 = static_cast<float>(y);
                        float fy1 = static_cast<float>(y + 1);

                        vs.mesh.append(sf::Vertex(sf::Vector2f(fx0, fy0), color));
                        vs.mesh.append(sf::Vertex(sf::Vector2f(fx1, fy0), color));
                        vs.mesh.append(sf::Vertex(sf::Vector2f(fx1, fy1), color));

                        vs.mesh.append(sf::Vertex(sf::Vector2f(fx0, fy0), color));
                        vs.mesh.append(sf::Vertex(sf::Vector2f(fx1, fy1), color));
                        vs.mesh.append(sf::Vertex(sf::Vector2f(fx0, fy1), color));
                    }
                    else {
                        x++;
                    }
                }
            }

            if (vs.mesh.getVertexCount() > 0) {
                auto insertPos = m_vectorStrokes.end();
                for (auto it = m_vectorStrokes.begin(); it != m_vectorStrokes.end(); ++it) {
                    if (it->frame == currentFrame && it->layer == activeLayer) {
                        insertPos = it;
                        break;
                    }
                }
                m_vectorStrokes.insert(insertPos, std::move(vs));
            }
        }

        isDirty = true;
        return;
    }

    // --- PIXEL ART MODE (Retained) ---
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
    isDirty = true;
}

void Canvas::flipSelectionHorizontal(int currentFrame) {
    if (!frames.empty() && currentFrame >= 0 && currentFrame < static_cast<int>(frames.size())) {
        if (selection.getState() == SelectionState::Selected) {
            saveUndoState();
            extractFloatingStrokes(currentFrame);
            selection.extractFromLayer(frames[currentFrame].layers[activeLayer].texture.get(), true);
        }
        selection.flipHorizontal();
        flipFloatingStrokes(true);
    }
}

void Canvas::flipSelectionVertical(int currentFrame) {
    if (!frames.empty() && currentFrame >= 0 && currentFrame < static_cast<int>(frames.size())) {
        if (selection.getState() == SelectionState::Selected) {
            saveUndoState();
            extractFloatingStrokes(currentFrame);
            selection.extractFromLayer(frames[currentFrame].layers[activeLayer].texture.get(), true);
        }
        selection.flipVertical();
        flipFloatingStrokes(false);
    }
}

void Canvas::duplicateSelection(int currentFrame) {
    if (!frames.empty() && currentFrame >= 0 && currentFrame < static_cast<int>(frames.size())) {
        saveUndoState();
        copySelection(currentFrame);
        commitSelection(currentFrame);

        addLayer(currentFrame, frames[currentFrame].layers[activeLayer].name + " Duplicate");
        selection.paste(canvasLogicalSize);

        setActiveTool(ToolType::Select);
    }
}

void Canvas::cropSelection(int currentFrame) {
    if (!frames.empty() && currentFrame >= 0 && currentFrame < static_cast<int>(frames.size())) {
        saveUndoState();
        bakeLayerStrokes(currentFrame, activeLayer);

        sf::Image layerImg = frames[currentFrame].layers[activeLayer].texture->getTexture().copyToImage();
        sf::Image croppedImg;
        croppedImg.create(layerImg.getSize().x, layerImg.getSize().y, sf::Color::Transparent);
        for (unsigned int x = 0; x < layerImg.getSize().x; ++x) {
            for (unsigned int y = 0; y < layerImg.getSize().y; ++y) {
                if (selection.isPointInsideSelection(sf::Vector2f(static_cast<float>(x), static_cast<float>(y)))) {
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

void Canvas::setActiveTool(ToolType tool, int currentFrame) {
    if (tool != ToolType::Text && m_textManager) {
        TextObject* t = m_textManager->getEditingText();
        if (t) {
            if (!t->text.isEmpty()) {
                m_textManager->rasterizeText(currentFrame, activeLayer, t->id, *this);
            }
            else {
                m_textManager->deleteText(currentFrame, t->id);
            }
        }
    }

    if (selection.isActive() && tool != ToolType::Select) {
        commitSelection(currentFrame);
        selection.clearSelection();
    }

    activeTool = tool;
    isDrawing = false;
    isDeforming = false;

    if (tool == ToolType::Symmetry) {
        m_symmetryDragMode = SymmetryDragMode::None;
        if (std::hypot(symmetryManager.direction.x, symmetryManager.direction.y) > 0.001f) {
            symmetryManager.visible = true;
        }
    }
    deformPixels.clear();
    currentDeformedPixels.clear();
    m_contourPoints.clear();
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
    UndoState state;
    state.frames = frames;
    state.vectorStrokes = m_vectorStrokes;
    undoHistory.push_back(state);
    if (undoHistory.size() > maxUndoHistory) {
        undoHistory.erase(undoHistory.begin());
    }
    redoHistory.clear();
}

void Canvas::undo() {
    if (!undoHistory.empty()) {
        UndoState currentState;
        currentState.frames = frames;
        currentState.vectorStrokes = m_vectorStrokes;
        redoHistory.push_back(currentState);

        UndoState prevState = undoHistory.back();
        undoHistory.pop_back();

        frames = prevState.frames;
        m_vectorStrokes = prevState.vectorStrokes;
        m_floatingVectorStrokes.clear();

        selection.clearSelection();
        transformMode = TransformState::None;
        pendingTransform = false;
        isDeforming = false;
        deformPixels.clear();
        currentDeformedPixels.clear();
        m_deformStrokeIndex = -1;
        m_originalDeformMesh.clear();
        m_contourPoints.clear();
        m_isVectorStrokeActive = false;
        m_activeStrokeIsErase = false;
        m_activeVectorMesh.clear();
        isDrawing = false;
    }
}

void Canvas::redo() {
    if (!redoHistory.empty()) {
        UndoState currentState;
        currentState.frames = frames;
        currentState.vectorStrokes = m_vectorStrokes;
        undoHistory.push_back(currentState);

        UndoState nextState = redoHistory.back();
        redoHistory.pop_back();

        frames = nextState.frames;
        m_vectorStrokes = nextState.vectorStrokes;
        m_floatingVectorStrokes.clear();

        selection.clearSelection();
        transformMode = TransformState::None;
        pendingTransform = false;
        isDeforming = false;
        deformPixels.clear();
        currentDeformedPixels.clear();
        m_deformStrokeIndex = -1;
        m_originalDeformMesh.clear();
        m_contourPoints.clear();
        m_isVectorStrokeActive = false;
        m_activeStrokeIsErase = false;
        m_activeVectorMesh.clear();
        isDrawing = false;
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

sf::Image Canvas::flattenFrameToImage(int frameIndex, unsigned int scaleFactor) {
    if (scaleFactor < 1) scaleFactor = 1;

    unsigned int w = canvasLogicalSize.x * scaleFactor;
    unsigned int h = canvasLogicalSize.y * scaleFactor;

    sf::RenderTexture out;
    sf::ContextSettings ctx;
    ctx.antialiasingLevel = isPixelMode ? 0 : 8;
    if (!out.create(w, h, ctx)) {
        if (!out.create(w, h)) return sf::Image();
    }
    out.setSmooth(!isPixelMode);
    out.clear(sf::Color::Transparent);

    if (frameIndex < 0 || frameIndex >= static_cast<int>(frames.size())) {
        out.display();
        return out.getTexture().copyToImage();
    }

    float s = static_cast<float>(scaleFactor);

    for (size_t i = 0; i < frames[frameIndex].layers.size(); ++i) {
        const auto& layer = frames[frameIndex].layers[i];
        if (!layer.visible) continue;

        sf::RenderStates states;
        states.blendMode = getSFMLBlendMode(layer.blendMode).blendMode;
        states.transform.scale(s, s);

        drawLayerContent(out, frameIndex, static_cast<int>(i), states, false);

        if (m_textManager) {
            m_textManager->render(out, frameIndex, static_cast<int>(i), isPixelMode, states, canvasLogicalSize);
        }
    }

    out.display();
    return out.getTexture().copyToImage();
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

    if (isPixelMode) {
        sf::RenderTexture* targetTex = frames[currentFrame].layers[activeLayer].texture.get();
        if (!targetTex) return;
        drawBresenhamLine(static_cast<int>(from.x), static_cast<int>(from.y),
            static_cast<int>(to.x), static_cast<int>(to.y),
            col, currentFrame);
        targetTex->display();
        isDirty = true;
        return;
    }

    bool erasing = (activeTool == ToolType::Eraser);
    float radius = brushEngine.getActivePreset().size * 0.5f;
    sf::Color meshCol = erasing ? sf::Color::White : col;

    VectorStroke vs;
    vs.mesh.setPrimitiveType(sf::Triangles);
    vs.layer = activeLayer;
    vs.frame = currentFrame;
    vs.isErase = erasing;

    appendVectorCap(vs.mesh, from, radius, meshCol);
    appendVectorSegment(vs.mesh, from, to, radius, meshCol);
    appendVectorCap(vs.mesh, to, radius, meshCol);

    m_vectorStrokes.push_back(std::move(vs));
    isDirty = true;
}

float Canvas::computeDeformWeight(float t) const {
    if (deformMode == 1) {
        float u = std::clamp(t, 0.0f, 1.0f);
        return u * u;
    }
    if (deformMode == 2) {
        float u = std::clamp(1.0f - t, 0.0f, 1.0f);
        return u * u;
    }
    if (t <= deformT0) {
        if (deformT0 <= 0.0001f) return 1.0f;
        float u = std::clamp(t / deformT0, 0.0f, 1.0f);
        return u * u * (3.0f - 2.0f * u);
    }
    else {
        if (deformT0 >= 0.9999f) return 1.0f;
        float u = std::clamp((1.0f - t) / (1.0f - deformT0), 0.0f, 1.0f);
        return u * u * (3.0f - 2.0f * u);
    }
}

void Canvas::updateDeformPixels(sf::Vector2f delta) {
    currentDeformedPixels.clear();
    if (deformPixels.empty()) return;

    int boxW = deformMaxX - deformMinX + 1;
    int boxH = deformMaxY - deformMinY + 1;
    if (boxW <= 0 || boxH <= 0) return;

    float L = deformIsHorizontal ? static_cast<float>(std::max(1, boxW - 1)) : static_cast<float>(std::max(1, boxH - 1));

    std::vector<int> pixelGrid(boxW * boxH, -1);
    std::vector<sf::Vector2i> mapped(deformPixels.size());

    for (size_t i = 0; i < deformPixels.size(); ++i) {
        int gx = deformPixels[i].x - deformMinX;
        int gy = deformPixels[i].y - deformMinY;
        if (gx >= 0 && gx < boxW && gy >= 0 && gy < boxH) {
            pixelGrid[gy * boxW + gx] = static_cast<int>(i);
        }

        float coord = deformIsHorizontal ? static_cast<float>(deformPixels[i].x - deformMinX) : static_cast<float>(deformPixels[i].y - deformMinY);
        float t = std::clamp(coord / L, 0.0f, 1.0f);
        float w = computeDeformWeight(t);

        int dx = static_cast<int>(std::round(delta.x * w));
        int dy = static_cast<int>(std::round(delta.y * w));
        mapped[i] = sf::Vector2i(deformPixels[i].x + dx, deformPixels[i].y + dy);
    }

    std::vector<bool> placedMask(canvasLogicalSize.x * canvasLogicalSize.y, false);

    auto putPixel = [&](int px, int py, sf::Color col) {
        if (px >= 0 && py >= 0 && px < static_cast<int>(canvasLogicalSize.x) && py < static_cast<int>(canvasLogicalSize.y)) {
            size_t pidx = static_cast<size_t>(py) * canvasLogicalSize.x + px;
            if (!placedMask[pidx]) {
                placedMask[pidx] = true;
                currentDeformedPixels.push_back({ px, py, col });
            }
        }
        };

    for (size_t i = 0; i < deformPixels.size(); ++i) {
        putPixel(mapped[i].x, mapped[i].y, deformPixels[i].color);
    }

    for (int gy = 0; gy < boxH; ++gy) {
        for (int gx = 0; gx < boxW; ++gx) {
            int i = pixelGrid[gy * boxW + gx];
            if (i == -1) continue;

            if (gx + 1 < boxW) {
                int j = pixelGrid[gy * boxW + (gx + 1)];
                if (j != -1) {
                    sf::Vector2i p1 = mapped[i];
                    sf::Vector2i p2 = mapped[j];
                    if (std::abs(p1.x - p2.x) > 1 || std::abs(p1.y - p2.y) > 1) {
                        auto pts = getBresenhamPoints(p1.x, p1.y, p2.x, p2.y);
                        for (const auto& pt : pts) {
                            putPixel(pt.x, pt.y, deformPixels[i].color);
                        }
                    }
                }
            }

            if (gy + 1 < boxH) {
                int j = pixelGrid[(gy + 1) * boxW + gx];
                if (j != -1) {
                    sf::Vector2i p1 = mapped[i];
                    sf::Vector2i p2 = mapped[j];
                    if (std::abs(p1.x - p2.x) > 1 || std::abs(p1.y - p2.y) > 1) {
                        auto pts = getBresenhamPoints(p1.x, p1.y, p2.x, p2.y);
                        for (const auto& pt : pts) {
                            putPixel(pt.x, pt.y, deformPixels[i].color);
                        }
                    }
                }
            }
        }
    }
}

void Canvas::fillPolygonContour(const std::vector<sf::Vector2f>& points, sf::Color color, int currentFrame) {
    if (frames.empty() || currentFrame < 0 || currentFrame >= static_cast<int>(frames.size())) return;
    if (frames[currentFrame].layers[activeLayer].locked || !frames[currentFrame].layers[activeLayer].visible) return;
    if (points.size() < 3) return;

    saveUndoState();

    if (!isPixelMode) {
        VectorStroke vs;
        vs.mesh.setPrimitiveType(sf::Triangles);
        vs.layer = activeLayer;
        vs.frame = currentFrame;
        vs.isErase = false;

        sf::Vector2f c(0.f, 0.f);
        for (const auto& p : points) c += p;
        c /= static_cast<float>(points.size());

        for (size_t i = 0; i < points.size(); ++i) {
            const sf::Vector2f& a = points[i];
            const sf::Vector2f& b = points[(i + 1) % points.size()];
            vs.mesh.append(sf::Vertex(c, color));
            vs.mesh.append(sf::Vertex(a, color));
            vs.mesh.append(sf::Vertex(b, color));
        }

        m_vectorStrokes.push_back(std::move(vs));
        isDirty = true;
        return;
    }

    sf::RenderTexture* targetTex = frames[currentFrame].layers[activeLayer].texture.get();
    if (!targetTex) return;

    int n = static_cast<int>(points.size());

    float minY = points[0].y;
    float maxY = points[0].y;
    for (int i = 1; i < n; ++i) {
        if (points[i].y < minY) minY = points[i].y;
        if (points[i].y > maxY) maxY = points[i].y;
    }

    int startY = std::max(0, static_cast<int>(std::floor(minY)));
    int endY = std::min(static_cast<int>(canvasLogicalSize.y) - 1, static_cast<int>(std::ceil(maxY)));

    std::vector<float> nodeX;
    for (int y = startY; y <= endY; ++y) {
        nodeX.clear();
        float curY = static_cast<float>(y) + 0.5f;

        for (int i = 0; i < n; ++i) {
            int next = (i + 1) % n;
            float y1 = points[i].y;
            float y2 = points[next].y;
            float x1 = points[i].x;
            float x2 = points[next].x;

            if ((y1 < curY && y2 >= curY) || (y2 < curY && y1 >= curY)) {
                float intersectX = x1 + (curY - y1) / (y2 - y1) * (x2 - x1);
                nodeX.push_back(intersectX);
            }
        }

        std::sort(nodeX.begin(), nodeX.end());

        for (size_t i = 0; i + 1 < nodeX.size(); i += 2) {
            int xStart = std::max(0, static_cast<int>(std::floor(nodeX[i])));
            int xEnd = std::min(static_cast<int>(canvasLogicalSize.x) - 1, static_cast<int>(std::ceil(nodeX[i + 1])));

            for (int x = xStart; x <= xEnd; ++x) {
                drawPixelExact(x, y, color, currentFrame);
            }
        }
    }

    targetTex->display();
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
    if (activeTool == ToolType::None) return;

    float scaleX = static_cast<float>(canvasLogicalSize.x) / drawArea.width;
    float scaleY = static_cast<float>(canvasLogicalSize.y) / drawArea.height;
    sf::Vector2f localPos((logicalPos.x - drawArea.left) * scaleX, (logicalPos.y - drawArea.top) * scaleY);
    localPos.x = std::clamp(localPos.x, 0.0f, static_cast<float>(canvasLogicalSize.x));
    localPos.y = std::clamp(localPos.y, 0.0f, static_cast<float>(canvasLogicalSize.y));

    if (isPixelMode && pixelSnapEnabled && activeTool != ToolType::Select) {
        localPos.x = std::floor(localPos.x);
        localPos.y = std::floor(localPos.y);
    }

    if (rightClick) {
        if (activeTool == ToolType::Curve && isDeforming) {
            if (!isPixelMode && m_deformStrokeIndex >= 0 && m_deformStrokeIndex < static_cast<int>(m_vectorStrokes.size())) {
                m_vectorStrokes[m_deformStrokeIndex].mesh = m_originalDeformMesh;
            }
            isDeforming = false;
            m_deformStrokeIndex = -1;
            m_originalDeformMesh.clear();
            deformPixels.clear();
            currentDeformedPixels.clear();
        }
        return;
    }

    if (drawArea.contains(logicalPos)) {
        if (!frames[currentFrame].layers[activeLayer].locked && frames[currentFrame].layers[activeLayer].visible) {

            sf::Color drawCol = primaryColor;

            if (activeTool == ToolType::Curve) {
                if (!isPixelMode) {
                    auto sign = [](sf::Vector2f p1, sf::Vector2f p2, sf::Vector2f p3) {
                        return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
                        };
                    auto ptInTri = [&](sf::Vector2f pt, sf::Vector2f v1, sf::Vector2f v2, sf::Vector2f v3) {
                        float d1 = sign(pt, v1, v2);
                        float d2 = sign(pt, v2, v3);
                        float d3 = sign(pt, v3, v1);
                        return !(((d1 < 0) || (d2 < 0) || (d3 < 0)) && ((d1 > 0) || (d2 > 0) || (d3 > 0)));
                        };

                    int hitStrokeIdx = -1;
                    for (int s = static_cast<int>(m_vectorStrokes.size()) - 1; s >= 0; --s) {
                        const auto& vs = m_vectorStrokes[s];
                        if (vs.frame == currentFrame && vs.layer == activeLayer && !vs.isErase) {
                            for (size_t v = 0; v + 2 < vs.mesh.getVertexCount(); v += 3) {
                                if (ptInTri(localPos, vs.mesh[v].position, vs.mesh[v + 1].position, vs.mesh[v + 2].position)) {
                                    hitStrokeIdx = s;
                                    break;
                                }
                                if (std::hypot(vs.mesh[v].position.x - localPos.x, vs.mesh[v].position.y - localPos.y) <= 10.f ||
                                    std::hypot(vs.mesh[v + 1].position.x - localPos.x, vs.mesh[v + 1].position.y - localPos.y) <= 10.f ||
                                    std::hypot(vs.mesh[v + 2].position.x - localPos.x, vs.mesh[v + 2].position.y - localPos.y) <= 10.f) {
                                    hitStrokeIdx = s;
                                    break;
                                }
                            }
                            if (hitStrokeIdx != -1) break;
                        }
                    }

                    if (hitStrokeIdx == -1) return;

                    saveUndoState();
                    m_deformStrokeIndex = hitStrokeIdx;
                    m_originalDeformMesh = m_vectorStrokes[hitStrokeIdx].mesh;

                    m_vDeformMinX = m_originalDeformMesh[0].position.x;
                    m_vDeformMaxX = m_originalDeformMesh[0].position.x;
                    m_vDeformMinY = m_originalDeformMesh[0].position.y;
                    m_vDeformMaxY = m_originalDeformMesh[0].position.y;

                    for (size_t v = 1; v < m_originalDeformMesh.getVertexCount(); ++v) {
                        m_vDeformMinX = std::min(m_vDeformMinX, m_originalDeformMesh[v].position.x);
                        m_vDeformMaxX = std::max(m_vDeformMaxX, m_originalDeformMesh[v].position.x);
                        m_vDeformMinY = std::min(m_vDeformMinY, m_originalDeformMesh[v].position.y);
                        m_vDeformMaxY = std::max(m_vDeformMaxY, m_originalDeformMesh[v].position.y);
                    }

                    float boxW = std::max(1.f, m_vDeformMaxX - m_vDeformMinX);
                    float boxH = std::max(1.f, m_vDeformMaxY - m_vDeformMinY);
                    deformIsHorizontal = (boxW >= boxH);

                    float L = deformIsHorizontal ? boxW : boxH;
                    float origin = deformIsHorizontal ? m_vDeformMinX : m_vDeformMinY;
                    float curCoord = deformIsHorizontal ? localPos.x : localPos.y;
                    deformT0 = (L > 0.0001f) ? std::clamp((curCoord - origin) / L, 0.0f, 1.0f) : 0.5f;

                    if (deformT0 < 0.15f) deformMode = 2;
                    else if (deformT0 > 0.85f) deformMode = 1;
                    else deformMode = 0;

                    deformClickPos = localPos;
                    deformCurrentPos = localPos;
                    isDeforming = true;
                    return;
                }

                sf::RenderTexture* targetTex = frames[currentFrame].layers[activeLayer].texture.get();
                if (!targetTex) return;

                int w = static_cast<int>(canvasLogicalSize.x);
                int h = static_cast<int>(canvasLogicalSize.y);
                int sx = static_cast<int>(localPos.x);
                int sy = static_cast<int>(localPos.y);

                deformPixels.clear();
                currentDeformedPixels.clear();

                int targetLayerIndex = -1;
                sf::Image targetImg;

                if (selection.isActive()) {
                    targetLayerIndex = activeLayer;
                    targetImg = targetTex->getTexture().copyToImage();
                    deformMinX = w; deformMaxX = 0; deformMinY = h; deformMaxY = 0;
                    for (int y = 0; y < h; ++y) {
                        for (int x = 0; x < w; ++x) {
                            if (selection.isPointInsideSelection(sf::Vector2f(static_cast<float>(x), static_cast<float>(y)))) {
                                sf::Color c = targetImg.getPixel(x, y);
                                if (c.a > 0) {
                                    deformPixels.push_back({ x, y, c });
                                    deformMinX = std::min(deformMinX, x);
                                    deformMaxX = std::max(deformMaxX, x);
                                    deformMinY = std::min(deformMinY, y);
                                    deformMaxY = std::max(deformMaxY, y);
                                }
                            }
                        }
                    }
                }
                else {
                    sf::Image activeImg = targetTex->getTexture().copyToImage();
                    if (sx >= 0 && sy >= 0 && sx < w && sy < h && activeImg.getPixel(sx, sy).a > 0) {
                        targetLayerIndex = activeLayer;
                        targetImg = activeImg;
                    }

                    if (targetLayerIndex == -1) {
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
                    }

                    if (targetLayerIndex == -1) {
                        float bestDist = 9999.0f;
                        int foundX = -1, foundY = -1;
                        for (int i = static_cast<int>(frames[currentFrame].layers.size()) - 1; i >= 0; --i) {
                            if (!frames[currentFrame].layers[i].visible || frames[currentFrame].layers[i].locked) continue;
                            sf::Image tempImg = frames[currentFrame].layers[i].texture->getTexture().copyToImage();
                            int imgW = static_cast<int>(tempImg.getSize().x);
                            int imgH = static_cast<int>(tempImg.getSize().y);
                            for (int r = 1; r <= 5; ++r) {
                                for (int dy = -r; dy <= r; ++dy) {
                                    for (int dx = -r; dx <= r; ++dx) {
                                        int nx = sx + dx;
                                        int ny = sy + dy;
                                        if (nx >= 0 && ny >= 0 && nx < imgW && ny < imgH) {
                                            if (tempImg.getPixel(nx, ny).a > 0) {
                                                float d = static_cast<float>(dx * dx + dy * dy);
                                                if (d < bestDist) {
                                                    bestDist = d;
                                                    foundX = nx;
                                                    foundY = ny;
                                                    targetLayerIndex = i;
                                                    targetImg = tempImg;
                                                }
                                            }
                                        }
                                    }
                                }
                                if (targetLayerIndex != -1) break;
                            }
                            if (targetLayerIndex != -1) break;
                        }
                        if (targetLayerIndex != -1) {
                            sx = foundX;
                            sy = foundY;
                        }
                    }

                    if (targetLayerIndex == -1) return;

                    activeLayer = targetLayerIndex;
                    targetTex = frames[currentFrame].layers[activeLayer].texture.get();

                    std::vector<bool> visited(w * h, false);
                    std::vector<sf::Vector2i> stack;
                    stack.push_back({ sx, sy });
                    visited[sy * w + sx] = true;

                    deformMinX = sx; deformMaxX = sx; deformMinY = sy; deformMaxY = sy;

                    const int dx8[8] = { 1, -1, 0, 0, 1, 1, -1, -1 };
                    const int dy8[8] = { 0, 0, 1, -1, 1, -1, 1, -1 };

                    while (!stack.empty()) {
                        sf::Vector2i p = stack.back();
                        stack.pop_back();

                        sf::Color c = targetImg.getPixel(p.x, p.y);
                        deformPixels.push_back({ p.x, p.y, c });

                        deformMinX = std::min(deformMinX, p.x);
                        deformMaxX = std::max(deformMaxX, p.x);
                        deformMinY = std::min(deformMinY, p.y);
                        deformMaxY = std::max(deformMaxY, p.y);

                        for (int d = 0; d < 8; ++d) {
                            int nx = p.x + dx8[d];
                            int ny = p.y + dy8[d];
                            if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                                int idx = ny * w + nx;
                                if (!visited[idx] && targetImg.getPixel(nx, ny).a > 0) {
                                    visited[idx] = true;
                                    stack.push_back({ nx, ny });
                                }
                            }
                        }
                    }
                }

                if (deformPixels.empty()) return;

                int boxW = std::max(1, deformMaxX - deformMinX + 1);
                int boxH = std::max(1, deformMaxY - deformMinY + 1);
                deformIsHorizontal = (boxW >= boxH);

                float L = deformIsHorizontal ? static_cast<float>(std::max(1, boxW - 1)) : static_cast<float>(std::max(1, boxH - 1));
                float origin = deformIsHorizontal ? static_cast<float>(deformMinX) : static_cast<float>(deformMinY);
                float curCoord = deformIsHorizontal ? static_cast<float>(sx) : static_cast<float>(sy);
                deformT0 = (L > 0.f) ? std::clamp((curCoord - origin) / L, 0.0f, 1.0f) : 0.5f;

                if (deformT0 < 0.15f) deformMode = 2;
                else if (deformT0 > 0.85f) deformMode = 1;
                else deformMode = 0;

                deformClickPos = sf::Vector2f(static_cast<float>(sx), static_cast<float>(sy));
                deformCurrentPos = deformClickPos;
                isDeforming = true;
                updateDeformPixels(sf::Vector2f(0.f, 0.f));
                return;
            }

            if (activeTool == ToolType::FilledContour) {
                m_contourPoints.clear();
                m_contourPoints.push_back(localPos);
                isDrawing = true;
                return;
            }

            if (activeTool == ToolType::Symmetry) {
                isDrawing = true;
                float hitRadius = computeHandleHitRadius() * 1.5f;

                float existingLen = std::hypot(symmetryManager.endPoint.x - symmetryManager.startPoint.x,
                    symmetryManager.endPoint.y - symmetryManager.startPoint.y);

                if (symmetryManager.visible && existingLen > 2.0f) {
                    // Grab start handle to reposition/resize
                    if (std::hypot(localPos.x - symmetryManager.startPoint.x, localPos.y - symmetryManager.startPoint.y) <= hitRadius) {
                        m_symmetryDragMode = SymmetryDragMode::StartHandle;
                        return;
                    }
                    // Grab end handle to reposition/resize
                    if (std::hypot(localPos.x - symmetryManager.endPoint.x, localPos.y - symmetryManager.endPoint.y) <= hitRadius) {
                        m_symmetryDragMode = SymmetryDragMode::EndHandle;
                        return;
                    }
                    // Grab line body to translate whole line
                    sf::Vector2f pA = symmetryManager.startPoint;
                    sf::Vector2f pB = symmetryManager.endPoint;
                    sf::Vector2f d = pB - pA;
                    float dLenSq = d.x * d.x + d.y * d.y;
                    if (dLenSq > 0.001f) {
                        float t = std::clamp(((localPos.x - pA.x) * d.x + (localPos.y - pA.y) * d.y) / dLenSq, 0.0f, 1.0f);
                        sf::Vector2f proj = pA + t * d;
                        if (std::hypot(localPos.x - proj.x, localPos.y - proj.y) <= hitRadius) {
                            m_symmetryDragMode = SymmetryDragMode::MoveEntire;
                            m_symmetryDragOffsetStart = symmetryManager.startPoint - localPos;
                            m_symmetryDragOffsetEnd = symmetryManager.endPoint - localPos;
                            return;
                        }
                    }
                }

                // If not grabbing existing points/line, drag to create new line
                m_symmetryDragMode = SymmetryDragMode::NewAxis;
                startPos = localPos;
                symmetryManager.setEndpoints(localPos, localPos);
                symmetryManager.visible = true;
                symmetryManager.enabled = false;
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
                        extractFloatingStrokes(currentFrame);
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

            if (activeTool == ToolType::Fill) {
                if (isPixelMode) {
                    fillTolerance = 0.0f;
                    saveUndoState();
                    sf::RenderTexture* targetTex = frames[currentFrame].layers[activeLayer].texture.get();
                    if (!targetTex) return;

                    sf::Image img = targetTex->getTexture().copyToImage();
                    sf::Vector2i pixelPos(static_cast<int>(localPos.x), static_cast<int>(localPos.y));

                    if (pixelPos.x >= 0 && pixelPos.y >= 0 && pixelPos.x < static_cast<int>(img.getSize().x) && pixelPos.y < static_cast<int>(img.getSize().y)) {
                        sf::Color targetCol = img.getPixel(pixelPos.x, pixelPos.y);

                        if (fillContiguous) executeQueueFill(pixelPos, targetCol, drawCol, img);
                        else executeGlobalFill(targetCol, drawCol, img);

                        sf::Texture tex;
                        tex.loadFromImage(img);
                        sf::Sprite spr(tex);
                        targetTex->clear(sf::Color::Transparent);
                        targetTex->draw(spr, sf::RenderStates(sf::BlendNone));
                        targetTex->display();
                        isDirty = true;
                    }
                    return;
                }
                else {
                    sf::RenderTexture scratch;
                    if (!renderLayerToTexture(currentFrame, activeLayer, scratch)) return;

                    sf::Image img = scratch.getTexture().copyToImage();
                    sf::Vector2i pixelPos(static_cast<int>(localPos.x), static_cast<int>(localPos.y));

                    int w = static_cast<int>(img.getSize().x);
                    int h = static_cast<int>(img.getSize().y);
                    if (pixelPos.x < 0 || pixelPos.x >= w || pixelPos.y < 0 || pixelPos.y >= h) return;

                    sf::Color targetCol = img.getPixel(pixelPos.x, pixelPos.y);

                    // =========================================================================
                    // 1. RECOLOR STROKE DIRECTLY IF CLICKED ON A LINE / STROKE
                    // =========================================================================
                    if (targetCol.a > 120) {
                        auto sign = [](sf::Vector2f p1, sf::Vector2f p2, sf::Vector2f p3) {
                            return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
                            };
                        auto ptInTri = [&](sf::Vector2f pt, sf::Vector2f v1, sf::Vector2f v2, sf::Vector2f v3) {
                            float d1 = sign(pt, v1, v2);
                            float d2 = sign(pt, v2, v3);
                            float d3 = sign(pt, v3, v1);
                            return !(((d1 < 0) || (d2 < 0) || (d3 < 0)) && ((d1 > 0) || (d2 > 0) || (d3 > 0)));
                            };

                        int hitStrokeIdx = -1;
                        for (int s = static_cast<int>(m_vectorStrokes.size()) - 1; s >= 0; --s) {
                            auto& vs = m_vectorStrokes[s];
                            if (vs.frame == currentFrame && vs.layer == activeLayer && !vs.isErase) {
                                for (size_t v = 0; v + 2 < vs.mesh.getVertexCount(); v += 3) {
                                    if (ptInTri(localPos, vs.mesh[v].position, vs.mesh[v + 1].position, vs.mesh[v + 2].position)) {
                                        hitStrokeIdx = s;
                                        break;
                                    }
                                }
                                if (hitStrokeIdx != -1) break;
                            }
                        }

                        if (hitStrokeIdx != -1) {
                            saveUndoState();
                            for (size_t v = 0; v < m_vectorStrokes[hitStrokeIdx].mesh.getVertexCount(); ++v) {
                                m_vectorStrokes[hitStrokeIdx].mesh[v].color = drawCol;
                            }
                            isDirty = true;
                            return;
                        }
                    }

                    // =========================================================================
                    // 2. FILL ENCLOSED AREA (TUCKED UNDER SOLID CORE OF THE STROKE)
                    // =========================================================================
                    if (colorMatches(targetCol, drawCol)) return;

                    std::vector<bool> fillMask(w * h, false);
                    std::queue<sf::Vector2i> q;
                    q.push(pixelPos);
                    fillMask[pixelPos.y * w + pixelPos.x] = true;

                    while (!q.empty()) {
                        sf::Vector2i p = q.front();
                        q.pop();

                        const int dx[4] = { 1, -1, 0, 0 };
                        const int dy[4] = { 0, 0, 1, -1 };
                        for (int d = 0; d < 4; ++d) {
                            int nx = p.x + dx[d];
                            int ny = p.y + dy[d];
                            if (nx >= 0 && nx < w && ny >= 0 && ny < h && !fillMask[ny * w + nx]) {
                                sf::Color c = img.getPixel(nx, ny);
                                // Traverse empty space and faint anti-aliased edges (alpha <= 120)
                                if (c.a <= 120) {
                                    fillMask[ny * w + nx] = true;
                                    q.push({ nx, ny });
                                }
                            }
                        }
                    }

                    // 8-way 2px dilation pushes the fill completely under the solid core of the stroke
                    std::vector<bool> dilatedMask = fillMask;
                    const int dx8[8] = { 1, -1, 0, 0, 1, 1, -1, -1 };
                    const int dy8[8] = { 0, 0, 1, -1, 1, -1, 1, -1 };

                    for (int pass = 0; pass < 2; ++pass) {
                        std::vector<bool> passMask = dilatedMask;
                        for (int y = 0; y < h; ++y) {
                            for (int x = 0; x < w; ++x) {
                                if (passMask[y * w + x]) {
                                    for (int d = 0; d < 8; ++d) {
                                        int nx = x + dx8[d];
                                        int ny = y + dy8[d];
                                        if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                                            dilatedMask[ny * w + nx] = true;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    VectorStroke vs;
                    vs.mesh.setPrimitiveType(sf::Triangles);
                    vs.layer = activeLayer;
                    vs.frame = currentFrame;
                    vs.isErase = false;

                    for (int y = 0; y < h; ++y) {
                        int x = 0;
                        while (x < w) {
                            if (dilatedMask[y * w + x]) {
                                int xStart = x;
                                while (x < w && dilatedMask[y * w + x]) {
                                    x++;
                                }
                                int xEnd = x;

                                float fx0 = static_cast<float>(xStart);
                                float fx1 = static_cast<float>(xEnd);
                                float fy0 = static_cast<float>(y);
                                float fy1 = static_cast<float>(y + 1);

                                vs.mesh.append(sf::Vertex(sf::Vector2f(fx0, fy0), drawCol));
                                vs.mesh.append(sf::Vertex(sf::Vector2f(fx1, fy0), drawCol));
                                vs.mesh.append(sf::Vertex(sf::Vector2f(fx1, fy1), drawCol));

                                vs.mesh.append(sf::Vertex(sf::Vector2f(fx0, fy0), drawCol));
                                vs.mesh.append(sf::Vertex(sf::Vector2f(fx1, fy1), drawCol));
                                vs.mesh.append(sf::Vertex(sf::Vector2f(fx0, fy1), drawCol));
                            }
                            else {
                                x++;
                            }
                        }
                    }

                    if (vs.mesh.getVertexCount() > 0) {
                        saveUndoState();
                        auto insertPos = m_vectorStrokes.end();
                        for (auto it = m_vectorStrokes.begin(); it != m_vectorStrokes.end(); ++it) {
                            if (it->frame == currentFrame && it->layer == activeLayer) {
                                insertPos = it;
                                break;
                            }
                        }
                        m_vectorStrokes.insert(insertPos, std::move(vs));
                        isDirty = true;
                    }
                    return;
                }
            }

            bool canDrawLine = (activeTool == ToolType::Brush || activeTool == ToolType::Pencil || activeTool == ToolType::Eraser);
            bool isShift = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);

            if (selection.isActive() && canDrawLine) {
                commitSelection(currentFrame);
                selection.clearSelection();
            }

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

            if (!isPixelMode && m_isVectorStrokeActive && m_activeVectorMesh.getVertexCount() > 0) {
                VectorStroke vs;
                vs.mesh = m_activeVectorMesh;
                vs.layer = activeLayer;
                vs.frame = currentFrame;
                vs.isErase = m_activeStrokeIsErase;
                m_vectorStrokes.push_back(std::move(vs));
                m_isVectorStrokeActive = false;
                m_activeVectorMesh.clear();
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
                if (activeTool == ToolType::Brush || activeTool == ToolType::Pencil || activeTool == ToolType::Eraser) {
                    m_isVectorStrokeActive = true;
                    m_activeStrokeIsErase = (activeTool == ToolType::Eraser);
                    m_stabilizedPos = localPos;
                    m_vPrevPoint = localPos;
                    m_vPrevMidPoint = localPos;
                    m_activeVectorMesh.clear();
                    m_activeVectorMesh.setPrimitiveType(sf::Triangles);

                    float radius = brushEngine.getActivePreset().size * 0.5f;
                    sf::Color meshCol = m_activeStrokeIsErase ? sf::Color::White : drawCol;
                    appendVectorCap(m_activeVectorMesh, localPos, radius, meshCol);

                    if (symmetryManager.enabled) {
                        auto symPts = symmetryManager.getSymmetricPoints(localPos);
                        if (symPts.size() > 1) {
                            sf::Vector2f sp = symPts[1];
                            if (sp.x >= 0.f && sp.x <= static_cast<float>(canvasLogicalSize.x) &&
                                sp.y >= 0.f && sp.y <= static_cast<float>(canvasLogicalSize.y)) {
                                appendVectorCap(m_activeVectorMesh, sp, radius, meshCol);
                            }
                        }
                    }

                    if (m_activeStrokeIsErase) {
                        sf::RenderTexture* targetTex = frames[currentFrame].layers[activeLayer].texture.get();
                        if (targetTex) {
                            sf::RenderStates rs(sf::BlendNone);
                            sf::CircleShape circle(radius);
                            circle.setOrigin(radius, radius);
                            circle.setPosition(localPos);
                            circle.setFillColor(sf::Color::Transparent);
                            targetTex->draw(circle, rs);

                            if (symmetryManager.enabled) {
                                auto symPts = symmetryManager.getSymmetricPoints(localPos);
                                if (symPts.size() > 1) {
                                    sf::Vector2f sp = symPts[1];
                                    if (sp.x >= 0.f && sp.x <= static_cast<float>(canvasLogicalSize.x) &&
                                        sp.y >= 0.f && sp.y <= static_cast<float>(canvasLogicalSize.y)) {
                                        circle.setPosition(sp);
                                        targetTex->draw(circle, rs);
                                    }
                                }
                            }
                            targetTex->display();
                        }
                    }
                }
                else {
                    brushEngine.resetStroke(localPos);
                }
            }
        }
    }
}

void Canvas::handleMouseReleased(sf::Vector2f logicalPos, int currentFrame) {
    float scaleX = static_cast<float>(canvasLogicalSize.x) / drawArea.width;
    float scaleY = static_cast<float>(canvasLogicalSize.y) / drawArea.height;
    sf::Vector2f localPos((logicalPos.x - drawArea.left) * scaleX, (logicalPos.y - drawArea.top) * scaleY);

    if (isPixelMode && pixelSnapEnabled && activeTool != ToolType::Select) {
        localPos.x = std::floor(localPos.x);
        localPos.y = std::floor(localPos.y);
    }

    if (activeTool == ToolType::Curve) {
        if (isDeforming) {
            deformCurrentPos = localPos;
            sf::Vector2f delta = deformCurrentPos - deformClickPos;

            if (!isPixelMode) {
                if (std::hypot(delta.x, delta.y) < 1.0f && m_deformStrokeIndex >= 0 && m_deformStrokeIndex < static_cast<int>(m_vectorStrokes.size())) {
                    m_vectorStrokes[m_deformStrokeIndex].mesh = m_originalDeformMesh;
                }
                else {
                    isDirty = true;
                }
                isDeforming = false;
                m_deformStrokeIndex = -1;
                m_originalDeformMesh.clear();
                return;
            }

            if (std::abs(delta.x) >= 1.0f || std::abs(delta.y) >= 1.0f) {
                saveUndoState();
                updateDeformPixels(delta);

                sf::RenderTexture* targetTex = frames[currentFrame].layers[activeLayer].texture.get();
                if (targetTex) {
                    sf::RenderStates rsNone;
                    rsNone.blendMode = sf::BlendNone;
                    sf::RectangleShape clearPx(sf::Vector2f(1.f, 1.f));
                    clearPx.setFillColor(sf::Color::Transparent);

                    for (const auto& dp : deformPixels) {
                        clearPx.setPosition(static_cast<float>(dp.x), static_cast<float>(dp.y));
                        targetTex->draw(clearPx, rsNone);
                    }

                    sf::RectangleShape drawPx(sf::Vector2f(1.f, 1.f));
                    for (const auto& dp : currentDeformedPixels) {
                        drawPx.setPosition(static_cast<float>(dp.x), static_cast<float>(dp.y));
                        drawPx.setFillColor(dp.color);
                        targetTex->draw(drawPx, rsNone);
                    }
                    targetTex->display();
                }
                isDirty = true;
            }

            isDeforming = false;
            deformPixels.clear();
            currentDeformedPixels.clear();
        }
        return;
    }

    if (activeTool == ToolType::FilledContour) {
        if (isDrawing) {
            isDrawing = false;
            if (m_contourPoints.size() >= 3) {
                fillPolygonContour(m_contourPoints, primaryColor, currentFrame);
            }
            m_contourPoints.clear();
        }
        return;
    }

    if (activeTool == ToolType::Symmetry) {
        isDrawing = false;
        float finalLen = std::hypot(symmetryManager.endPoint.x - symmetryManager.startPoint.x,
            symmetryManager.endPoint.y - symmetryManager.startPoint.y);

        if (finalLen > 10.f) {
            symmetryManager.enabled = true;
            symmetryManager.visible = true;
        }
        else {
            clearSymmetry();
        }
        m_symmetryDragMode = SymmetryDragMode::None;
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

    if (!isPixelMode && m_isVectorStrokeActive) {
        float radius = brushEngine.getActivePreset().size * 0.5f;
        sf::Color meshCol = m_activeStrokeIsErase ? sf::Color::White : primaryColor;

        float dEnd = std::hypot(localPos.x - m_vPrevPoint.x, localPos.y - m_vPrevPoint.y);
        if (dEnd > 1.0f) {
            appendVectorSegment(m_activeVectorMesh, m_vPrevMidPoint, m_vPrevPoint, radius, meshCol);
            appendVectorSegment(m_activeVectorMesh, m_vPrevPoint, localPos, radius, meshCol);
            appendVectorCap(m_activeVectorMesh, localPos, radius, meshCol);

            if (symmetryManager.enabled) {
                auto p1 = symmetryManager.getSymmetricPoints(m_vPrevMidPoint);
                auto p2 = symmetryManager.getSymmetricPoints(m_vPrevPoint);
                auto p3 = symmetryManager.getSymmetricPoints(localPos);
                if (p1.size() > 1 && p2.size() > 1 && p3.size() > 1) {
                    float cw = static_cast<float>(canvasLogicalSize.x);
                    float ch = static_cast<float>(canvasLogicalSize.y);
                    if (p1[1].x >= 0.f && p1[1].x <= cw && p1[1].y >= 0.f && p1[1].y <= ch &&
                        p2[1].x >= 0.f && p2[1].x <= cw && p2[1].y >= 0.f && p2[1].y <= ch &&
                        p3[1].x >= 0.f && p3[1].x <= cw && p3[1].y >= 0.f && p3[1].y <= ch) {
                        appendVectorSegment(m_activeVectorMesh, p1[1], p2[1], radius, meshCol);
                        appendVectorSegment(m_activeVectorMesh, p2[1], p3[1], radius, meshCol);
                        appendVectorCap(m_activeVectorMesh, p3[1], radius, meshCol);
                    }
                }
            }
        }
        else {
            appendVectorSegment(m_activeVectorMesh, m_vPrevMidPoint, m_vPrevPoint, radius, meshCol);
            appendVectorCap(m_activeVectorMesh, m_vPrevPoint, radius, meshCol);

            if (symmetryManager.enabled) {
                auto p1 = symmetryManager.getSymmetricPoints(m_vPrevMidPoint);
                auto p2 = symmetryManager.getSymmetricPoints(m_vPrevPoint);
                if (p1.size() > 1 && p2.size() > 1) {
                    float cw = static_cast<float>(canvasLogicalSize.x);
                    float ch = static_cast<float>(canvasLogicalSize.y);
                    if (p1[1].x >= 0.f && p1[1].x <= cw && p1[1].y >= 0.f && p1[1].y <= ch &&
                        p2[1].x >= 0.f && p2[1].x <= cw && p2[1].y >= 0.f && p2[1].y <= ch) {
                        appendVectorSegment(m_activeVectorMesh, p1[1], p2[1], radius, meshCol);
                        appendVectorCap(m_activeVectorMesh, p2[1], radius, meshCol);
                    }
                }
            }
        }

        if (m_activeVectorMesh.getVertexCount() > 0) {
            VectorStroke vs;
            vs.mesh = m_activeVectorMesh;
            vs.layer = activeLayer;
            vs.frame = currentFrame;
            vs.isErase = m_activeStrokeIsErase;
            m_vectorStrokes.push_back(std::move(vs));
        }
        m_isVectorStrokeActive = false;
        m_activeVectorMesh.clear();
    }

    if (isDrawing) {
        shiftAnchor = localPos;
        hasShiftAnchor = true;
    }
    isDrawing = false;
}

void Canvas::handleMouseMoved(sf::Vector2f logicalPos, sf::Vector2f rawPos, int currentFrame) {
    isHoveringCanvas = drawArea.contains(logicalPos);
    rawMousePos = rawPos;

    float scaleX = static_cast<float>(canvasLogicalSize.x) / drawArea.width;
    float scaleY = static_cast<float>(canvasLogicalSize.y) / drawArea.height;
    sf::Vector2f localPos((logicalPos.x - drawArea.left) * scaleX, (logicalPos.y - drawArea.top) * scaleY);
    localPos.x = std::clamp(localPos.x, 0.0f, static_cast<float>(canvasLogicalSize.x));
    localPos.y = std::clamp(localPos.y, 0.0f, static_cast<float>(canvasLogicalSize.y));

    if (isPixelMode && pixelSnapEnabled && activeTool != ToolType::Select) {
        localPos.x = std::floor(localPos.x);
        localPos.y = std::floor(localPos.y);
    }

    lastHoverLocalPos = localPos;

    if (activeTool == ToolType::Fill) return;

    if (activeTool == ToolType::Curve) {
        if (isDeforming) {
            deformCurrentPos = localPos;
            if (isPixelMode) {
                updateDeformPixels(deformCurrentPos - deformClickPos);
            }
            else {
                if (m_deformStrokeIndex >= 0 && m_deformStrokeIndex < static_cast<int>(m_vectorStrokes.size())) {
                    sf::Vector2f delta = deformCurrentPos - deformClickPos;
                    float boxW = std::max(1.f, m_vDeformMaxX - m_vDeformMinX);
                    float boxH = std::max(1.f, m_vDeformMaxY - m_vDeformMinY);
                    float L = deformIsHorizontal ? boxW : boxH;
                    float origin = deformIsHorizontal ? m_vDeformMinX : m_vDeformMinY;

                    auto& mesh = m_vectorStrokes[m_deformStrokeIndex].mesh;
                    for (size_t v = 0; v < mesh.getVertexCount(); ++v) {
                        float coord = deformIsHorizontal ? m_originalDeformMesh[v].position.x : m_originalDeformMesh[v].position.y;
                        float t = (L > 0.0001f) ? std::clamp((coord - origin) / L, 0.0f, 1.0f) : 0.5f;
                        float w = computeDeformWeight(t);
                        mesh[v].position = m_originalDeformMesh[v].position + delta * w;
                    }
                    isDirty = true;
                }
            }
        }
        return;
    }

    if (activeTool == ToolType::FilledContour) {
        if (isDrawing) {
            if (m_contourPoints.empty() ||
                std::hypot(localPos.x - m_contourPoints.back().x, localPos.y - m_contourPoints.back().y) >= 1.0f) {
                m_contourPoints.push_back(localPos);
            }
        }
        return;
    }

    if (activeTool == ToolType::Symmetry) {
        if (isDrawing) {
            bool shiftSnap = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);

            if (m_symmetryDragMode == SymmetryDragMode::StartHandle) {
                sf::Vector2f newStart = localPos;
                if (shiftSnap) {
                    sf::Vector2f diff = newStart - symmetryManager.endPoint;
                    float angle = std::atan2(diff.y, diff.x);
                    float snapped = std::round(angle / (3.14159265f / 4.f)) * (3.14159265f / 4.f);
                    float dLen = std::hypot(diff.x, diff.y);
                    newStart.x = symmetryManager.endPoint.x + dLen * std::cos(snapped);
                    newStart.y = symmetryManager.endPoint.y + dLen * std::sin(snapped);
                }
                symmetryManager.startPoint = newStart;
                symmetryManager.updateVectors();
            }
            else if (m_symmetryDragMode == SymmetryDragMode::EndHandle) {
                sf::Vector2f newEnd = localPos;
                if (shiftSnap) {
                    sf::Vector2f diff = newEnd - symmetryManager.startPoint;
                    float angle = std::atan2(diff.y, diff.x);
                    float snapped = std::round(angle / (3.14159265f / 4.f)) * (3.14159265f / 4.f);
                    float dLen = std::hypot(diff.x, diff.y);
                    newEnd.x = symmetryManager.startPoint.x + dLen * std::cos(snapped);
                    newEnd.y = symmetryManager.startPoint.y + dLen * std::sin(snapped);
                }
                symmetryManager.endPoint = newEnd;
                symmetryManager.updateVectors();
            }
            else if (m_symmetryDragMode == SymmetryDragMode::MoveEntire) {
                symmetryManager.startPoint = localPos + m_symmetryDragOffsetStart;
                symmetryManager.endPoint = localPos + m_symmetryDragOffsetEnd;
                symmetryManager.updateVectors();
            }
            else if (m_symmetryDragMode == SymmetryDragMode::NewAxis) {
                sf::Vector2f endPos = localPos;
                if (shiftSnap) {
                    sf::Vector2f diff = endPos - startPos;
                    float angle = std::atan2(diff.y, diff.x);
                    float snapped = std::round(angle / (3.14159265f / 4.f)) * (3.14159265f / 4.f);
                    float dLen = std::hypot(diff.x, diff.y);
                    endPos.x = startPos.x + dLen * std::cos(snapped);
                    endPos.y = startPos.y + dLen * std::sin(snapped);
                }
                symmetryManager.setEndpoints(startPos, endPos);
            }
        }
        return;
    }

    if (activeTool == ToolType::Select) {
        bool allowOutside = isImageResourceActive(currentFrame);

        if (selection.isResizing()) {
            selection.resize(localPos, canvasLogicalSize, allowOutside);
            return;
        }
        if (selection.getState() == SelectionState::Drawing) {
            selection.addLassoPoint(localPos, canvasLogicalSize);
        }
        else if (selection.getState() == SelectionState::Floating) {
            selection.drag(localPos, canvasLogicalSize, allowOutside);
        }
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
            if ((activeTool == ToolType::Brush || activeTool == ToolType::Pencil || activeTool == ToolType::Eraser)
                && m_isVectorStrokeActive) {
                float stab = brushEngine.getActivePreset().stabilization;
                if (stab > 0.0f) {
                    float weight = std::clamp(1.0f - stab, 0.05f, 1.0f);
                    m_stabilizedPos += (targetPos - m_stabilizedPos) * weight;
                    targetPos = m_stabilizedPos;
                }
                else {
                    m_stabilizedPos = targetPos;
                }

                float dx = targetPos.x - m_vPrevPoint.x;
                float dy = targetPos.y - m_vPrevPoint.y;
                if (dx * dx + dy * dy >= 0.8f) {
                    sf::Vector2f midPoint = (m_vPrevPoint + targetPos) * 0.5f;
                    float radius = brushEngine.getActivePreset().size * 0.5f;
                    sf::Color meshCol = m_activeStrokeIsErase ? sf::Color::White : primaryColor;
                    const int segments = 6;
                    sf::Vector2f lastP = m_vPrevMidPoint;

                    for (int i = 1; i <= segments; ++i) {
                        float t = static_cast<float>(i) / static_cast<float>(segments);
                        float invT = 1.0f - t;
                        sf::Vector2f curveP = (invT * invT * m_vPrevMidPoint) + (2.0f * invT * t * m_vPrevPoint) + (t * t * midPoint);
                        appendVectorSegment(m_activeVectorMesh, lastP, curveP, radius, meshCol);
                        if (symmetryManager.enabled) {
                            auto p1Sym = symmetryManager.getSymmetricPoints(lastP);
                            auto p2Sym = symmetryManager.getSymmetricPoints(curveP);
                            if (p1Sym.size() > 1 && p2Sym.size() > 1) {
                                sf::Vector2f s1 = p1Sym[1];
                                sf::Vector2f s2 = p2Sym[1];
                                float cw = static_cast<float>(canvasLogicalSize.x);
                                float ch = static_cast<float>(canvasLogicalSize.y);
                                if (s1.x >= 0.f && s1.x <= cw && s1.y >= 0.f && s1.y <= ch &&
                                    s2.x >= 0.f && s2.x <= cw && s2.y >= 0.f && s2.y <= ch) {
                                    appendVectorSegment(m_activeVectorMesh, s1, s2, radius, meshCol);
                                }
                            }
                        }
                        lastP = curveP;
                    }

                    // Erase line segment directly on layer texture for text/raster content
                    if (m_activeStrokeIsErase) {
                        sf::RenderTexture* targetTex = frames[currentFrame].layers[activeLayer].texture.get();
                        if (targetTex) {
                            sf::RenderStates rs(sf::BlendNone);
                            float length = std::hypot(targetPos.x - lastPos.x, targetPos.y - lastPos.y);
                            if (length > 0.001f) {
                                sf::RectangleShape line(sf::Vector2f(length, radius * 2.0f));
                                line.setOrigin(0.0f, radius);
                                line.setPosition(lastPos);
                                line.setRotation(std::atan2(targetPos.y - lastPos.y, targetPos.x - lastPos.x) * 180.f / 3.14159265f);
                                line.setFillColor(sf::Color::Transparent);

                                sf::CircleShape circle(radius);
                                circle.setOrigin(radius, radius);
                                circle.setPosition(targetPos);
                                circle.setFillColor(sf::Color::Transparent);

                                targetTex->draw(line, rs);
                                targetTex->draw(circle, rs);

                                if (symmetryManager.enabled) {
                                    auto fromSym = symmetryManager.getSymmetricPoints(lastPos);
                                    auto toSym = symmetryManager.getSymmetricPoints(targetPos);
                                    if (fromSym.size() > 1 && toSym.size() > 1) {
                                        sf::Vector2f s1 = fromSym[1];
                                        sf::Vector2f s2 = toSym[1];
                                        float cw = static_cast<float>(canvasLogicalSize.x);
                                        float ch = static_cast<float>(canvasLogicalSize.y);
                                        if (s1.x >= 0.f && s1.x <= cw && s1.y >= 0.f && s1.y <= ch &&
                                            s2.x >= 0.f && s2.x <= cw && s2.y >= 0.f && s2.y <= ch) {
                                            line.setPosition(s1);
                                            line.setRotation(std::atan2(s2.y - s1.y, s2.x - s1.x) * 180.f / 3.14159265f);
                                            circle.setPosition(s2);
                                            targetTex->draw(line, rs);
                                            targetTex->draw(circle, rs);
                                        }
                                    }
                                }
                                targetTex->display();
                            }
                        }
                    }

                    m_vPrevPoint = targetPos;
                    m_vPrevMidPoint = midPoint;
                }
            }
        }

        lastPos = targetPos;
        shiftAnchor = targetPos;
        hasShiftAnchor = true;

        if (!isHoveringCanvas && isDrawing) {
            if (!isPixelMode && m_isVectorStrokeActive) {
                float radius = brushEngine.getActivePreset().size * 0.5f;
                sf::Color meshCol = m_activeStrokeIsErase ? sf::Color::White : primaryColor;
                appendVectorSegment(m_activeVectorMesh, m_vPrevMidPoint, m_vPrevPoint, radius, meshCol);
                appendVectorCap(m_activeVectorMesh, m_vPrevPoint, radius, meshCol);

                if (m_activeVectorMesh.getVertexCount() > 0) {
                    VectorStroke vs;
                    vs.mesh = m_activeVectorMesh;
                    vs.layer = activeLayer;
                    vs.frame = currentFrame;
                    vs.isErase = m_activeStrokeIsErase;
                    m_vectorStrokes.push_back(std::move(vs));
                }
                m_isVectorStrokeActive = false;
                m_activeVectorMesh.clear();
            }
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
    bakeLayerStrokes(currentFrame, activeLayer);

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

void Canvas::drawLayerThumbnail(sf::RenderTarget& target, int frameIndex, int layerIndex, sf::FloatRect bounds) {
    if (frameIndex < 0 || frameIndex >= static_cast<int>(frames.size())) return;
    if (layerIndex < 0 || layerIndex >= static_cast<int>(frames[frameIndex].layers.size())) return;

    float cw = static_cast<float>(canvasLogicalSize.x);
    float ch = static_cast<float>(canvasLogicalSize.y);
    if (cw <= 0.f || ch <= 0.f) return;

    float s = std::min(bounds.width / cw, bounds.height / ch);

    sf::RenderStates states;
    states.transform.translate(bounds.left, bounds.top);
    states.transform.scale(s, s);

    drawLayerContent(target, frameIndex, layerIndex, states, false);

    if (m_textManager) {
        m_textManager->render(target, frameIndex, layerIndex, isPixelMode, states, canvasLogicalSize);
    }
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

        // Hardcoded 1px pixel grid removed
    }

    if (symmetryManager.visible) {
        symmetryManager.drawGuides(window, innerStates, sf::FloatRect(0, 0, static_cast<float>(canvasLogicalSize.x), static_cast<float>(canvasLogicalSize.y)), viewScale);
    }

    // Prominent instructional banner when Symmetry tool is active
    bool hasLine = (std::hypot(symmetryManager.endPoint.x - symmetryManager.startPoint.x,
        symmetryManager.endPoint.y - symmetryManager.startPoint.y) > 2.0f);

    if (!isPlaying && activeTool == ToolType::Symmetry && !hasLine && !isDrawing) {
        float cx = drawArea.left + drawArea.width * 0.5f;
        float cy = drawArea.top + 20.f;
        float bannerW = 260.f;
        float bannerH = 28.f;

        sf::RectangleShape bannerBg(sf::Vector2f(bannerW, bannerH));
        bannerBg.setPosition(cx - bannerW * 0.5f, cy);
        bannerBg.setFillColor(sf::Color(15, 12, 22, 180));
        bannerBg.setOutlineThickness(1.f);
        bannerBg.setOutlineColor(sf::Color(0, 220, 255, 140));
        window.draw(bannerBg, states);

        static sf::Font promptFont;
        static bool pfLoaded = promptFont.loadFromFile("assets/font.otf");
        if (pfLoaded) {
            sf::Text msg("Click & drag to draw symmetry line", promptFont, 11);
            sf::FloatRect mb = msg.getLocalBounds();
            msg.setOrigin(mb.left + mb.width * 0.5f, mb.top + mb.height * 0.5f);
            msg.setPosition(cx, cy + bannerH * 0.5f);
            msg.setFillColor(sf::Color(255, 226, 110));
            window.draw(msg, states);
        }
    }

    if (!isPlaying && onionSkinEnabled) {
        for (int i = 1; i <= onionSkinPrevCount; ++i) {
            int prevIdx = currentFrame - i;
            if (prevIdx >= 0 && prevIdx < static_cast<int>(frames.size())) {
                float fadeOpac = onionSkinPrevOpacity * (1.0f - (static_cast<float>(i) - 1.0f) / static_cast<float>(onionSkinPrevCount));
                float tint = std::clamp(fadeOpac / 255.0f, 0.f, 1.f);
                for (size_t li = 0; li < frames[prevIdx].layers.size(); ++li) {
                    const auto& layer = frames[prevIdx].layers[li];
                    if (!layer.visible) continue;

                    sf::Sprite onionSpr(layer.texture->getTexture());
                    onionSpr.setColor(sf::Color(255, 100, 100, static_cast<sf::Uint8>(fadeOpac)));
                    sf::RenderStates oStates = innerStates;
                    oStates.blendMode = getSFMLBlendMode(layer.blendMode).blendMode;
                    window.draw(onionSpr, oStates);

                    if (!isPixelMode) {
                        for (const auto& vs : m_vectorStrokes) {
                            if (vs.frame != prevIdx || vs.layer != static_cast<int>(li) || vs.isErase) continue;
                            sf::VertexArray ghost = meshWithOpacity(vs.mesh, tint);
                            for (std::size_t v = 0; v < ghost.getVertexCount(); ++v) {
                                ghost[v].color.r = 255; ghost[v].color.g = 100; ghost[v].color.b = 100;
                            }
                            window.draw(ghost, oStates);
                        }
                    }
                }
            }
        }
        for (int i = 1; i <= onionSkinNextCount; ++i) {
            int nextIdx = currentFrame + i;
            if (nextIdx >= 0 && nextIdx < static_cast<int>(frames.size())) {
                float fadeOpac = onionSkinNextOpacity * (1.0f - (static_cast<float>(i) - 1.0f) / static_cast<float>(onionSkinNextCount));
                float tint = std::clamp(fadeOpac / 255.0f, 0.f, 1.f);
                for (size_t li = 0; li < frames[nextIdx].layers.size(); ++li) {
                    const auto& layer = frames[nextIdx].layers[li];
                    if (!layer.visible) continue;

                    sf::Sprite onionSpr(layer.texture->getTexture());
                    onionSpr.setColor(sf::Color(100, 255, 100, static_cast<sf::Uint8>(fadeOpac)));
                    sf::RenderStates oStates = innerStates;
                    oStates.blendMode = getSFMLBlendMode(layer.blendMode).blendMode;
                    window.draw(onionSpr, oStates);

                    if (!isPixelMode) {
                        for (const auto& vs : m_vectorStrokes) {
                            if (vs.frame != nextIdx || vs.layer != static_cast<int>(li) || vs.isErase) continue;
                            sf::VertexArray ghost = meshWithOpacity(vs.mesh, tint);
                            for (std::size_t v = 0; v < ghost.getVertexCount(); ++v) {
                                ghost[v].color.r = 100; ghost[v].color.g = 255; ghost[v].color.b = 100;
                            }
                            window.draw(ghost, oStates);
                        }
                    }
                }
            }
        }
    }

    if (currentFrame >= 0 && currentFrame < static_cast<int>(frames.size())) {
        for (size_t i = 0; i < frames[currentFrame].layers.size(); ++i) {
            const auto& layer = frames[currentFrame].layers[i];
            if (!layer.visible) continue;

            sf::RenderStates layerStates = innerStates;
            layerStates.blendMode = getSFMLBlendMode(layer.blendMode).blendMode;

            bool isActive = (static_cast<int>(i) == activeLayer);
            drawLayerContent(window, currentFrame, static_cast<int>(i), layerStates, isActive);

            if (isActive) {
                bool needsComposite = !isPixelMode && (layerHasErase(currentFrame, static_cast<int>(i)) ||
                    (m_isVectorStrokeActive && m_activeStrokeIsErase));

                if (!isPixelMode && !m_floatingVectorStrokes.empty() &&
                    selection.getState() == SelectionState::Floating && !needsComposite) {
                    sf::RenderStates fs = layerStates;
                    fs.transform *= selection.getFloatingTransform();
                    for (const auto& vs : m_floatingVectorStrokes) {
                        window.draw(vs.mesh, fs);
                    }
                }

                if (selection.getState() == SelectionState::Floating) {
                    selection.drawPixels(window, layerStates);
                }
            }

            if (m_textManager) {
                m_textManager->render(window, currentFrame, static_cast<int>(i), isPixelMode, layerStates, canvasLogicalSize);
            }
        }
    }

    if (activeTool == ToolType::Curve && isDeforming && isPixelMode) {
        sf::VertexArray va(sf::Quads);
        for (const auto& dp : currentDeformedPixels) {
            float fx = static_cast<float>(dp.x);
            float fy = static_cast<float>(dp.y);
            va.append(sf::Vertex(sf::Vector2f(fx, fy), dp.color));
            va.append(sf::Vertex(sf::Vector2f(fx + 1.0f, fy), dp.color));
            va.append(sf::Vertex(sf::Vector2f(fx + 1.0f, fy + 1.0f), dp.color));
            va.append(sf::Vertex(sf::Vector2f(fx, fy + 1.0f), dp.color));
        }
        window.draw(va, innerStates);
    }

    if (activeTool == ToolType::FilledContour && isDrawing && m_contourPoints.size() >= 2) {
        sf::VertexArray contourVtx(sf::LineStrip);
        sf::Color previewCol = primaryColor;
        for (const auto& pt : m_contourPoints) {
            contourVtx.append(sf::Vertex(pt, previewCol));
        }
        contourVtx.append(sf::Vertex(m_contourPoints.front(), sf::Color(previewCol.r, previewCol.g, previewCol.b, 140)));
        window.draw(contourVtx, innerStates);
    }

    float worldPerLogicalPixel = drawArea.width / static_cast<float>(canvasLogicalSize.x);
    float handleDenom = std::max(0.0001f, worldPerLogicalPixel * viewScale);
    selection.setHandleVisualSize(8.0f / handleDenom);
    selection.setShowHandles(pendingTransform);

    const float frameThickness = 16.f;
    sf::Color frameColor(45, 35, 25);
    sf::Color shadowColor(20, 15, 10);

    if (hasFrameAssets) {
        float cx = 0.f;
        float cy = 0.f;
        float cw = std::round(drawArea.width);
        float ch = std::round(drawArea.height);

        float tH = static_cast<float>(frameTex[1].getSize().y);
        float trW = static_cast<float>(frameTex[2].getSize().x);
        float lW = static_cast<float>(frameTex[3].getSize().x);
        float rW = static_cast<float>(frameTex[4].getSize().x);
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

    if (customGridEnabled && customGridSize >= 1) {
        sf::VertexArray lines(sf::Lines);
        sf::Color gridLine = customGridColor;
        unsigned int step = static_cast<unsigned int>(customGridSize);
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

    selection.draw(window, innerStates);

    sf::Vector2i mousePosI = sf::Mouse::getPosition(window);
    sf::Vector2f currentRawMousePos = window.mapPixelToCoords(mousePosI);

    sf::Vector2f logicalPos = getInverseTransform().transformPoint(currentRawMousePos);
    bool currentlyHovering = drawArea.contains(logicalPos);

    if (!isPlaying && currentlyHovering && (activeTool == ToolType::Brush || activeTool == ToolType::Pencil || activeTool == ToolType::Eraser || activeTool == ToolType::Curve || activeTool == ToolType::FilledContour)) {
        if (isPixelMode) {
            float sX = static_cast<float>(canvasLogicalSize.x) / drawArea.width;
            float sY = static_cast<float>(canvasLogicalSize.y) / drawArea.height;

            sf::Vector2f lp = getInverseTransform().transformPoint(currentRawMousePos);
            lp.x = (lp.x - drawArea.left) * sX;
            lp.y = (lp.y - drawArea.top) * sY;

            float tx = std::floor(lp.x) - std::floor(static_cast<float>(pixelBrushSize) / 2.0f);
            float ty = std::floor(lp.y) - std::floor(static_cast<float>(pixelBrushSize) / 2.0f);

            sf::RectangleShape pxHover(sf::Vector2f(static_cast<float>(pixelBrushSize), static_cast<float>(pixelBrushSize)));
            pxHover.setFillColor(sf::Color(20, 10, 30, 40));
            pxHover.setOutlineThickness(0.2f);
            pxHover.setOutlineColor(sf::Color(20, 10, 30, 220));
            pxHover.setPosition(tx, ty);

            window.draw(pxHover, innerStates);
        }
        else {
            float brushDiameter = brushEngine.getActivePreset().size;
            float worldPerLogical = drawArea.width / static_cast<float>(canvasLogicalSize.x);
            float screenRadius = (brushDiameter * 0.5f) * worldPerLogical * viewScale;
            screenRadius = std::max(2.0f, screenRadius);

            sf::CircleShape outerRing(screenRadius);
            outerRing.setOrigin(screenRadius, screenRadius);
            outerRing.setPosition(currentRawMousePos);
            outerRing.setFillColor(sf::Color::Transparent);
            outerRing.setOutlineThickness(1.5f);
            outerRing.setOutlineColor(sf::Color(15, 10, 25, 230));
            window.draw(outerRing);

            if (screenRadius > 2.5f) {
                sf::CircleShape innerRing(screenRadius - 1.0f);
                innerRing.setOrigin(screenRadius - 1.0f, screenRadius - 1.0f);
                innerRing.setPosition(currentRawMousePos);
                innerRing.setFillColor(sf::Color::Transparent);
                innerRing.setOutlineThickness(1.0f);
                innerRing.setOutlineColor(sf::Color(255, 255, 255, 230));
                window.draw(innerRing);
            }

            sf::CircleShape dot(1.5f);
            dot.setOrigin(1.5f, 1.5f);
            dot.setPosition(currentRawMousePos);
            dot.setFillColor(sf::Color(255, 215, 60));
            dot.setOutlineThickness(1.0f);
            dot.setOutlineColor(sf::Color(15, 10, 25));
            window.draw(dot);
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

void Canvas::addVectorMesh(const sf::VertexArray& mesh, int frame, int layer) {
    if (mesh.getVertexCount() == 0) return;
    VectorStroke vs;
    vs.mesh = mesh;
    vs.layer = layer;
    vs.frame = frame;
    vs.isErase = false;
    m_vectorStrokes.push_back(std::move(vs));
    isDirty = true;
}

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

void Canvas::setPixelMode(bool enabled) {
    if (enabled && !isPixelMode && !m_vectorStrokes.empty()) {
        for (size_t f = 0; f < frames.size(); ++f) {
            for (size_t l = 0; l < frames[f].layers.size(); ++l) {
                bakeLayerStrokes(static_cast<int>(f), static_cast<int>(l));
            }
        }
        m_vectorStrokes.clear();
        m_floatingVectorStrokes.clear();
        m_activeVectorMesh.clear();
        m_isVectorStrokeActive = false;
        m_activeStrokeIsErase = false;
    }

    isPixelMode = enabled;

    for (auto& frame : frames) {
        for (auto& layer : frame.layers) {
            if (layer.texture) {
                layer.texture->setSmooth(!isPixelMode);
            }
        }
    }
}

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
        extractFloatingStrokes(currentFrame);
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

    if (!isPixelMode) {
        int foundStrokeIdx = -1;
        int foundLayerIdx = -1;
        const float hitRadiusSq = 12.0f * 12.0f;

        for (int l = static_cast<int>(frames[currentFrame].layers.size()) - 1; l >= 0; --l) {
            if (!frames[currentFrame].layers[l].visible || frames[currentFrame].layers[l].locked) continue;

            for (int s = static_cast<int>(m_vectorStrokes.size()) - 1; s >= 0; --s) {
                const auto& vs = m_vectorStrokes[s];
                if (vs.frame == currentFrame && vs.layer == l && !vs.isErase) {
                    for (size_t v = 0; v < vs.mesh.getVertexCount(); ++v) {
                        float dx = vs.mesh[v].position.x - pos.x;
                        float dy = vs.mesh[v].position.y - pos.y;
                        if (dx * dx + dy * dy <= hitRadiusSq) {
                            foundStrokeIdx = s;
                            foundLayerIdx = l;
                            break;
                        }
                    }
                }
                if (foundStrokeIdx != -1) break;
            }
            if (foundStrokeIdx != -1) break;
        }

        if (foundStrokeIdx != -1) {
            activeLayer = foundLayerIdx;
            const auto& vs = m_vectorStrokes[foundStrokeIdx];

            float minX = vs.mesh[0].position.x, maxX = minX;
            float minY = vs.mesh[0].position.y, maxY = minY;
            for (size_t v = 1; v < vs.mesh.getVertexCount(); ++v) {
                minX = std::min(minX, vs.mesh[v].position.x);
                maxX = std::max(maxX, vs.mesh[v].position.x);
                minY = std::min(minY, vs.mesh[v].position.y);
                maxY = std::max(maxY, vs.mesh[v].position.y);
            }

            const float pad = 4.0f;
            selection.startLasso(sf::Vector2f(minX - pad, minY - pad), canvasLogicalSize);
            selection.addLassoPoint(sf::Vector2f(maxX + pad, minY - pad), canvasLogicalSize);
            selection.addLassoPoint(sf::Vector2f(maxX + pad, maxY + pad), canvasLogicalSize);
            selection.addLassoPoint(sf::Vector2f(minX - pad, maxY + pad), canvasLogicalSize);
            selection.endLasso();
            return;
        }
    }

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

        minX = std::min(minX, p.x);
        maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y);
        maxY = std::max(maxY, p.y);

        const int dx8[8] = { 1, -1, 0, 0, 1, 1, -1, -1 };
        const int dy8[8] = { 0, 0, 1, -1, 1, -1, 1, -1 };

        for (int i = 0; i < 8; ++i) {
            int nx = p.x + dx8[i];
            int ny = p.y + dy8[i];
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