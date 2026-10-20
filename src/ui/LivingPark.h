#pragma once
#include <SFML/Graphics.hpp>
#include <map>
#include <random>
#include <string>
#include <vector>

// The app's background brought to life: the park scene with the coaster's lights switching on and off,
// the Ferris wheel turning with its own lights, lamps and signs glowing, the balloon seller's balloons
// swaying on their strings, windows lighting up in the city and now and then birds. It starts closed, dark and still, and opens the first time open() is called.
// Everything comes from assets/park, made by
// scripts/tools/make_living_park.py; without it the app keeps its still background.
class LivingPark {
public:
    // False when assets/park is missing or incomplete
    bool init(const std::string& dir = "assets/park");
    bool isReady() const { return m_ready; }
    void update(float dt);
    // Opens the park: the lights come on one after another and the wheel starts turning. Only the first
    // call does anything
    void open();
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
        float opens = 0.f;     // seconds into the opening when it comes on
    };
    struct Sign {
        sf::FloatRect box;
        sf::Color color;
        float off = 0.f;
        float next = 4.f;
        float opens = 0.f;
    };
    struct Bird {
        sf::Vector2f pos;
        float speed = 40.f, flap = 0.f;
    };
    // A balloon on its string: where the string is tied, where the balloon's knot is at rest
    struct Balloon {
        std::string sprite;
        sf::Vector2f tied, knot;
        float phase = 0.f;
    };

    bool m_ready = false;
    std::mt19937 m_rng{ 20261019 };
    float m_time = 0.f;

    sf::Texture m_background, m_atlas, m_wheelFront;
    sf::Vector2f m_wheelFrontAt;
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
    std::vector<Balloon> m_balloons;
    float m_birdWait = 6.f;

    // When open() was first called, below zero while the park is still closed
    float m_openedAt = -1.f;

    float random(float lo, float hi);
    // Seconds since the park opened, below zero while it is closed
    float sinceOpening() const { return m_openedAt < 0.f ? -1.f : m_time - m_openedAt; }
    // How lit a light is that comes on this many seconds into the opening: off before, a couple of quick
    // flickers as it catches, then on
    float lightUp(float opens) const;
    // How dark the closed park is, 1 closed and 0 once it is open
    float closedDarkness() const;
    // How fast the wheel turns, 0 while the park is closed and 1 once it is up to speed
    float wheelSpin() const;
    // Whether bulb i of a string of lights is lit. The strings take turns at a few patterns: every other
    // bulb blinking, runs of light chasing along, and all of them switching on and off together
    bool bulbOn(int i) const;
    // Bulbs as small lights, lit ones glowing, unlit ones dim
    // While the park opens they come on one after another, bulb k at opens + k * step, and stay on until
    // the whole string is lit
    void drawBulbs(sf::VertexArray& lit, sf::VertexArray& unlit, const std::vector<sf::Vector2f>& at, int offset,
        sf::Color a, sf::Color b, float radius, float opens, float step);

    void quad(sf::VertexArray& va, const sf::IntRect& src, sf::Vector2f pos, sf::Vector2f scale, float angleDeg,
        sf::Vector2f origin, sf::Color color = sf::Color::White, bool flip = false) const;
    void drawWheel(sf::RenderTarget& target);
    void drawBalloons(sf::RenderTarget& target);
};
