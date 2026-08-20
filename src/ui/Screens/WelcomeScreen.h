#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

struct WelcomeActionTile {
    std::string id;
    std::string title;
    std::string subtitle;
    std::string shortcutHint;
    sf::FloatRect bounds;
    bool isAccent;
    float hoverAlpha = 0.0f;
    float scale = 1.0f;
};

struct WelcomePixelEmber {
    float x;
    float y;
    float vx;
    float vy;
    float size;
    float life;
    float maxLife;
    sf::Color color;
};

class WelcomeScreen {
private:
    sf::Font font;
    std::vector<WelcomeActionTile> actionTiles;
    std::vector<WelcomePixelEmber> embers;
    std::vector<std::string> proTips;

    int currentTipIndex;
    float tipTimer;
    float globalTime;

    std::string aiProviderName;
    bool isAiReady;
    int targetFps;
    bool isHwAccel;

    sf::FloatRect heroBannerBounds;
    sf::FloatRect telemetryPanelBounds;
    sf::FloatRect tipPanelBounds;
    sf::FloatRect bottomStatusStripBounds;

    void spawnEmbers();

public:
    WelcomeScreen();
    void init();
    void update(float dt, sf::Vector2f mousePos);
    void updateHover(sf::Vector2f mousePos);
    void updateStatus(bool configured, const std::string& provider, int fpsLimit = 60, bool hwAccel = true);
    void draw(sf::RenderWindow& window);
    std::string handleClick(sf::Vector2f mousePos);
};