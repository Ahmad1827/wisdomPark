#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

// The arcade welcome screen: a maze game whose doors open the main menu's screens.
class ArcadeGame {
public:
    ArcadeGame();

    void reset();
    // Advances the game. Returns the id of the station the player just walked into, or "".
    std::string update(float dt, sf::Vector2f mousePos, bool paused);
    void draw(sf::RenderWindow& window, const sf::Font& font);

    std::string stationAt(sf::Vector2f point) const;
    void flashStation(const std::string& id);

private:
    enum class Phase { Waiting, Ready, Playing, Dying, LevelClear, GameOver };
    enum class GhostState { House, Leaving, Roaming, Frightened, Eyes };
    enum class Fruit { Cherry, Orange, Grape, Key };

    // Travels from one tile centre to the next; t is how far along that step it is.
    struct Mover {
        sf::Vector2i tile = { 0, 0 };
        int dir = -1;
        float t = 0.f;
        bool moving = false;
    };

    struct Ghost {
        Mover m;
        GhostState state = GhostState::House;
        int kind = 0;
        sf::Color color = sf::Color::White;
        sf::Vector2i corner = { 0, 0 };
        float release = 0.f;
        float anim = 0.f;
    };

    struct Station {
        std::string id, title, subtitle, key;
        sf::Color color = sf::Color::White;
        sf::FloatRect sign;
        sf::Vector2i door = { 0, 0 };
        float hover = 0.f;
        float flash = 0.f;
        float pulse = 0.f;
    };

    struct Particle {
        sf::Vector2f pos, vel;
        float life = 0.f, maxLife = 1.f, size = 3.f;
        sf::Color color = sf::Color::White;
    };

    struct Floater {
        std::string text;
        sf::Vector2f pos;
        float life = 0.f, maxLife = 1.f;
        sf::Color color = sf::Color::White;
    };

    std::vector<char> m_tiles;
    std::vector<unsigned char> m_dotLayout;
    std::vector<unsigned char> m_dots;
    std::vector<int> m_exitField;
    std::vector<int> m_homeField;
    std::vector<Station> m_stations;
    sf::VertexArray m_walls;
    sf::VertexArray m_wallsFlash;
    sf::VertexArray m_scanlines;

    sf::Vector2i m_pacSpawn = { 0, 0 };
    sf::Vector2i m_gateFront = { 0, 0 };
    sf::Vector2i m_houseCenter = { 0, 0 };
    sf::Vector2i m_fruitTile = { 0, 0 };

    Phase m_phase = Phase::Waiting;
    float m_phaseTimer = 0.f;
    float m_time = 0.f;
    float m_freeze = 0.f;
    bool m_wasPaused = false;

    Mover m_pac;
    int m_want = -1;
    float m_mouth = 0.f;
    std::vector<Ghost> m_ghosts;

    int m_score = 0;
    int m_highScore = 0;
    int m_savedHighScore = 0;
    int m_lives = 3;
    int m_level = 1;
    int m_dotsLeft = 0;
    int m_dotsTotal = 0;
    bool m_extraLifeGiven = false;

    float m_frightTimer = 0.f;
    int m_ghostChain = 0;
    int m_modeIndex = 0;
    float m_modeTimer = 0.f;
    float m_fruitTimer = 0.f;
    int m_fruitsShown = 0;

    std::string m_triggered;
    std::vector<Particle> m_particles;
    std::vector<Floater> m_floaters;

    void buildMaze();
    void buildWalls(sf::VertexArray& out, sf::Color fill, sf::Color edge) const;
    void buildField(std::vector<int>& field, sf::Vector2i from) const;
    void startLevel();
    void resetActors();
    void saveHighScore();

    char tileAt(int c, int r) const;
    bool pacCanEnter(sf::Vector2i t) const;
    bool ghostCanRoam(sf::Vector2i t) const;
    sf::Vector2i next(sf::Vector2i t, int dir) const;
    sf::Vector2f centerOf(sf::Vector2i t) const;
    sf::Vector2f posOf(const Mover& m) const;

    int readInput() const;
    void stepPac(float dt);
    void arrive(sf::Vector2i t);
    void stepGhost(Ghost& g, float dt);
    int pickGhostDir(const Ghost& g) const;
    sf::Vector2i ghostTarget(const Ghost& g) const;
    bool scattering() const;
    void reverse(Mover& m) const;
    void frighten();
    void addScore(int points);
    void burst(sf::Vector2f pos, sf::Color color, int count, float speed);
    void floatText(const std::string& text, sf::Vector2f pos, sf::Color color);

    Fruit fruitKind() const;
    int fruitPoints() const;

    void drawPac(sf::RenderWindow& window, sf::Vector2f pos, int dir, float mouthHalfAngle, float radius) const;
    void drawGhost(sf::RenderWindow& window, const Ghost& g) const;
    void drawFruit(sf::RenderWindow& window, sf::Vector2f pos, Fruit kind) const;
    void drawStation(sf::RenderWindow& window, const sf::Font& font, const Station& s) const;
};
