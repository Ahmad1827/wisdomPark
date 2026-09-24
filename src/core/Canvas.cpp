#include "Canvas.h"
#include <cmath>
#include <cmath>
#include <algorithm>
#include <iostream>
#include <stack>
#include <queue>
#include <filesystem>
#include <map>
#include "TextSystem.h"

static const sf::RenderWindow* g_activeWindow = nullptr;
static sf::Image s_globalClipboardImage;
static bool s_hasGlobalClipboard = false;
static std::vector<VectorStroke> s_globalClipboardVectorStrokes;
static bool s_hasGlobalVectorClipboard = false;
static sf::Vector2f s_globalVectorClipboardOrigin;

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

static sf::FloatRect getStrokeBounds(const VectorStroke& vs) {
    if (vs.mesh.getVertexCount() == 0) return sf::FloatRect();
    float minX = vs.mesh[0].position.x, maxX = minX;
    float minY = vs.mesh[0].position.y, maxY = minY;
    for (size_t i = 1; i < vs.mesh.getVertexCount(); ++i) {
        minX = std::min(minX, vs.mesh[i].position.x);
        maxX = std::max(maxX, vs.mesh[i].position.x);
        minY = std::min(minY, vs.mesh[i].position.y);
        maxY = std::max(maxY, vs.mesh[i].position.y);
    }
    return sf::FloatRect(minX, minY, maxX - minX, maxY - minY);
}

static bool strokeHitTest(const VectorStroke& vs, sf::Vector2f pt, float hitDist) {
    float rSq = hitDist * hitDist;
    for (size_t i = 0; i < vs.mesh.getVertexCount(); ++i) {
        float dx = vs.mesh[i].position.x - pt.x;
        float dy = vs.mesh[i].position.y - pt.y;
        if (dx * dx + dy * dy <= rSq) return true;
    }
    return false;
}

static float getStrokeMinDistanceSq(const VectorStroke& vs, sf::Vector2f pt) {
    float bestSq = 1e9f;
    for (size_t i = 0; i < vs.mesh.getVertexCount(); ++i) {
        float dx = vs.mesh[i].position.x - pt.x;
        float dy = vs.mesh[i].position.y - pt.y;
        float dSq = dx * dx + dy * dy;
        if (dSq < bestSq) bestSq = dSq;
    }
    return bestSq;
}

static bool strokesCollide(const VectorStroke& s1, const VectorStroke& s2, float unused = 0.f) {
    sf::FloatRect b1 = getStrokeBounds(s1);
    sf::FloatRect b2 = getStrokeBounds(s2);
    if (!b1.intersects(b2)) return false;

    float ox1 = std::max(b1.left, b2.left);
    float oy1 = std::max(b1.top, b2.top);
    float ox2 = std::min(b1.left + b1.width, b2.left + b2.width);
    float oy2 = std::min(b1.top + b1.height, b2.top + b2.height);
    sf::FloatRect overlap(ox1, oy1, ox2 - ox1, oy2 - oy1);

    struct Tri {
        sf::Vector2f a, b, c;
        sf::FloatRect box;
    };

    std::vector<Tri> t1;
    for (size_t i = 0; i + 2 < s1.mesh.getVertexCount(); i += 3) {
        sf::Vector2f pa = s1.mesh[i].position;
        sf::Vector2f pb = s1.mesh[i + 1].position;
        sf::Vector2f pc = s1.mesh[i + 2].position;
        float minX = std::min({ pa.x, pb.x, pc.x });
        float maxX = std::max({ pa.x, pb.x, pc.x });
        float minY = std::min({ pa.y, pb.y, pc.y });
        float maxY = std::max({ pa.y, pb.y, pc.y });
        sf::FloatRect tb(minX, minY, std::max(0.5f, maxX - minX), std::max(0.5f, maxY - minY));
        if (tb.intersects(overlap)) {
            t1.push_back({ pa, pb, pc, tb });
        }
    }
    if (t1.empty()) return false;

    std::vector<Tri> t2;
    for (size_t i = 0; i + 2 < s2.mesh.getVertexCount(); i += 3) {
        sf::Vector2f pa = s2.mesh[i].position;
        sf::Vector2f pb = s2.mesh[i + 1].position;
        sf::Vector2f pc = s2.mesh[i + 2].position;
        float minX = std::min({ pa.x, pb.x, pc.x });
        float maxX = std::max({ pa.x, pb.x, pc.x });
        float minY = std::min({ pa.y, pb.y, pc.y });
        float maxY = std::max({ pa.y, pb.y, pc.y });
        sf::FloatRect tb(minX, minY, std::max(0.5f, maxX - minX), std::max(0.5f, maxY - minY));
        if (tb.intersects(overlap)) {
            t2.push_back({ pa, pb, pc, tb });
        }
    }
    if (t2.empty()) return false;

    auto segIntersect = [](sf::Vector2f p1, sf::Vector2f p2, sf::Vector2f p3, sf::Vector2f p4) -> bool {
        auto ccw = [](sf::Vector2f a, sf::Vector2f b, sf::Vector2f c) {
            return (c.y - a.y) * (b.x - a.x) > (b.y - a.y) * (c.x - a.x);
            };
        return (ccw(p1, p2, p3) != ccw(p1, p2, p4)) && (ccw(p3, p4, p1) != ccw(p3, p4, p2));
        };

    auto ptInTri = [](sf::Vector2f p, sf::Vector2f a, sf::Vector2f b, sf::Vector2f c) -> bool {
        float d1 = (p.x - b.x) * (a.y - b.y) - (a.x - b.x) * (p.y - b.y);
        float d2 = (p.x - c.x) * (b.y - c.y) - (b.x - c.x) * (p.y - c.y);
        float d3 = (p.x - a.x) * (c.y - a.y) - (c.x - a.x) * (p.y - a.y);
        bool has_neg = (d1 < 0.f) || (d2 < 0.f) || (d3 < 0.f);
        bool has_pos = (d1 > 0.f) || (d2 > 0.f) || (d3 > 0.f);
        return !(has_neg && has_pos);
        };

    for (const auto& tri1 : t1) {
        for (const auto& tri2 : t2) {
            if (!tri1.box.intersects(tri2.box)) continue;

            if (segIntersect(tri1.a, tri1.b, tri2.a, tri2.b)) return true;
            if (segIntersect(tri1.a, tri1.b, tri2.b, tri2.c)) return true;
            if (segIntersect(tri1.a, tri1.b, tri2.c, tri2.a)) return true;

            if (segIntersect(tri1.b, tri1.c, tri2.a, tri2.b)) return true;
            if (segIntersect(tri1.b, tri1.c, tri2.b, tri2.c)) return true;
            if (segIntersect(tri1.b, tri1.c, tri2.c, tri2.a)) return true;

            if (segIntersect(tri1.c, tri1.a, tri2.a, tri2.b)) return true;
            if (segIntersect(tri1.c, tri1.a, tri2.b, tri2.c)) return true;
            if (segIntersect(tri1.c, tri1.a, tri2.c, tri2.a)) return true;

            if (ptInTri(tri1.a, tri2.a, tri2.b, tri2.c)) return true;
            if (ptInTri(tri2.a, tri1.a, tri1.b, tri1.c)) return true;
        }
    }

    return false;
}

static bool strokeImageCollide(const VectorStroke& vs, const sf::FloatRect& imgBounds, float unused = 0.f) {
    if (!getStrokeBounds(vs).intersects(imgBounds)) return false;
    for (size_t i = 0; i < vs.mesh.getVertexCount(); ++i) {
        if (imgBounds.contains(vs.mesh[i].position)) return true;
    }
    return false;
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
isPixelMode(false), pixelBrushSize(1), m_pixelBrushShape(PixelBrushShape::Square), pixelGridEnabled(true), pixelSnapEnabled(true), tileModeX(false), tileModeY(false), pixelPerfectEnabled(false), isDirty(false),
transformMode(TransformState::None), pendingTransform(false), currentRotation(0.0f), currentScale(1.0f, 1.0f), hasFrameAssets(false) {
    brushEngine.initDefaultPresets();
    rebuildPixelBrushMask();
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
        if (isPixelMode) {
            static sf::RenderTexture s_pixelComposite;
            if (s_pixelComposite.getSize() != canvasLogicalSize) {
                s_pixelComposite.create(canvasLogicalSize.x, canvasLogicalSize.y);
                s_pixelComposite.setSmooth(false);
            }

            s_pixelComposite.clear(sf::Color::Transparent);

            if (layer.texture) {
                sf::Sprite base(layer.texture->getTexture());
                s_pixelComposite.draw(base);
            }

            for (const auto& cImg : m_canvasImages) {
                if (cImg.frame == frameIndex && cImg.layer == layerIndex && cImg.texture &&
                    cImg.texture->getSize().x > 0 && cImg.texture->getSize().y > 0) {
                    sf::Sprite spr(*cImg.texture);
                    spr.setPosition(std::round(cImg.bounds.left), std::round(cImg.bounds.top));
                    float sx = std::round(cImg.bounds.width) / static_cast<float>(cImg.texture->getSize().x);
                    float sy = std::round(cImg.bounds.height) / static_cast<float>(cImg.texture->getSize().y);
                    spr.setScale(sx, sy);
                    s_pixelComposite.draw(spr);
                }
            }

            s_pixelComposite.display();

            sf::Sprite out(s_pixelComposite.getTexture());
            out.setColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(255.0f * op)));
            target.draw(out, layerStates);
            return;
        }

        if (layer.texture) {
            sf::Sprite spr(layer.texture->getTexture());
            spr.setColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(255.0f * op)));
            target.draw(spr, layerStates);
        }

        for (const auto& cImg : m_canvasImages) {
            if (cImg.frame == frameIndex && cImg.layer == layerIndex && cImg.texture &&
                cImg.texture->getSize().x > 0 && cImg.texture->getSize().y > 0) {
                sf::Sprite spr(*cImg.texture);
                spr.setPosition(cImg.bounds.left, cImg.bounds.top);
                spr.setScale(cImg.bounds.width / static_cast<float>(cImg.texture->getSize().x),
                    cImg.bounds.height / static_cast<float>(cImg.texture->getSize().y));
                spr.setColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(255.f * op)));
                target.draw(spr, layerStates);
            }
        }

        for (const auto& vs : m_vectorStrokes) {
            if (vs.frame != frameIndex || vs.layer != layerIndex) continue;
            if (op < 0.999f) target.draw(meshWithOpacity(vs.mesh, op), layerStates);
            else target.draw(vs.mesh, layerStates);
        }
        if (isActiveLayerForPreview && m_isVectorStrokeActive && m_activeVectorMesh.getVertexCount() > 0) {
            target.draw(m_activeVectorMesh, layerStates);
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

    for (const auto& cImg : m_canvasImages) {
        if (cImg.frame == frameIndex && cImg.layer == layerIndex && cImg.texture) {
            sf::Sprite spr(*cImg.texture);
            spr.setPosition(cImg.bounds.left, cImg.bounds.top);
            spr.setScale(cImg.bounds.width / static_cast<float>(cImg.texture->getSize().x),
                cImg.bounds.height / static_cast<float>(cImg.texture->getSize().y));
            spr.setColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(255.f * op)));
            target.draw(spr, layerStates);
        }
    }

    if (isActiveLayerForPreview && selection.getState() == SelectionState::Floating && !m_floatingImages.empty()) {
        sf::RenderStates imgStates = layerStates;
        imgStates.transform *= selection.getFloatingTransform();
        for (const auto& cImg : m_floatingImages) {
            if (cImg.texture) {
                sf::Sprite spr(*cImg.texture);
                spr.setPosition(cImg.bounds.left, cImg.bounds.top);
                spr.setScale(cImg.bounds.width / static_cast<float>(cImg.texture->getSize().x),
                    cImg.bounds.height / static_cast<float>(cImg.texture->getSize().y));
                spr.setColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(255.f * op)));
                target.draw(spr, imgStates);
            }
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

    sf::RenderTexture* targetTex = frames[frameIndex].layers[layerIndex].texture.get();
    if (!targetTex) return;

    bool hasStrokes = false;
    for (const auto& vs : m_vectorStrokes) {
        if (vs.frame == frameIndex && vs.layer == layerIndex) {
            hasStrokes = true;
            sf::RenderStates st = sf::RenderStates::Default;
            st.blendMode = vs.isErase ? eraseBlendMode() : sf::BlendAlpha;
            targetTex->draw(vs.mesh, st);
        }
    }

    if (hasStrokes) {
        targetTex->display();
        for (auto it = m_vectorStrokes.begin(); it != m_vectorStrokes.end(); ) {
            if (it->frame == frameIndex && it->layer == layerIndex) it = m_vectorStrokes.erase(it);
            else ++it;
        }
        isDirty = true;
    }
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
    canvasSprite.setOrigin(0.f, 0.f);
    canvasSprite.setPosition(
        std::floor(1920.f / 2.f - targetWidth / 2.f),
        std::floor(deskSprite.getPosition().y - (379.f / 2.f) - targetHeight + 120.f)
    );

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

    m_canvasImages.clear();
    m_floatingImages.clear();
    m_selectedStrokes.clear();
    m_selectedImages.clear();
    m_isMultiSelectionGroup = false;

    undoHistory.clear();
    redoHistory.clear();
    selection.clearSelection();
    clearSymmetry();
    transformMode = TransformState::None;
    pendingTransform = false;
    isDrawing = false;
    isDeforming = false;
    activeTool = ToolType::Brush;

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
    m_canvasImages.clear();
    m_floatingImages.clear();
    m_selectedStrokes.clear();
    m_selectedImages.clear();
    m_isMultiSelectionGroup = false;
    selection.clearSelection();
    transformMode = TransformState::None;
    pendingTransform = false;
    isDrawing = false;
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

    // Retain independent object entities so moving one next to another doesn't merge them

    if (selection.getState() == SelectionState::Floating) {
        saveUndoState();

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

        for (auto& img : m_floatingImages) {
            sf::Vector2f p1 = t.transformPoint(img.bounds.left, img.bounds.top);
            sf::Vector2f p2 = t.transformPoint(img.bounds.left + img.bounds.width, img.bounds.top + img.bounds.height);
            img.bounds = sf::FloatRect(std::min(p1.x, p2.x), std::min(p1.y, p2.y), std::abs(p2.x - p1.x), std::abs(p2.y - p1.y));
            img.frame = currentFrame;
            img.layer = activeLayer;
            m_canvasImages.push_back(std::move(img));
        }
        m_floatingImages.clear();

        selection.commitToLayer(frames[currentFrame].layers[activeLayer].texture.get());
    }

    clearObjectSelection();
    transformMode = TransformState::None;
    pendingTransform = false;
}

void Canvas::copySelection(int currentFrame) {
    if (frames.empty() || currentFrame < 0 || currentFrame >= static_cast<int>(frames.size())) return;
    if (!selection.isActive()) return;

    sf::FloatRect bb = selection.getBoundingBox();

    if (!isPixelMode) {
        s_globalClipboardVectorStrokes.clear();
        s_globalVectorClipboardOrigin = sf::Vector2f(bb.left, bb.top);

        for (const auto& vs : m_vectorStrokes) {
            if (vs.frame != currentFrame || vs.layer != activeLayer) continue;

            VectorStroke copied;
            copied.layer = activeLayer;
            copied.frame = currentFrame;
            copied.isErase = vs.isErase;
            copied.mesh.setPrimitiveType(sf::Triangles);

            for (size_t v = 0; v + 2 < vs.mesh.getVertexCount(); v += 3) {
                sf::Vector2f centroid = (vs.mesh[v].position + vs.mesh[v + 1].position + vs.mesh[v + 2].position) / 3.0f;
                if (selection.isPointInsideSelection(centroid)) {
                    for (int k = 0; k < 3; ++k) {
                        sf::Vertex vert = vs.mesh[v + k];
                        vert.position -= s_globalVectorClipboardOrigin;
                        copied.mesh.append(vert);
                    }
                }
            }

            if (copied.mesh.getVertexCount() > 0) {
                s_globalClipboardVectorStrokes.push_back(std::move(copied));
            }
        }

        if (!s_globalClipboardVectorStrokes.empty()) {
            s_hasGlobalVectorClipboard = true;
        }
    }

    sf::RenderTexture tmp;
    sf::Image layerImg;
    if (!isPixelMode) {
        if (renderLayerToTexture(currentFrame, activeLayer, tmp)) {
            selection.copy(&tmp);
            layerImg = tmp.getTexture().copyToImage();
        }
    }
    else {
        auto tex = frames[currentFrame].layers[activeLayer].texture;
        if (tex) {
            selection.copy(tex.get());
            layerImg = tex->getTexture().copyToImage();
        }
    }

    if (layerImg.getSize().x > 0) {
        int minX = std::max(0, static_cast<int>(std::floor(bb.left)));
        int minY = std::max(0, static_cast<int>(std::floor(bb.top)));
        int maxX = std::min(static_cast<int>(canvasLogicalSize.x), static_cast<int>(std::ceil(bb.left + bb.width)));
        int maxY = std::min(static_cast<int>(canvasLogicalSize.y), static_cast<int>(std::ceil(bb.top + bb.height)));

        int w = maxX - minX;
        int h = maxY - minY;
        if (w > 0 && h > 0) {
            sf::Image clipImg;
            clipImg.create(w, h, sf::Color::Transparent);
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    int srcX = minX + x;
                    int srcY = minY + y;
                    if (selection.isPointInsideSelection(sf::Vector2f(static_cast<float>(srcX), static_cast<float>(srcY)))) {
                        clipImg.setPixel(x, y, layerImg.getPixel(srcX, srcY));
                    }
                }
            }
            s_globalClipboardImage = clipImg;
            s_hasGlobalClipboard = true;
            std::filesystem::create_directories("assets");
            s_globalClipboardImage.saveToFile("assets/clipboard_cache.png");
        }
    }
}

bool Canvas::hasGlobalClipboard() const {
    if (s_hasGlobalClipboard && s_globalClipboardImage.getSize().x > 0) return true;
    if (std::filesystem::exists("assets/clipboard_cache.png")) {
        if (s_globalClipboardImage.loadFromFile("assets/clipboard_cache.png")) {
            s_hasGlobalClipboard = true;
            return true;
        }
    }
    return false;
}

const sf::Image& Canvas::getGlobalClipboardImage() const {
    return s_globalClipboardImage;
}

bool Canvas::isClipboardVector() const {
    return !isPixelMode && s_hasGlobalVectorClipboard && !s_globalClipboardVectorStrokes.empty();
}

void Canvas::pasteVectorStrokes(const std::vector<VectorStroke>& strokes, sf::Vector2f offset, int currentFrame) {
    if (strokes.empty()) return;
    if (currentFrame < 0 || currentFrame >= static_cast<int>(frames.size())) return;

    saveUndoState();

    float minX = 99999.f, maxX = -99999.f, minY = 99999.f, maxY = -99999.f;

    for (const auto& vs : strokes) {
        VectorStroke placed;
        placed.layer = activeLayer;
        placed.frame = currentFrame;
        placed.isErase = vs.isErase;
        placed.mesh.setPrimitiveType(sf::Triangles);

        for (size_t v = 0; v < vs.mesh.getVertexCount(); ++v) {
            sf::Vertex vert = vs.mesh[v];
            vert.position += offset;
            placed.mesh.append(vert);

            minX = std::min(minX, vert.position.x);
            maxX = std::max(maxX, vert.position.x);
            minY = std::min(minY, vert.position.y);
            maxY = std::max(maxY, vert.position.y);
        }

        m_vectorStrokes.push_back(std::move(placed));
    }

    commitSelection(currentFrame);
    if (minX <= maxX && minY <= maxY) {
        const float pad = 4.0f;
        selection.startLasso(sf::Vector2f(minX - pad, minY - pad), canvasLogicalSize);
        selection.addLassoPoint(sf::Vector2f(maxX + pad, minY - pad), canvasLogicalSize);
        selection.addLassoPoint(sf::Vector2f(maxX + pad, maxY + pad), canvasLogicalSize);
        selection.addLassoPoint(sf::Vector2f(minX - pad, maxY + pad), canvasLogicalSize);
        selection.endLasso();
    }

    isDirty = true;
}

bool Canvas::pasteGlobalClipboard(int currentFrame) {
    if (!isPixelMode && s_hasGlobalVectorClipboard && !s_globalClipboardVectorStrokes.empty()) {
        float minX = 99999.f, maxX = -99999.f, minY = 99999.f, maxY = -99999.f;
        for (const auto& vs : s_globalClipboardVectorStrokes) {
            for (size_t i = 0; i < vs.mesh.getVertexCount(); ++i) {
                minX = std::min(minX, vs.mesh[i].position.x);
                maxX = std::max(maxX, vs.mesh[i].position.x);
                minY = std::min(minY, vs.mesh[i].position.y);
                maxY = std::max(maxY, vs.mesh[i].position.y);
            }
        }
        float w = maxX - minX;
        float h = maxY - minY;

        sf::Vector2f targetPos(
            std::floor((static_cast<float>(canvasLogicalSize.x) - w) * 0.5f),
            std::floor((static_cast<float>(canvasLogicalSize.y) - h) * 0.5f)
        );

        pasteVectorStrokes(s_globalClipboardVectorStrokes, targetPos, currentFrame);
        return true;
    }

    if (!hasGlobalClipboard()) return false;
    pasteImage(s_globalClipboardImage, currentFrame, false);
    return true;
}

void Canvas::resizeCanvas(unsigned int newWidth, unsigned int newHeight) {
    if (newWidth == 0 || newHeight == 0) return;
    if (newWidth == canvasLogicalSize.x && newHeight == canvasLogicalSize.y) return;

    saveUndoState();

    canvasLogicalSize = sf::Vector2u(newWidth, newHeight);

    sf::ContextSettings ctx;
    ctx.antialiasingLevel = isPixelMode ? 0 : 8;

    std::map<sf::RenderTexture*, std::shared_ptr<sf::RenderTexture>> replaced;

    for (auto& frame : frames) {
        for (auto& layer : frame.layers) {
            if (!layer.texture) continue;

            auto it = replaced.find(layer.texture.get());
            if (it != replaced.end()) {
                layer.texture = it->second;
            }
            else {
                auto oldTex = layer.texture;
                sf::Image oldImg = oldTex->getTexture().copyToImage();

                auto newTex = std::make_shared<sf::RenderTexture>();
                if (!newTex->create(newWidth, newHeight, ctx)) {
                    newTex->create(newWidth, newHeight);
                }
                newTex->setSmooth(!isPixelMode);
                newTex->clear(sf::Color::Transparent);

                sf::Texture backup;
                backup.setSmooth(false);
                backup.loadFromImage(oldImg);

                sf::Sprite spr(backup);
                spr.setPosition(0.f, 0.f);

                newTex->setView(sf::View(sf::FloatRect(0.f, 0.f, static_cast<float>(newWidth), static_cast<float>(newHeight))));
                newTex->draw(spr, sf::RenderStates(sf::BlendNone));
                newTex->display();

                replaced[oldTex.get()] = newTex;
                layer.texture = newTex;
            }
        }
    }

    float maxViewportHeight = 700.f;
    float maxViewportWidth = 1400.f;

    float aspectCanvas = static_cast<float>(canvasLogicalSize.x) / static_cast<float>(canvasLogicalSize.y);

    float targetHeight = maxViewportHeight;
    float targetWidth = targetHeight * aspectCanvas;

    if (targetWidth > maxViewportWidth) {
        targetWidth = maxViewportWidth;
        targetHeight = targetWidth / aspectCanvas;
    }

    if (canvasSprite.getLocalBounds().width > 0.f && canvasSprite.getLocalBounds().height > 0.f) {
        canvasSprite.setScale(targetWidth / canvasSprite.getLocalBounds().width, targetHeight / canvasSprite.getLocalBounds().height);
        canvasSprite.setOrigin(0.f, 0.f);
        canvasSprite.setPosition(
            std::floor(1920.f / 2.f - targetWidth / 2.f),
            std::floor(deskSprite.getPosition().y - (379.f / 2.f) - targetHeight + 120.f)
        );
    }

    drawArea = canvasSprite.getGlobalBounds();
    selection.clearSelection();
    isDirty = true;
}


void Canvas::pasteSelection(int currentFrame) {
    commitSelection(currentFrame);
    saveUndoState();

    addLayer(currentFrame, "Pasted Object");

    selection.paste(canvasLogicalSize);
    setActiveTool(ToolType::Select);
}

void Canvas::deleteSelection(int currentFrame) {
    if (!selection.isActive()) return;

    saveUndoState();

    int cw = static_cast<int>(canvasLogicalSize.x);
    int ch = static_cast<int>(canvasLogicalSize.y);
    sf::FloatRect selBox = selection.getBoundingBox();

    if (!isPixelMode) {
        if (!m_selectedStrokes.empty()) {
            std::sort(m_selectedStrokes.rbegin(), m_selectedStrokes.rend());
            for (int idx : m_selectedStrokes) {
                if (idx >= 0 && idx < static_cast<int>(m_vectorStrokes.size())) {
                    m_vectorStrokes.erase(m_vectorStrokes.begin() + idx);
                }
            }
            m_selectedStrokes.clear();
        }

        if (selection.isMagicWandStyle() && !selection.getMagicWandPixels().empty()) {
            const auto& wandPts = selection.getMagicWandPixels();
            std::vector<bool> wandMask(cw * ch, false);
            for (const auto& pt : wandPts) {
                if (pt.x >= 0 && pt.y >= 0 && pt.x < cw && pt.y < ch) {
                    wandMask[pt.y * cw + pt.x] = true;
                }
            }

            for (auto it = m_vectorStrokes.begin(); it != m_vectorStrokes.end(); ) {
                if (it->frame == currentFrame && it->layer == activeLayer && !it->isErase) {
                    if (getStrokeBounds(*it).intersects(selBox)) {
                        int insideCount = 0;
                        for (size_t v = 0; v < it->mesh.getVertexCount(); ++v) {
                            int vx = static_cast<int>(std::floor(it->mesh[v].position.x));
                            int vy = static_cast<int>(std::floor(it->mesh[v].position.y));
                            if (vx >= 0 && vy >= 0 && vx < cw && vy < ch && wandMask[vy * cw + vx]) {
                                insideCount++;
                            }
                        }
                        if (insideCount > static_cast<int>(it->mesh.getVertexCount()) * 0.4f) {
                            it = m_vectorStrokes.erase(it);
                            continue;
                        }
                    }
                }
                ++it;
            }
        }

        clearObjectSelection();
        selection.clearSelection();
        isDirty = true;
        return;
    }

    std::vector<bool> wandMask;
    bool isWand = selection.isMagicWandStyle() && !selection.getMagicWandPixels().empty();
    if (isWand) {
        wandMask.assign(cw * ch, false);
        for (const auto& pt : selection.getMagicWandPixels()) {
            if (pt.x >= 0 && pt.y >= 0 && pt.x < cw && pt.y < ch) {
                wandMask[pt.y * cw + pt.x] = true;
            }
        }
    }

    auto isInside = [&](float x, float y) -> bool {
        if (isWand) {
            int ix = static_cast<int>(std::floor(x));
            int iy = static_cast<int>(std::floor(y));
            if (ix >= 0 && iy >= 0 && ix < cw && iy < ch) {
                return wandMask[iy * cw + ix];
            }
            return false;
        }
        return selection.isPointInsideSelection(sf::Vector2f(x, y));
        };

    std::sort(m_selectedImages.rbegin(), m_selectedImages.rend());
    for (int idx : m_selectedImages) {
        if (idx >= 0 && idx < static_cast<int>(m_canvasImages.size())) {
            m_canvasImages.erase(m_canvasImages.begin() + idx);
        }
    }
    m_selectedImages.clear();

    for (auto it = m_canvasImages.begin(); it != m_canvasImages.end(); ) {
        if (it->frame == currentFrame && it->layer == activeLayer && it->texture) {
            if (it->bounds.intersects(selBox)) {
                sf::Image img = it->texture->copyToImage();
                int iw = static_cast<int>(img.getSize().x);
                int ih = static_cast<int>(img.getSize().y);
                int bx = static_cast<int>(std::round(it->bounds.left));
                int by = static_cast<int>(std::round(it->bounds.top));
                bool modified = false;
                int remainingAlpha = 0;

                for (int ly = 0; ly < ih; ++ly) {
                    for (int lx = 0; lx < iw; ++lx) {
                        if (isInside(static_cast<float>(bx + lx) + 0.5f, static_cast<float>(by + ly) + 0.5f)) {
                            if (img.getPixel(lx, ly).a > 0) {
                                img.setPixel(lx, ly, sf::Color::Transparent);
                                modified = true;
                            }
                        }
                        else if (img.getPixel(lx, ly).a > 0) {
                            remainingAlpha++;
                        }
                    }
                }
                if (modified) {
                    if (remainingAlpha == 0) {
                        it = m_canvasImages.erase(it);
                        continue;
                    }
                    else {
                        auto newTex = std::make_shared<sf::Texture>();
                        newTex->setSmooth(!isPixelMode);
                        newTex->loadFromImage(img);
                        it->texture = newTex;
                        it->image = std::move(img);
                    }
                }
            }
        }
        ++it;
    }

    if (currentFrame >= 0 && currentFrame < static_cast<int>(frames.size())) {
        sf::RenderTexture* targetTex = frames[currentFrame].layers[activeLayer].texture.get();
        if (targetTex) {
            sf::Image img = targetTex->getTexture().copyToImage();
            int x0 = std::max(0, static_cast<int>(std::floor(selBox.left)));
            int y0 = std::max(0, static_cast<int>(std::floor(selBox.top)));
            int x1 = std::min(cw, static_cast<int>(std::ceil(selBox.left + selBox.width)));
            int y1 = std::min(ch, static_cast<int>(std::ceil(selBox.top + selBox.height)));

            bool mod = false;
            for (int y = y0; y < y1; ++y) {
                for (int x = x0; x < x1; ++x) {
                    if (isInside(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f)) {
                        if (img.getPixel(x, y).a > 0) {
                            img.setPixel(x, y, sf::Color::Transparent);
                            mod = true;
                        }
                    }
                }
            }
            if (mod) {
                sf::Texture newTex;
                newTex.loadFromImage(img);
                targetTex->clear(sf::Color::Transparent);
                targetTex->draw(sf::Sprite(newTex), sf::RenderStates(sf::BlendNone));
                targetTex->display();
            }
        }
    }

    clearObjectSelection();
    selection.clearSelection();
    isDirty = true;
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

    if (!selection.isActive()) return;

    if (color == sf::Color::Transparent || color.a == 0) {
        sf::RenderTexture* targetTex = frames[currentFrame].layers[activeLayer].texture.get();
        if (targetTex) {
            sf::Image img = targetTex->getTexture().copyToImage();
            sf::FloatRect bb = selection.getBoundingBox();
            int x0 = std::max(0, static_cast<int>(std::floor(bb.left)));
            int y0 = std::max(0, static_cast<int>(std::floor(bb.top)));
            int x1 = std::min(static_cast<int>(canvasLogicalSize.x), static_cast<int>(std::ceil(bb.left + bb.width)));
            int y1 = std::min(static_cast<int>(canvasLogicalSize.y), static_cast<int>(std::ceil(bb.top + bb.height)));

            for (int y = y0; y < y1; ++y) {
                for (int x = x0; x < x1; ++x) {
                    if (selection.isPointInsideSelection(sf::Vector2f(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f))) {
                        img.setPixel(x, y, sf::Color::Transparent);
                    }
                }
            }
            sf::Texture newTex; newTex.loadFromImage(img);
            targetTex->clear(sf::Color::Transparent);
            targetTex->draw(sf::Sprite(newTex), sf::RenderStates(sf::BlendNone));
            targetTex->display();
        }

        for (auto& ci : m_canvasImages) {
            if (ci.frame == currentFrame && ci.layer == activeLayer && ci.texture) {
                sf::Image img = ci.texture->copyToImage();
                int iw = static_cast<int>(img.getSize().x);
                int ih = static_cast<int>(img.getSize().y);
                int bx = static_cast<int>(std::round(ci.bounds.left));
                int by = static_cast<int>(std::round(ci.bounds.top));
                bool mod = false;

                for (int ly = 0; ly < ih; ++ly) {
                    for (int lx = 0; lx < iw; ++lx) {
                        if (selection.isPointInsideSelection(sf::Vector2f(static_cast<float>(bx + lx) + 0.5f, static_cast<float>(by + ly) + 0.5f))) {
                            img.setPixel(lx, ly, sf::Color::Transparent);
                            mod = true;
                        }
                    }
                }
                if (mod) {
                    auto newTex = std::make_shared<sf::Texture>();
                    newTex->setSmooth(!isPixelMode);
                    newTex->loadFromImage(img);
                    ci.texture = newTex;
                }
            }
        }
        isDirty = true;
        return;
    }

    recolorActiveSelection(color);
}

void Canvas::flipSelectionHorizontal(int currentFrame) {
    if (!selection.isActive()) return;
    saveUndoState();

    sf::FloatRect box = selection.getBoundingBox();
    float midX = box.left + box.width * 0.5f;

    for (int sIdx : m_selectedStrokes) {
        if (sIdx >= 0 && sIdx < static_cast<int>(m_vectorStrokes.size())) {
            auto& mesh = m_vectorStrokes[sIdx].mesh;
            for (size_t v = 0; v < mesh.getVertexCount(); ++v) {
                mesh[v].position.x = 2.0f * midX - mesh[v].position.x;
            }
        }
    }

    for (int iIdx : m_selectedImages) {
        if (iIdx >= 0 && iIdx < static_cast<int>(m_canvasImages.size())) {
            auto& b = m_canvasImages[iIdx].bounds;
            b.left = isPixelMode ? std::round(2.0f * midX - (b.left + b.width)) : (2.0f * midX - (b.left + b.width));
            if (m_canvasImages[iIdx].texture) {
                sf::Image img = m_canvasImages[iIdx].texture->copyToImage();
                img.flipHorizontally();
                auto newTex = std::make_shared<sf::Texture>();
                newTex->setSmooth(!isPixelMode);
                newTex->loadFromImage(img);
                m_canvasImages[iIdx].texture = newTex;
            }
        }
    }

    std::vector<sf::FloatRect> updatedSub;
    for (int sIdx : m_selectedStrokes) updatedSub.push_back(getStrokeBounds(m_vectorStrokes[sIdx]));
    for (int iIdx : m_selectedImages) updatedSub.push_back(m_canvasImages[iIdx].bounds);

    selection.flipPathHorizontal(midX);
    selection.setSubItemBoxes(updatedSub);
    isDirty = true;
}

void Canvas::flipSelectionVertical(int currentFrame) {
    if (!selection.isActive()) return;
    saveUndoState();

    sf::FloatRect box = selection.getBoundingBox();
    float midY = box.top + box.height * 0.5f;

    for (int sIdx : m_selectedStrokes) {
        if (sIdx >= 0 && sIdx < static_cast<int>(m_vectorStrokes.size())) {
            auto& mesh = m_vectorStrokes[sIdx].mesh;
            for (size_t v = 0; v < mesh.getVertexCount(); ++v) {
                mesh[v].position.y = 2.0f * midY - mesh[v].position.y;
            }
        }
    }

    for (int iIdx : m_selectedImages) {
        if (iIdx >= 0 && iIdx < static_cast<int>(m_canvasImages.size())) {
            auto& b = m_canvasImages[iIdx].bounds;
            b.top = isPixelMode ? std::round(2.0f * midY - (b.top + b.height)) : (2.0f * midY - (b.top + b.height));
            if (m_canvasImages[iIdx].texture) {
                sf::Image img = m_canvasImages[iIdx].texture->copyToImage();
                img.flipVertically();
                auto newTex = std::make_shared<sf::Texture>();
                newTex->setSmooth(!isPixelMode);
                newTex->loadFromImage(img);
                m_canvasImages[iIdx].texture = newTex;
            }
        }
    }

    std::vector<sf::FloatRect> updatedSub;
    for (int sIdx : m_selectedStrokes) updatedSub.push_back(getStrokeBounds(m_vectorStrokes[sIdx]));
    for (int iIdx : m_selectedImages) updatedSub.push_back(m_canvasImages[iIdx].bounds);

    selection.flipPathVertical(midY);
    selection.setSubItemBoxes(updatedSub);
    isDirty = true;
}
void Canvas::mergeSelectedObjects(int currentFrame) {
    if (frames.empty() || currentFrame < 0 || currentFrame >= static_cast<int>(frames.size())) return;
    if (!selection.isActive()) return;

    saveUndoState();

    if (isPixelMode) {
        if (m_selectedImages.size() <= 1) return;

        float minX = 999999.f, minY = 999999.f, maxX = -999999.f, maxY = -999999.f;
        for (int idx : m_selectedImages) {
            if (idx >= 0 && idx < static_cast<int>(m_canvasImages.size())) {
                const auto& b = m_canvasImages[idx].bounds;
                minX = std::min(minX, b.left);
                minY = std::min(minY, b.top);
                maxX = std::max(maxX, b.left + b.width);
                maxY = std::max(maxY, b.top + b.height);
            }
        }

        int masterX = static_cast<int>(std::floor(minX));
        int masterY = static_cast<int>(std::floor(minY));
        int masterW = static_cast<int>(std::ceil(maxX - minX));
        int masterH = static_cast<int>(std::ceil(maxY - minY));

        if (masterW <= 0 || masterH <= 0) return;

        sf::Image mergedImg;
        mergedImg.create(masterW, masterH, sf::Color::Transparent);

        std::vector<int> sortedIndices = m_selectedImages;
        std::sort(sortedIndices.begin(), sortedIndices.end());

        for (int idx : sortedIndices) {
            if (idx >= 0 && idx < static_cast<int>(m_canvasImages.size())) {
                const auto& ci = m_canvasImages[idx];
                if (!ci.texture) continue;
                sf::Image img = ci.texture->copyToImage();
                int iw = static_cast<int>(img.getSize().x);
                int ih = static_cast<int>(img.getSize().y);
                int bx = static_cast<int>(std::round(ci.bounds.left));
                int by = static_cast<int>(std::round(ci.bounds.top));

                for (int y = 0; y < ih; ++y) {
                    for (int x = 0; x < iw; ++x) {
                        sf::Color c = img.getPixel(x, y);
                        if (c.a > 0) {
                            int destX = (bx + x) - masterX;
                            int destY = (by + y) - masterY;
                            if (destX >= 0 && destX < masterW && destY >= 0 && destY < masterH) {
                                sf::Color dst = mergedImg.getPixel(destX, destY);
                                if (dst.a == 0) {
                                    mergedImg.setPixel(destX, destY, c);
                                }
                                else {
                                    float alpha = c.a / 255.f;
                                    float inv = 1.f - alpha;
                                    sf::Uint8 r = static_cast<sf::Uint8>(c.r * alpha + dst.r * inv);
                                    sf::Uint8 g = static_cast<sf::Uint8>(c.g * alpha + dst.g * inv);
                                    sf::Uint8 b = static_cast<sf::Uint8>(c.b * alpha + dst.b * inv);
                                    sf::Uint8 a = static_cast<sf::Uint8>(std::min(255.f, c.a + dst.a * inv));
                                    mergedImg.setPixel(destX, destY, sf::Color(r, g, b, a));
                                }
                            }
                        }
                    }
                }
            }
        }

        std::sort(sortedIndices.rbegin(), sortedIndices.rend());
        for (int idx : sortedIndices) {
            if (idx >= 0 && idx < static_cast<int>(m_canvasImages.size())) {
                m_canvasImages.erase(m_canvasImages.begin() + idx);
            }
        }

        CanvasImage newCi;
        newCi.id = ++m_nextImageId;
        newCi.frame = currentFrame;
        newCi.layer = activeLayer;
        newCi.bounds = sf::FloatRect(static_cast<float>(masterX), static_cast<float>(masterY),
            static_cast<float>(masterW), static_cast<float>(masterH));
        newCi.texture = std::make_shared<sf::Texture>();
        newCi.texture->setSmooth(false);
        newCi.texture->loadFromImage(mergedImg);

        m_canvasImages.push_back(std::move(newCi));
        int newIdx = static_cast<int>(m_canvasImages.size()) - 1;

        m_selectedImages = { newIdx };
        m_selectedStrokes.clear();
        m_isMultiSelectionGroup = false;

        sf::FloatRect newBox(static_cast<float>(masterX), static_cast<float>(masterY),
            static_cast<float>(masterW), static_cast<float>(masterH));
        selection.setSelectionBoxes(newBox, { newBox });
        selection.setShowHandles(pendingTransform);
        isDirty = true;
    }
    else {
        if (m_selectedStrokes.size() <= 1) return;

        std::vector<int> sortedIndices = m_selectedStrokes;
        std::sort(sortedIndices.begin(), sortedIndices.end());

        VectorStroke mergedVs;
        mergedVs.layer = activeLayer;
        mergedVs.frame = currentFrame;
        mergedVs.isErase = false;
        mergedVs.mesh.setPrimitiveType(sf::Triangles);

        for (int idx : sortedIndices) {
            if (idx >= 0 && idx < static_cast<int>(m_vectorStrokes.size())) {
                const auto& vs = m_vectorStrokes[idx];
                for (size_t v = 0; v < vs.mesh.getVertexCount(); ++v) {
                    mergedVs.mesh.append(vs.mesh[v]);
                }
            }
        }

        std::sort(sortedIndices.rbegin(), sortedIndices.rend());
        for (int idx : sortedIndices) {
            if (idx >= 0 && idx < static_cast<int>(m_vectorStrokes.size())) {
                m_vectorStrokes.erase(m_vectorStrokes.begin() + idx);
            }
        }

        m_vectorStrokes.push_back(std::move(mergedVs));
        int newIdx = static_cast<int>(m_vectorStrokes.size()) - 1;

        m_selectedStrokes = { newIdx };
        m_selectedImages.clear();
        m_isMultiSelectionGroup = false;

        sf::FloatRect newBox = getStrokeBounds(m_vectorStrokes[newIdx]);
        selection.setSelectionBoxes(newBox, { newBox });
        selection.setShowHandles(pendingTransform);
        isDirty = true;
    }
}

void Canvas::duplicateSelection(int currentFrame) {
    if (!selection.isActive()) return;
    if (m_selectedStrokes.empty() && m_selectedImages.empty()) return;

    saveUndoState();

    sf::Vector2f offset = isPixelMode ? sf::Vector2f(2.f, 2.f) : sf::Vector2f(20.f, 20.f);
    std::vector<int> newStrokeIndices;
    std::vector<int> newImageIndices;

    for (int sIdx : m_selectedStrokes) {
        if (sIdx >= 0 && sIdx < static_cast<int>(m_vectorStrokes.size())) {
            VectorStroke clone = m_vectorStrokes[sIdx];
            for (size_t v = 0; v < clone.mesh.getVertexCount(); ++v) {
                clone.mesh[v].position += offset;
            }
            newStrokeIndices.push_back(static_cast<int>(m_vectorStrokes.size()));
            m_vectorStrokes.push_back(clone);
        }
    }

    for (int iIdx : m_selectedImages) {
        if (iIdx >= 0 && iIdx < static_cast<int>(m_canvasImages.size())) {
            CanvasImage clone = m_canvasImages[iIdx];
            clone.id = ++m_nextImageId;
            clone.bounds.left += offset.x;
            clone.bounds.top += offset.y;
            newImageIndices.push_back(static_cast<int>(m_canvasImages.size()));
            m_canvasImages.push_back(clone);
        }
    }

    m_selectedStrokes = newStrokeIndices;
    m_selectedImages = newImageIndices;

    std::vector<sf::FloatRect> newSub;
    sf::FloatRect masterBox;
    bool first = true;
    for (int sIdx : m_selectedStrokes) {
        sf::FloatRect b = getStrokeBounds(m_vectorStrokes[sIdx]);
        newSub.push_back(b);
        if (first) { masterBox = b; first = false; }
        else {
            float minX = std::min(masterBox.left, b.left);
            float minY = std::min(masterBox.top, b.top);
            float maxX = std::max(masterBox.left + masterBox.width, b.left + b.width);
            float maxY = std::max(masterBox.top + masterBox.height, b.top + b.height);
            masterBox = sf::FloatRect(minX, minY, maxX - minX, maxY - minY);
        }
    }
    for (int iIdx : m_selectedImages) {
        sf::FloatRect b = m_canvasImages[iIdx].bounds;
        newSub.push_back(b);
        if (first) { masterBox = b; first = false; }
        else {
            float minX = std::min(masterBox.left, b.left);
            float minY = std::min(masterBox.top, b.top);
            float maxX = std::max(masterBox.left + masterBox.width, b.left + b.width);
            float maxY = std::max(masterBox.top + masterBox.height, b.top + b.height);
            masterBox = sf::FloatRect(minX, minY, maxX - minX, maxY - minY);
        }
    }

    selection.moveSelection(offset);
    selection.setSubItemBoxes(newSub);
    selection.setBoundingBox(masterBox);
    selection.setShowHandles(pendingTransform);
    isDirty = true;
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

void Canvas::moveSelectionZOrder(int delta, int currentFrame) {
    setSelectionZOrder(getSelectionZOrder(currentFrame) + delta, currentFrame);
}

void Canvas::bringSelectionToFront(int currentFrame) {
    setSelectionZOrder(getMaxZOrder(currentFrame), currentFrame);
}

void Canvas::sendSelectionToBack(int currentFrame) {
    setSelectionZOrder(0, currentFrame);
}

int Canvas::getSelectionZOrder(int currentFrame) const {
    if (!selection.isActive()) return 0;
    if (isPixelMode) {
        if (m_selectedImages.empty()) return 0;
        int targetIdx = m_selectedImages[0];
        int rank = 0;
        for (size_t i = 0; i < m_canvasImages.size(); ++i) {
            if (m_canvasImages[i].frame == currentFrame && m_canvasImages[i].layer == activeLayer) {
                if (static_cast<int>(i) == targetIdx) return rank;
                rank++;
            }
        }
        return 0;
    }
    else {
        if (m_selectedStrokes.empty()) return 0;
        int targetIdx = m_selectedStrokes[0];
        int rank = 0;
        for (size_t i = 0; i < m_vectorStrokes.size(); ++i) {
            if (m_vectorStrokes[i].frame == currentFrame && m_vectorStrokes[i].layer == activeLayer) {
                if (static_cast<int>(i) == targetIdx) return rank;
                rank++;
            }
        }
        return 0;
    }
}

int Canvas::getMaxZOrder(int currentFrame) const {
    int count = 0;
    if (isPixelMode) {
        for (const auto& ci : m_canvasImages) {
            if (ci.frame == currentFrame && ci.layer == activeLayer) count++;
        }
    }
    else {
        for (const auto& vs : m_vectorStrokes) {
            if (vs.frame == currentFrame && vs.layer == activeLayer) count++;
        }
    }
    return std::max(0, count - 1);
}

void Canvas::setSelectionZOrder(int newZ, int currentFrame) {
    if (!selection.isActive()) return;
    saveUndoState();

    if (isPixelMode) {
        if (m_selectedImages.empty()) return;
        std::vector<bool> isSel(m_canvasImages.size(), false);
        for (int idx : m_selectedImages) {
            if (idx >= 0 && idx < static_cast<int>(m_canvasImages.size())) isSel[idx] = true;
        }

        std::vector<CanvasImage> selectedImgs;
        std::vector<CanvasImage> otherImgsOnLayer;
        std::vector<CanvasImage> remainingAll;

        for (size_t i = 0; i < m_canvasImages.size(); ++i) {
            if (m_canvasImages[i].frame == currentFrame && m_canvasImages[i].layer == activeLayer) {
                if (isSel[i]) selectedImgs.push_back(std::move(m_canvasImages[i]));
                else otherImgsOnLayer.push_back(std::move(m_canvasImages[i]));
            }
            else {
                remainingAll.push_back(std::move(m_canvasImages[i]));
            }
        }

        int targetZ = std::clamp(newZ, 0, static_cast<int>(otherImgsOnLayer.size()));
        otherImgsOnLayer.insert(otherImgsOnLayer.begin() + static_cast<std::ptrdiff_t>(targetZ),
            std::make_move_iterator(selectedImgs.begin()),
            std::make_move_iterator(selectedImgs.end()));

        m_canvasImages = std::move(remainingAll);
        std::vector<int> newSelectedIndices;
        for (size_t i = 0; i < otherImgsOnLayer.size(); ++i) {
            int newIdx = static_cast<int>(m_canvasImages.size());
            if (static_cast<int>(i) >= targetZ && static_cast<int>(i) < targetZ + static_cast<int>(selectedImgs.size())) {
                newSelectedIndices.push_back(newIdx);
            }
            m_canvasImages.push_back(std::move(otherImgsOnLayer[i]));
        }
        m_selectedImages = newSelectedIndices;
    }
    else {
        if (m_selectedStrokes.empty()) return;
        std::vector<bool> isSel(m_vectorStrokes.size(), false);
        for (int idx : m_selectedStrokes) {
            if (idx >= 0 && idx < static_cast<int>(m_vectorStrokes.size())) isSel[idx] = true;
        }

        std::vector<VectorStroke> selectedStr;
        std::vector<VectorStroke> otherStrOnLayer;
        std::vector<VectorStroke> remainingAll;

        for (size_t i = 0; i < m_vectorStrokes.size(); ++i) {
            if (m_vectorStrokes[i].frame == currentFrame && m_vectorStrokes[i].layer == activeLayer) {
                if (isSel[i]) selectedStr.push_back(std::move(m_vectorStrokes[i]));
                else otherStrOnLayer.push_back(std::move(m_vectorStrokes[i]));
            }
            else {
                remainingAll.push_back(std::move(m_vectorStrokes[i]));
            }
        }

        int targetZ = std::clamp(newZ, 0, static_cast<int>(otherStrOnLayer.size()));
        otherStrOnLayer.insert(otherStrOnLayer.begin() + static_cast<std::ptrdiff_t>(targetZ),
            std::make_move_iterator(selectedStr.begin()),
            std::make_move_iterator(selectedStr.end()));

        m_vectorStrokes = std::move(remainingAll);
        std::vector<int> newSelectedIndices;
        for (size_t i = 0; i < otherStrOnLayer.size(); ++i) {
            int newIdx = static_cast<int>(m_vectorStrokes.size());
            if (static_cast<int>(i) >= targetZ && static_cast<int>(i) < targetZ + static_cast<int>(selectedStr.size())) {
                newSelectedIndices.push_back(newIdx);
            }
            m_vectorStrokes.push_back(std::move(otherStrOnLayer[i]));
        }
        m_selectedStrokes = newSelectedIndices;
    }

    isDirty = true;
}

void Canvas::recolorActiveSelection(sf::Color newColor) {
    if (!selection.isActive()) return;

    if (!m_recolorUndoSaved) {
        saveUndoState();
        m_recolorUndoSaved = true;
    }

    int curFrame = std::clamp(m_currentFrame, 0, static_cast<int>(frames.size()) - 1);
    int cw = static_cast<int>(canvasLogicalSize.x);
    int ch = static_cast<int>(canvasLogicalSize.y);
    sf::FloatRect selBox = selection.getBoundingBox();

    if (!isPixelMode) {
        if (!m_selectedStrokes.empty()) {
            for (int sIdx : m_selectedStrokes) {
                if (sIdx >= 0 && sIdx < static_cast<int>(m_vectorStrokes.size())) {
                    auto& vs = m_vectorStrokes[sIdx];
                    if (!vs.isErase) {
                        for (size_t v = 0; v < vs.mesh.getVertexCount(); ++v) {
                            vs.mesh[v].color = newColor;
                        }
                    }
                }
            }
            isDirty = true;
            return;
        }

        if (selection.isMagicWandStyle() && !selection.getMagicWandPixels().empty()) {
            const auto& wandPts = selection.getMagicWandPixels();
            std::vector<bool> wandMask(cw * ch, false);
            for (const auto& pt : wandPts) {
                if (pt.x >= 0 && pt.y >= 0 && pt.x < cw && pt.y < ch) {
                    wandMask[pt.y * cw + pt.x] = true;
                }
            }

            bool recoloredExisting = false;
            for (auto& vs : m_vectorStrokes) {
                if (vs.frame == curFrame && vs.layer == activeLayer && !vs.isErase) {
                    if (getStrokeBounds(vs).intersects(selBox)) {
                        int insideCount = 0;
                        for (size_t v = 0; v < vs.mesh.getVertexCount(); ++v) {
                            int vx = static_cast<int>(std::floor(vs.mesh[v].position.x));
                            int vy = static_cast<int>(std::floor(vs.mesh[v].position.y));
                            if (vx >= 0 && vy >= 0 && vx < cw && vy < ch && wandMask[vy * cw + vx]) {
                                insideCount++;
                            }
                        }
                        if (insideCount > static_cast<int>(vs.mesh.getVertexCount()) * 0.4f) {
                            for (size_t v = 0; v < vs.mesh.getVertexCount(); ++v) {
                                vs.mesh[v].color = newColor;
                            }
                            recoloredExisting = true;
                        }
                    }
                }
            }

            if (!recoloredExisting && newColor.a > 0) {
                VectorStroke vs;
                vs.mesh.setPrimitiveType(sf::Triangles);
                vs.layer = activeLayer;
                vs.frame = curFrame;
                vs.isErase = false;

                int x0 = std::max(0, static_cast<int>(std::floor(selBox.left)));
                int y0 = std::max(0, static_cast<int>(std::floor(selBox.top)));
                int x1 = std::min(cw, static_cast<int>(std::ceil(selBox.left + selBox.width)));
                int y1 = std::min(ch, static_cast<int>(std::ceil(selBox.top + selBox.height)));

                for (int y = y0; y < y1; ++y) {
                    int x = x0;
                    while (x < x1) {
                        if (wandMask[y * cw + x]) {
                            int xStart = x;
                            while (x < x1 && wandMask[y * cw + x]) x++;
                            int xEnd = x;

                            float fx0 = static_cast<float>(xStart);
                            float fx1 = static_cast<float>(xEnd);
                            float fy0 = static_cast<float>(y);
                            float fy1 = static_cast<float>(y + 1);

                            vs.mesh.append(sf::Vertex(sf::Vector2f(fx0, fy0), newColor));
                            vs.mesh.append(sf::Vertex(sf::Vector2f(fx1, fy0), newColor));
                            vs.mesh.append(sf::Vertex(sf::Vector2f(fx1, fy1), newColor));

                            vs.mesh.append(sf::Vertex(sf::Vector2f(fx0, fy0), newColor));
                            vs.mesh.append(sf::Vertex(sf::Vector2f(fx1, fy1), newColor));
                            vs.mesh.append(sf::Vertex(sf::Vector2f(fx0, fy1), newColor));
                        }
                        else {
                            x++;
                        }
                    }
                }

                if (vs.mesh.getVertexCount() > 0) {
                    auto insertPos = m_vectorStrokes.end();
                    for (auto it = m_vectorStrokes.begin(); it != m_vectorStrokes.end(); ++it) {
                        if (it->frame == curFrame && it->layer == activeLayer) {
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
    }

    std::vector<bool> wandMask;
    bool isWand = selection.isMagicWandStyle() && !selection.getMagicWandPixels().empty();
    if (isWand) {
        wandMask.assign(cw * ch, false);
        for (const auto& pt : selection.getMagicWandPixels()) {
            if (pt.x >= 0 && pt.y >= 0 && pt.x < cw && pt.y < ch) {
                wandMask[pt.y * cw + pt.x] = true;
            }
        }
    }

    auto isInside = [&](float x, float y) -> bool {
        if (isWand) {
            int ix = static_cast<int>(std::floor(x));
            int iy = static_cast<int>(std::floor(y));
            if (ix >= 0 && iy >= 0 && ix < cw && iy < ch) {
                return wandMask[iy * cw + ix];
            }
            return false;
        }
        return selection.isPointInsideSelection(sf::Vector2f(x, y));
        };

    for (auto& ci : m_canvasImages) {
        if (ci.frame == curFrame && ci.layer == activeLayer && ci.texture) {
            if (!ci.bounds.intersects(selBox)) continue;

            sf::Image& img = getCanvasImageCPU(ci);
            int iw = static_cast<int>(img.getSize().x);
            int ih = static_cast<int>(img.getSize().y);
            int bx = static_cast<int>(std::round(ci.bounds.left));
            int by = static_cast<int>(std::round(ci.bounds.top));
            bool imgMod = false;

            int x0 = std::max(0, static_cast<int>(std::floor(selBox.left)) - bx);
            int y0 = std::max(0, static_cast<int>(std::floor(selBox.top)) - by);
            int x1 = std::min(iw, static_cast<int>(std::ceil(selBox.left + selBox.width)) - bx);
            int y1 = std::min(ih, static_cast<int>(std::ceil(selBox.top + selBox.height)) - by);

            for (int ly = y0; ly < y1; ++ly) {
                for (int lx = x0; lx < x1; ++lx) {
                    if (isInside(static_cast<float>(bx + lx) + 0.5f, static_cast<float>(by + ly) + 0.5f)) {
                        sf::Color c = img.getPixel(lx, ly);
                        if (c.a > 0) {
                            sf::Uint8 a = (newColor.a == 255) ? c.a : static_cast<sf::Uint8>((static_cast<int>(c.a) * static_cast<int>(newColor.a)) / 255);
                            img.setPixel(lx, ly, sf::Color(newColor.r, newColor.g, newColor.b, a));
                            imgMod = true;
                        }
                    }
                }
            }

            if (imgMod) {
                ci.texture->update(img);
            }
        }
    }

    sf::RenderTexture* targetTex = frames[curFrame].layers[activeLayer].texture.get();
    if (targetTex) {
        sf::Image img = targetTex->getTexture().copyToImage();
        int x0 = std::max(0, static_cast<int>(std::floor(selBox.left)));
        int y0 = std::max(0, static_cast<int>(std::floor(selBox.top)));
        int x1 = std::min(cw, static_cast<int>(std::ceil(selBox.left + selBox.width)));
        int y1 = std::min(ch, static_cast<int>(std::ceil(selBox.top + selBox.height)));

        bool texMod = false;
        for (int y = y0; y < y1; ++y) {
            for (int x = x0; x < x1; ++x) {
                if (isInside(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f)) {
                    sf::Color c = img.getPixel(x, y);
                    if (c.a > 0) {
                        sf::Uint8 a = (newColor.a == 255) ? c.a : static_cast<sf::Uint8>((static_cast<int>(c.a) * static_cast<int>(newColor.a)) / 255);
                        img.setPixel(x, y, sf::Color(newColor.r, newColor.g, newColor.b, a));
                        texMod = true;
                    }
                }
            }
        }

        if (texMod) {
            sf::Texture newTex;
            newTex.loadFromImage(img);
            targetTex->clear(sf::Color::Transparent);
            targetTex->draw(sf::Sprite(newTex), sf::RenderStates(sf::BlendNone));
            targetTex->display();
        }
    }

    isDirty = true;
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

    if (tool != ToolType::Brush && tool != ToolType::Pencil && tool != ToolType::Eraser) {
        hasShiftAnchor = false;
    }

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

void Canvas::setPrimaryColor(sf::Color color) {
    primaryColor = color;
    if (selection.isActive()) {
        recolorActiveSelection(color);
    }
}
void Canvas::setSecondaryColor(sf::Color color) { secondaryColor = color; }
sf::Color Canvas::getPrimaryColor() const { return primaryColor; }
sf::Color Canvas::getSecondaryColor() const { return secondaryColor; }
void Canvas::setFillSettings(float tolerance, bool contiguous) { fillTolerance = tolerance; fillContiguous = contiguous; }

void Canvas::saveUndoState() {
    isDirty = true;
    UndoState state;
    state.frames = frames;
    state.vectorStrokes = m_vectorStrokes;
    state.canvasImages = m_canvasImages;
    state.selectionState = selection.getState();
    state.selectionBoundingBox = selection.getBoundingBox();
    state.selectionSubItemBoxes = selection.getSubItemBoxes();
    state.selectionPathPoints = selection.getPathPoints();
    state.isLassoSelection = selection.getIsLassoMode();
    state.showHandles = selection.isShowingHandles();
    state.selectedStrokes = m_selectedStrokes;
    state.selectedImages = m_selectedImages;
    state.isMultiSelectionGroup = m_isMultiSelectionGroup;
    state.pendingTransform = pendingTransform;
    state.transformMode = transformMode;
    state.symmetryEnabled = symmetryManager.enabled;
    state.symmetryVisible = symmetryManager.visible;
    state.symmetryStartPoint = symmetryManager.startPoint;
    state.symmetryEndPoint = symmetryManager.endPoint;
    state.isMagicWandStyle = selection.isMagicWandStyle();
    state.magicWandPixels = selection.getMagicWandPixels();
    undoHistory.push_back(state);
    if (undoHistory.size() > maxUndoHistory) {
        undoHistory.erase(undoHistory.begin());
    }
    redoHistory.clear();
}

void Canvas::bakeAllStrokes() {
    if (isPixelMode) {
        for (const auto& ci : m_canvasImages) {
            if (ci.frame >= 0 && ci.frame < static_cast<int>(frames.size())) {
                if (ci.layer >= 0 && ci.layer < static_cast<int>(frames[ci.frame].layers.size())) {
                    auto targetTex = frames[ci.frame].layers[ci.layer].texture.get();
                    if (targetTex && ci.texture) {
                        sf::Sprite spr(*ci.texture);
                        spr.setPosition(std::round(ci.bounds.left), std::round(ci.bounds.top));
                        float sx = std::round(ci.bounds.width) / static_cast<float>(ci.texture->getSize().x);
                        float sy = std::round(ci.bounds.height) / static_cast<float>(ci.texture->getSize().y);
                        spr.setScale(sx, sy);
                        targetTex->draw(spr);
                        targetTex->display();
                    }
                }
            }
        }
        m_canvasImages.clear();
        clearObjectSelection();
    }
    else {
        for (size_t f = 0; f < frames.size(); ++f) {
            for (size_t l = 0; l < frames[f].layers.size(); ++l) {
                bakeLayerStrokes(static_cast<int>(f), static_cast<int>(l));
            }
        }
    }
}

void Canvas::undo() {
    if (!undoHistory.empty()) {
        UndoState currentState;
        currentState.frames = frames;
        currentState.vectorStrokes = m_vectorStrokes;
        currentState.canvasImages = m_canvasImages;
        currentState.selectionState = selection.getState();
        currentState.selectionBoundingBox = selection.getBoundingBox();
        currentState.selectionSubItemBoxes = selection.getSubItemBoxes();
        currentState.selectionPathPoints = selection.getPathPoints();
        currentState.isLassoSelection = selection.getIsLassoMode();
        currentState.showHandles = selection.isShowingHandles();
        currentState.selectedStrokes = m_selectedStrokes;
        currentState.selectedImages = m_selectedImages;
        currentState.isMultiSelectionGroup = m_isMultiSelectionGroup;
        currentState.pendingTransform = pendingTransform;
        currentState.transformMode = transformMode;
        currentState.symmetryEnabled = symmetryManager.enabled;
        currentState.symmetryVisible = symmetryManager.visible;
        currentState.symmetryStartPoint = symmetryManager.startPoint;
        currentState.symmetryEndPoint = symmetryManager.endPoint;
        currentState.isMagicWandStyle = selection.isMagicWandStyle();
        currentState.magicWandPixels = selection.getMagicWandPixels();
        redoHistory.push_back(currentState);

        UndoState prevState = undoHistory.back();
        undoHistory.pop_back();

        frames = prevState.frames;
        m_vectorStrokes = prevState.vectorStrokes;
        m_canvasImages = prevState.canvasImages;
        m_floatingVectorStrokes.clear();

        if (prevState.isMagicWandStyle && !prevState.magicWandPixels.empty()) {
            selection.setPixelSelection(prevState.magicWandPixels, canvasLogicalSize, prevState.selectionSubItemBoxes);
        }
        else {
            selection.setState(prevState.selectionState);
            selection.setBoundingBox(prevState.selectionBoundingBox);
            selection.setSubItemBoxes(prevState.selectionSubItemBoxes);
            selection.setPathPoints(prevState.selectionPathPoints);
            selection.setLassoMode(prevState.isLassoSelection);
            selection.setShowHandles(prevState.showHandles);
            selection.setMagicWandStyle(false);
        }
        m_selectedStrokes = prevState.selectedStrokes;
        m_selectedImages = prevState.selectedImages;
        m_isMultiSelectionGroup = prevState.isMultiSelectionGroup;
        pendingTransform = prevState.pendingTransform;
        transformMode = prevState.transformMode;

        symmetryManager.enabled = prevState.symmetryEnabled;
        symmetryManager.visible = prevState.symmetryVisible;
        symmetryManager.startPoint = prevState.symmetryStartPoint;
        symmetryManager.endPoint = prevState.symmetryEndPoint;
        symmetryManager.updateVectors();

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
        currentState.canvasImages = m_canvasImages;
        currentState.selectionState = selection.getState();
        currentState.selectionBoundingBox = selection.getBoundingBox();
        currentState.selectionSubItemBoxes = selection.getSubItemBoxes();
        currentState.selectionPathPoints = selection.getPathPoints();
        currentState.isLassoSelection = selection.getIsLassoMode();
        currentState.showHandles = selection.isShowingHandles();
        currentState.selectedStrokes = m_selectedStrokes;
        currentState.selectedImages = m_selectedImages;
        currentState.isMultiSelectionGroup = m_isMultiSelectionGroup;
        currentState.pendingTransform = pendingTransform;
        currentState.transformMode = transformMode;
        currentState.symmetryEnabled = symmetryManager.enabled;
        currentState.symmetryVisible = symmetryManager.visible;
        currentState.symmetryStartPoint = symmetryManager.startPoint;
        currentState.symmetryEndPoint = symmetryManager.endPoint;
        currentState.isMagicWandStyle = selection.isMagicWandStyle();
        currentState.magicWandPixels = selection.getMagicWandPixels();
        undoHistory.push_back(currentState);

        UndoState nextState = redoHistory.back();
        redoHistory.pop_back();

        frames = nextState.frames;
        m_vectorStrokes = nextState.vectorStrokes;
        m_canvasImages = nextState.canvasImages;
        m_floatingVectorStrokes.clear();

        if (nextState.isMagicWandStyle && !nextState.magicWandPixels.empty()) {
            selection.setPixelSelection(nextState.magicWandPixels, canvasLogicalSize, nextState.selectionSubItemBoxes);
        }
        else {
            selection.setState(nextState.selectionState);
            selection.setBoundingBox(nextState.selectionBoundingBox);
            selection.setSubItemBoxes(nextState.selectionSubItemBoxes);
            selection.setPathPoints(nextState.selectionPathPoints);
            selection.setLassoMode(nextState.isLassoSelection);
            selection.setShowHandles(nextState.showHandles);
            selection.setMagicWandStyle(false);
        }
        m_selectedStrokes = nextState.selectedStrokes;
        m_selectedImages = nextState.selectedImages;
        m_isMultiSelectionGroup = nextState.isMultiSelectionGroup;
        pendingTransform = nextState.pendingTransform;
        transformMode = nextState.transformMode;

        symmetryManager.enabled = nextState.symmetryEnabled;
        symmetryManager.visible = nextState.symmetryVisible;
        symmetryManager.startPoint = nextState.symmetryStartPoint;
        symmetryManager.endPoint = nextState.symmetryEndPoint;
        symmetryManager.updateVectors();

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

void Canvas::drawPixelExact(int x, int y, sf::Color c, int frameIdx) {
    drawBresenhamLine(x, y, x, y, c, frameIdx);
}

void Canvas::drawBresenhamLine(int x0, int y0, int x1, int y1, sf::Color c, int frameIdx) {
    sf::RenderTexture* target = getActiveRenderTexture(frameIdx);
    if (!target) return;

    auto pts = getBresenhamPoints(x0, y0, x1, y1);
    if (pts.empty()) return;

    int maxCW = static_cast<int>(canvasLogicalSize.x);
    int maxCH = static_cast<int>(canvasLogicalSize.y);
    if (maxCW <= 0 || maxCH <= 0) return;

    static std::vector<int> s_visitedPixels;
    static int s_visitToken = 0;
    s_visitToken++;
    if (s_visitToken <= 0) {
        s_visitToken = 1;
        s_visitedPixels.assign(maxCW * maxCH, 0);
    }
    if (s_visitedPixels.size() != static_cast<size_t>(maxCW * maxCH)) {
        s_visitedPixels.assign(maxCW * maxCH, 0);
    }

    sf::RenderStates states;
    bool isErasing = (activeTool == ToolType::Eraser || c == sf::Color::Transparent);
    if (isErasing) states.blendMode = sf::BlendNone;

    sf::VertexArray va(sf::Quads);

    auto addQuad = [&](float fx, float fy) {
        va.append(sf::Vertex(sf::Vector2f(fx, fy), c));
        va.append(sf::Vertex(sf::Vector2f(fx + 1.0f, fy), c));
        va.append(sf::Vertex(sf::Vector2f(fx + 1.0f, fy + 1.0f), c));
        va.append(sf::Vertex(sf::Vector2f(fx, fy + 1.0f), c));
        };

    std::vector<bool> ciModified(m_canvasImages.size(), false);
    bool anyCiModified = false;

    for (const auto& p : pts) {
        if (useDithering && !ditherManager.shouldDrawPixel(p.x, p.y)) continue;

        auto points = symmetryManager.getSymmetricPoints(sf::Vector2f(static_cast<float>(p.x), static_cast<float>(p.y)));

        for (const auto& pt : points) {
            int baseIX = static_cast<int>(std::round(pt.x));
            int baseIY = static_cast<int>(std::round(pt.y));

            for (const auto& offset : m_pixelBrushMask) {
                int ix = baseIX + offset.x;
                int iy = baseIY + offset.y;

                if (ix < 0 || iy < 0 || ix >= maxCW || iy >= maxCH) continue;

                size_t pIdx = static_cast<size_t>(iy) * maxCW + ix;
                if (s_visitedPixels[pIdx] == s_visitToken) continue;
                s_visitedPixels[pIdx] = s_visitToken;

                if (selection.isActive() && !selection.isPointInsideSelection(sf::Vector2f(static_cast<float>(ix) + 0.5f, static_cast<float>(iy) + 0.5f))) continue;

                if (isDrawing && (activeTool == ToolType::Brush || activeTool == ToolType::Pencil) && !isErasing) {
                    m_pixelStrokePoints.push_back({ ix, iy, c });
                }

                if (isErasing) {
                    for (size_t i = 0; i < m_canvasImages.size(); ++i) {
                        auto& ci = m_canvasImages[i];
                        if (ci.frame == frameIdx && ci.layer == activeLayer && ci.texture) {
                            if (ci.bounds.contains(static_cast<float>(ix), static_cast<float>(iy))) {
                                int lx = ix - static_cast<int>(std::round(ci.bounds.left));
                                int ly = iy - static_cast<int>(std::round(ci.bounds.top));
                                sf::Image& cpuImg = getCanvasImageCPU(ci);
                                if (lx >= 0 && ly >= 0 && lx < static_cast<int>(cpuImg.getSize().x) && ly < static_cast<int>(cpuImg.getSize().y)) {
                                    if (cpuImg.getPixel(lx, ly).a > 0) {
                                        cpuImg.setPixel(lx, ly, sf::Color::Transparent);
                                        ciModified[i] = true;
                                        anyCiModified = true;
                                    }
                                }
                            }
                        }
                    }
                }

                addQuad(static_cast<float>(ix), static_cast<float>(iy));

                if (tileModeX) {
                    addQuad(static_cast<float>(ix - maxCW), static_cast<float>(iy));
                    addQuad(static_cast<float>(ix + maxCW), static_cast<float>(iy));
                }
                if (tileModeY) {
                    addQuad(static_cast<float>(ix), static_cast<float>(iy - maxCH));
                    addQuad(static_cast<float>(ix), static_cast<float>(iy + maxCH));
                }
                if (tileModeX && tileModeY) {
                    addQuad(static_cast<float>(ix - maxCW), static_cast<float>(iy - maxCH));
                    addQuad(static_cast<float>(ix + maxCW), static_cast<float>(iy - maxCH));
                    addQuad(static_cast<float>(ix - maxCW), static_cast<float>(iy + maxCH));
                    addQuad(static_cast<float>(ix + maxCW), static_cast<float>(iy + maxCH));
                }
            }
        }
    }

    if (va.getVertexCount() > 0) {
        target->draw(va, states);
    }

    if (anyCiModified) {
        for (size_t i = 0; i < m_canvasImages.size(); ++i) {
            if (ciModified[i] && m_canvasImages[i].texture) {
                m_canvasImages[i].texture->loadFromImage(m_canvasImages[i].image);
            }
        }
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

    if (symmetryManager.enabled) {
        auto p1 = symmetryManager.getSymmetricPoints(from);
        auto p2 = symmetryManager.getSymmetricPoints(to);
        if (p1.size() > 1 && p2.size() > 1) {
            appendVectorCap(vs.mesh, p1[1], radius, meshCol);
            appendVectorSegment(vs.mesh, p1[1], p2[1], radius, meshCol);
            appendVectorCap(vs.mesh, p2[1], radius, meshCol);
        }
    }

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
    return 24.0f / denom;
}

bool Canvas::isImageResourceActive(int currentFrame) const {
    if (currentFrame < 0 || currentFrame >= static_cast<int>(frames.size())) return false;
    if (activeLayer < 0 || activeLayer >= static_cast<int>(frames[currentFrame].layers.size())) return false;
    return frames[currentFrame].layers[activeLayer].isImageResource;
}

void Canvas::handleMousePressed(sf::Vector2f logicalPos, bool rightClick, int currentFrame) {
    if (currentFrame < 0 || currentFrame >= static_cast<int>(frames.size())) return;
    m_currentFrame = currentFrame;
    if (activeTool == ToolType::None) return;

    float scaleX = static_cast<float>(canvasLogicalSize.x) / drawArea.width;
    float scaleY = static_cast<float>(canvasLogicalSize.y) / drawArea.height;
    sf::Vector2f localPos((logicalPos.x - drawArea.left) * scaleX, (logicalPos.y - drawArea.top) * scaleY);
    localPos.x = std::clamp(localPos.x, 0.0f, static_cast<float>(canvasLogicalSize.x));
    localPos.y = std::clamp(localPos.y, 0.0f, static_cast<float>(canvasLogicalSize.y));

    if (isPixelMode) {
        localPos.x = std::floor(localPos.x);
        localPos.y = std::floor(localPos.y);
    }

    if (rightClick) {
        if ((activeTool == ToolType::Select || activeTool == ToolType::MagicWand) && selection.isActive()) {
            saveUndoState();
            commitSelection(currentFrame);
            clearObjectSelection();
            return;
        }

        if (activeTool == ToolType::Curve && isDeforming) {
            if (!isPixelMode && m_deformStrokeIndex >= 0 && m_deformStrokeIndex < static_cast<int>(m_vectorStrokes.size())) {
                m_vectorStrokes[m_deformStrokeIndex].mesh = m_originalDeformMesh;
            }
            else if (isPixelMode) {
                sf::RenderTexture* targetTex = frames[currentFrame].layers[activeLayer].texture.get();
                if (targetTex) {
                    sf::RenderStates rsNone;
                    rsNone.blendMode = sf::BlendNone;
                    sf::RectangleShape drawPx(sf::Vector2f(1.f, 1.f));
                    for (const auto& dp : deformPixels) {
                        drawPx.setPosition(static_cast<float>(dp.x), static_cast<float>(dp.y));
                        drawPx.setFillColor(dp.color);
                        targetTex->draw(drawPx, rsNone);
                    }
                    targetTex->display();
                }
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
                    auto distToSeg = [](sf::Vector2f pt, sf::Vector2f p1, sf::Vector2f p2) -> float {
                        sf::Vector2f d = p2 - p1;
                        float lenSq = d.x * d.x + d.y * d.y;
                        if (lenSq < 0.0001f) return std::hypot(pt.x - p1.x, pt.y - p1.y);
                        float t = std::clamp(((pt.x - p1.x) * d.x + (pt.y - p1.y) * d.y) / lenSq, 0.0f, 1.0f);
                        sf::Vector2f proj = p1 + t * d;
                        return std::hypot(pt.x - proj.x, pt.y - proj.y);
                        };

                    int hitStrokeIdx = -1;
                    for (int s = static_cast<int>(m_vectorStrokes.size()) - 1; s >= 0; --s) {
                        const auto& vs = m_vectorStrokes[s];
                        if (vs.frame == currentFrame && vs.layer == activeLayer && !vs.isErase) {
                            for (size_t v = 0; v + 2 < vs.mesh.getVertexCount(); v += 3) {
                                sf::Vector2f pa = vs.mesh[v].position;
                                sf::Vector2f pb = vs.mesh[v + 1].position;
                                sf::Vector2f pc = vs.mesh[v + 2].position;

                                if (ptInTri(localPos, pa, pb, pc)) {
                                    hitStrokeIdx = s;
                                    break;
                                }

                                float d1 = distToSeg(localPos, pa, pb);
                                float d2 = distToSeg(localPos, pb, pc);
                                float d3 = distToSeg(localPos, pc, pa);
                                if (std::min({ d1, d2, d3 }) <= 12.0f) {
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

                // Flatten any floating canvas images on this layer into targetTex first
                for (const auto& ci : m_canvasImages) {
                    if (ci.frame == currentFrame && ci.layer == activeLayer && ci.texture) {
                        sf::Sprite spr(*ci.texture);
                        spr.setPosition(std::round(ci.bounds.left), std::round(ci.bounds.top));
                        float sx = std::round(ci.bounds.width) / static_cast<float>(ci.texture->getSize().x);
                        float sy = std::round(ci.bounds.height) / static_cast<float>(ci.texture->getSize().y);
                        spr.setScale(sx, sy);
                        targetTex->draw(spr);
                    }
                }
                targetTex->display();
                m_canvasImages.erase(std::remove_if(m_canvasImages.begin(), m_canvasImages.end(),
                    [&](const CanvasImage& ci) { return ci.frame == currentFrame && ci.layer == activeLayer; }),
                    m_canvasImages.end());

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

                    // 1. Direct hit on the active layer
                    if (sx >= 0 && sy >= 0 && sx < w && sy < h && activeImg.getPixel(sx, sy).a > 0) {
                        targetLayerIndex = activeLayer;
                        targetImg = activeImg;
                    }
                    else {
                        // 2. Proximity search (up to 4px) strictly on the ACTIVE layer first
                        float bestDist = 9999.0f;
                        int foundX = -1, foundY = -1;
                        for (int r = 1; r <= 4; ++r) {
                            for (int dy = -r; dy <= r; ++dy) {
                                for (int dx = -r; dx <= r; ++dx) {
                                    int nx = sx + dx;
                                    int ny = sy + dy;
                                    if (nx >= 0 && ny >= 0 && nx < w && ny < h) {
                                        if (activeImg.getPixel(nx, ny).a > 0) {
                                            float d = static_cast<float>(dx * dx + dy * dy);
                                            if (d < bestDist) {
                                                bestDist = d;
                                                foundX = nx;
                                                foundY = ny;
                                            }
                                        }
                                    }
                                }
                            }
                            if (foundX != -1) break;
                        }

                        if (foundX != -1) {
                            targetLayerIndex = activeLayer;
                            targetImg = activeImg;
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

                saveUndoState();

                deformClickPos = sf::Vector2f(static_cast<float>(sx), static_cast<float>(sy));
                deformCurrentPos = deformClickPos;
                isDeforming = true;
                updateDeformPixels(sf::Vector2f(0.f, 0.f));

                sf::RenderStates rsNone;
                rsNone.blendMode = sf::BlendNone;
                sf::RectangleShape clearPx(sf::Vector2f(1.f, 1.f));
                clearPx.setFillColor(sf::Color::Transparent);
                for (const auto& dp : deformPixels) {
                    clearPx.setPosition(static_cast<float>(dp.x), static_cast<float>(dp.y));
                    targetTex->draw(clearPx, rsNone);
                }
                targetTex->display();
                return;
            }

            if (activeTool == ToolType::FilledContour) {
                m_contourPoints.clear();
                m_contourPoints.push_back(localPos);
                isDrawing = true;
                return;
            }

            if (activeTool == ToolType::Symmetry) {
                saveUndoState();
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
                float handleRad = computeHandleHitRadius();
                if (selection.isActive() && pendingTransform && selection.hitTestHandle(localPos, handleRad) != -1) {
                    saveUndoState();
                    m_resizeStartBox = selection.getBoundingBox();
                    m_resizeImageSnapshots.clear();
                    for (int iIdx : m_selectedImages) {
                        if (iIdx >= 0 && iIdx < static_cast<int>(m_canvasImages.size())) {
                            m_resizeImageSnapshots.push_back({ iIdx, m_canvasImages[iIdx].bounds, m_canvasImages[iIdx].texture });
                        }
                    }
                    m_resizeStrokeSnapshots.clear();
                    for (int sIdx : m_selectedStrokes) {
                        if (sIdx >= 0 && sIdx < static_cast<int>(m_vectorStrokes.size())) {
                            m_resizeStrokeSnapshots.push_back({ sIdx, m_vectorStrokes[sIdx].mesh });
                        }
                    }
                    selection.startResize(localPos, handleRad);
                    return;
                }

                bool isDoubleClick = (m_selectClickClock.getElapsedTime().asMilliseconds() < 350 &&
                    std::hypot(localPos.x - m_lastClickPos.x, localPos.y - m_lastClickPos.y) < 10.0f);
                m_selectClickClock.restart();
                m_lastClickPos = localPos;

                // Double-click isolates a single drawing from the group (both Vector and Pixel modes)
                if (isDoubleClick && selection.isActive() && m_isMultiSelectionGroup) {
                    int hitStroke = -1;
                    for (int sIdx : m_selectedStrokes) {
                        if (sIdx >= 0 && sIdx < static_cast<int>(m_vectorStrokes.size()) && strokeHitTest(m_vectorStrokes[sIdx], localPos, 12.0f)) {
                            hitStroke = sIdx; break;
                        }
                    }
                    int hitImg = -1;
                    for (int iIdx : m_selectedImages) {
                        if (iIdx >= 0 && iIdx < static_cast<int>(m_canvasImages.size()) && m_canvasImages[iIdx].bounds.contains(localPos)) {
                            if (m_canvasImages[iIdx].texture) {
                                sf::Image ciImg = m_canvasImages[iIdx].texture->copyToImage();
                                int lx = static_cast<int>(localPos.x - m_canvasImages[iIdx].bounds.left);
                                int ly = static_cast<int>(localPos.y - m_canvasImages[iIdx].bounds.top);
                                if (lx >= 0 && ly >= 0 && lx < static_cast<int>(ciImg.getSize().x) && ly < static_cast<int>(ciImg.getSize().y)) {
                                    if (ciImg.getPixel(lx, ly).a > 0) {
                                        hitImg = iIdx;
                                        break;
                                    }
                                }
                            }
                            else {
                                hitImg = iIdx;
                                break;
                            }
                        }
                    }

                    if (hitImg != -1) {
                        m_selectedImages = { hitImg };
                        m_selectedStrokes.clear();
                        m_isMultiSelectionGroup = false;
                        sf::FloatRect ib = m_canvasImages[hitImg].bounds;
                        selection.setSelectionBoxes(ib, { ib });
                        selection.setShowHandles(pendingTransform);
                        m_lastDragPos = localPos;
                        selection.startDrag(localPos);
                        return;
                    }
                    else if (hitStroke != -1) {
                        m_selectedStrokes = { hitStroke };
                        m_selectedImages.clear();
                        m_isMultiSelectionGroup = false;
                        sf::FloatRect sb = getStrokeBounds(m_vectorStrokes[hitStroke]);
                        selection.setSelectionBoxes(sb, { sb });
                        selection.setShowHandles(pendingTransform);
                        m_lastDragPos = localPos;
                        selection.startDrag(localPos);
                        return;
                    }
                }

                if (selection.isActive() && selection.isPointInsideSelection(localPos)) {
                    saveUndoState();
                    m_lastDragPos = localPos;
                    selection.startDrag(localPos);
                    return;
                }

                if (selection.isActive()) {
                    saveUndoState();
                    commitSelection(currentFrame);
                }

                bool hitAny = false;
                if (isPixelMode) {
                    // Check if clicked directly on a pixel inside an existing object
                    for (const auto& ci : m_canvasImages) {
                        if (ci.frame == currentFrame && ci.layer == activeLayer && ci.bounds.contains(localPos)) {
                            if (ci.texture) {
                                sf::Image img = ci.texture->copyToImage();
                                int lx = static_cast<int>(localPos.x - ci.bounds.left);
                                int ly = static_cast<int>(localPos.y - ci.bounds.top);
                                if (lx >= 0 && ly >= 0 && lx < static_cast<int>(img.getSize().x) && ly < static_cast<int>(img.getSize().y)) {
                                    if (img.getPixel(lx, ly).a > 0) { hitAny = true; break; }
                                }
                            }
                        }
                    }
                    // Check if clicked directly on a non-transparent pixel on the base canvas
                    if (!hitAny) {
                        sf::RenderTexture* targetTex = frames[currentFrame].layers[activeLayer].texture.get();
                        if (targetTex) {
                            int px = static_cast<int>(localPos.x);
                            int py = static_cast<int>(localPos.y);
                            if (px >= 0 && py >= 0 && px < static_cast<int>(targetTex->getSize().x) && py < static_cast<int>(targetTex->getSize().y)) {
                                sf::Image img = targetTex->getTexture().copyToImage();
                                if (img.getPixel(px, py).a > 0) hitAny = true;
                            }
                        }
                    }
                }
                else {
                    for (const auto& vs : m_vectorStrokes) {
                        if (vs.frame == currentFrame && vs.layer == activeLayer && strokeHitTest(vs, localPos, 12.0f)) {
                            hitAny = true; break;
                        }
                    }
                    if (!hitAny) {
                        for (const auto& ci : m_canvasImages) {
                            if (ci.frame == currentFrame && ci.layer == activeLayer && ci.bounds.contains(localPos)) {
                                hitAny = true; break;
                            }
                        }
                    }
                }

                selection.setMagicWandStyle(false);
                m_dragStartMousePos = localPos;
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
            m_pixelStrokePoints.clear();
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
    m_recolorUndoSaved = false;
    float scaleX = static_cast<float>(canvasLogicalSize.x) / drawArea.width;
    float scaleY = static_cast<float>(canvasLogicalSize.y) / drawArea.height;
    sf::Vector2f localPos((logicalPos.x - drawArea.left) * scaleX, (logicalPos.y - drawArea.top) * scaleY);
    localPos.x = std::clamp(localPos.x, 0.0f, static_cast<float>(canvasLogicalSize.x));
    localPos.y = std::clamp(localPos.y, 0.0f, static_cast<float>(canvasLogicalSize.y));

    if (isPixelMode) {
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

            sf::RenderTexture* targetTex = frames[currentFrame].layers[activeLayer].texture.get();
            if (targetTex) {
                sf::RenderStates rsNone;
                rsNone.blendMode = sf::BlendNone;
                sf::RectangleShape drawPx(sf::Vector2f(1.f, 1.f));

                if (std::abs(delta.x) >= 1.0f || std::abs(delta.y) >= 1.0f) {
                    updateDeformPixels(delta);

                    for (const auto& dp : currentDeformedPixels) {
                        drawPx.setPosition(static_cast<float>(dp.x), static_cast<float>(dp.y));
                        drawPx.setFillColor(dp.color);
                        targetTex->draw(drawPx, rsNone);
                    }
                    isDirty = true;
                }
                else {
                    for (const auto& dp : deformPixels) {
                        drawPx.setPosition(static_cast<float>(dp.x), static_cast<float>(dp.y));
                        drawPx.setFillColor(dp.color);
                        targetTex->draw(drawPx, rsNone);
                    }
                    if (!undoHistory.empty()) {
                        undoHistory.pop_back();
                    }
                }
                targetTex->display();
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
        if (selection.isResizing()) {
            selection.endResize();
            if (isPixelMode) {
                for (const auto& snap : m_resizeImageSnapshots) {
                    if (snap.index >= 0 && snap.index < static_cast<int>(m_canvasImages.size())) {
                        auto& ci = m_canvasImages[snap.index];
                        if (snap.texture) {
                            int newW = static_cast<int>(std::max(1.f, std::round(ci.bounds.width)));
                            int newH = static_cast<int>(std::max(1.f, std::round(ci.bounds.height)));
                            sf::Image src = snap.texture->copyToImage();
                            int srcW = static_cast<int>(src.getSize().x);
                            int srcH = static_cast<int>(src.getSize().y);
                            if (newW != srcW || newH != srcH) {
                                sf::Image dst;
                                dst.create(newW, newH, sf::Color::Transparent);
                                for (int y = 0; y < newH; ++y) {
                                    int sy = std::clamp(static_cast<int>((static_cast<float>(y) + 0.5f) * static_cast<float>(srcH) / static_cast<float>(newH)), 0, srcH - 1);
                                    for (int x = 0; x < newW; ++x) {
                                        int sx = std::clamp(static_cast<int>((static_cast<float>(x) + 0.5f) * static_cast<float>(srcW) / static_cast<float>(newW)), 0, srcW - 1);
                                        dst.setPixel(x, y, src.getPixel(sx, sy));
                                    }
                                }
                                auto nTex = std::make_shared<sf::Texture>();
                                nTex->setSmooth(false);
                                nTex->loadFromImage(dst);
                                ci.texture = nTex;
                                ci.bounds.left = std::round(ci.bounds.left);
                                ci.bounds.top = std::round(ci.bounds.top);
                                ci.bounds.width = static_cast<float>(newW);
                                ci.bounds.height = static_cast<float>(newH);
                            }
                        }
                    }
                }

                std::vector<sf::FloatRect> updatedSub;
                sf::FloatRect masterBox;
                bool first = true;
                for (int sIdx : m_selectedStrokes) {
                    sf::FloatRect b = getStrokeBounds(m_vectorStrokes[sIdx]);
                    updatedSub.push_back(b);
                    if (first) { masterBox = b; first = false; }
                    else {
                        float x0 = std::min(masterBox.left, b.left);
                        float y0 = std::min(masterBox.top, b.top);
                        float x1 = std::max(masterBox.left + masterBox.width, b.left + b.width);
                        float y1 = std::max(masterBox.top + masterBox.height, b.top + b.height);
                        masterBox = sf::FloatRect(x0, y0, x1 - x0, y1 - y0);
                    }
                }
                for (int iIdx : m_selectedImages) {
                    sf::FloatRect b = m_canvasImages[iIdx].bounds;
                    updatedSub.push_back(b);
                    if (first) { masterBox = b; first = false; }
                    else {
                        float x0 = std::min(masterBox.left, b.left);
                        float y0 = std::min(masterBox.top, b.top);
                        float x1 = std::max(masterBox.left + masterBox.width, b.left + b.width);
                        float y1 = std::max(masterBox.top + masterBox.height, b.top + b.height);
                        masterBox = sf::FloatRect(x0, y0, x1 - x0, y1 - y0);
                    }
                }
                if (!updatedSub.empty()) {
                    selection.setBoundingBox(masterBox);
                    selection.setSubItemBoxes(updatedSub);
                }
            }
            m_resizeImageSnapshots.clear();
            m_resizeStrokeSnapshots.clear();
            return;
        }

        if (selection.isDragging()) {
            selection.endDrag();
            return;
        }

        if (selection.getState() == SelectionState::Drawing) {
            selection.endLasso();

            bool isClick = (selection.getState() == SelectionState::Inactive) ||
                (selection.getBoundingBox().width <= 1.5f && selection.getBoundingBox().height <= 1.5f);

            if (isClick) {
                autoSelectObject(localPos, currentFrame);
            }
            else if (selection.getState() == SelectionState::Selected) {
                // In Pixel Mode: extract each drawing inside the selection into an independent object
                // In Pixel Mode: cut through drawings along discrete pixel boundaries
                if (isPixelMode) {
                    saveUndoState();
                    m_selectedStrokes.clear();
                    m_selectedImages.clear();

                    std::vector<int> newlySelectedIndices;
                    std::vector<CanvasImage> newSplitImages;

                    // 1. Cut through any existing CanvasImage objects that intersect lasso
                    for (int iIdx = 0; iIdx < static_cast<int>(m_canvasImages.size()); ++iIdx) {
                        auto& ci = m_canvasImages[iIdx];
                        if (ci.frame != currentFrame || ci.layer != activeLayer || !ci.texture) continue;
                        if (!selection.getBoundingBox().intersects(ci.bounds)) continue;

                        sf::Image img = ci.texture->copyToImage();
                        int iw = static_cast<int>(img.getSize().x);
                        int ih = static_cast<int>(img.getSize().y);
                        int bx = static_cast<int>(std::round(ci.bounds.left));
                        int by = static_cast<int>(std::round(ci.bounds.top));

                        std::vector<sf::Vector2i> cutPts;
                        std::vector<sf::Vector2i> keptPts;

                        for (int y = 0; y < ih; ++y) {
                            for (int x = 0; x < iw; ++x) {
                                if (img.getPixel(x, y).a == 0) continue;
                                int gx = bx + x;
                                int gy = by + y;
                                if (selection.isPointInsideSelection(sf::Vector2f(static_cast<float>(gx) + 0.5f, static_cast<float>(gy) + 0.5f))) {
                                    cutPts.push_back({ gx, gy });
                                }
                                else {
                                    keptPts.push_back({ gx, gy });
                                }
                            }
                        }

                        if (cutPts.empty()) continue;

                        if (keptPts.empty()) {
                            newlySelectedIndices.push_back(iIdx);
                            continue;
                        }

                        int kMinX = keptPts[0].x, kMaxX = keptPts[0].x, kMinY = keptPts[0].y, kMaxY = keptPts[0].y;
                        for (const auto& p : keptPts) {
                            kMinX = std::min(kMinX, p.x); kMaxX = std::max(kMaxX, p.x);
                            kMinY = std::min(kMinY, p.y); kMaxY = std::max(kMaxY, p.y);
                        }
                        int kW = kMaxX - kMinX + 1;
                        int kH = kMaxY - kMinY + 1;
                        sf::Image keptImg;
                        keptImg.create(kW, kH, sf::Color::Transparent);
                        for (const auto& p : keptPts) {
                            keptImg.setPixel(p.x - kMinX, p.y - kMinY, img.getPixel(p.x - bx, p.y - by));
                        }
                        auto keptTex = std::make_shared<sf::Texture>();
                        keptTex->setSmooth(false);
                        keptTex->loadFromImage(keptImg);
                        ci.texture = keptTex;
                        ci.bounds = sf::FloatRect(static_cast<float>(kMinX), static_cast<float>(kMinY),
                            static_cast<float>(kW), static_cast<float>(kH));

                        int cMinX = cutPts[0].x, cMaxX = cutPts[0].x, cMinY = cutPts[0].y, cMaxY = cutPts[0].y;
                        for (const auto& p : cutPts) {
                            cMinX = std::min(cMinX, p.x); cMaxX = std::max(cMaxX, p.x);
                            cMinY = std::min(cMinY, p.y); cMaxY = std::max(cMaxY, p.y);
                        }
                        int cW = cMaxX - cMinX + 1;
                        int cH = cMaxY - cMinY + 1;
                        sf::Image cutImg;
                        cutImg.create(cW, cH, sf::Color::Transparent);
                        for (const auto& p : cutPts) {
                            cutImg.setPixel(p.x - cMinX, p.y - cMinY, img.getPixel(p.x - bx, p.y - by));
                        }
                        auto cutTex = std::make_shared<sf::Texture>();
                        cutTex->setSmooth(false);
                        cutTex->loadFromImage(cutImg);

                        CanvasImage newCi;
                        newCi.id = ++m_nextImageId;
                        newCi.frame = currentFrame;
                        newCi.layer = activeLayer;
                        newCi.texture = cutTex;
                        newCi.image = std::move(cutImg);
                        newCi.bounds = sf::FloatRect(static_cast<float>(cMinX), static_cast<float>(cMinY),
                            static_cast<float>(cW), static_cast<float>(cH));
                        newSplitImages.push_back(std::move(newCi));
                    }

                    // 2. Cut through drawings on the base layer texture
                    sf::RenderTexture* targetTex = frames[currentFrame].layers[activeLayer].texture.get();
                    if (targetTex) {
                        sf::Image origLayerImg = targetTex->getTexture().copyToImage();
                        sf::Image layerImg = origLayerImg;
                        int w = static_cast<int>(layerImg.getSize().x);
                        int h = static_cast<int>(layerImg.getSize().y);
                        sf::FloatRect bb = selection.getBoundingBox();
                        int x0 = std::max(0, static_cast<int>(std::floor(bb.left)));
                        int y0 = std::max(0, static_cast<int>(std::floor(bb.top)));
                        int x1 = std::min(w, static_cast<int>(std::ceil(bb.left + bb.width)));
                        int y1 = std::min(h, static_cast<int>(std::ceil(bb.top + bb.height)));

                        std::vector<sf::Vector2i> cutLayerPts;
                        for (int y = y0; y < y1; ++y) {
                            for (int x = x0; x < x1; ++x) {
                                if (layerImg.getPixel(x, y).a > 0 &&
                                    selection.isPointInsideSelection(sf::Vector2f(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f))) {
                                    cutLayerPts.push_back({ x, y });
                                }
                            }
                        }

                        if (!cutLayerPts.empty()) {
                            for (const auto& p : cutLayerPts) {
                                layerImg.setPixel(p.x, p.y, sf::Color::Transparent);
                            }
                            sf::Texture updatedLayerTex;
                            updatedLayerTex.loadFromImage(layerImg);
                            targetTex->clear(sf::Color::Transparent);
                            targetTex->draw(sf::Sprite(updatedLayerTex), sf::RenderStates(sf::BlendNone));
                            targetTex->display();

                            std::map<int, bool> inCut;
                            for (const auto& p : cutLayerPts) inCut[p.y * w + p.x] = true;

                            const int dx8[8] = { 1, -1, 0, 0, 1, 1, -1, -1 };
                            const int dy8[8] = { 0, 0, 1, -1, 1, -1, 1, -1 };

                            for (const auto& startP : cutLayerPts) {
                                int startIdx = startP.y * w + startP.x;
                                if (!inCut[startIdx]) continue;

                                std::vector<sf::Vector2i> comp;
                                std::vector<sf::Vector2i> q;
                                q.push_back(startP);
                                inCut[startIdx] = false;

                                size_t head = 0;
                                while (head < q.size()) {
                                    sf::Vector2i cur = q[head++];
                                    comp.push_back(cur);
                                    for (int d = 0; d < 8; ++d) {
                                        int nx = cur.x + dx8[d];
                                        int ny = cur.y + dy8[d];
                                        if (nx >= x0 && nx < x1 && ny >= y0 && ny < y1) {
                                            int nidx = ny * w + nx;
                                            if (inCut[nidx]) {
                                                inCut[nidx] = false;
                                                q.push_back({ nx, ny });
                                            }
                                        }
                                    }
                                }

                                int minX = comp[0].x, maxX = comp[0].x, minY = comp[0].y, maxY = comp[0].y;
                                for (const auto& p : comp) {
                                    minX = std::min(minX, p.x); maxX = std::max(maxX, p.x);
                                    minY = std::min(minY, p.y); maxY = std::max(maxY, p.y);
                                }
                                int compW = maxX - minX + 1;
                                int compH = maxY - minY + 1;
                                sf::Image compImg;
                                compImg.create(compW, compH, sf::Color::Transparent);

                                for (const auto& p : comp) {
                                    compImg.setPixel(p.x - minX, p.y - minY, origLayerImg.getPixel(p.x, p.y));
                                }

                                auto tex = std::make_shared<sf::Texture>();
                                tex->setSmooth(false);
                                tex->loadFromImage(compImg);

                                CanvasImage ci;
                                ci.id = ++m_nextImageId;
                                ci.frame = currentFrame;
                                ci.layer = activeLayer;
                                ci.texture = tex;
                                ci.bounds = sf::FloatRect(static_cast<float>(minX), static_cast<float>(minY),
                                    static_cast<float>(compW), static_cast<float>(compH));
                                newSplitImages.push_back(std::move(ci));
                            }
                        }
                    }

                    for (auto& nci : newSplitImages) {
                        newlySelectedIndices.push_back(static_cast<int>(m_canvasImages.size()));
                        m_canvasImages.push_back(std::move(nci));
                    }
                    m_selectedImages = newlySelectedIndices;

                    std::vector<sf::FloatRect> subBoxes;
                    sf::FloatRect masterBox;
                    bool first = true;
                    for (int iIdx : m_selectedImages) {
                        const auto& b = m_canvasImages[iIdx].bounds;
                        subBoxes.push_back(b);
                        if (first) { masterBox = b; first = false; }
                        else {
                            float mx0 = std::min(masterBox.left, b.left);
                            float my0 = std::min(masterBox.top, b.top);
                            float mx1 = std::max(masterBox.left + masterBox.width, b.left + b.width);
                            float my1 = std::max(masterBox.top + masterBox.height, b.top + b.height);
                            masterBox = sf::FloatRect(mx0, my0, mx1 - mx0, my1 - my0);
                        }
                    }

                    if (!subBoxes.empty()) {
                        m_isMultiSelectionGroup = (subBoxes.size() > 1);
                        selection.setSelectionBoxes(masterBox, subBoxes);
                        selection.setShowHandles(pendingTransform);
                        isDirty = true;
                    }
                    else {
                        clearObjectSelection();
                    }
                    return;
                }

                saveUndoState();

                m_selectedStrokes.clear();
                m_selectedImages.clear();

                std::vector<VectorStroke> newlyCutStrokes;
                std::vector<int> selectedStrokeIndices;

                for (size_t s = 0; s < m_vectorStrokes.size(); ++s) {
                    auto& vs = m_vectorStrokes[s];
                    if (vs.frame != currentFrame || vs.layer != activeLayer || vs.isErase) continue;

                    sf::FloatRect sb = getStrokeBounds(vs);
                    if (!selection.getBoundingBox().intersects(sb)) continue;

                    sf::VertexArray kept(sf::Triangles);
                    sf::VertexArray cut(sf::Triangles);

                    for (size_t v = 0; v + 2 < vs.mesh.getVertexCount(); v += 3) {
                        sf::Vector2f centroid = (vs.mesh[v].position + vs.mesh[v + 1].position + vs.mesh[v + 2].position) / 3.0f;
                        if (selection.isPointInsideSelection(centroid)) {
                            cut.append(vs.mesh[v]);
                            cut.append(vs.mesh[v + 1]);
                            cut.append(vs.mesh[v + 2]);
                        }
                        else {
                            kept.append(vs.mesh[v]);
                            kept.append(vs.mesh[v + 1]);
                            kept.append(vs.mesh[v + 2]);
                        }
                    }

                    if (cut.getVertexCount() > 0 && kept.getVertexCount() > 0) {
                        vs.mesh = kept;
                        VectorStroke newVs;
                        newVs.mesh = cut;
                        newVs.layer = activeLayer;
                        newVs.frame = currentFrame;
                        newVs.isErase = vs.isErase;
                        newlyCutStrokes.push_back(std::move(newVs));
                    }
                    else if (cut.getVertexCount() > 0 && kept.getVertexCount() == 0) {
                        selectedStrokeIndices.push_back(static_cast<int>(s));
                    }
                }

                for (auto& nvs : newlyCutStrokes) {
                    selectedStrokeIndices.push_back(static_cast<int>(m_vectorStrokes.size()));
                    m_vectorStrokes.push_back(std::move(nvs));
                }

                m_selectedStrokes = selectedStrokeIndices;

                for (size_t i = 0; i < m_canvasImages.size(); ++i) {
                    if (m_canvasImages[i].frame == currentFrame && m_canvasImages[i].layer == activeLayer) {
                        const auto& b = m_canvasImages[i].bounds;
                        if (selection.getBoundingBox().intersects(b)) {
                            bool imgCut = selection.isPointInsideSelection(sf::Vector2f(b.left, b.top)) ||
                                selection.isPointInsideSelection(sf::Vector2f(b.left + b.width, b.top)) ||
                                selection.isPointInsideSelection(sf::Vector2f(b.left + b.width, b.top + b.height)) ||
                                selection.isPointInsideSelection(sf::Vector2f(b.left, b.top + b.height)) ||
                                selection.isPointInsideSelection(sf::Vector2f(b.left + b.width * 0.5f, b.top + b.height * 0.5f));
                            if (imgCut) {
                                m_selectedImages.push_back(static_cast<int>(i));
                            }
                        }
                    }
                }

                std::vector<sf::FloatRect> subBoxes;
                for (int sIdx : m_selectedStrokes) {
                    subBoxes.push_back(getStrokeBounds(m_vectorStrokes[sIdx]));
                }
                for (int iIdx : m_selectedImages) {
                    subBoxes.push_back(m_canvasImages[iIdx].bounds);
                }

                if (!subBoxes.empty()) {
                    m_isMultiSelectionGroup = (subBoxes.size() > 1);
                    selection.setSubItemBoxes(subBoxes);
                    selection.setShowHandles(pendingTransform);
                    isDirty = true;
                }
                else {
                    clearObjectSelection();
                }
            }
        }
        return;
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
        if (isPixelMode && (activeTool == ToolType::Brush || activeTool == ToolType::Pencil) && !m_pixelStrokePoints.empty()) {
            sf::RenderTexture* targetTex = frames[currentFrame].layers[activeLayer].texture.get();
            if (targetTex) {
                int minX = m_pixelStrokePoints[0].x, maxX = m_pixelStrokePoints[0].x;
                int minY = m_pixelStrokePoints[0].y, maxY = m_pixelStrokePoints[0].y;
                for (const auto& pt : m_pixelStrokePoints) {
                    minX = std::min(minX, pt.x); maxX = std::max(maxX, pt.x);
                    minY = std::min(minY, pt.y); maxY = std::max(maxY, pt.y);
                }
                int sw = maxX - minX + 1;
                int sh = maxY - minY + 1;

                if (sw > 0 && sh > 0) {
                    // Clear just the stroke pixels from the layer texture without GPU readback
                    sf::RenderStates rsNone;
                    rsNone.blendMode = sf::BlendNone;
                    sf::RectangleShape clearPx(sf::Vector2f(1.f, 1.f));
                    clearPx.setFillColor(sf::Color::Transparent);

                    for (const auto& pt : m_pixelStrokePoints) {
                        clearPx.setPosition(static_cast<float>(pt.x), static_cast<float>(pt.y));
                        targetTex->draw(clearPx, rsNone);
                    }
                    targetTex->display();

                    // Generate stroke texture directly with exact drawn colors
                    // Generate stroke texture directly with exact drawn colors
                    sf::Image strokeImg;
                    strokeImg.create(sw, sh, sf::Color::Transparent);

                    for (const auto& pt : m_pixelStrokePoints) {
                        strokeImg.setPixel(pt.x - minX, pt.y - minY, pt.color);
                    }

                    auto tex = std::make_shared<sf::Texture>();
                    tex->setSmooth(false);
                    tex->loadFromImage(strokeImg);

                    CanvasImage ci;
                    ci.id = ++m_nextImageId;
                    ci.frame = currentFrame;
                    ci.layer = activeLayer;
                    ci.texture = tex;
                    ci.image = std::move(strokeImg);
                    ci.bounds = sf::FloatRect(static_cast<float>(minX), static_cast<float>(minY),
                        static_cast<float>(sw), static_cast<float>(sh));
                    m_canvasImages.push_back(std::move(ci));
                    isDirty = true;
                }
            }
            m_pixelStrokePoints.clear();
        }

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

    if (isPixelMode) {
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
        if (selection.isResizing()) {
            selection.resize(localPos, canvasLogicalSize);
            sf::FloatRect newBox = selection.getBoundingBox();

            if (m_resizeStartBox.width > 0.001f && m_resizeStartBox.height > 0.001f) {
                float totalSx = newBox.width / m_resizeStartBox.width;
                float totalSy = newBox.height / m_resizeStartBox.height;

                for (const auto& snap : m_resizeStrokeSnapshots) {
                    if (snap.index >= 0 && snap.index < static_cast<int>(m_vectorStrokes.size())) {
                        auto& mesh = m_vectorStrokes[snap.index].mesh;
                        const auto& origMesh = snap.mesh;
                        for (size_t v = 0; v < origMesh.getVertexCount(); ++v) {
                            mesh[v].position.x = newBox.left + (origMesh[v].position.x - m_resizeStartBox.left) * totalSx;
                            mesh[v].position.y = newBox.top + (origMesh[v].position.y - m_resizeStartBox.top) * totalSy;
                        }
                    }
                }

                for (const auto& snap : m_resizeImageSnapshots) {
                    if (snap.index >= 0 && snap.index < static_cast<int>(m_canvasImages.size())) {
                        auto& b = m_canvasImages[snap.index].bounds;
                        if (isPixelMode) {
                            b.left = std::round(newBox.left + (snap.bounds.left - m_resizeStartBox.left) * totalSx);
                            b.top = std::round(newBox.top + (snap.bounds.top - m_resizeStartBox.top) * totalSy);
                            b.width = std::max(1.f, std::round(snap.bounds.width * totalSx));
                            b.height = std::max(1.f, std::round(snap.bounds.height * totalSy));
                        }
                        else {
                            b.left = newBox.left + (snap.bounds.left - m_resizeStartBox.left) * totalSx;
                            b.top = newBox.top + (snap.bounds.top - m_resizeStartBox.top) * totalSy;
                            b.width = snap.bounds.width * totalSx;
                            b.height = snap.bounds.height * totalSy;
                        }
                    }
                }

                std::vector<sf::FloatRect> updatedSub;
                for (int sIdx : m_selectedStrokes) updatedSub.push_back(getStrokeBounds(m_vectorStrokes[sIdx]));
                for (int iIdx : m_selectedImages) updatedSub.push_back(m_canvasImages[iIdx].bounds);
                selection.setSubItemBoxes(updatedSub);
            }
            isDirty = true;
            return;
        }

        if (selection.isDragging()) {
            sf::Vector2f delta = localPos - m_lastDragPos;
            m_lastDragPos = localPos;

            selection.drag(localPos, canvasLogicalSize, true);

            for (int sIdx : m_selectedStrokes) {
                if (sIdx >= 0 && sIdx < static_cast<int>(m_vectorStrokes.size())) {
                    auto& mesh = m_vectorStrokes[sIdx].mesh;
                    for (size_t v = 0; v < mesh.getVertexCount(); ++v) {
                        mesh[v].position += delta;
                    }
                }
            }

            for (int iIdx : m_selectedImages) {
                if (iIdx >= 0 && iIdx < static_cast<int>(m_canvasImages.size())) {
                    if (isPixelMode) {
                        m_canvasImages[iIdx].bounds.left = std::round(m_canvasImages[iIdx].bounds.left + delta.x);
                        m_canvasImages[iIdx].bounds.top = std::round(m_canvasImages[iIdx].bounds.top + delta.y);
                    }
                    else {
                        m_canvasImages[iIdx].bounds.left += delta.x;
                        m_canvasImages[iIdx].bounds.top += delta.y;
                    }
                }
            }

            if (isPixelMode && !m_selectedImages.empty()) {
                float maxCW = static_cast<float>(canvasLogicalSize.x);
                float maxCH = static_cast<float>(canvasLogicalSize.y);
                std::vector<sf::FloatRect> subBoxes;
                sf::FloatRect masterBox;
                bool first = true;
                for (int iIdx : m_selectedImages) {
                    if (iIdx >= 0 && iIdx < static_cast<int>(m_canvasImages.size())) {
                        const auto& b = m_canvasImages[iIdx].bounds;
                        float x0 = std::clamp(b.left, 0.f, maxCW);
                        float y0 = std::clamp(b.top, 0.f, maxCH);
                        float x1 = std::clamp(b.left + b.width, 0.f, maxCW);
                        float y1 = std::clamp(b.top + b.height, 0.f, maxCH);
                        sf::FloatRect visB(x0, y0, std::max(0.f, x1 - x0), std::max(0.f, y1 - y0));
                        subBoxes.push_back(visB);
                        if (first) { masterBox = visB; first = false; }
                        else {
                            float mx0 = std::min(masterBox.left, visB.left);
                            float my0 = std::min(masterBox.top, visB.top);
                            float mx1 = std::max(masterBox.left + masterBox.width, visB.left + visB.width);
                            float my1 = std::max(masterBox.top + masterBox.height, visB.top + visB.height);
                            masterBox = sf::FloatRect(mx0, my0, mx1 - mx0, my1 - my0);
                        }
                    }
                }
                if (!subBoxes.empty() && masterBox.width > 0.f && masterBox.height > 0.f) {
                    selection.setBoundingBox(masterBox);
                    selection.setSubItemBoxes(subBoxes);
                }
            }

            isDirty = true;
            return;
        }

        if (selection.getState() == SelectionState::Drawing) {
            selection.addLassoPoint(localPos, canvasLogicalSize);
            return;
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
    m_currentFrame = currentFrame;

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
    selection.setHandleVisualSize(12.0f / handleDenom);
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

    if (symmetryManager.visible) {
        symmetryManager.drawGuides(window, innerStates, sf::FloatRect(0, 0, static_cast<float>(canvasLogicalSize.x), static_cast<float>(canvasLogicalSize.y)), viewScale);
    }

    sf::Vector2i mousePosI = sf::Mouse::getPosition(window);
    sf::Vector2f currentRawMousePos = window.mapPixelToCoords(mousePosI);

    sf::Vector2f logicalPos = getInverseTransform().transformPoint(currentRawMousePos);
    bool currentlyHovering = drawArea.contains(logicalPos);

    float sXHover = static_cast<float>(canvasLogicalSize.x) / drawArea.width;
    float sYHover = static_cast<float>(canvasLogicalSize.y) / drawArea.height;
    sf::Vector2f curLocalPos((logicalPos.x - drawArea.left) * sXHover, (logicalPos.y - drawArea.top) * sYHover);
    curLocalPos.x = std::clamp(curLocalPos.x, 0.0f, static_cast<float>(canvasLogicalSize.x));
    curLocalPos.y = std::clamp(curLocalPos.y, 0.0f, static_cast<float>(canvasLogicalSize.y));
    if (isPixelMode) {
        curLocalPos.x = std::floor(curLocalPos.x);
        curLocalPos.y = std::floor(curLocalPos.y);
    }

    bool isShift = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);
    bool canDrawLine = (activeTool == ToolType::Brush || activeTool == ToolType::Pencil || activeTool == ToolType::Eraser);

    // Dynamic Aseprite-style straight line preview while Shift is held
    if (!isPlaying && currentlyHovering && canDrawLine && hasShiftAnchor && isShift && !isDrawing) {
        if (isPixelMode) {
            auto pts = getBresenhamPoints(
                static_cast<int>(shiftAnchor.x), static_cast<int>(shiftAnchor.y),
                static_cast<int>(curLocalPos.x), static_cast<int>(curLocalPos.y)
            );

            sf::Color previewCol = (activeTool == ToolType::Eraser)
                ? sf::Color(255, 90, 90, 180)
                : primaryColor;

            sf::VertexArray previewPixels(sf::Quads);

            for (const auto& pt : pts) {
                std::vector<sf::Vector2f> symPoints;
                if (symmetryManager.enabled) {
                    symPoints = symmetryManager.getSymmetricPoints(sf::Vector2f(static_cast<float>(pt.x), static_cast<float>(pt.y)));
                }
                else {
                    symPoints.push_back(sf::Vector2f(static_cast<float>(pt.x), static_cast<float>(pt.y)));
                }

                for (const auto& sp : symPoints) {
                    int baseIX = static_cast<int>(std::round(sp.x));
                    int baseIY = static_cast<int>(std::round(sp.y));

                    for (const auto& offset : m_pixelBrushMask) {
                        float ix = static_cast<float>(baseIX + offset.x);
                        float iy = static_cast<float>(baseIY + offset.y);

                        previewPixels.append(sf::Vertex(sf::Vector2f(ix, iy), previewCol));
                        previewPixels.append(sf::Vertex(sf::Vector2f(ix + 1.0f, iy), previewCol));
                        previewPixels.append(sf::Vertex(sf::Vector2f(ix + 1.0f, iy + 1.0f), previewCol));
                        previewPixels.append(sf::Vertex(sf::Vector2f(ix, iy + 1.0f), previewCol));
                    }
                }
            }
            window.draw(previewPixels, innerStates);
        }
        else {
            sf::VertexArray previewMesh(sf::Triangles);
            float radius = brushEngine.getActivePreset().size * 0.5f;
            sf::Color meshCol = (activeTool == ToolType::Eraser)
                ? sf::Color(255, 90, 90, 180)
                : primaryColor;

            appendVectorCap(previewMesh, shiftAnchor, radius, meshCol);
            appendVectorSegment(previewMesh, shiftAnchor, curLocalPos, radius, meshCol);
            appendVectorCap(previewMesh, curLocalPos, radius, meshCol);

            if (symmetryManager.enabled) {
                auto p1 = symmetryManager.getSymmetricPoints(shiftAnchor);
                auto p2 = symmetryManager.getSymmetricPoints(curLocalPos);
                if (p1.size() > 1 && p2.size() > 1) {
                    appendVectorCap(previewMesh, p1[1], radius, meshCol);
                    appendVectorSegment(previewMesh, p1[1], p2[1], radius, meshCol);
                    appendVectorCap(previewMesh, p2[1], radius, meshCol);
                }
            }

            window.draw(previewMesh, innerStates);
        }
    }

    if (!isPlaying && currentlyHovering && (activeTool == ToolType::Brush || activeTool == ToolType::Pencil || activeTool == ToolType::Eraser || activeTool == ToolType::Curve || activeTool == ToolType::FilledContour)) {
        if (isPixelMode) {
            float sX = static_cast<float>(canvasLogicalSize.x) / drawArea.width;
            float sY = static_cast<float>(canvasLogicalSize.y) / drawArea.height;

            sf::Vector2f lp = getInverseTransform().transformPoint(currentRawMousePos);
            lp.x = (lp.x - drawArea.left) * sX;
            lp.y = (lp.y - drawArea.top) * sY;

            int baseIX = static_cast<int>(std::floor(lp.x));
            int baseIY = static_cast<int>(std::floor(lp.y));

            sf::VertexArray pxHover(sf::Quads);
            for (const auto& offset : m_pixelBrushMask) {
                float fx = static_cast<float>(baseIX + offset.x);
                float fy = static_cast<float>(baseIY + offset.y);

                pxHover.append(sf::Vertex(sf::Vector2f(fx, fy), sf::Color(20, 10, 30, 140)));
                pxHover.append(sf::Vertex(sf::Vector2f(fx + 1.0f, fy), sf::Color(20, 10, 30, 140)));
                pxHover.append(sf::Vertex(sf::Vector2f(fx + 1.0f, fy + 1.0f), sf::Color(20, 10, 30, 140)));
                pxHover.append(sf::Vertex(sf::Vector2f(fx, fy + 1.0f), sf::Color(20, 10, 30, 140)));
            }
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

sf::Image& Canvas::getCanvasImageCPU(CanvasImage& ci) {
    if (ci.image.getSize().x == 0 && ci.texture && ci.texture->getSize().x > 0) {
        ci.image = ci.texture->copyToImage();
    }
    return ci.image;
}

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
    clearObjectSelection();
    isDrawing = false;
    isDeforming = false;

    for (auto& frame : frames) {
        for (auto& layer : frame.layers) {
            if (layer.texture) {
                layer.texture->setSmooth(!isPixelMode);
            }
        }
    }
}

bool Canvas::getPixelMode() const { return isPixelMode; }
void Canvas::setPixelBrushSize(int size) {
    pixelBrushSize = std::max(1, size);
    rebuildPixelBrushMask();
}

int Canvas::getPixelBrushSize() const { return pixelBrushSize; }
bool Canvas::getIsDirty() const { return isDirty; }
void Canvas::clearIsDirty() { isDirty = false; }

void Canvas::cyclePixelBrushSize() {
    if (pixelBrushSize == 1) pixelBrushSize = 2;
    else if (pixelBrushSize == 2) pixelBrushSize = 4;
    else if (pixelBrushSize == 4) pixelBrushSize = 8;
    else if (pixelBrushSize == 8) pixelBrushSize = 16;
    else pixelBrushSize = 1;
    rebuildPixelBrushMask();
}

void Canvas::setPixelBrushShape(PixelBrushShape shape) {
    m_pixelBrushShape = shape;
    rebuildPixelBrushMask();
}

void Canvas::rebuildPixelBrushMask() {
    m_pixelBrushMask.clear();

    if (m_pixelBrushShape == PixelBrushShape::Square) {
        int half = pixelBrushSize / 2;
        int oddOffset = (pixelBrushSize % 2 == 0) ? 1 : 0;
        for (int y = -half; y <= half - oddOffset; ++y) {
            for (int x = -half; x <= half - oddOffset; ++x) {
                m_pixelBrushMask.push_back({ x, y });
            }
        }
    }
    else if (m_pixelBrushShape == PixelBrushShape::Circle) {
        float r = static_cast<float>(pixelBrushSize) * 0.5f;
        float rSq = r * r;
        int half = static_cast<int>(std::ceil(r));
        for (int y = -half; y <= half; ++y) {
            for (int x = -half; x <= half; ++x) {
                float distSq = (static_cast<float>(x) + 0.5f) * (static_cast<float>(x) + 0.5f) +
                    (static_cast<float>(y) + 0.5f) * (static_cast<float>(y) + 0.5f);
                if (distSq <= rSq) {
                    m_pixelBrushMask.push_back({ x, y });
                }
            }
        }
        if (m_pixelBrushMask.empty()) m_pixelBrushMask.push_back({ 0, 0 });
    }
    else if (m_pixelBrushShape == PixelBrushShape::Rectangle) {
        int w = std::max(2, pixelBrushSize);
        int h = std::max(1, pixelBrushSize / 2);
        int halfW = w / 2;
        int halfH = h / 2;
        int oddW = (w % 2 == 0) ? 1 : 0;
        int oddH = (h % 2 == 0) ? 1 : 0;
        for (int y = -halfH; y <= halfH - oddH; ++y) {
            for (int x = -halfW; x <= halfW - oddW; ++x) {
                m_pixelBrushMask.push_back({ x, y });
            }
        }
        if (m_pixelBrushMask.empty()) m_pixelBrushMask.push_back({ 0, 0 });
    }
    else if (m_pixelBrushShape == PixelBrushShape::Slash) {
        int half = pixelBrushSize / 2;
        for (int i = -half; i <= half; ++i) {
            m_pixelBrushMask.push_back({ i, -i });
        }
        if (m_pixelBrushMask.empty()) m_pixelBrushMask.push_back({ 0, 0 });
    }
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


void Canvas::enterTransformMode(int currentFrame) {
    if (!selection.isActive()) return;
    pendingTransform = true;
    selection.setShowHandles(true);
    transformMode = TransformState::Scaling;
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

    // --- PIXEL ART MODE: Pixel-exact click and multi-object continuous grouping ---
    if (isPixelMode) {
        sf::RenderTexture* targetTex = frames[currentFrame].layers[activeLayer].texture.get();
        int maxCW = static_cast<int>(canvasLogicalSize.x);
        int maxCH = static_cast<int>(canvasLogicalSize.y);

        struct CachedImg {
            sf::Image img;
            int w = 0, h = 0;
        };
        std::vector<CachedImg> cachedImgs(m_canvasImages.size());
        for (size_t i = 0; i < m_canvasImages.size(); ++i) {
            if (m_canvasImages[i].texture && m_canvasImages[i].texture->getSize().x > 0 && m_canvasImages[i].texture->getSize().y > 0) {
                cachedImgs[i].img = m_canvasImages[i].texture->copyToImage();
                cachedImgs[i].w = static_cast<int>(cachedImgs[i].img.getSize().x);
                cachedImgs[i].h = static_cast<int>(cachedImgs[i].img.getSize().y);
            }
        }

        auto hasPixel = [&](int imgIdx, int cx, int cy) -> bool {
            if (imgIdx < 0 || imgIdx >= static_cast<int>(m_canvasImages.size())) return false;
            const auto& ci = m_canvasImages[imgIdx];
            const auto& c = cachedImgs[imgIdx];
            if (c.w == 0 || c.h == 0) return false;
            if (cx < ci.bounds.left || cy < ci.bounds.top ||
                cx >= ci.bounds.left + ci.bounds.width || cy >= ci.bounds.top + ci.bounds.height) return false;
            float u = (static_cast<float>(cx) - ci.bounds.left + 0.5f) / ci.bounds.width;
            float v = (static_cast<float>(cy) - ci.bounds.top + 0.5f) / ci.bounds.height;
            int tx = std::clamp(static_cast<int>(u * static_cast<float>(c.w)), 0, c.w - 1);
            int ty = std::clamp(static_cast<int>(v * static_cast<float>(c.h)), 0, c.h - 1);
            return c.img.getPixel(tx, ty).a > 0;
            };

        int clickedImageIdx = -1;
        int clickX = static_cast<int>(std::floor(pos.x));
        int clickY = static_cast<int>(std::floor(pos.y));

        // 1. Check existing CanvasImage objects (topmost first)
        for (int i = static_cast<int>(m_canvasImages.size()) - 1; i >= 0; --i) {
            if (m_canvasImages[i].frame == currentFrame && m_canvasImages[i].layer == activeLayer) {
                if (hasPixel(i, clickX, clickY)) {
                    clickedImageIdx = i;
                    break;
                }
            }
        }

        // 2. Check base layer texture
        sf::Image layerImg;
        bool layerImgLoaded = false;
        if (targetTex) {
            layerImg = targetTex->getTexture().copyToImage();
            layerImgLoaded = true;
        }

        const int dx8[9] = { 0, 1, -1, 0, 0, 1, 1, -1, -1 };
        const int dy8[9] = { 0, 0, 0, 1, -1, 1, -1, 1, -1 };

        // If clicked on base layer pixel (and not on an existing object), extract it
        if (clickedImageIdx == -1 && layerImgLoaded) {
            if (clickX >= 0 && clickY >= 0 && clickX < maxCW && clickY < maxCH && layerImg.getPixel(clickX, clickY).a > 0) {
                std::vector<bool> visited(maxCW * maxCH, false);
                std::vector<sf::Vector2i> q;
                q.push_back({ clickX, clickY });
                visited[clickY * maxCW + clickX] = true;

                int minX = clickX, maxX = clickX, minY = clickY, maxY = clickY;
                size_t head = 0;
                while (head < q.size()) {
                    sf::Vector2i p = q[head++];
                    minX = std::min(minX, p.x); maxX = std::max(maxX, p.x);
                    minY = std::min(minY, p.y); maxY = std::max(maxY, p.y);
                    for (int d = 1; d <= 8; ++d) {
                        int nx = p.x + dx8[d];
                        int ny = p.y + dy8[d];
                        if (nx >= 0 && nx < maxCW && ny >= 0 && ny < maxCH) {
                            int idx = ny * maxCW + nx;
                            if (!visited[idx] && layerImg.getPixel(nx, ny).a > 0) {
                                visited[idx] = true;
                                q.push_back({ nx, ny });
                            }
                        }
                    }
                }

                int compW = maxX - minX + 1;
                int compH = maxY - minY + 1;
                sf::Image compImg;
                compImg.create(compW, compH, sf::Color::Transparent);

                for (const auto& pt : q) {
                    compImg.setPixel(pt.x - minX, pt.y - minY, layerImg.getPixel(pt.x, pt.y));
                    layerImg.setPixel(pt.x, pt.y, sf::Color::Transparent);
                }

                sf::Texture updatedLayerTex;
                updatedLayerTex.loadFromImage(layerImg);
                targetTex->clear(sf::Color::Transparent);
                targetTex->draw(sf::Sprite(updatedLayerTex), sf::RenderStates(sf::BlendNone));
                targetTex->display();

                auto tex = std::make_shared<sf::Texture>();
                tex->setSmooth(false);
                tex->loadFromImage(compImg);

                CanvasImage ci;
                ci.id = ++m_nextImageId;
                ci.frame = currentFrame;
                ci.layer = activeLayer;
                ci.texture = tex;
                ci.image = compImg;
                ci.bounds = sf::FloatRect(static_cast<float>(minX), static_cast<float>(minY),
                    static_cast<float>(compW), static_cast<float>(compH));
                m_canvasImages.push_back(ci);

                CachedImg nci;
                nci.img = compImg;
                nci.w = compW;
                nci.h = compH;
                cachedImgs.push_back(nci);

                clickedImageIdx = static_cast<int>(m_canvasImages.size()) - 1;
            }
        }

        if (clickedImageIdx == -1) {
            clearObjectSelection();
            return;
        }

        // Fast canvas-space object touching test
        auto objectsTouch = [&](int idxA, int idxB) -> bool {
            const auto& a = m_canvasImages[idxA];
            const auto& b = m_canvasImages[idxB];
            float ix0 = std::max(a.bounds.left - 1.5f, b.bounds.left - 1.5f);
            float iy0 = std::max(a.bounds.top - 1.5f, b.bounds.top - 1.5f);
            float ix1 = std::min(a.bounds.left + a.bounds.width + 1.5f, b.bounds.left + b.bounds.width + 1.5f);
            float iy1 = std::min(a.bounds.top + a.bounds.height + 1.5f, b.bounds.top + b.bounds.height + 1.5f);
            if (ix0 >= ix1 || iy0 >= iy1) return false;

            int sx = static_cast<int>(std::floor(ix0));
            int ex = static_cast<int>(std::ceil(ix1));
            int sy = static_cast<int>(std::floor(iy0));
            int ey = static_cast<int>(std::ceil(iy1));

            for (int cy = sy; cy <= ey; ++cy) {
                for (int cx = sx; cx <= ex; ++cx) {
                    if (hasPixel(idxA, cx, cy)) {
                        for (int d = 0; d < 9; ++d) {
                            if (hasPixel(idxB, cx + dx8[d], cy + dy8[d])) return true;
                        }
                    }
                }
            }
            return false;
            };


        bool isDoubleClick = (m_selectClickClock.getElapsedTime().asMilliseconds() < 400 &&
            m_lastClickedImageIdx == clickedImageIdx &&
            m_selectedImages.size() > 1);
        m_selectClickClock.restart();
        m_lastClickedImageIdx = clickedImageIdx;

        if (isDoubleClick) {
            // Double-click: drill down to isolate strictly the clicked stroke
            m_selectedImages = { clickedImageIdx };
            m_selectedStrokes.clear();
            m_isMultiSelectionGroup = false;

            const auto& b = m_canvasImages[clickedImageIdx].bounds;
            float x0 = std::clamp(b.left, 0.f, static_cast<float>(maxCW));
            float y0 = std::clamp(b.top, 0.f, static_cast<float>(maxCH));
            float x1 = std::clamp(b.left + b.width, 0.f, static_cast<float>(maxCW));
            float y1 = std::clamp(b.top + b.height, 0.f, static_cast<float>(maxCH));
            sf::FloatRect visB(x0, y0, std::max(0.f, x1 - x0), std::max(0.f, y1 - y0));

            selection.setSelectionBoxes(visB, { visB });
            selection.setShowHandles(pendingTransform);
            isDirty = true;
            return;
        }

        // Single-click: chain all touching/glued objects together
        std::vector<int> chain;
        std::vector<bool> inChain(m_canvasImages.size(), false);
        chain.push_back(clickedImageIdx);
        inChain[clickedImageIdx] = true;

        size_t cHead = 0;
        while (cHead < chain.size()) {
            int curr = chain[cHead++];

            for (size_t next = 0; next < m_canvasImages.size(); ++next) {
                if (next >= inChain.size()) inChain.resize(m_canvasImages.size(), false);
                if (!inChain[next] && m_canvasImages[next].frame == currentFrame && m_canvasImages[next].layer == activeLayer) {
                    if (objectsTouch(curr, static_cast<int>(next))) {
                        inChain[next] = true;
                        chain.push_back(static_cast<int>(next));
                    }
                }
            }
        }

        m_selectedImages = chain;
        m_selectedStrokes.clear();
        std::vector<sf::FloatRect> subBoxes;
        sf::FloatRect masterBox;
        bool first = true;

        for (int idx : chain) {
            const auto& b = m_canvasImages[idx].bounds;
            float x0 = std::clamp(b.left, 0.f, static_cast<float>(maxCW));
            float y0 = std::clamp(b.top, 0.f, static_cast<float>(maxCH));
            float x1 = std::clamp(b.left + b.width, 0.f, static_cast<float>(maxCW));
            float y1 = std::clamp(b.top + b.height, 0.f, static_cast<float>(maxCH));
            sf::FloatRect visB(x0, y0, std::max(0.f, x1 - x0), std::max(0.f, y1 - y0));
            subBoxes.push_back(visB);
            if (first) { masterBox = visB; first = false; }
            else {
                float mx0 = std::min(masterBox.left, visB.left);
                float my0 = std::min(masterBox.top, visB.top);
                float mx1 = std::max(masterBox.left + masterBox.width, visB.left + visB.width);
                float my1 = std::max(masterBox.top + masterBox.height, visB.top + visB.height);
                masterBox = sf::FloatRect(mx0, my0, mx1 - mx0, my1 - my0);
            }
        }

        m_isMultiSelectionGroup = (chain.size() > 1);
        selection.setSelectionBoxes(masterBox, subBoxes);
        selection.setShowHandles(pendingTransform);
        isDirty = true;
        return;
    }

    struct SelectableEntity {
        enum Type { Stroke, Image };
        Type type;
        int originalIndex;
        sf::FloatRect bounds;
    };

    std::vector<SelectableEntity> entities;
    for (size_t s = 0; s < m_vectorStrokes.size(); ++s) {
        if (m_vectorStrokes[s].frame == currentFrame && m_vectorStrokes[s].layer == activeLayer && !m_vectorStrokes[s].isErase) {
            entities.push_back({ SelectableEntity::Stroke, static_cast<int>(s), getStrokeBounds(m_vectorStrokes[s]) });
        }
    }
    for (size_t i = 0; i < m_canvasImages.size(); ++i) {
        if (m_canvasImages[i].frame == currentFrame && m_canvasImages[i].layer == activeLayer) {
            entities.push_back({ SelectableEntity::Image, static_cast<int>(i), m_canvasImages[i].bounds });
        }
    }

    int clickedEntityIdx = -1;
    float bestDistSq = 14.0f * 14.0f;

    for (int i = 0; i < static_cast<int>(entities.size()); ++i) {
        if (entities[i].type == SelectableEntity::Image) {
            if (entities[i].bounds.contains(pos)) {
                clickedEntityIdx = i;
                bestDistSq = 0.f;
            }
        }
        else {
            float dSq = getStrokeMinDistanceSq(m_vectorStrokes[entities[i].originalIndex], pos);
            if (dSq < bestDistSq) {
                bestDistSq = dSq;
                clickedEntityIdx = i;
            }
        }
    }

    if (clickedEntityIdx == -1) {
        commitSelection(currentFrame);
        return;
    }

    const auto& clickedEnt = entities[clickedEntityIdx];

    bool isDoubleClick = (m_selectClickClock.getElapsedTime().asMilliseconds() < 400 &&
        m_lastClickedEntityIdx == clickedEntityIdx &&
        (m_selectedStrokes.size() + m_selectedImages.size()) > 1);
    m_selectClickClock.restart();
    m_lastClickedEntityIdx = clickedEntityIdx;

    if (isDoubleClick) {
        // Double-click: isolate the specific clicked stroke or image
        m_selectedStrokes.clear();
        m_selectedImages.clear();
        if (clickedEnt.type == SelectableEntity::Stroke) {
            m_selectedStrokes.push_back(clickedEnt.originalIndex);
        }
        else {
            m_selectedImages.push_back(clickedEnt.originalIndex);
        }
        m_isMultiSelectionGroup = false;
        selection.setSelectionBoxes(clickedEnt.bounds, { clickedEnt.bounds });
        selection.setShowHandles(pendingTransform);
        return;
    }

    // Single-click: chain touching strokes and images
    std::vector<bool> inChain(entities.size(), false);
    std::vector<int> q;
    q.push_back(clickedEntityIdx);
    inChain[clickedEntityIdx] = true;

    size_t head = 0;
    while (head < q.size()) {
        int curr = q[head++];

        for (size_t next = 0; next < entities.size(); ++next) {
            if (inChain[next]) continue;

            bool touches = false;
            const auto& e1 = entities[curr];
            const auto& e2 = entities[next];

            if (e1.type == SelectableEntity::Stroke && e2.type == SelectableEntity::Stroke) {
                touches = strokesCollide(m_vectorStrokes[e1.originalIndex], m_vectorStrokes[e2.originalIndex], 6.0f);
            }
            else if (e1.type == SelectableEntity::Stroke && e2.type == SelectableEntity::Image) {
                touches = strokeImageCollide(m_vectorStrokes[e1.originalIndex], m_canvasImages[e2.originalIndex].bounds, 6.0f);
            }
            else if (e1.type == SelectableEntity::Image && e2.type == SelectableEntity::Stroke) {
                touches = strokeImageCollide(m_vectorStrokes[e2.originalIndex], m_canvasImages[e1.originalIndex].bounds, 6.0f);
            }
            else if (e1.type == SelectableEntity::Image && e2.type == SelectableEntity::Image) {
                touches = e1.bounds.intersects(e2.bounds);
            }

            if (touches) {
                inChain[next] = true;
                q.push_back(static_cast<int>(next));
            }
        }
    }

    m_selectedStrokes.clear();
    m_selectedImages.clear();
    std::vector<sf::FloatRect> subBoxes;
    sf::FloatRect masterBox = entities[q[0]].bounds;

    for (int idx : q) {
        const auto& ent = entities[idx];
        if (ent.type == SelectableEntity::Stroke) m_selectedStrokes.push_back(ent.originalIndex);
        else m_selectedImages.push_back(ent.originalIndex);

        subBoxes.push_back(ent.bounds);

        float minX = std::min(masterBox.left, ent.bounds.left);
        float minY = std::min(masterBox.top, ent.bounds.top);
        float maxX = std::max(masterBox.left + masterBox.width, ent.bounds.left + ent.bounds.width);
        float maxY = std::max(masterBox.top + masterBox.height, ent.bounds.top + ent.bounds.height);
        masterBox = sf::FloatRect(minX, minY, maxX - minX, maxY - minY);
    }

    m_isMultiSelectionGroup = (q.size() > 1);
    selection.setSelectionBoxes(masterBox, subBoxes);
    selection.setShowHandles(pendingTransform);
}

void Canvas::cleanVectorLayers() {
    if (isPixelMode) return;
    for (auto& frame : frames) {
        for (auto& layer : frame.layers) {
            if (!layer.isImageResource && layer.texture) {
                layer.texture->clear(sf::Color::Transparent);
                layer.texture->display();
            }
        }
    }
}


void Canvas::pasteImage(const sf::Image& img, int currentFrame, bool originalResolution) {
    if (currentFrame < 0 || currentFrame >= static_cast<int>(frames.size())) return;
    if (img.getSize().x == 0 || img.getSize().y == 0) return;

    saveUndoState();

    sf::Vector2u texSize = img.getSize();
    float tw = static_cast<float>(texSize.x);
    float th = static_cast<float>(texSize.y);

    float maxCanvasW = static_cast<float>(canvasLogicalSize.x);
    float maxCanvasH = static_cast<float>(canvasLogicalSize.y);

    float targetW = tw;
    float targetH = th;
    float posX = 0.f;
    float posY = 0.f;

    auto tex = std::make_shared<sf::Texture>();

    if (originalResolution) {
        targetW = tw;
        targetH = th;
        posX = std::floor((maxCanvasW - targetW) * 0.5f);
        posY = std::floor((maxCanvasH - targetH) * 0.5f);

        tex->setSmooth(!isPixelMode);
        tex->loadFromImage(img);
    }
    else {
        float maxW = maxCanvasW * 0.85f;
        float maxH = maxCanvasH * 0.85f;
        float scale = 1.0f;
        if (tw > maxW || th > maxH) {
            scale = std::min(maxW / tw, maxH / th);
        }

        targetW = std::max(1.0f, std::floor(tw * scale));
        targetH = std::max(1.0f, std::floor(th * scale));
        posX = std::floor((maxCanvasW - targetW) * 0.5f);
        posY = std::floor((maxCanvasH - targetH) * 0.5f);

        if (isPixelMode) {
            int dstW = static_cast<int>(targetW);
            int dstH = static_cast<int>(targetH);
            sf::Image pixelScaled;
            pixelScaled.create(dstW, dstH, sf::Color::Transparent);

            int srcW = static_cast<int>(tw);
            int srcH = static_cast<int>(th);

            for (int y = 0; y < dstH; ++y) {
                int sy = std::clamp(static_cast<int>((static_cast<float>(y) + 0.5f) * static_cast<float>(srcH) / static_cast<float>(dstH)), 0, srcH - 1);
                for (int x = 0; x < dstW; ++x) {
                    int sx = std::clamp(static_cast<int>((static_cast<float>(x) + 0.5f) * static_cast<float>(srcW) / static_cast<float>(dstW)), 0, srcW - 1);
                    pixelScaled.setPixel(x, y, img.getPixel(sx, sy));
                }
            }
            tex->setSmooth(false);
            tex->loadFromImage(pixelScaled);
        }
        else {
            tex->setSmooth(true);
            tex->loadFromImage(img);
        }
    }

    CanvasImage ci;
    ci.id = ++m_nextImageId;
    ci.frame = currentFrame;
    ci.layer = activeLayer;
    ci.texture = tex;
    ci.bounds = sf::FloatRect(posX, posY, targetW, targetH);
    m_canvasImages.push_back(ci);

    clearObjectSelection();
    m_selectedImages.push_back(static_cast<int>(m_canvasImages.size()) - 1);
    m_isMultiSelectionGroup = false;

    if (isPixelMode) {
        float x0 = std::clamp(ci.bounds.left, 0.f, maxCanvasW);
        float y0 = std::clamp(ci.bounds.top, 0.f, maxCanvasH);
        float x1 = std::clamp(ci.bounds.left + ci.bounds.width, 0.f, maxCanvasW);
        float y1 = std::clamp(ci.bounds.top + ci.bounds.height, 0.f, maxCanvasH);
        sf::FloatRect visBox(x0, y0, std::max(0.f, x1 - x0), std::max(0.f, y1 - y0));
        selection.setSelectionBoxes(visBox, { visBox });
    }
    else {
        selection.setSelectionBoxes(ci.bounds, { ci.bounds });
    }

    selection.setShowHandles(true);
    setActiveTool(ToolType::Select);
    isDirty = true;
}

void Canvas::importImageToActiveLayer(const std::string& filepath, int currentFrame) {
    if (currentFrame < 0 || currentFrame >= static_cast<int>(frames.size())) return;
    sf::Image img;
    if (img.loadFromFile(filepath)) {
        pasteImage(img, currentFrame, false);
    }
}

void Canvas::replaceFrameImage(int frameIndex, const sf::Image& img) {
    if (frameIndex < 0 || frameIndex >= static_cast<int>(frames.size())) return;
    Frame* f = getFrame(frameIndex);
    if (!f || f->layers.empty()) return;

    clearObjectSelection();
    clearCanvasImages();

    for (size_t i = 0; i < f->layers.size(); ++i) {
        if (f->layers[i].texture) {
            f->layers[i].texture->clear(sf::Color::Transparent);
            f->layers[i].texture->display();
        }
    }

    int targetLayer = (f->layers.size() > 1) ? 1 : 0;
    setActiveLayer(targetLayer, frameIndex);

    if (f->layers[targetLayer].texture) {
        sf::Texture tex;
        if (tex.loadFromImage(img)) {
            tex.setSmooth(false);
            sf::Sprite spr(tex);

            float cW = static_cast<float>(getCanvasSize().x);
            float cH = static_cast<float>(getCanvasSize().y);
            float iW = static_cast<float>(img.getSize().x);
            float iH = static_cast<float>(img.getSize().y);

            if (iW != cW || iH != cH) {
                spr.setScale(cW / iW, cH / iH);
            }

            f->layers[targetLayer].texture->clear(sf::Color::Transparent);
            f->layers[targetLayer].texture->draw(spr);
            f->layers[targetLayer].texture->display();
        }
    }
}