#pragma once
#ifndef AIHELPER_H
#define AIHELPER_H

#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <string>

class AIHelper {
private:
    sf::CircleShape mascot;
    bool active;

    std::string determineMood(int r, int b, int g, int blk, int total) const;

public:
    AIHelper();
    void toggle();
    bool isActive() const;
    void analyzeFrame(const sf::RenderTexture& frame);
    void draw(sf::RenderWindow& window);
    sf::FloatRect getBounds() const;
};

#endif