#include "ExportManager.h"
#include <iostream>
#include <cmath>
#include <algorithm>

// Export resolution multiplier. Strokes are retained geometry, so raising this
// re-rasterizes them larger instead of upscaling a bitmap — 2 or 4 costs almost
// nothing in quality. Set to 1 for exact canvas-size output.
static const unsigned int EXPORT_SCALE = 1;

sf::Image ExportManager::flattenFrame(Canvas& canvas, int frameIndex) {
    // CHANGED: was compositing only layer.texture, which meant every vector
    // stroke was missing from exported images. Canvas::flattenFrameToImage
    // draws the raster and the strokes together, with blend modes and opacity.
    return canvas.flattenFrameToImage(frameIndex, EXPORT_SCALE);
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

sf::Image ExportManager::createHorizontalSpriteStrip(Canvas& canvas, bool transparentBg) {
    size_t count = canvas.getFrameCount();
    if (count == 0) return sf::Image();

    sf::Image first = flattenFrame(canvas, 0);
    unsigned int fw = first.getSize().x;
    unsigned int fh = first.getSize().y;

    sf::Image strip;
    strip.create(fw * static_cast<unsigned int>(count), fh, transparentBg ? sf::Color(0, 0, 0, 0) : sf::Color::White);

    for (size_t i = 0; i < count; ++i) {
        sf::Image frame = (i == 0) ? first : flattenFrame(canvas, static_cast<int>(i));
        if (!transparentBg) {
            sf::IntRect bounds(0, 0, fw, fh);
            frame = applyCropAndBackground(frame, bounds, false);
        }
        strip.copy(frame, static_cast<unsigned int>(i * fw), 0, sf::IntRect(0, 0, fw, fh), false);
    }
    return strip;
}