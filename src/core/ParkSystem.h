#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <unordered_map>

struct ParkVisitor {
    sf::Vector2f position;
    float speed;
    int direction;
    int animFrame;
    float animTimer;
};

struct ParkBuilding {
    uint32_t id;
    std::string type;
    sf::Vector2f position;
    sf::FloatRect bounds;
    sf::Sprite sprite;
    int level;
    int cost;
    int incomeRate;
};

class ParkSystem {
private:
    float m_boulevardY;
    std::unordered_map<std::string, sf::Texture> m_textures;

    sf::Sprite m_skylineSprite;
    sf::Sprite m_boulevardSprite;
    sf::Texture m_visitorTexture;

    std::vector<ParkBuilding> m_buildings;
    std::vector<ParkVisitor> m_visitors;

    int m_inspiration;
    float m_incomeTimer;

    int m_selectedBuildingIndex;

    bool loadTexture(const std::string& key, const std::string& filename);

public:
    ParkSystem();

    void init();
    void update(float dt);
    void draw(sf::RenderTarget& target);
    bool handleEvent(const sf::Event& event, const sf::Vector2f& worldPos);

    void addInspiration(int amount);
    int getInspiration() const;

    void addBuilding(const std::string& type, sf::Vector2f pos);
    void upgradeSelectedBuilding();
};