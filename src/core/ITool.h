#pragma once
#include <SFML/Graphics.hpp>

class ITool {
public:
    virtual ~ITool() = default;

    virtual void Initialize() = 0;
    virtual void HandleEvent(const sf::Event& event, const sf::RenderWindow& window) = 0;
    virtual void Update(float deltaTime, const sf::RenderWindow& window) = 0;
    virtual void Render(sf::RenderWindow& window) = 0;
    virtual void SetBounds(const sf::FloatRect& bounds) = 0;
};