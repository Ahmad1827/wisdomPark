#pragma once
#include <SFML/Graphics.hpp>
#include <functional>
#include <vector>

class PencilStroke {
public:
    void begin(sf::Vector2f pos, float baseRadius, sf::Color color);
    void addPoint(sf::Vector2f pos);
    void finish() { m_finished = true; m_endSpeed = m_speed; }
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
    sf::Color m_graphite = sf::Color(46, 48, 54);
    float m_maxAlpha = 190.f;
    sf::Clock m_clock;
    float m_lastT = 0.f;
    float m_accDist = 0.f;
    float m_speed = 0.f;
    float m_endSpeed = 0.f;
    float m_pressure = 0.8f;
    bool m_active = false;
    bool m_finished = false;
};