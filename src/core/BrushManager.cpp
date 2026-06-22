#include "BrushManager.h"
#include <cmath>

BrushManager::BrushManager() {
    initDefaultPresets();
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
    presets[pencil.name] = pencil;

    BrushPreset ink;
    ink.name = "Ink";
    ink.type = BrushType::Ink;
    ink.size = 5.0f;
    ink.opacity = 1.0f;
    ink.hardness = 1.0f;
    ink.spacing = 0.05f;
    ink.stabilization = 0.5f;
    ink.smoothing = 0.6f;
    ink.scatter = 0.0f;
    ink.rotation = 0.0f;
    ink.pressureSensitivity = true;
    presets[ink.name] = ink;

    BrushPreset paint;
    paint.name = "Paint";
    paint.type = BrushType::Paint;
    paint.size = 12.0f;
    paint.opacity = 0.9f;
    paint.hardness = 0.5f;
    paint.spacing = 0.15f;
    paint.stabilization = 0.3f;
    paint.smoothing = 0.4f;
    paint.scatter = 0.05f;
    paint.rotation = 45.0f;
    paint.pressureSensitivity = true;
    presets[paint.name] = paint;

    BrushPreset marker;
    marker.name = "Marker";
    marker.type = BrushType::Marker;
    marker.size = 8.0f;
    marker.opacity = 0.4f;
    marker.hardness = 0.8f;
    marker.spacing = 0.1f;
    marker.stabilization = 0.2f;
    marker.smoothing = 0.3f;
    marker.scatter = 0.0f;
    marker.rotation = 90.0f;
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

    BrushPreset watercolor;
    watercolor.name = "Watercolor";
    watercolor.type = BrushType::Watercolor;
    watercolor.size = 20.0f;
    watercolor.opacity = 0.2f;
    watercolor.hardness = 0.1f;
    watercolor.spacing = 0.3f;
    watercolor.stabilization = 0.4f;
    watercolor.smoothing = 0.8f;
    watercolor.scatter = 0.15f;
    watercolor.rotation = 12.0f;
    watercolor.pressureSensitivity = true;
    presets[watercolor.name] = watercolor;

    activePreset = pencil;
    lastStabilizedPosition = sf::Vector2f(0.0f, 0.0f);
}

void BrushManager::addPreset(const BrushPreset& preset) {
    presets[preset.name] = preset;
}

void BrushManager::selectPreset(const std::string& name) {
    if (presets.find(name) != presets.end()) {
        activePreset = presets[name];
    }
}

BrushPreset& BrushManager::getActivePreset() {
    return activePreset;
}

const BrushPreset& BrushManager::getActivePreset() const {
    return activePreset;
}

void BrushManager::setBrushSize(float size) {
    activePreset.size = std::max(1.0f, size);
}

void BrushManager::setBrushOpacity(float opacity) {
    activePreset.opacity = std::max(0.0f, std::min(opacity, 1.0f));
}

void BrushManager::setBrushHardness(float hardness) {
    activePreset.hardness = std::max(0.0f, std::min(hardness, 1.0f));
}

void BrushManager::setBrushSpacing(float spacing) {
    activePreset.spacing = std::max(0.01f, spacing);
}

void BrushManager::setBrushRotation(float rotation) {
    activePreset.rotation = rotation;
}

void BrushManager::setBrushScatter(float scatter) {
    activePreset.scatter = std::max(0.0f, scatter);
}

void BrushManager::setStabilization(float value) {
    activePreset.stabilization = std::max(0.0f, std::min(value, 1.0f));
}

void BrushManager::setSmoothing(float value) {
    activePreset.smoothing = std::max(0.0f, std::min(value, 1.0f));
}

sf::Vector2f BrushManager::applyStabilization(sf::Vector2f currentTarget, float strength) {
    if (strength <= 0.0f) {
        lastStabilizedPosition = currentTarget;
        return currentTarget;
    }
    sf::Vector2f diff = currentTarget - lastStabilizedPosition;
    lastStabilizedPosition += diff * (1.0f - strength);
    return lastStabilizedPosition;
}

void BrushManager::paintStroke(sf::Image& layerImage, sf::Vector2f start, sf::Vector2f end, sf::Color color, float pressure) {
    float finalSize = activePreset.size;
    if (activePreset.pressureSensitivity) {
        finalSize *= pressure;
    }
    if (finalSize < 1.0f) {
        finalSize = 1.0f;
    }

    float finalOpacity = activePreset.opacity;
    sf::Color drawColor = color;
    drawColor.a = static_cast<sf::Uint8>(finalOpacity * 255.0f);

    sf::Vector2i imgSize = sf::Vector2i(layerImage.getSize().x, layerImage.getSize().y);
    float dx = end.x - start.x;
    float dy = end.y - start.y;
    float distance = std::sqrt(dx * dx + dy * dy);
    int steps = static_cast<int>(distance / (finalSize * activePreset.spacing));
    if (steps < 1) steps = 1;

    for (int i = 0; i <= steps; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(steps);
        sf::Vector2f pt = start + (end - start) * t;

        if (activePreset.scatter > 0.0f) {
            float angle = static_cast<float>(rand() % 360) * 3.14159f / 180.0f;
            float offset = static_cast<float>(rand() % 100) / 100.0f * activePreset.scatter * finalSize;
            pt.x += std::cos(angle) * offset;
            pt.y += std::sin(angle) * offset;
        }

        int radius = static_cast<int>(finalSize / 2.0f);
        if (activePreset.type == BrushType::PixelBrush || radius < 1) {
            int px = static_cast<int>(std::round(pt.x));
            int py = static_cast<int>(std::round(pt.y));
            if (px >= 0 && px < imgSize.x && py >= 0 && py < imgSize.y) {
                layerImage.setPixel(px, py, drawColor);
            }
            continue;
        }

        for (int h = -radius; h <= radius; ++h) {
            for (int w = -radius; w <= radius; ++w) {
                int px = static_cast<int>(pt.x) + w;
                int py = static_cast<int>(pt.y) + h;

                if (px >= 0 && px < imgSize.x && py >= 0 && py < imgSize.y) {
                    float dist = std::sqrt(static_cast<float>(w * w + h * h));
                    if (dist <= radius) {
                        float factor = 1.0f;
                        if (activePreset.hardness < 1.0f) {
                            float edgeStart = radius * activePreset.hardness;
                            if (dist > edgeStart) {
                                factor = 1.0f - ((dist - edgeStart) / (radius - edgeStart));
                            }
                        }
                        sf::Color originalColor = layerImage.getPixel(px, py);
                        sf::Uint8 blendAlpha = static_cast<sf::Uint8>(drawColor.a * factor);

                        if (activePreset.type == BrushType::Watercolor) {
                            blendAlpha = static_cast<sf::Uint8>(blendAlpha * 0.3f);
                        }

                        float outA = blendAlpha + originalColor.a * (1.0f - blendAlpha / 255.0f);
                        if (outA > 0.0f) {
                            sf::Uint8 r = static_cast<sf::Uint8>((drawColor.r * blendAlpha + originalColor.r * originalColor.a * (1.0f - blendAlpha / 255.0f)) / outA);
                            sf::Uint8 g = static_cast<sf::Uint8>((drawColor.g * blendAlpha + originalColor.g * originalColor.a * (1.0f - blendAlpha / 255.0f)) / outA);
                            sf::Uint8 b = static_cast<sf::Uint8>((drawColor.b * blendAlpha + originalColor.b * originalColor.a * (1.0f - blendAlpha / 255.0f)) / outA);
                            layerImage.setPixel(px, py, sf::Color(r, g, b, static_cast<sf::Uint8>(std::min(255.0f, outA))));
                        }
                    }
                }
            }
        }
    }
}

void BrushManager::updatePreviewCursor(sf::RenderWindow& window, sf::Vector2f mousePos, sf::Color color) {
    sf::CircleShape cursor(activePreset.size / 2.0f);
    cursor.setOrigin(activePreset.size / 2.0f, activePreset.size / 2.0f);
    cursor.setPosition(mousePos);
    cursor.setFillColor(sf::Color::Transparent);
    cursor.setOutlineColor(color);
    cursor.setOutlineThickness(1.0f);
    window.draw(cursor);
}