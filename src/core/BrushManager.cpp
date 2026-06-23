#include "BrushManager.h"
#include <cmath>

BrushManager::BrushManager() : distanceAccumulator(0.0f) {
    initDefaultPresets();
}

sf::Texture BrushManager::generateBrushTexture(float hardness, int size) {
    sf::Image img;
    img.create(size, size, sf::Color::Transparent);
    float radius = size / 2.0f;
    float center = radius;

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float dx = x - center + 0.5f;
            float dy = y - center + 0.5f;
            float dist = std::sqrt(dx * dx + dy * dy);

            if (dist <= radius) {
                float alpha = 1.0f;
                if (hardness < 1.0f) {
                    float coreRadius = radius * hardness;
                    if (dist > coreRadius) {
                        alpha = 1.0f - ((dist - coreRadius) / (radius - coreRadius));
                    }
                }
                img.setPixel(x, y, sf::Color(255, 255, 255, static_cast<sf::Uint8>(alpha * 255.0f)));
            }
        }
    }
    sf::Texture tex;
    tex.loadFromImage(img);
    return tex;
}

void BrushManager::initDefaultPresets() {
    BrushPreset pencil;
    pencil.name = "Pencil";
    pencil.type = BrushType::Pencil;
    pencil.size = 2.0f;
    pencil.opacity = 0.8f;
    pencil.hardness = 0.9f;
    pencil.spacing = 0.1f;
    pencil.stabilization = 0.1f;
    pencil.smoothing = 0.2f;
    pencil.scatter = 0.0f;
    pencil.rotation = 0.0f;
    pencil.pressureSensitivity = true;
    pencil.brushTexture = generateBrushTexture(pencil.hardness, 64);
    presets[pencil.name] = pencil;

    BrushPreset ink;
    ink.name = "Ink";
    ink.type = BrushType::Ink;
    ink.size = 5.0f;
    ink.opacity = 1.0f;
    ink.hardness = 1.0f;
    ink.spacing = 0.05f;
    ink.stabilization = 0.6f;
    ink.smoothing = 0.8f;
    ink.scatter = 0.0f;
    ink.rotation = 0.0f;
    ink.pressureSensitivity = true;
    ink.brushTexture = generateBrushTexture(ink.hardness, 64);
    presets[ink.name] = ink;

    BrushPreset paint;
    paint.name = "Paint";
    paint.type = BrushType::Paint;
    paint.size = 15.0f;
    paint.opacity = 0.9f;
    paint.hardness = 0.3f;
    paint.spacing = 0.15f;
    paint.stabilization = 0.4f;
    paint.smoothing = 0.5f;
    paint.scatter = 0.05f;
    paint.rotation = 0.0f;
    paint.pressureSensitivity = true;
    paint.brushTexture = generateBrushTexture(paint.hardness, 64);
    presets[paint.name] = paint;

    BrushPreset marker;
    marker.name = "Marker";
    marker.type = BrushType::Marker;
    marker.size = 10.0f;
    marker.opacity = 0.4f;
    marker.hardness = 0.7f;
    marker.spacing = 0.1f;
    marker.stabilization = 0.3f;
    marker.smoothing = 0.3f;
    marker.scatter = 0.0f;
    marker.rotation = 45.0f;
    marker.pressureSensitivity = false;
    marker.brushTexture = generateBrushTexture(marker.hardness, 64);
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
    sf::Image pxImg; pxImg.create(1, 1, sf::Color::White);
    pixel.brushTexture.loadFromImage(pxImg);
    presets[pixel.name] = pixel;

    BrushPreset eraser;
    eraser.name = "Eraser";
    eraser.type = BrushType::Pencil;
    eraser.size = 20.0f;
    eraser.opacity = 1.0f;
    eraser.hardness = 1.0f;
    eraser.spacing = 0.1f;
    eraser.stabilization = 0.1f;
    eraser.smoothing = 0.2f;
    eraser.scatter = 0.0f;
    eraser.rotation = 0.0f;
    eraser.pressureSensitivity = false;
    eraser.brushTexture = generateBrushTexture(eraser.hardness, 64);
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
void BrushManager::setBrushOpacity(float opacity) { presets[activePresetName].opacity = std::max(0.0f, std::min(opacity, 1.0f)); }
void BrushManager::setBrushHardness(float hardness) {
    presets[activePresetName].hardness = std::max(0.0f, std::min(hardness, 1.0f));
    presets[activePresetName].brushTexture = generateBrushTexture(presets[activePresetName].hardness, 64);
}
void BrushManager::setBrushSpacing(float spacing) { presets[activePresetName].spacing = std::max(0.01f, spacing); }
void BrushManager::setBrushRotation(float rotation) { presets[activePresetName].rotation = rotation; }
void BrushManager::setBrushScatter(float scatter) { presets[activePresetName].scatter = std::max(0.0f, scatter); }
void BrushManager::setStabilization(float value) { presets[activePresetName].stabilization = std::max(0.0f, std::min(value, 1.0f)); }
void BrushManager::setSmoothing(float value) { presets[activePresetName].smoothing = std::max(0.0f, std::min(value, 1.0f)); }

void BrushManager::resetStroke(sf::Vector2f startPos) {
    lastStabilizedPos = startPos;
    lastDrawnPos = startPos;
    distanceAccumulator = 0.0f;
}

void BrushManager::applyStamp(sf::RenderTexture* targetTex, sf::Vector2f pos, float currentSize, float currentOpacity, sf::Color color) {
    BrushPreset& act = getActivePreset();

    if (act.type == BrushType::PixelBrush) {
        sf::RectangleShape rect(sf::Vector2f(currentSize, currentSize));
        rect.setOrigin(currentSize / 2.f, currentSize / 2.f);
        rect.setPosition(std::round(pos.x), std::round(pos.y));
        rect.setFillColor(color);
        targetTex->draw(rect, sf::RenderStates::Default);
        return;
    }

    sf::Sprite stamp(act.brushTexture);
    stamp.setOrigin(static_cast<float>(act.brushTexture.getSize().x) / 2.0f, static_cast<float>(act.brushTexture.getSize().y) / 2.0f);
    stamp.setPosition(pos);
    stamp.setRotation(act.rotation);

    float scale = currentSize / static_cast<float>(act.brushTexture.getSize().x);
    stamp.setScale(scale, scale);

    sf::Color stampCol = color;
    stampCol.a = static_cast<sf::Uint8>(currentOpacity * 255.0f);
    stamp.setColor(stampCol);

    sf::RenderStates rs;
    if (act.type == BrushType::Watercolor) {
        rs.blendMode = sf::BlendAlpha;
        stampCol.a = static_cast<sf::Uint8>(currentOpacity * 100.0f);
        stamp.setColor(stampCol);
    }
    else {
        rs.blendMode = sf::BlendAlpha;
    }

    targetTex->draw(stamp, rs);
}

void BrushManager::paintStroke(sf::RenderTexture* targetTex, sf::Vector2f targetPos, sf::Color color, float pressure) {
    BrushPreset& act = getActivePreset();

    sf::Vector2f diff = targetPos - lastStabilizedPos;
    if (act.stabilization > 0.0f) {
        lastStabilizedPos += diff * (1.0f - act.stabilization);
    }
    else {
        lastStabilizedPos = targetPos;
    }

    float strokeSize = act.size;
    if (act.pressureSensitivity) {
        strokeSize *= pressure;
    }
    strokeSize = std::max(1.0f, strokeSize);

    float requiredSpacing = std::max(1.0f, strokeSize * act.spacing);

    sf::Vector2f delta = lastStabilizedPos - lastDrawnPos;
    float dist = std::sqrt(delta.x * delta.x + delta.y * delta.y);

    distanceAccumulator += dist;

    while (distanceAccumulator >= requiredSpacing) {
        sf::Vector2f drawDir;
        if (dist > 0.0f) {
            drawDir = delta / dist;
        }
        else {
            drawDir = sf::Vector2f(0.0f, 0.0f);
        }

        lastDrawnPos += drawDir * requiredSpacing;
        distanceAccumulator -= requiredSpacing;

        sf::Vector2f stampPos = lastDrawnPos;

        if (act.scatter > 0.0f) {
            float angle = static_cast<float>(rand() % 360) * 3.14159f / 180.0f;
            float offset = (static_cast<float>(rand() % 100) / 100.0f) * act.scatter * strokeSize;
            stampPos.x += std::cos(angle) * offset;
            stampPos.y += std::sin(angle) * offset;
        }

        applyStamp(targetTex, stampPos, strokeSize, act.opacity, color);
    }
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