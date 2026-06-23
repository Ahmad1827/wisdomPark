#include "ExportManager.h"
#include <iostream>
#include <cmath>

bool ExportManager::exportPNGSequence(Canvas& canvas, const std::string& directoryPath) {
    int w = canvas.getCanvasSize().x;
    int h = canvas.getCanvasSize().y;

    for (size_t i = 0; i < canvas.getFrameCount(); ++i) {
        sf::RenderTexture merged;
        if (!merged.create(w, h)) return false;
        merged.clear(sf::Color::Transparent);

        const Frame* frame = canvas.getFrameReadOnly(static_cast<int>(i));
        if (!frame) continue;

        for (const auto& layer : frame->layers) {
            if (layer.visible) {
                sf::Sprite spr(layer.texture->getTexture());
                spr.setColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(layer.opacity * 255.0f)));
                merged.draw(spr);
            }
        }
        merged.display();

        std::string path = directoryPath + "/frame_" + std::to_string(i) + ".png";
        if (!merged.getTexture().copyToImage().saveToFile(path)) {
            std::cerr << "Failed to save frame " << i << " to " << path << "\n";
        }
    }
    return true;
}

bool ExportManager::exportSpriteSheet(Canvas& canvas, const std::string& filepath, int columns) {
    int frameCount = static_cast<int>(canvas.getFrameCount());
    if (frameCount == 0 || columns <= 0) return false;

    int rows = static_cast<int>(std::ceil(static_cast<float>(frameCount) / static_cast<float>(columns)));
    int w = canvas.getCanvasSize().x;
    int h = canvas.getCanvasSize().y;

    sf::RenderTexture sheet;
    if (!sheet.create(w * columns, h * rows)) return false;
    sheet.clear(sf::Color::Transparent);

    for (int i = 0; i < frameCount; ++i) {
        sf::RenderTexture merged;
        merged.create(w, h);
        merged.clear(sf::Color::Transparent);

        const Frame* frame = canvas.getFrameReadOnly(i);
        if (frame) {
            for (const auto& layer : frame->layers) {
                if (layer.visible) {
                    sf::Sprite spr(layer.texture->getTexture());
                    spr.setColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(layer.opacity * 255.0f)));
                    merged.draw(spr);
                }
            }
        }
        merged.display();

        int col = i % columns;
        int row = i / columns;

        sf::Sprite frameSpr(merged.getTexture());
        frameSpr.setPosition(static_cast<float>(col * w), static_cast<float>(row * h));
        sheet.draw(frameSpr);
    }

    sheet.display();
    return sheet.getTexture().copyToImage().saveToFile(filepath);
}