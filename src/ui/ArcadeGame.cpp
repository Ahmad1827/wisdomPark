#include "ArcadeGame.h"
#include "UITheme.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <queue>

namespace {
    const int kCols = 43;
    const int kRows = 21;
    const float kTile = 40.f;
    const sf::Vector2f kOrigin(100.f, 105.f);
    const float kPi = 3.14159265f;
    const char* kHighScoreFile = "arcade_highscore.txt";

    // Left 22 columns of the maze; the right side mirrors it around column 21.
    // # wall   . dot   o power pellet   S station sign   D station door (a sign without one is click only)
    // - ghost house gate   G ghost house   P player start   space = open, no dot
    const char* kHalfMaze[kRows] = {
        "######################",
        "#o....................",
        "#.SSSSS.###.SSSSS.####",
        "#.SSSSS.###.SSSSS.####",
        "#.##D##.###.##D##.####",
        "#.....................",
        "#.####.#####.####.####",
        "#.####.#####.####.....",
        "#.####.......####.###-",
        "#.####.#####.####.#GGG",
        " .................####",
        "#.####.#####.####....P",
        "#.####.#####.####.####",
        "#......#####.####.####",
        "#.####.#####.####.####",
        "#.....................",
        "#.##D##.###.######.###",
        "#.SSSSS.###.######.SSS",
        "#.SSSSS.###.######.SSS",
        "#o....................",
        "######################",
    };

    // Directions: 0 right, 1 up, 2 left, 3 down.
    const int kDirX[4] = { 1, 0, -1, 0 };
    const int kDirY[4] = { 0, -1, 0, 1 };

    struct StationInfo {
        const char* id; const char* title; const char* subtitle; const char* key;
        sf::Color color;
    };

    // In reading order of the signs: top row left to right, then the bottom row.
    const StationInfo kStations[] = {
        { "new_project", "1P START", "NEW CANVAS", "SPACE / CLICK", sf::Color(0, 255, 200) },
        { "keybinds", "KEYS", "SHORTCUTS", "K", sf::Color(255, 215, 60) },
        { "credits", "FAMOUS", "HALL OF FAME", "C", sf::Color(255, 160, 40) },
        { "projects", "ARCHIVES", "PROJECT VAULT", "O", sf::Color(255, 110, 255) },
        { "settings", "CONFIG", "SETTINGS", "ESC", sf::Color(80, 255, 120) },
        { "exit", "POWER OFF", "TERMINATE", "CLICK", sf::Color(255, 50, 70) },
        { "tutorials", "HOW TO PLAY", "CODEX", "F1", sf::Color(255, 140, 60) },
    };

    const sf::Color kBackdrop(8, 5, 16);
    const sf::Color kInk(14, 4, 20);

    // Seconds of scatter, chase, scatter, chase... the last chase never ends.
    const float kModeTimes[] = { 6.f, 20.f, 6.f, 20.f, 4.f };
    const int kModeCount = 5;

    void addQuad(sf::VertexArray& va, float x, float y, float w, float h, sf::Color c) {
        va.append(sf::Vertex(sf::Vector2f(x, y), c));
        va.append(sf::Vertex(sf::Vector2f(x + w, y), c));
        va.append(sf::Vertex(sf::Vector2f(x + w, y + h), c));
        va.append(sf::Vertex(sf::Vector2f(x, y + h), c));
    }

    void fillRect(sf::RenderWindow& window, float x, float y, float w, float h, sf::Color c) {
        sf::RectangleShape r(sf::Vector2f(w, h));
        r.setPosition(x, y);
        r.setFillColor(c);
        window.draw(r);
    }

    void fillCircle(sf::RenderWindow& window, float cx, float cy, float radius, sf::Color c) {
        sf::CircleShape s(radius);
        s.setOrigin(radius, radius);
        s.setPosition(cx, cy);
        s.setFillColor(c);
        window.draw(s);
    }
}

ArcadeGame::ArcadeGame() {
    std::ifstream in(kHighScoreFile);
    if (in) in >> m_highScore;
    m_highScore = std::max(0, m_highScore);
    m_savedHighScore = m_highScore;

    buildMaze();
    reset();
}

void ArcadeGame::buildMaze() {
    m_tiles.assign(kCols * kRows, '#');
    m_dotLayout.assign(kCols * kRows, 0);
    m_dotsTotal = 0;

    for (int r = 0; r < kRows; ++r) {
        for (int c = 0; c < kCols; ++c) {
            char ch = kHalfMaze[r][c <= kCols / 2 ? c : kCols - 1 - c];
            int i = r * kCols + c;
            if (ch == '.' || ch == 'o') {
                m_tiles[i] = ' ';
                m_dotLayout[i] = (ch == 'o') ? 2 : 1;
                ++m_dotsTotal;
            }
            else if (ch == 'P') {
                m_tiles[i] = ' ';
                m_pacSpawn = { c, r };
            }
            else {
                m_tiles[i] = ch;
                if (ch == '-') {
                    m_gateFront = { c, r - 1 };
                    m_houseCenter = { c, r + 1 };
                }
            }
        }
    }
    m_fruitTile = { m_pacSpawn.x, m_pacSpawn.y + 4 };

    // Each block of sign tiles is one station; its door sits just above or below the middle.
    m_stations.clear();
    std::vector<bool> seen(kCols * kRows, false);
    for (int r = 0; r < kRows; ++r) {
        for (int c = 0; c < kCols; ++c) {
            if (tileAt(c, r) != 'S' || seen[r * kCols + c]) continue;
            int c1 = c, r1 = r;
            while (c1 + 1 < kCols && tileAt(c1 + 1, r) == 'S') ++c1;
            while (r1 + 1 < kRows && tileAt(c, r1 + 1) == 'S') ++r1;
            for (int y = r; y <= r1; ++y)
                for (int x = c; x <= c1; ++x) seen[y * kCols + x] = true;

            size_t index = m_stations.size();
            if (index >= sizeof(kStations) / sizeof(kStations[0])) continue;
            const StationInfo& info = kStations[index];
            Station s;
            s.id = info.id; s.title = info.title; s.subtitle = info.subtitle; s.key = info.key;
            s.color = info.color;
            s.sign = sf::FloatRect(kOrigin.x + c * kTile, kOrigin.y + r * kTile, (c1 - c + 1) * kTile, (r1 - r + 1) * kTile);
            int mid = (c + c1) / 2;
            if (tileAt(mid, r1 + 1) == 'D') s.door = { mid, r1 + 1 };
            else if (tileAt(mid, r - 1) == 'D') s.door = { mid, r - 1 };
            else s.door = { -1, -1 };
            s.pulse = static_cast<float>(index) * 0.9f;
            m_stations.push_back(s);
        }
    }

    buildField(m_exitField, m_gateFront);
    buildField(m_homeField, m_houseCenter);
    buildWalls(m_walls, sf::Color(24, 16, 58), sf::Color(96, 112, 255));
    buildWalls(m_wallsFlash, sf::Color(70, 60, 110), sf::Color(255, 255, 255));

    m_scanlines.setPrimitiveType(sf::Quads);
    m_scanlines.clear();
    for (float y = kOrigin.y; y < kOrigin.y + kRows * kTile; y += 4.f)
        addQuad(m_scanlines, kOrigin.x, y, kCols * kTile, 1.5f, sf::Color(0, 0, 0, 38));
}

// Fills every wall tile and outlines the sides that face a corridor.
void ArcadeGame::buildWalls(sf::VertexArray& out, sf::Color fill, sf::Color edge) const {
    out.setPrimitiveType(sf::Quads);
    out.clear();
    const float e = 3.f;

    auto solid = [this](int c, int r) {
        if (c < 0 || c >= kCols || r < 0 || r >= kRows) return true;
        char t = m_tiles[r * kCols + c];
        return t == '#' || t == 'S';
    };

    for (int r = 0; r < kRows; ++r) {
        for (int c = 0; c < kCols; ++c) {
            if (!solid(c, r)) continue;
            float x = kOrigin.x + c * kTile;
            float y = kOrigin.y + r * kTile;
            addQuad(out, x, y, kTile, kTile, fill);

            bool right = solid(c + 1, r), left = solid(c - 1, r), up = solid(c, r - 1), down = solid(c, r + 1);
            if (!up) addQuad(out, x, y, kTile, e, edge);
            if (!down) addQuad(out, x, y + kTile - e, kTile, e, edge);
            if (!left) addQuad(out, x, y, e, kTile, edge);
            if (!right) addQuad(out, x + kTile - e, y, e, kTile, edge);

            // Inside corners, where two outlined runs meet.
            if (up && left && !solid(c - 1, r - 1)) addQuad(out, x, y, e, e, edge);
            if (up && right && !solid(c + 1, r - 1)) addQuad(out, x + kTile - e, y, e, e, edge);
            if (down && left && !solid(c - 1, r + 1)) addQuad(out, x, y + kTile - e, e, e, edge);
            if (down && right && !solid(c + 1, r + 1)) addQuad(out, x + kTile - e, y + kTile - e, e, e, edge);
        }
    }
}

// Steps to reach `from` out of every tile a ghost may pass, the gate and the house included.
void ArcadeGame::buildField(std::vector<int>& field, sf::Vector2i from) const {
    field.assign(kCols * kRows, -1);
    std::queue<sf::Vector2i> open;
    field[from.y * kCols + from.x] = 0;
    open.push(from);
    while (!open.empty()) {
        sf::Vector2i t = open.front();
        open.pop();
        for (int d = 0; d < 4; ++d) {
            sf::Vector2i n = next(t, d);
            char ch = tileAt(n.x, n.y);
            if (ch != ' ' && ch != '-' && ch != 'G') continue;
            int& slot = field[n.y * kCols + n.x];
            if (slot >= 0) continue;
            slot = field[t.y * kCols + t.x] + 1;
            open.push(n);
        }
    }
}

void ArcadeGame::reset() {
    saveHighScore();
    m_score = 0;
    m_lives = 3;
    m_level = 1;
    m_extraLifeGiven = false;
    m_time = 0.f;
    m_freeze = 0.f;
    m_triggered.clear();
    m_particles.clear();
    m_floaters.clear();
    startLevel();
    m_phase = Phase::Waiting;
}

void ArcadeGame::startLevel() {
    m_dots = m_dotLayout;
    m_dotsLeft = m_dotsTotal;
    m_fruitTimer = 0.f;
    m_fruitsShown = 0;
    resetActors();
    m_phase = Phase::Ready;
    m_phaseTimer = 1.6f;
}

void ArcadeGame::resetActors() {
    m_pac = Mover();
    m_pac.tile = m_pacSpawn;
    m_pac.dir = 2;
    m_want = -1;
    m_mouth = 0.f;

    m_frightTimer = 0.f;
    m_ghostChain = 0;
    m_modeIndex = 0;
    m_modeTimer = kModeTimes[0];

    const sf::Color colors[4] = { sf::Color(255, 45, 85), sf::Color(255, 140, 210), sf::Color(50, 220, 255), sf::Color(255, 175, 45) };
    const sf::Vector2i corners[4] = { { kCols - 2, 0 }, { 1, 0 }, { kCols - 2, kRows - 1 }, { 1, kRows - 1 } };
    const int houseOffset[4] = { 0, 0, -1, 1 };
    const float release[4] = { 0.f, 1.f, 5.f, 9.f };

    m_ghosts.assign(4, Ghost());
    for (int i = 0; i < 4; ++i) {
        Ghost& g = m_ghosts[i];
        g.kind = i;
        g.color = colors[i];
        g.corner = corners[i];
        g.release = release[i];
        g.anim = static_cast<float>(i) * 1.3f;
        if (i == 0) {
            g.state = GhostState::Roaming;
            g.m.tile = m_gateFront;
            g.m.dir = 2;
            g.m.moving = true;
        }
        else {
            g.state = GhostState::House;
            g.m.tile = { m_houseCenter.x + houseOffset[i], m_houseCenter.y };
        }
    }
}

void ArcadeGame::saveHighScore() {
    if (m_highScore <= m_savedHighScore) return;
    std::ofstream out(kHighScoreFile);
    if (out) {
        out << m_highScore;
        m_savedHighScore = m_highScore;
    }
}

char ArcadeGame::tileAt(int c, int r) const {
    if (r < 0 || r >= kRows) return '#';
    c = ((c % kCols) + kCols) % kCols;
    return m_tiles[r * kCols + c];
}

bool ArcadeGame::pacCanEnter(sf::Vector2i t) const {
    char ch = tileAt(t.x, t.y);
    return ch == ' ' || ch == 'D';
}

bool ArcadeGame::ghostCanRoam(sf::Vector2i t) const {
    return tileAt(t.x, t.y) == ' ';
}

// The tunnel row wraps from one side of the maze to the other.
sf::Vector2i ArcadeGame::next(sf::Vector2i t, int dir) const {
    return { (t.x + kDirX[dir] + kCols) % kCols, t.y + kDirY[dir] };
}

sf::Vector2f ArcadeGame::centerOf(sf::Vector2i t) const {
    return { kOrigin.x + (static_cast<float>(t.x) + 0.5f) * kTile, kOrigin.y + (static_cast<float>(t.y) + 0.5f) * kTile };
}

sf::Vector2f ArcadeGame::posOf(const Mover& m) const {
    sf::Vector2f p = centerOf(m.tile);
    if (m.moving && m.dir >= 0) {
        p.x += static_cast<float>(kDirX[m.dir]) * m.t * kTile;
        p.y += static_cast<float>(kDirY[m.dir]) * m.t * kTile;
    }
    return p;
}

// The held direction; when two are held, the one that is not already being followed.
int ArcadeGame::readInput() const {
    using K = sf::Keyboard;
    const bool held[4] = {
        K::isKeyPressed(K::D) || K::isKeyPressed(K::Right),
        K::isKeyPressed(K::W) || K::isKeyPressed(K::Up),
        K::isKeyPressed(K::A) || K::isKeyPressed(K::Left),
        K::isKeyPressed(K::S) || K::isKeyPressed(K::Down),
    };
    int pick = -1;
    for (int d = 0; d < 4; ++d) {
        if (!held[d]) continue;
        if (d != m_want) return d;
        pick = d;
    }
    return pick;
}

bool ArcadeGame::scattering() const {
    return m_modeIndex < kModeCount && m_modeIndex % 2 == 0;
}

void ArcadeGame::reverse(Mover& m) const {
    if (m.dir < 0 || !m.moving) return;
    m.tile = next(m.tile, m.dir);
    m.dir = (m.dir + 2) % 4;
    m.t = 1.f - m.t;
}

void ArcadeGame::frighten() {
    m_frightTimer = std::max(2.5f, 7.f - static_cast<float>(m_level - 1) * 0.75f);
    m_ghostChain = 0;
    for (Ghost& g : m_ghosts) {
        if (g.state == GhostState::Roaming) {
            g.state = GhostState::Frightened;
            reverse(g.m);
        }
    }
}

void ArcadeGame::addScore(int points) {
    m_score += points;
    if (m_score > m_highScore) m_highScore = m_score;
    if (!m_extraLifeGiven && m_score >= 10000) {
        m_extraLifeGiven = true;
        ++m_lives;
        floatText("EXTRA LIFE", posOf(m_pac), WisdomUI::Theme::SunsetGlow);
    }
}

void ArcadeGame::burst(sf::Vector2f pos, sf::Color color, int count, float speed) {
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.pos = pos;
        float ang = static_cast<float>(rand() % 360) * kPi / 180.f;
        float s = speed * (0.4f + static_cast<float>(rand() % 100) * 0.008f);
        p.vel = { std::cos(ang) * s, std::sin(ang) * s };
        p.maxLife = 0.35f + static_cast<float>(rand() % 10) * 0.03f;
        p.life = p.maxLife;
        p.size = 2.f + static_cast<float>(rand() % 4);
        p.color = color;
        m_particles.push_back(p);
    }
}

void ArcadeGame::floatText(const std::string& text, sf::Vector2f pos, sf::Color color) {
    Floater f;
    f.text = text;
    f.pos = pos;
    f.maxLife = 0.9f;
    f.life = f.maxLife;
    f.color = color;
    m_floaters.push_back(f);
}

ArcadeGame::Fruit ArcadeGame::fruitKind() const {
    return static_cast<Fruit>(std::min(m_level - 1, 3));
}

int ArcadeGame::fruitPoints() const {
    const int points[4] = { 100, 500, 1000, 3000 };
    return points[std::min(m_level - 1, 3)];
}

void ArcadeGame::arrive(sf::Vector2i t) {
    int i = t.y * kCols + t.x;
    if (m_dots[i]) {
        bool pellet = m_dots[i] == 2;
        m_dots[i] = 0;
        --m_dotsLeft;
        addScore(pellet ? 50 : 10);
        if (pellet) {
            frighten();
            burst(centerOf(t), WisdomUI::Theme::SunsetGold, 18, 220.f);
        }

        int eaten = m_dotsTotal - m_dotsLeft;
        if (m_fruitsShown < 2 && eaten >= (m_fruitsShown + 1) * m_dotsTotal / 3) {
            ++m_fruitsShown;
            m_fruitTimer = 9.f;
        }
        if (m_dotsLeft == 0) {
            m_phase = Phase::LevelClear;
            m_phaseTimer = 2.2f;
        }
    }

    if (tileAt(t.x, t.y) == 'D') {
        for (const Station& s : m_stations) {
            if (s.door == t) m_triggered = s.id;
        }
    }
}

void ArcadeGame::stepPac(float dt) {
    Mover& m = m_pac;
    if (m_want >= 0) {
        if (m.moving && m_want == (m.dir + 2) % 4) {
            reverse(m);
        }
        else if (!m.moving && pacCanEnter(next(m.tile, m_want))) {
            m.dir = m_want;
            m.moving = true;
            m.t = 0.f;
        }
    }
    if (!m.moving) return;

    float speed = 7.5f + static_cast<float>(std::min(m_level - 1, 4)) * 0.25f;
    m_mouth += dt * 14.f;
    m.t += speed * dt;
    while (m.t >= 1.f) {
        m.t -= 1.f;
        m.tile = next(m.tile, m.dir);
        arrive(m.tile);
        if (!m_triggered.empty() || m_phase != Phase::Playing) {
            m.moving = false;
            m.t = 0.f;
            break;
        }
        if (m_want >= 0 && m_want != m.dir && pacCanEnter(next(m.tile, m_want))) {
            m.dir = m_want;
        }
        else if (!pacCanEnter(next(m.tile, m.dir))) {
            m.moving = false;
            m.t = 0.f;
            break;
        }
    }
}

sf::Vector2i ArcadeGame::ghostTarget(const Ghost& g) const {
    if (scattering()) return g.corner;

    sf::Vector2i pac = m_pac.tile;
    int pd = std::max(0, m_pac.dir);
    auto ahead = [&](int steps) { return sf::Vector2i(pac.x + kDirX[pd] * steps, pac.y + kDirY[pd] * steps); };

    if (g.kind == 1) return ahead(4);
    if (g.kind == 2) {
        sf::Vector2i pivot = ahead(2);
        sf::Vector2i lead = m_ghosts[0].m.tile;
        return { pivot.x * 2 - lead.x, pivot.y * 2 - lead.y };
    }
    if (g.kind == 3) {
        int dx = g.m.tile.x - pac.x, dy = g.m.tile.y - pac.y;
        return (dx * dx + dy * dy > 64) ? pac : g.corner;
    }
    return pac;
}

int ArcadeGame::pickGhostDir(const Ghost& g) const {
    sf::Vector2i t = g.m.tile;

    // Leaving the house and returning to it both follow the shortest way.
    if (g.state == GhostState::Leaving || g.state == GhostState::Eyes) {
        const std::vector<int>& field = (g.state == GhostState::Leaving) ? m_exitField : m_homeField;
        int best = -1;
        int bestValue = field[t.y * kCols + t.x];
        for (int d = 0; d < 4; ++d) {
            sf::Vector2i n = next(t, d);
            if (n.y < 0 || n.y >= kRows) continue;
            int v = field[n.y * kCols + n.x];
            if (v >= 0 && (bestValue < 0 || v < bestValue)) {
                bestValue = v;
                best = d;
            }
        }
        return best;
    }

    int back = (g.m.dir >= 0) ? (g.m.dir + 2) % 4 : -1;
    const int order[4] = { 1, 2, 3, 0 };
    int options[4];
    int count = 0;
    for (int k = 0; k < 4; ++k) {
        int d = order[k];
        if (d != back && ghostCanRoam(next(t, d))) options[count++] = d;
    }
    if (count == 0) return (back >= 0 && ghostCanRoam(next(t, back))) ? back : -1;
    if (g.state == GhostState::Frightened) return options[rand() % count];

    sf::Vector2i target = ghostTarget(g);
    int best = options[0];
    int bestDist = -1;
    for (int k = 0; k < count; ++k) {
        sf::Vector2i n(t.x + kDirX[options[k]], t.y + kDirY[options[k]]);
        int dist = (n.x - target.x) * (n.x - target.x) + (n.y - target.y) * (n.y - target.y);
        if (bestDist < 0 || dist < bestDist) {
            bestDist = dist;
            best = options[k];
        }
    }
    return best;
}

void ArcadeGame::stepGhost(Ghost& g, float dt) {
    g.anim += dt * 8.f;

    if (g.state == GhostState::House) {
        g.release -= dt;
        if (g.release > 0.f) return;
        g.state = GhostState::Leaving;
        g.m.dir = pickGhostDir(g);
        g.m.moving = g.m.dir >= 0;
        g.m.t = 0.f;
        return;
    }

    if (!g.m.moving || g.m.dir < 0) {
        g.m.dir = pickGhostDir(g);
        g.m.moving = g.m.dir >= 0;
        g.m.t = 0.f;
        if (!g.m.moving) return;
    }

    float speed = 6.6f + static_cast<float>(std::min(m_level - 1, 6)) * 0.25f;
    if (g.state == GhostState::Frightened) speed = 4.4f;
    else if (g.state == GhostState::Eyes) speed = 16.f;
    else if (g.state == GhostState::Leaving) speed = 4.5f;

    g.m.t += speed * dt;
    while (g.m.t >= 1.f) {
        g.m.t -= 1.f;
        g.m.tile = next(g.m.tile, g.m.dir);

        if (g.state == GhostState::Leaving && g.m.tile == m_gateFront) {
            g.state = GhostState::Roaming;
            g.m.dir = -1;
        }
        else if (g.state == GhostState::Eyes && g.m.tile == m_houseCenter) {
            g.state = GhostState::Leaving;
        }

        g.m.dir = pickGhostDir(g);
        if (g.m.dir < 0) {
            g.m.moving = false;
            g.m.t = 0.f;
            break;
        }
    }
}

std::string ArcadeGame::update(float dt, sf::Vector2f mousePos, bool paused) {
    dt = std::min(dt, 0.05f);

    if (paused) {
        m_wasPaused = true;
        return "";
    }
    if (m_wasPaused) {
        // Coming back from a menu screen: a moment to find the player again.
        m_wasPaused = false;
        m_want = -1;
        if (m_phase == Phase::Playing) {
            m_phase = Phase::Ready;
            m_phaseTimer = 1.0f;
        }
    }

    m_time += dt;

    sf::Vector2f pacPos = posOf(m_pac);
    for (Station& s : m_stations) {
        bool lit = s.sign.contains(mousePos) || m_pac.tile == s.door;
        s.hover += ((lit ? 1.f : 0.f) - s.hover) * std::min(1.f, 16.f * dt);
        s.flash = std::max(0.f, s.flash - 3.f * dt);
        s.pulse += dt * 3.f;
    }

    for (Particle& p : m_particles) {
        p.life -= dt;
        p.pos += p.vel * dt;
    }
    m_particles.erase(std::remove_if(m_particles.begin(), m_particles.end(), [](const Particle& p) { return p.life <= 0.f; }), m_particles.end());
    for (Floater& f : m_floaters) {
        f.life -= dt;
        f.pos.y -= 70.f * dt;
    }
    m_floaters.erase(std::remove_if(m_floaters.begin(), m_floaters.end(), [](const Floater& f) { return f.life <= 0.f; }), m_floaters.end());

    int input = readInput();

    if (m_phase == Phase::Waiting) {
        if (input >= 0) {
            m_want = input;
            m_phase = Phase::Playing;
        }
        return "";
    }
    if (m_phase == Phase::GameOver) {
        m_phaseTimer -= dt;
        if (m_phaseTimer <= 0.f && input >= 0) {
            reset();
            m_want = input;
            m_phase = Phase::Playing;
        }
        return "";
    }
    if (m_phase == Phase::Ready) {
        if (input >= 0) m_want = input;
        m_phaseTimer -= dt;
        if (m_phaseTimer <= 0.f) m_phase = Phase::Playing;
        return "";
    }
    if (m_phase == Phase::LevelClear) {
        m_phaseTimer -= dt;
        if (m_phaseTimer <= 0.f) {
            ++m_level;
            saveHighScore();
            startLevel();
        }
        return "";
    }
    if (m_phase == Phase::Dying) {
        m_phaseTimer -= dt;
        if (m_phaseTimer <= 0.f) {
            --m_lives;
            if (m_lives <= 0) {
                saveHighScore();
                m_phase = Phase::GameOver;
                m_phaseTimer = 1.2f;
            }
            else {
                resetActors();
                m_phase = Phase::Ready;
                m_phaseTimer = 1.6f;
            }
        }
        return "";
    }

    // Playing
    if (input >= 0) m_want = input;
    if (m_freeze > 0.f) {
        m_freeze -= dt;
        return "";
    }

    if (m_frightTimer > 0.f) {
        m_frightTimer -= dt;
        if (m_frightTimer <= 0.f) {
            for (Ghost& g : m_ghosts)
                if (g.state == GhostState::Frightened) g.state = GhostState::Roaming;
        }
    }
    else if (m_modeIndex < kModeCount) {
        m_modeTimer -= dt;
        if (m_modeTimer <= 0.f) {
            ++m_modeIndex;
            if (m_modeIndex < kModeCount) m_modeTimer = kModeTimes[m_modeIndex];
            for (Ghost& g : m_ghosts)
                if (g.state == GhostState::Roaming) reverse(g.m);
        }
    }

    if (m_fruitTimer > 0.f) m_fruitTimer -= dt;

    stepPac(dt);
    if (!m_triggered.empty()) {
        std::string id = m_triggered;
        m_triggered.clear();
        m_want = -1;
        saveHighScore();
        return id;
    }
    if (m_phase != Phase::Playing) return "";

    for (Ghost& g : m_ghosts) stepGhost(g, dt);

    pacPos = posOf(m_pac);
    if (m_fruitTimer > 0.f) {
        sf::Vector2f f = centerOf(m_fruitTile);
        if (std::hypot(f.x - pacPos.x, f.y - pacPos.y) < kTile * 0.6f) {
            m_fruitTimer = 0.f;
            addScore(fruitPoints());
            burst(f, WisdomUI::Theme::SunsetAmber, 16, 180.f);
            floatText("+" + std::to_string(fruitPoints()), f, WisdomUI::Theme::SunsetAmber);
        }
    }

    for (Ghost& g : m_ghosts) {
        if (g.state != GhostState::Roaming && g.state != GhostState::Frightened) continue;
        sf::Vector2f gp = posOf(g.m);
        if (std::hypot(gp.x - pacPos.x, gp.y - pacPos.y) >= kTile * 0.6f) continue;

        if (g.state == GhostState::Frightened) {
            int points = 200 << std::min(m_ghostChain, 3);
            ++m_ghostChain;
            g.state = GhostState::Eyes;
            addScore(points);
            m_freeze = 0.35f;
            burst(gp, sf::Color(80, 200, 255), 22, 260.f);
            floatText("+" + std::to_string(points), gp, sf::Color(80, 200, 255));
        }
        else {
            m_phase = Phase::Dying;
            m_phaseTimer = 1.4f;
            m_pac.moving = false;
            burst(pacPos, WisdomUI::Theme::SunsetGold, 30, 300.f);
            break;
        }
    }
    return "";
}

std::string ArcadeGame::stationAt(sf::Vector2f point) const {
    for (const Station& s : m_stations) {
        if (s.sign.contains(point)) return s.id;
    }
    return "";
}

void ArcadeGame::flashStation(const std::string& id) {
    for (Station& s : m_stations) {
        if (s.id != id) continue;
        s.flash = 1.f;
        burst({ s.sign.left + s.sign.width / 2.f, s.sign.top + s.sign.height / 2.f }, s.color, 32, 340.f);
    }
}

void ArcadeGame::drawPac(sf::RenderWindow& window, sf::Vector2f pos, int dir, float mouthHalfAngle, float radius) const {
    const float facing[4] = { 0.f, -kPi / 2.f, kPi, kPi / 2.f };
    float a0 = facing[std::max(0, dir)] + mouthHalfAngle;
    float sweep = 2.f * kPi - 2.f * mouthHalfAngle;
    if (sweep <= 0.01f) return;

    const int segments = 28;
    sf::VertexArray fan(sf::TriangleFan);
    fan.append(sf::Vertex(pos, WisdomUI::Theme::SunsetGold));
    for (int i = 0; i <= segments; ++i) {
        float a = a0 + sweep * static_cast<float>(i) / static_cast<float>(segments);
        fan.append(sf::Vertex(sf::Vector2f(pos.x + std::cos(a) * radius, pos.y + std::sin(a) * radius), WisdomUI::Theme::SunsetGold));
    }
    window.draw(fan);
}

void ArcadeGame::drawGhost(sf::RenderWindow& window, const Ghost& g) const {
    sf::Vector2f p = posOf(g.m);
    if (g.state == GhostState::House) p.y += std::sin(g.anim * 0.6f) * 4.f;
    float x = std::floor(p.x), y = std::floor(p.y);

    float lookX = 0.f, lookY = 0.f;
    if (g.m.dir >= 0) {
        lookX = static_cast<float>(kDirX[g.m.dir]) * 2.f;
        lookY = static_cast<float>(kDirY[g.m.dir]) * 2.f;
    }

    if (g.state != GhostState::Eyes) {
        sf::Color body = g.color;
        if (g.state == GhostState::Frightened) {
            bool flash = m_frightTimer < 2.f && static_cast<int>(m_time * 8.f) % 2 == 0;
            body = flash ? sf::Color(245, 245, 255) : sf::Color(40, 80, 230);
        }

        fillCircle(window, x, y - 3.f, 14.f, body);
        fillRect(window, x - 14.f, y - 3.f, 28.f, 13.f, body);

        float wave = std::sin(g.anim) * 2.5f;
        for (int i = 0; i < 3; ++i) {
            sf::ConvexShape foot(3);
            float fx = x - 14.f + static_cast<float>(i) * 9.33f;
            foot.setPoint(0, sf::Vector2f(fx, y + 10.f));
            foot.setPoint(1, sf::Vector2f(fx + 9.33f, y + 10.f));
            foot.setPoint(2, sf::Vector2f(fx + 4.66f, y + 15.f + (i % 2 == 0 ? wave : -wave)));
            foot.setFillColor(body);
            window.draw(foot);
        }

        if (g.state == GhostState::Frightened) {
            fillCircle(window, x - 5.f, y - 4.f, 2.5f, sf::Color(255, 190, 190));
            fillCircle(window, x + 5.f, y - 4.f, 2.5f, sf::Color(255, 190, 190));
            return;
        }
    }

    fillCircle(window, x - 6.f, y - 5.f, 4.5f, sf::Color::White);
    fillCircle(window, x + 6.f, y - 5.f, 4.5f, sf::Color::White);
    fillCircle(window, x - 6.f + lookX, y - 5.f + lookY, 2.2f, sf::Color(20, 40, 150));
    fillCircle(window, x + 6.f + lookX, y - 5.f + lookY, 2.2f, sf::Color(20, 40, 150));
}

void ArcadeGame::drawFruit(sf::RenderWindow& window, sf::Vector2f pos, Fruit kind) const {
    float x = std::floor(pos.x), y = std::floor(pos.y);
    if (kind == Fruit::Cherry) {
        fillRect(window, x - 1.f, y - 12.f, 2.f, 10.f, sf::Color(80, 220, 90));
        fillCircle(window, x - 5.f, y + 3.f, 7.f, sf::Color(255, 30, 70));
        fillCircle(window, x + 6.f, y + 5.f, 7.f, sf::Color(255, 30, 70));
    }
    else if (kind == Fruit::Orange) {
        fillCircle(window, x, y + 2.f, 10.f, sf::Color(255, 140, 30));
        fillRect(window, x - 2.f, y - 12.f, 6.f, 4.f, sf::Color(90, 230, 90));
    }
    else if (kind == Fruit::Grape) {
        fillCircle(window, x - 5.f, y - 4.f, 6.f, sf::Color(180, 60, 255));
        fillCircle(window, x + 5.f, y - 4.f, 6.f, sf::Color(180, 60, 255));
        fillCircle(window, x, y + 5.f, 6.f, sf::Color(150, 40, 235));
    }
    else {
        fillCircle(window, x, y - 6.f, 7.f, WisdomUI::Theme::SunsetGold);
        fillRect(window, x - 2.f, y - 1.f, 4.f, 14.f, WisdomUI::Theme::SunsetGold);
        fillRect(window, x + 2.f, y + 7.f, 6.f, 3.f, WisdomUI::Theme::SunsetGold);
    }
}

void ArcadeGame::drawStation(sf::RenderWindow& window, const sf::Font& font, const Station& s) const {
    using WisdomUI::Theme;
    sf::FloatRect b(s.sign.left + 4.f, s.sign.top + 4.f, s.sign.width - 8.f, s.sign.height - 8.f);
    bool lit = s.flash > 0.05f;
    bool hov = s.hover > 0.2f;

    sf::RectangleShape base(sf::Vector2f(b.width, b.height));
    base.setPosition(b.left, b.top);
    if (lit) {
        base.setFillColor(sf::Color(255, 245, 210));
        base.setOutlineThickness(3.f);
        base.setOutlineColor(sf::Color::White);
    }
    else if (hov) {
        base.setFillColor(s.id == "exit" ? sf::Color(140, 20, 40) : sf::Color(48, 16, 62));
        base.setOutlineThickness(2.f);
        base.setOutlineColor(s.color);
    }
    else {
        base.setFillColor(sf::Color(16, 8, 24));
        base.setOutlineThickness(1.5f);
        base.setOutlineColor(s.color);
    }
    window.draw(base);

    float cx = b.left + b.width / 2.f;
    sf::Color titleColor = lit ? sf::Color(16, 4, 22) : (hov ? sf::Color::White : s.color);
    Theme::DrawCrispText(window, font, s.title, 15, cx, b.top + 15.f, titleColor, lit ? sf::Color::Transparent : kInk, true, true);
    Theme::DrawCrispText(window, font, s.subtitle, 10, cx, b.top + 35.f, lit ? sf::Color(50, 10, 60) : sf::Color(190, 190, 190), sf::Color::Transparent, true, true);

    sf::RectangleShape badge(sf::Vector2f(104.f, 15.f));
    badge.setPosition(cx - 52.f, b.top + b.height - 21.f);
    badge.setFillColor(sf::Color(10, 4, 16));
    badge.setOutlineThickness(1.f);
    badge.setOutlineColor(s.color);
    window.draw(badge);
    Theme::DrawCrispText(window, font, s.key, 9, cx, b.top + b.height - 13.5f, hov ? sf::Color::White : Theme::SunsetGold, sf::Color::Transparent, true, true);

    if (s.door.x < 0) return;

    // The door: a lit doorway on the side of the sign the corridor is on.
    sf::Vector2f d = centerOf(s.door);
    float glow = 0.5f + 0.5f * std::sin(s.pulse);
    sf::Color fill = s.color;
    fill.a = static_cast<sf::Uint8>(45.f + 55.f * glow + 100.f * s.hover);
    fillRect(window, d.x - kTile / 2.f + 3.f, d.y - kTile / 2.f, kTile - 6.f, kTile, fill);

    bool doorBelow = d.y > s.sign.top;
    sf::ConvexShape arrow(3);
    float tip = doorBelow ? -7.f : 7.f;
    arrow.setPoint(0, sf::Vector2f(d.x - 7.f, d.y - tip * 0.6f));
    arrow.setPoint(1, sf::Vector2f(d.x + 7.f, d.y - tip * 0.6f));
    arrow.setPoint(2, sf::Vector2f(d.x, d.y + tip));
    arrow.setFillColor(s.color);
    window.draw(arrow);
}

void ArcadeGame::draw(sf::RenderWindow& window, const sf::Font& font) {
    using WisdomUI::Theme;
    const float mazeW = kCols * kTile, mazeH = kRows * kTile;

    fillRect(window, 0.f, 0.f, 1920.f, 1080.f, kBackdrop);

    bool clearFlash = m_phase == Phase::LevelClear && static_cast<int>(m_phaseTimer * 5.f) % 2 == 0;
    window.draw(clearFlash ? m_wallsFlash : m_walls);

    // Ghost house gate.
    sf::Vector2f gate = centerOf({ m_gateFront.x, m_gateFront.y + 1 });
    fillRect(window, gate.x - kTile / 2.f, gate.y - 3.f, kTile, 6.f, sf::Color(255, 150, 210));

    sf::VertexArray dots(sf::Quads);
    for (int r = 0; r < kRows; ++r) {
        for (int c = 0; c < kCols; ++c) {
            unsigned char kind = m_dots[r * kCols + c];
            sf::Vector2f p = centerOf({ c, r });
            if (kind == 1) {
                addQuad(dots, p.x - 3.f, p.y - 3.f, 6.f, 6.f, sf::Color(255, 215, 160));
            }
            else if (kind == 2 && static_cast<int>(m_time * 4.f) % 2 == 0) {
                fillCircle(window, p.x, p.y, 10.f, Theme::SunsetGold);
            }
            else if (kind == 2) {
                fillCircle(window, p.x, p.y, 8.f, Theme::SunsetAmber);
            }
        }
    }
    window.draw(dots);

    if (m_fruitTimer > 0.f) {
        sf::Vector2f f = centerOf(m_fruitTile);
        f.y += std::sin(m_time * 4.f) * 2.f;
        drawFruit(window, f, fruitKind());
    }

    for (const Station& s : m_stations) drawStation(window, font, s);

    if (m_phase != Phase::Dying && m_phase != Phase::GameOver) {
        for (const Ghost& g : m_ghosts) drawGhost(window, g);
    }

    sf::Vector2f pacPos = posOf(m_pac);
    if (m_phase == Phase::Dying) {
        float progress = std::clamp(1.f - m_phaseTimer / 1.4f, 0.f, 1.f);
        drawPac(window, pacPos, 1, progress * kPi, 15.f);
    }
    else if (m_phase != Phase::GameOver) {
        float mouth = m_pac.moving ? 0.08f + std::abs(std::sin(m_mouth)) * 0.7f : 0.35f;
        drawPac(window, pacPos, m_pac.dir, mouth, 15.f);
    }

    // Hide whatever is half way through the tunnel.
    fillRect(window, 0.f, kOrigin.y, kOrigin.x, mazeH, kBackdrop);
    fillRect(window, kOrigin.x + mazeW, kOrigin.y, 1920.f - kOrigin.x - mazeW, mazeH, kBackdrop);

    for (const Particle& p : m_particles) {
        sf::Color c = p.color;
        c.a = static_cast<sf::Uint8>(std::clamp(p.life / p.maxLife, 0.f, 1.f) * 255.f);
        fillRect(window, p.pos.x, p.pos.y, p.size, p.size, c);
    }
    for (const Floater& f : m_floaters) {
        sf::Uint8 a = static_cast<sf::Uint8>(std::clamp(f.life / f.maxLife, 0.f, 1.f) * 255.f);
        sf::Color c = f.color;
        c.a = a;
        Theme::DrawCrispText(window, font, f.text, 16, f.pos.x, f.pos.y, c, sf::Color(14, 4, 20, a), true, true);
    }

    // Marquee on the block above the ghost house, messages on the block below it.
    sf::Vector2f marquee = centerOf({ m_gateFront.x, 3 });
    Theme::DrawCrispText(window, font, "WISDOM PARK", 22, marquee.x, marquee.y - 12.f, Theme::SunsetGold, kInk, true, true);
    Theme::DrawCrispText(window, font, "ARCADE", 12, marquee.x, marquee.y + 16.f, Theme::SunsetCoral, kInk, true, true);

    sf::Vector2f board = centerOf({ m_pacSpawn.x, m_pacSpawn.y + 2 });
    bool blink = static_cast<int>(m_time * 2.5f) % 2 == 0;
    if (m_phase == Phase::Waiting) {
        Theme::DrawCrispText(window, font, "READY?", 20, board.x, board.y - 12.f, Theme::SunsetGold, kInk, true, true);
        if (blink) Theme::DrawCrispText(window, font, "ARROWS OR W A S D TO PLAY", 11, board.x, board.y + 16.f, sf::Color::White, kInk, true, true);
    }
    else if (m_phase == Phase::Ready) {
        Theme::DrawCrispText(window, font, "READY!", 20, board.x, board.y, Theme::SunsetGold, kInk, true, true);
    }
    else if (m_phase == Phase::LevelClear) {
        Theme::DrawCrispText(window, font, "LEVEL CLEAR", 20, board.x, board.y, Theme::SunsetGlow, kInk, true, true);
    }
    else if (m_phase == Phase::GameOver) {
        Theme::DrawCrispText(window, font, "GAME OVER", 20, board.x, board.y - 12.f, sf::Color(255, 60, 90), kInk, true, true);
        if (blink && m_phaseTimer <= 0.f) Theme::DrawCrispText(window, font, "ARROWS OR W A S D TO RETRY", 11, board.x, board.y + 16.f, sf::Color::White, kInk, true, true);
    }

    window.draw(m_scanlines);

    Theme::DrawCrispText(window, font, "1UP", 15, 120.f, 34.f, sf::Color(255, 60, 90), kInk);
    Theme::DrawCrispText(window, font, std::to_string(m_score), 20, 120.f, 56.f, Theme::SunsetGold, kInk);

    Theme::DrawCrispText(window, font, "LIVES", 15, 420.f, 34.f, sf::Color(255, 60, 90), kInk);
    for (int i = 0; i < m_lives; ++i) {
        drawPac(window, { 432.f + static_cast<float>(i) * 28.f, 70.f }, 0, 0.5f, 10.f);
    }

    Theme::DrawCrispText(window, font, "HIGH SCORE", 15, 960.f, 34.f, sf::Color(255, 60, 90), kInk, true, false);
    Theme::DrawCrispText(window, font, std::to_string(m_highScore), 20, 960.f, 56.f, Theme::SunsetGold, kInk, true, false);

    Theme::DrawCrispText(window, font, "LEVEL", 15, 1220.f, 34.f, sf::Color(255, 60, 90), kInk);
    Theme::DrawCrispText(window, font, std::to_string(m_level), 20, 1220.f, 56.f, Theme::SunsetGold, kInk);

    Theme::DrawCrispText(window, font, "STEER INTO A DOOR TO OPEN IT  -  OR CLICK ITS SIGN", 12, 960.f, 985.f, Theme::SunsetAmber, kInk, true, true);
}
