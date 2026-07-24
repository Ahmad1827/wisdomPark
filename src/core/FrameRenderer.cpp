#include "FrameRenderer.h"
#include <cmath>
#include <iostream>

FrameRenderer::FrameRenderer() : useFallback(true), frameThickness(16.f), fallbackColor(sf::Color(45, 35, 25)) {}

void FrameRenderer::loadTheme(const std::string& directory) {
    const std::string files[8] = {
        "top_left.png", "top.png", "top_right.png",
        "left.png", "right.png",
        "bottom_left.png", "bottom.png", "bottom_right.png"
    };

    useFallback = false;
    for (int i = 0; i < 8; ++i) {
        if (!textures[i].loadFromFile(directory + "/" + files[i])) {
            std::cout << "Missing frame asset: " << files[i] << " - Using fallback.\n";
            useFallback = true;
            return;
        }

        // Pixel Perfect Rule: Disable smoothing/filtering
        textures[i].setSmooth(false);

        // Tiling Rule: Only straight edges tile (top, left, right, bottom)
        if (i == 1 || i == 3 || i == 4 || i == 6) {
            textures[i].setRepeated(true);
        }
        sprites[i].setTexture(textures[i]);
    }
}

sf::FloatRect FrameRenderer::assembleFrame(float canvasWidth, float canvasHeight) {
    float cW = std::round(canvasWidth);
    float cH = std::round(canvasHeight);

    if (useFallback) {
        innerBounds = sf::FloatRect(frameThickness, frameThickness, cW, cH);
        return innerBounds;
    }

    // Explicitly use actual unscaled PNG dimensions
    float tlW = std::round(textures[0].getSize().x);
    float tlH = std::round(textures[0].getSize().y);
    float tH = std::round(textures[1].getSize().y);
    float trW = std::round(textures[2].getSize().x);
    float trH = std::round(textures[2].getSize().y);

    float lW = std::round(textures[3].getSize().x);
    float rW = std::round(textures[4].getSize().x);

    float blW = std::round(textures[5].getSize().x);
    float bH = std::round(textures[6].getSize().y);
    float brW = std::round(textures[7].getSize().x);

    // Set the anchor point for the Canvas drawing space
    float cX = tlW;
    float cY = tlH;

    // --- BORDERS ---
    // Span exactly the width/height of the canvas. No stretching.
    sprites[1].setPosition(cX, cY - tH);
    sprites[1].setTextureRect(sf::IntRect(0, 0, static_cast<int>(cW), static_cast<int>(tH)));

    sprites[6].setPosition(cX, cY + cH);
    sprites[6].setTextureRect(sf::IntRect(0, 0, static_cast<int>(cW), static_cast<int>(bH)));

    sprites[3].setPosition(cX - lW, cY);
    sprites[3].setTextureRect(sf::IntRect(0, 0, static_cast<int>(lW), static_cast<int>(cH)));

    sprites[4].setPosition(cX + cW, cY);
    sprites[4].setTextureRect(sf::IntRect(0, 0, static_cast<int>(rW), static_cast<int>(cH)));

    // --- CORNERS ---
    // Placed strictly adjacent to the canvas corners.
    // Because TL is naturally 1px wider/taller, cX - tlW forces it exactly 1px left of LEFT and 1px up of TOP.
    sprites[0].setPosition(cX - tlW, cY - tlH); // TL
    sprites[2].setPosition(cX + cW, cY - trH);  // TR
    sprites[5].setPosition(cX - blW, cY + cH);  // BL
    sprites[7].setPosition(cX + cW, cY + cH);   // BR

    innerBounds = sf::FloatRect(cX, cY, cW, cH);
    return innerBounds;
}

void FrameRenderer::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    if (useFallback) {
        float t = frameThickness;
        float cW = innerBounds.width;
        float cH = innerBounds.height;
        float outerW = cW + 2.f * t;
        float outerH = cH + 2.f * t;

        sf::RectangleShape topEdge(sf::Vector2f(outerW, t));
        topEdge.setPosition(0.f, 0.f); topEdge.setFillColor(fallbackColor);
        sf::RectangleShape bottomEdge(sf::Vector2f(outerW, t));
        bottomEdge.setPosition(0.f, outerH - t); bottomEdge.setFillColor(fallbackColor);
        sf::RectangleShape leftEdge(sf::Vector2f(t, cH));
        leftEdge.setPosition(0.f, t); leftEdge.setFillColor(fallbackColor);
        sf::RectangleShape rightEdge(sf::Vector2f(t, cH));
        rightEdge.setPosition(outerW - t, t); rightEdge.setFillColor(fallbackColor);

        sf::RectangleShape innerBorder(sf::Vector2f(cW, cH));
        innerBorder.setPosition(t, t);
        innerBorder.setFillColor(sf::Color::Transparent);
        innerBorder.setOutlineThickness(1.5f);
        innerBorder.setOutlineColor(sf::Color(20, 15, 10));

        target.draw(topEdge, states); target.draw(bottomEdge, states);
        target.draw(leftEdge, states); target.draw(rightEdge, states);
        target.draw(innerBorder, states);
    }
    else {
        for (int i = 0; i < 8; ++i) {
            target.draw(sprites[i], states);
        }
    }
}