#pragma once
#include <SFML/Graphics.hpp>
#include <functional>
#include <vector>

class PencilStroke {
public:
    void begin(sf::Vector2f pos, float baseRadius, sf::Color color);
    void addPoint(sf::Vector2f pos);
    void finish() { m_finished = true; }
    void clear();
    bool isActive() const { return m_active; }

    void appendMesh(sf::VertexArray& out,
        const sf::Vector2f* preview = nullptr,
        const std::function<sf::Vector2f(sf::Vector2f)>& xf = nullptr) const;

private:
    struct Sample {
        sf::Vector2f pos;
        float pressure;
    };

    std::vector<Sample> m_pts;
    float m_baseRadius = 1.0f;
    sf::Color m_coreColor = sf::Color(55, 58, 64, 135);
    sf::Color m_edgeColor = sf::Color(65, 68, 74, 45);
    sf::Clock m_clock;
    float m_lastT = 0.f;
    float m_pressure = 1.f;
    bool m_active = false;
    bool m_finished = false;
};