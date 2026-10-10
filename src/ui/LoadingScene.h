#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

// The little sea scene shown while the assistant works on a request.
// The boat follows the mouse, clicking the sky drops a star to catch and clicking the water wakes a fish.
class LoadingScene {
public:
    void reset();
    void setTitle(const std::string& text) { title = text; }
    void update(float dt, sf::Vector2f mouse);
    // Returns true when the click landed on Cancel
    bool handleClick(sf::Vector2f mouse);
    void draw(sf::RenderWindow& window, const sf::Font& font, sf::Vector2f mouse);

private:
    struct Star { float x, y, vy; bool floating; };
    struct Fish { sf::Vector2f pos, vel; };
    struct Ripple { float x, y, age; };
    struct Particle { sf::Vector2f pos, vel; float life, maxLife, size; sf::Color color; };

    float rand01();
    float waveY(float x, float base, float amp, float speed, float phase) const;
    float boatSurface(float x) const;
    void splash(float x, float y, int count, sf::Color color);
    void drawWaveLayer(sf::RenderWindow& window, float base, float amp, float speed, float phase, sf::Color top, sf::Color bottom, sf::Color foam) const;
    void drawBoat(sf::RenderWindow& window) const;

    std::string title = "Sketching it out";
    float time = 0.f;
    float boatX = 960.f;
    float boatVel = 0.f;
    float boatDir = 1.f;
    float starTimer = 2.f;
    float wakeTimer = 0.f;
    int starsCaught = 0;
    unsigned int seed = 7u;

    std::vector<Star> stars;
    std::vector<Fish> fish;
    std::vector<Ripple> ripples;
    std::vector<Particle> particles;
};
