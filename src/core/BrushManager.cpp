#include "BrushManager.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

BrushManager::BrushManager() : m_hasStartedStroke(false) {
    initDefaultPresets();
}

void BrushManager::initDefaultPresets() {
    BrushPreset pencil;
    pencil.name = "Pencil";
    pencil.type = BrushType::Pencil;
    pencil.size = 3.0f;
    pencil.opacity = 1.0f;
    pencil.hardness = 1.0f;
    pencil.spacing = 0.05f;
    pencil.stabilization = 0.1f;
    pencil.smoothing = 0.5f;
    pencil.scatter = 0.0f;
    pencil.rotation = 0.0f;
    pencil.pressureSensitivity = true;
    presets[pencil.name] = pencil;

    BrushPreset ink;
    ink.name = "Ink";
    ink.type = BrushType::Ink;
    ink.size = 5.0f;
    ink.opacity = 1.0f;
    ink.hardness = 1.0f;
    ink.spacing = 0.04f;
    ink.stabilization = 0.4f;
    ink.smoothing = 0.7f;
    ink.scatter = 0.0f;
    ink.rotation = 0.0f;
    ink.pressureSensitivity = true;
    presets[ink.name] = ink;

    BrushPreset paint;
    paint.name = "Paint";
    paint.type = BrushType::Paint;
    paint.size = 12.0f;
    paint.opacity = 0.95f;
    paint.hardness = 0.85f;
    paint.spacing = 0.08f;
    paint.stabilization = 0.3f;
    paint.smoothing = 0.4f;
    paint.scatter = 0.0f;
    paint.rotation = 0.0f;
    paint.pressureSensitivity = true;
    presets[paint.name] = paint;

    BrushPreset marker;
    marker.name = "Marker";
    marker.type = BrushType::Marker;
    marker.size = 14.0f;
    marker.opacity = 0.5f;
    marker.hardness = 0.9f;
    marker.spacing = 0.06f;
    marker.stabilization = 0.2f;
    marker.smoothing = 0.3f;
    marker.scatter = 0.0f;
    marker.rotation = 45.0f;
    marker.pressureSensitivity = false;
    presets[marker.name] = marker;

    BrushPreset pixel;
    pixel.name = "Pixel Brush";
    pixel.type = BrushType::PixelBrush;
    pixel.size = 1.0f;
    pixel.opacity = 1.0f;
    pixel.hardness = 1.0f;
    pixel.spacing = 1.0f;
    pixel.stabilization = 0.0f;
    pixel.smoothing = 0.0f;
    pixel.scatter = 0.0f;
    pixel.rotation = 0.0f;
    pixel.pressureSensitivity = false;
    presets[pixel.name] = pixel;

    BrushPreset eraser;
    eraser.name = "Eraser";
    eraser.type = BrushType::Pencil;
    eraser.size = 20.0f;
    eraser.opacity = 1.0f;
    eraser.hardness = 1.0f;
    eraser.spacing = 0.05f;
    eraser.stabilization = 0.1f;
    eraser.smoothing = 0.2f;
    eraser.scatter = 0.0f;
    eraser.rotation = 0.0f;
    eraser.pressureSensitivity = false;
    presets[eraser.name] = eraser;

    selectPreset("Pencil");
}

void BrushManager::addPreset(const BrushPreset& preset) {
    presets[preset.name] = preset;
}

void BrushManager::selectPreset(const std::string& name) {
    if (presets.find(name) != presets.end()) {
        activePresetName = name;
    }
}

BrushPreset& BrushManager::getActivePreset() {
    return presets[activePresetName];
}

const BrushPreset& BrushManager::getActivePreset() const {
    return presets.at(activePresetName);
}

void BrushManager::setBrushSize(float size) { presets[activePresetName].size = std::max(1.0f, size); }
void BrushManager::setBrushOpacity(float opacity) { presets[activePresetName].opacity = std::clamp(opacity, 0.0f, 1.0f); }
void BrushManager::setBrushHardness(float hardness) { presets[activePresetName].hardness = std::clamp(hardness, 0.0f, 1.0f); }
void BrushManager::setBrushSpacing(float spacing) { presets[activePresetName].spacing = std::max(0.01f, spacing); }
void BrushManager::setBrushRotation(float rotation) { presets[activePresetName].rotation = rotation; }
void BrushManager::setBrushScatter(float scatter) { presets[activePresetName].scatter = std::max(0.0f, scatter); }
void BrushManager::setStabilization(float value) { presets[activePresetName].stabilization = std::clamp(value, 0.0f, 1.0f); }
void BrushManager::setSmoothing(float value) { presets[activePresetName].smoothing = std::clamp(value, 0.0f, 1.0f); }

void BrushManager::resetStroke(sf::Vector2f startPos) {
    m_prevPoint = startPos;
    m_prevMidPoint = startPos;
    m_hasStartedStroke = true;
}

void BrushManager::appendCap(sf::RenderTexture* targetTex, sf::Vector2f center, float radius, sf::Color color, float opacity) {
    if (!targetTex) return;

    sf::Color col = color;
    col.a = static_cast<sf::Uint8>(std::clamp(opacity * 255.f, 0.f, 255.f));

    sf::CircleShape cap(radius, 24);
    cap.setOrigin(radius, radius);
    cap.setPosition(center);
    cap.setFillColor(col);
    targetTex->draw(cap);
}

void BrushManager::appendRibbonSegment(sf::RenderTexture* targetTex, sf::Vector2f p1, sf::Vector2f p2, float radius, sf::Color color, float opacity) {
    if (!targetTex) return;

    sf::Vector2f dir = p2 - p1;
    float len = std::hypot(dir.x, dir.y);
    if (len < 0.001f) return;

    sf::Vector2f normal(-dir.y / len * radius, dir.x / len * radius);

    sf::Color col = color;
    col.a = static_cast<sf::Uint8>(std::clamp(opacity * 255.f, 0.f, 255.f));

    sf::VertexArray va(sf::Triangles, 6);
    sf::Vector2f a = p1 + normal;
    sf::Vector2f b = p1 - normal;
    sf::Vector2f c = p2 + normal;
    sf::Vector2f d = p2 - normal;

    va[0] = sf::Vertex(a, col);
    va[1] = sf::Vertex(b, col);
    va[2] = sf::Vertex(c, col);

    va[3] = sf::Vertex(b, col);
    va[4] = sf::Vertex(d, col);
    va[5] = sf::Vertex(c, col);

    targetTex->draw(va);

    sf::CircleShape joint(radius, 16);
    joint.setOrigin(radius, radius);
    joint.setPosition(p2);
    joint.setFillColor(col);
    targetTex->draw(joint);
}

void BrushManager::paintStroke(sf::RenderTexture* targetTex, sf::Vector2f targetPos, sf::Color color, float pressure) {
    if (!targetTex) return;

    BrushPreset& act = getActivePreset();
    float strokeSize = std::max(1.0f, act.size * (act.pressureSensitivity ? pressure : 1.0f));
    float radius = strokeSize * 0.5f;

    if (!m_hasStartedStroke) {
        resetStroke(targetPos);
        appendCap(targetTex, targetPos, radius, color, act.opacity);
        return;
    }

    float dx = targetPos.x - m_prevPoint.x;
    float dy = targetPos.y - m_prevPoint.y;
    if (dx * dx + dy * dy < 1.0f) {
        return;
    }

    sf::Vector2f midPoint = (m_prevPoint + targetPos) * 0.5f;

    const int segments = 6;
    sf::Vector2f lastP = m_prevMidPoint;

    for (int i = 1; i <= segments; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(segments);
        float invT = 1.0f - t;
        sf::Vector2f curveP = (invT * invT * m_prevMidPoint) + (2.0f * invT * t * m_prevPoint) + (t * t * midPoint);

        appendRibbonSegment(targetTex, lastP, curveP, radius, color, act.opacity);
        lastP = curveP;
    }

    m_prevPoint = targetPos;
    m_prevMidPoint = midPoint;
}

void BrushManager::endStroke(sf::RenderTexture* targetTex, sf::Color color) {
    if (!m_hasStartedStroke || !targetTex) return;

    BrushPreset& act = getActivePreset();
    float radius = std::max(1.0f, act.size) * 0.5f;
    appendRibbonSegment(targetTex, m_prevMidPoint, m_prevPoint, radius, color, act.opacity);
    appendCap(targetTex, m_prevPoint, radius, color, act.opacity);
    m_hasStartedStroke = false;
}

void BrushManager::drawPreviewCursor(sf::RenderWindow& window, sf::Vector2f mousePos, sf::Color color, float scale) {
    float s = getActivePreset().size * scale;
    sf::CircleShape cursor(s / 2.0f);
    cursor.setOrigin(s / 2.0f, s / 2.0f);
    cursor.setPosition(mousePos);
    cursor.setFillColor(sf::Color::Transparent);
    cursor.setOutlineColor(color);
    cursor.setOutlineThickness(1.0f);
    window.draw(cursor);
}