#pragma once
#include <SFML/Graphics.hpp>
#include <map>
#include <random>
#include <string>
#include <vector>

// The app's background brought to life: the park scene with the coaster's lights switching on and off,
// the Ferris wheel turning with its own lights, lamps and signs glowing, windows lighting up in the city
// and now and then birds. Everything comes from assets/park, made by
// scripts/tools/make_living_park.py; without it the app keeps its still background.
class LivingPark {
public:
    // False when assets/park is missing or incomplete
    bool init(const std::string& dir = "assets/park");
    bool isReady() const { return m_ready; }
    void update(float dt);
    // Draws the whole scene over the 1920x1080 view
    void draw(sf::RenderTarget& target);

private:
    // A line through points, with the distance along it at each point, so things can move along it
    struct Track {
        std::vector<sf::Vector2f> points;
        std::vector<float> along;
        float length = 0.f;
        void build();
        sf::Vector2f at(float s) const;
    };
    struct Glow {
        sf::Vector2f pos;
        sf::Color color;
        float radius = 14.f;
        float phase = 0.f;
        float rate = 1.f;
        float strength = 1.f;
        float flicker = 0.f;   // seconds left of a flicker off
    };
    struct Sign {
        sf::FloatRect box;
        sf::Color color;
        float off = 0.f;
        float next = 4.f;
    };
    struct Bird {
        sf::Vector2f pos;
        float speed = 40.f, flap = 0.f;
    };

    bool m_ready = false;
    std::mt19937 m_rng{ 20261019 };
    float m_time = 0.f;

    sf::Texture m_background, m_atlas, m_wheelFront, m_atomFront;
    sf::Vector2f m_wheelFrontAt, m_atomFrontAt;
    std::map<std::string, sf::IntRect> m_sprites;

    // Bulbs along the coaster track
    std::vector<sf::Vector2f> m_coasterBulbs;


    sf::Vector2f m_wheelCenter;
    float m_wheelRim = 140.f, m_wheelAngle = 0.f, m_wheelTurn = 70.f;
    int m_gondolas = 12, m_spokes = 16;
    sf::RenderTexture m_wheelTex;

    std::vector<Glow> m_glows;
    std::vector<Sign> m_signs;
    std::vector<sf::Vector2f> m_windows;
    std::vector<float> m_windowPhase;
    sf::FloatRect m_sky;
    std::vector<Bird> m_birds;
    float m_birdWait = 6.f;

    float random(float lo, float hi);
    // Whether bulb i of a string of lights is lit. The strings take turns at a few patterns: every other
    // bulb blinking, runs of light chasing along, and all of them switching on and off together
    bool bulbOn(int i) const;
    // Bulbs as small lights, lit ones glowing, unlit ones dim
    void drawBulbs(sf::VertexArray& lit, sf::VertexArray& unlit, const std::vector<sf::Vector2f>& at, int offset,
        sf::Color a, sf::Color b, float radius);

    void quad(sf::VertexArray& va, const sf::IntRect& src, sf::Vector2f pos, sf::Vector2f scale, float angleDeg,
        sf::Vector2f origin, sf::Color color = sf::Color::White, bool flip = false) const;
    void drawWheel(sf::RenderTarget& target);
};
