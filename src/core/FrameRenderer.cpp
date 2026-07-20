#include "FrameRenderer.h"
#include <iostream>

FrameRenderer::FrameRenderer() : useFallback(true), frameThickness(24.f), fallbackColor(sf::Color(45, 35, 25)) {}

void FrameRenderer::loadTheme(const std::string& themeName) {
    if (themeName == "Fallback") {
        useFallback = true;
        return;
    }

    std::string basePath = "assets/themes/" + themeName + "/";
    const std::string files[8] = {
        "top_left.png", "top.png", "top_right.png",
        "left.png", "right.png",
        "bottom_left.png", "bottom.png", "bottom_right.png"
    };

    useFallback = false;
    for (int i = 0; i < 8; ++i) {
        if (!textures[i].loadFromFile(basePath + files[i])) {
            useFallback = true;
            break;
        }
        textures[i].setRepeated(true);
        sprites[i].setTexture(textures[i]);
    }
}

void FrameRenderer::draw(sf::RenderWindow& window, const sf::FloatRect& bounds, const sf::Transform& transform) {
    sf::RenderStates states;
    states.transform = transform;

    if (useFallback) {
        sf::RectangleShape topEdge({ bounds.width + 2 * frameThickness, frameThickness });
        topEdge.setPosition(bounds.left - frameThickness, bounds.top - frameThickness);
        topEdge.setFillColor(fallbackColor);

        sf::RectangleShape bottomEdge({ bounds.width + 2 * frameThickness, frameThickness });
        bottomEdge.setPosition(bounds.left - frameThickness, bounds.top + bounds.height);
        bottomEdge.setFillColor(fallbackColor);

        sf::RectangleShape leftEdge({ frameThickness, bounds.height });
        leftEdge.setPosition(bounds.left - frameThickness, bounds.top);
        leftEdge.setFillColor(fallbackColor);

        sf::RectangleShape rightEdge({ frameThickness, bounds.height });
        rightEdge.setPosition(bounds.left + bounds.width, bounds.top);
        rightEdge.setFillColor(fallbackColor);

        sf::RectangleShape innerBorder({ bounds.width, bounds.height });
        innerBorder.setPosition(bounds.left, bounds.top);
        innerBorder.setFillColor(sf::Color::Transparent);
        innerBorder.setOutlineThickness(2.f);
        innerBorder.setOutlineColor(sf::Color(20, 15, 10));

        window.draw(topEdge, states);
        window.draw(bottomEdge, states);
        window.draw(leftEdge, states);
        window.draw(rightEdge, states);
        window.draw(innerBorder, states);
    }
    else {
        // Architecture prepared for PNGs. 
        // Once assets exist, calculate TextureRects and corner offsets based on 'bounds'
    }
}