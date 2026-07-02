#include "ExportManager.h"
#include <iostream>
#include <cmath>
#include <algorithm>

sf::Image ExportManager::flattenFrame(Canvas& canvas, int frameIndex) {
    sf::Vector2u size = canvas.getCanvasSize();
    sf::RenderTexture renderTex;
    renderTex.create(size.x, size.y);
    renderTex.clear(sf::Color::Transparent);

    const Frame* frame = canvas.getFrameReadOnly(frameIndex);
    if (frame) {
        for (const auto& layer : frame->layers) {
            if (layer.visible) {
                sf::Sprite spr(layer.texture->getTexture());
                spr.setColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(255.0f * layer.opacity)));

                sf::RenderStates states;
                if (layer.blendMode == BlendMode::Multiply) states.blendMode = sf::BlendMultiply;
                else if (layer.blendMode == BlendMode::Additive) states.blendMode = sf::BlendAdd;
                else if (layer.blendMode == BlendMode::Screen) states.blendMode = sf::BlendMode(sf::BlendMode::One, sf::BlendMode::OneMinusSrcColor, sf::BlendMode::Add);
                else states.blendMode = sf::BlendAlpha;

                renderTex.draw(spr, states);
            }
        }
    }

    renderTex.display();
    return renderTex.getTexture().copyToImage();
}

sf::IntRect ExportManager::calculateAutoCrop(const sf::Image& img) {
    int minX = static_cast<int>(img.getSize().x);
    int minY = static_cast<int>(img.getSize().y);
    int maxX = -1;
    int maxY = -1;
    bool found = false;

    for (unsigned int y = 0; y < img.getSize().y; ++y) {
        for (unsigned int x = 0; x < img.getSize().x; ++x) {
            if (img.getPixel(x, y).a > 5) {
                if (static_cast<int>(x) < minX) minX = static_cast<int>(x);
                if (static_cast<int>(y) < minY) minY = static_cast<int>(y);
                if (static_cast<int>(x) > maxX) maxX = static_cast<int>(x);
                if (static_cast<int>(y) > maxY) maxY = static_cast<int>(y);
                found = true;
            }
        }
    }

    if (!found) return sf::IntRect(0, 0, img.getSize().x, img.getSize().y);
    return sf::IntRect(minX, minY, maxX - minX + 1, maxY - minY + 1);
}

sf::Image ExportManager::applyCropAndBackground(const sf::Image& img, sf::IntRect cropRect, bool transparentBg) {
    if (cropRect.width <= 0 || cropRect.height <= 0) cropRect = sf::IntRect(0, 0, img.getSize().x, img.getSize().y);

    if (cropRect.left < 0) cropRect.left = 0;
    if (cropRect.top < 0) cropRect.top = 0;
    if (cropRect.left + cropRect.width > static_cast<int>(img.getSize().x)) cropRect.width = img.getSize().x - cropRect.left;
    if (cropRect.top + cropRect.height > static_cast<int>(img.getSize().y)) cropRect.height = img.getSize().y - cropRect.top;

    sf::Image result;
    result.create(cropRect.width, cropRect.height, transparentBg ? sf::Color::Transparent : sf::Color::White);

    for (unsigned int y = 0; y < static_cast<unsigned int>(cropRect.height); ++y) {
        for (unsigned int x = 0; x < static_cast<unsigned int>(cropRect.width); ++x) {
            sf::Color px = img.getPixel(cropRect.left + x, cropRect.top + y);
            if (!transparentBg) {
                float a = px.a / 255.0f;
                px.r = static_cast<sf::Uint8>(px.r * a + 255 * (1.0f - a));
                px.g = static_cast<sf::Uint8>(px.g * a + 255 * (1.0f - a));
                px.b = static_cast<sf::Uint8>(px.b * a + 255 * (1.0f - a));
                px.a = 255;
            }
            if (px.a > 0 || !transparentBg) {
                result.setPixel(x, y, px);
            }
        }
    }
    return result;
}

bool ExportManager::exportSingleImage(Canvas& canvas, int currentFrame, const std::string& filepath, bool transparentBg, bool autoCrop) {
    sf::Image img = flattenFrame(canvas, currentFrame);
    sf::IntRect crop = autoCrop ? calculateAutoCrop(img) : sf::IntRect(0, 0, img.getSize().x, img.getSize().y);
    img = applyCropAndBackground(img, crop, transparentBg);
    return img.saveToFile(filepath);
}

bool ExportManager::exportPNGSequence(Canvas& canvas, const std::string& directoryPath, bool transparentBg, bool autoCrop) {
    size_t frameCount = canvas.getFrameCount();
    if (frameCount == 0) return false;

    std::vector<sf::Image> frames;
    int masterMinX = 999999;
    int masterMinY = 999999;
    int masterMaxX = -1;
    int masterMaxY = -1;

    for (size_t i = 0; i < frameCount; ++i) {
        sf::Image img = flattenFrame(canvas, static_cast<int>(i));
        frames.push_back(img);

        if (autoCrop) {
            sf::IntRect crop = calculateAutoCrop(img);
            if (crop.width > 0 && crop.height > 0) {
                masterMinX = std::min(masterMinX, crop.left);
                masterMinY = std::min(masterMinY, crop.top);
                masterMaxX = std::max(masterMaxX, crop.left + crop.width - 1);
                masterMaxY = std::max(masterMaxY, crop.top + crop.height - 1);
            }
        }
    }

    sf::IntRect activeCrop;
    if (autoCrop && masterMaxX != -1) {
        activeCrop = sf::IntRect(masterMinX, masterMinY, masterMaxX - masterMinX + 1, masterMaxY - masterMinY + 1);
    }
    else {
        activeCrop = sf::IntRect(0, 0, frames[0].getSize().x, frames[0].getSize().y);
    }

    for (size_t i = 0; i < frameCount; ++i) {
        sf::Image finalImg = applyCropAndBackground(frames[i], activeCrop, transparentBg);
        std::string path = directoryPath + "/frame_" + std::to_string(i) + ".png";
        finalImg.saveToFile(path);
    }
    return true;
}

bool ExportManager::exportSpriteSheet(Canvas& canvas, const std::string& filepath, int columns, bool transparentBg, bool autoCrop) {
    size_t frameCount = canvas.getFrameCount();
    if (frameCount == 0 || columns <= 0) return false;

    std::vector<sf::Image> frames;
    int masterMinX = 999999;
    int masterMinY = 999999;
    int masterMaxX = -1;
    int masterMaxY = -1;

    for (size_t i = 0; i < frameCount; ++i) {
        sf::Image img = flattenFrame(canvas, static_cast<int>(i));
        frames.push_back(img);

        if (autoCrop) {
            sf::IntRect crop = calculateAutoCrop(img);
            if (crop.width > 0 && crop.height > 0) {
                masterMinX = std::min(masterMinX, crop.left);
                masterMinY = std::min(masterMinY, crop.top);
                masterMaxX = std::max(masterMaxX, crop.left + crop.width - 1);
                masterMaxY = std::max(masterMaxY, crop.top + crop.height - 1);
            }
        }
    }

    sf::IntRect activeCrop;
    if (autoCrop && masterMaxX != -1) {
        activeCrop = sf::IntRect(masterMinX, masterMinY, masterMaxX - masterMinX + 1, masterMaxY - masterMinY + 1);
    }
    else {
        activeCrop = sf::IntRect(0, 0, frames[0].getSize().x, frames[0].getSize().y);
    }

    int rows = static_cast<int>(std::ceil(static_cast<float>(frameCount) / static_cast<float>(columns)));
    unsigned int fw = activeCrop.width;
    unsigned int fh = activeCrop.height;

    sf::Image spriteSheet;
    spriteSheet.create(fw * columns, fh * rows, transparentBg ? sf::Color::Transparent : sf::Color::White);

    for (size_t i = 0; i < frameCount; ++i) {
        sf::Image cropped = applyCropAndBackground(frames[i], activeCrop, transparentBg);
        int col = static_cast<int>(i) % columns;
        int row = static_cast<int>(i) / columns;
        spriteSheet.copy(cropped, col * fw, row * fh, sf::IntRect(0, 0, fw, fh), true);
    }

    return spriteSheet.saveToFile(filepath);
}