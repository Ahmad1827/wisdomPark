#include "LivingPark.h"
#include "json.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>

namespace {
    const float kPi = 3.14159265f;
    // Gondolas and birds are pixel art drawn at twice their size
    const float kPixel = 2.f;
    // How far apart the bulbs along the coaster are
    const float kBulbSpacing = 16.f;

    sf::Color withAlpha(sf::Color c, float a) {
        c.a = static_cast<sf::Uint8>(std::clamp(a, 0.f, 1.f) * 255.f);
        return c;
    }

    float smoothstep(float e0, float e1, float x) {
        float t = std::clamp((x - e0) / (e1 - e0), 0.f, 1.f);
        return t * t * (3.f - 2.f * t);
    }

    sf::Vector2f toVec(const nlohmann::json& j) { return { j[0].get<float>(), j[1].get<float>() }; }
}

// ---------------------------------------------------------------------------------------------- tracks

void LivingPark::Track::build() {
    along.assign(points.size(), 0.f);
    for (size_t i = 1; i < points.size(); ++i) {
        sf::Vector2f d = points[i] - points[i - 1];
        along[i] = along[i - 1] + std::sqrt(d.x * d.x + d.y * d.y);
    }
    length = along.empty() ? 0.f : along.back();
}

sf::Vector2f LivingPark::Track::at(float s) const {
    if (points.empty()) return {};
    if (s <= 0.f) return points.front();
    if (s >= length) return points.back();
    size_t i = static_cast<size_t>(std::upper_bound(along.begin(), along.end(), s) - along.begin());
    i = std::clamp<size_t>(i, 1, points.size() - 1);
    float seg = along[i] - along[i - 1];
    float t = seg > 0.f ? (s - along[i - 1]) / seg : 0.f;
    return points[i - 1] + (points[i] - points[i - 1]) * t;
}

// ---------------------------------------------------------------------------------------------- loading

float LivingPark::random(float lo, float hi) {
    return std::uniform_real_distribution<float>(lo, hi)(m_rng);
}

bool LivingPark::init(const std::string& dir) {
    m_ready = false;
    nlohmann::json L;
    try {
        std::ifstream in(dir + "/layout.json", std::ios::binary);
        if (!in) return false;
        L = nlohmann::json::parse(in);
    }
    catch (...) {
        return false;
    }
    if (!m_background.loadFromFile(dir + "/" + L.value("background", "background.png"))) return false;
    if (!m_atlas.loadFromFile(dir + "/" + L.value("atlas", "sprites.png"))) return false;
    if (!m_wheelFront.loadFromFile(dir + "/" + L["wheel"].value("front", "wheel_front.png"))) return false;
    if (!m_atomFront.loadFromFile(dir + "/" + L.value("atom_front", "atom_front.png"))) return false;
    for (sf::Texture* t : { &m_background, &m_atlas, &m_wheelFront, &m_atomFront }) t->setSmooth(false);
    m_wheelFrontAt = toVec(L["wheel"]["front_at"]);
    m_atomFrontAt = toVec(L["atom_front_at"]);

    try {
        for (auto& [name, r] : L["sprites"].items()) m_sprites[name] = sf::IntRect(r[0], r[1], r[2], r[3]);
        Track coaster;
        for (auto& p : L["coaster"]["points"]) coaster.points.push_back(toVec(p));
        coaster.build();
        for (float s = kBulbSpacing * 0.5f; s < coaster.length; s += kBulbSpacing) {
            sf::Vector2f p = coaster.at(s);
            m_coasterBulbs.push_back({ std::round(p.x), std::round(p.y) - 1.f });
        }

        const auto& wheel = L["wheel"];
        m_wheelCenter = { wheel["cx"].get<float>(), wheel["cy"].get<float>() };
        m_wheelRim = wheel["rim"].get<float>();
        m_gondolas = wheel.value("gondolas", 12);
        m_spokes = wheel.value("spokes", 16);
        m_wheelTurn = wheel.value("turn_seconds", 40.f);

        // Lamps glow warm and steady, with a slow breath and a rare flicker; the atom's spheres pulse in a
        // wave; the carousel's bulbs chase round its canopy
        for (auto& p : L["lamps"]) {
            Glow g{ toVec(p), sf::Color(255, 196, 120), random(15.f, 19.f), random(0.f, 6.28f), random(0.6f, 1.4f), 0.55f };
            m_glows.push_back(g);
        }
        for (auto& p : L["atom"]) {
            sf::Vector2f pos = toVec(p);
            Glow g{ pos, sf::Color(255, 170, 90), 30.f, (pos.x + pos.y) * 0.02f, 1.2f, 0.5f };
            m_glows.push_back(g);
        }
        int bulb = 0;
        for (auto& p : L["carousel"]) {
            Glow g{ toVec(p), bulb % 2 ? sf::Color(255, 120, 210) : sf::Color(255, 226, 130), 7.f, bulb * 0.55f, 4.f, 0.9f };
            m_glows.push_back(g);
            ++bulb;
        }
        for (auto& s : L["signs"]) {
            Sign sign;
            sign.box = sf::FloatRect(s[0], s[1], s[2].get<float>() - s[0].get<float>(), s[3].get<float>() - s[1].get<float>());
            sign.color = sf::Color(s[4][0], s[4][1], s[4][2]);
            sign.next = random(3.f, 12.f);
            m_signs.push_back(sign);
        }
        for (auto& p : L["windows"]) {
            m_windows.push_back(toVec(p));
            m_windowPhase.push_back(random(0.f, 60.f));
        }
        const auto& sky = L["sky"];
        m_sky = sf::FloatRect(sky[0], sky[1], sky[2].get<float>() - sky[0].get<float>(), sky[3].get<float>() - sky[1].get<float>());
    }
    catch (...) {
        return false;
    }
    if (m_coasterBulbs.empty()) return false;

    // The wheel is drawn at half size and shown at twice that, so its lines are pixels like the scene's
    unsigned side = static_cast<unsigned>(m_wheelRim + 10.f);
    if (!m_wheelTex.create(side, side)) return false;
    m_wheelTex.setSmooth(false);

    m_ready = true;
    return true;
}

// ---------------------------------------------------------------------------------------------- bulbs

bool LivingPark::bulbOn(int i) const {
    // Eight seconds of each pattern, round and round
    int pattern = static_cast<int>(m_time / 8.f) % 3;
    if (pattern == 0) return (i + static_cast<int>(m_time * 1.6f)) % 2 == 0;
    if (pattern == 1) return std::fmod(static_cast<float>(i) + 1000.f - std::fmod(m_time * 9.f, 1000.f), 6.f) < 3.f;
    return std::fmod(m_time, 1.4f) < 0.8f;
}

void LivingPark::drawBulbs(sf::VertexArray& lit, sf::VertexArray& unlit, const std::vector<sf::Vector2f>& at, int offset,
    sf::Color a, sf::Color b, float radius) {
    const sf::IntRect& dot = m_sprites["dot"];
    const sf::IntRect& glowSrc = m_sprites["glow"];
    sf::Vector2f glowMid(glowSrc.width * 0.5f, glowSrc.height * 0.5f);
    float size = radius * 2.f / glowSrc.width;
    for (size_t k = 0; k < at.size(); ++k) {
        int i = static_cast<int>(k) + offset;
        sf::Color c = i % 2 ? b : a;
        if (bulbOn(i)) {
            quad(lit, glowSrc, at[k], { size, size }, 0.f, glowMid, withAlpha(c, 0.95f));
            quad(lit, dot, at[k], { 2.f, 2.f }, 0.f, { 0.5f, 0.5f }, withAlpha(sf::Color(255, 246, 236), 0.95f));
        }
        else {
            quad(unlit, dot, at[k], { 2.f, 2.f }, 0.f, { 0.5f, 0.5f }, sf::Color(c.r / 3, c.g / 3, c.b / 3));
        }
    }
}

// ---------------------------------------------------------------------------------------------- update

void LivingPark::update(float dt) {
    if (!m_ready) return;
    dt = std::min(dt, 0.1f);
    m_time += dt;
    m_wheelAngle += 2.f * kPi / m_wheelTurn * dt;

    for (auto& g : m_glows) {
        if (g.flicker > 0.f) g.flicker -= dt;
        else if (g.rate < 2.f && random(0.f, 1.f) < dt * 0.01f) g.flicker = random(0.05f, 0.25f);
    }
    for (auto& s : m_signs) {
        if (s.off > 0.f) s.off -= dt;
        s.next -= dt;
        if (s.next <= 0.f) {
            s.off = random(0.06f, 0.3f);
            s.next = random(0.f, 1.f) < 0.3f ? random(0.15f, 0.4f) : random(5.f, 16.f);
        }
    }

    // Now and then a few birds cross the sky
    m_birdWait -= dt;
    if (m_birdWait <= 0.f) {
        m_birdWait = random(12.f, 28.f);
        bool leftToRight = random(0.f, 1.f) < 0.5f;
        float y = random(m_sky.top + 10.f, m_sky.top + m_sky.height);
        float speed = random(34.f, 52.f) * (leftToRight ? 1.f : -1.f);
        int n = static_cast<int>(random(3.f, 6.f));
        for (int i = 0; i < n; ++i) {
            Bird b;
            b.pos = sf::Vector2f(leftToRight ? -20.f - i * 14.f : 1940.f + i * 14.f, y + (i % 2 ? 6.f : -4.f) * (i / 2 + 1));
            b.speed = speed;
            b.flap = random(0.f, 1.f);
            m_birds.push_back(b);
        }
    }
    for (auto& b : m_birds) {
        b.pos.x += b.speed * dt;
        b.flap += dt * 2.4f;
    }
    m_birds.erase(std::remove_if(m_birds.begin(), m_birds.end(), [](const Bird& b) { return b.pos.x < -80.f || b.pos.x > 2000.f; }), m_birds.end());

}

// ---------------------------------------------------------------------------------------------- drawing

void LivingPark::quad(sf::VertexArray& va, const sf::IntRect& src, sf::Vector2f pos, sf::Vector2f scale, float angleDeg,
    sf::Vector2f origin, sf::Color color, bool flip) const {
    sf::Transform t;
    t.translate(pos).rotate(angleDeg).scale(scale).translate(-origin);
    float w = static_cast<float>(src.width), h = static_cast<float>(src.height);
    float u0 = static_cast<float>(src.left), u1 = static_cast<float>(src.left + src.width);
    float v0 = static_cast<float>(src.top), v1 = static_cast<float>(src.top + src.height);
    if (flip) std::swap(u0, u1);
    va.append(sf::Vertex(t.transformPoint(0.f, 0.f), color, { u0, v0 }));
    va.append(sf::Vertex(t.transformPoint(w, 0.f), color, { u1, v0 }));
    va.append(sf::Vertex(t.transformPoint(w, h), color, { u1, v1 }));
    va.append(sf::Vertex(t.transformPoint(0.f, h), color, { u0, v1 }));
}

void LivingPark::drawWheel(sf::RenderTarget& target) {
    // Rim, spokes and hub at half size into the wheel texture, then shown at twice that
    float side = static_cast<float>(m_wheelTex.getSize().x);
    sf::Vector2f c(side * 0.5f, side * 0.5f);
    float r = m_wheelRim * 0.5f;
    m_wheelTex.clear(sf::Color::Transparent);
    sf::VertexArray lines(sf::Lines);
    auto line = [&](sf::Vector2f a, sf::Vector2f b, sf::Color col) {
        lines.append(sf::Vertex(sf::Vector2f(std::floor(a.x) + 0.5f, std::floor(a.y) + 0.5f), col));
        lines.append(sf::Vertex(sf::Vector2f(std::floor(b.x) + 0.5f, std::floor(b.y) + 0.5f), col));
    };
    auto ring = [&](float radius, sf::Color col) {
        const int n = 96;
        for (int i = 0; i < n; ++i) {
            float a0 = 2.f * kPi * i / n, a1 = 2.f * kPi * (i + 1) / n;
            line(c + radius * sf::Vector2f(std::cos(a0), std::sin(a0)), c + radius * sf::Vector2f(std::cos(a1), std::sin(a1)), col);
        }
    };
    const sf::Color rim(206, 92, 206), rimDark(98, 40, 128), inner(150, 58, 160), cyan(84, 210, 228), magenta(200, 84, 200),
        cable(70, 44, 110), truss(128, 52, 150);
    const float outer = r, outerIn = r - 4.f, mid = r * 0.62f, midIn = mid - 3.f, hubR = 8.f;
    auto at = [&](float radius, float a) { return c + radius * sf::Vector2f(std::cos(a), std::sin(a)); };
    // Thin cables from the edge of the hub out to the rim, leaning both ways, behind everything else
    for (int i = 0; i < m_spokes; ++i) {
        float a = m_wheelAngle + 2.f * kPi * i / m_spokes;
        float lean = kPi / m_spokes;
        line(at(hubR, a), at(outerIn, a + lean), cable);
        line(at(hubR, a), at(outerIn, a - lean), cable);
    }
    // The rim and the middle ring are each a pair of rings with a zigzag truss between them
    const int teeth = m_spokes * 4;
    for (int i = 0; i < teeth; ++i) {
        float a0 = m_wheelAngle + 2.f * kPi * i / teeth, a1 = m_wheelAngle + 2.f * kPi * (i + 1) / teeth;
        line(at(i % 2 ? outer : outerIn, a0), at(i % 2 ? outerIn : outer, a1), truss);
        if (i % 2 == 0) line(at(mid, a0), at(midIn, a1), rimDark);
    }
    // The spokes, doubled out to the middle ring and single from there to the rim
    for (int i = 0; i < m_spokes; ++i) {
        float a = m_wheelAngle + 2.f * kPi * i / m_spokes;
        sf::Color col = i % 2 ? magenta : cyan;
        float side = 1.6f / hubR;
        line(at(hubR, a - side), at(midIn, a - side * hubR / midIn), col);
        line(at(hubR, a + side), at(midIn, a + side * hubR / midIn), col);
        line(at(mid, a), at(outerIn, a), col);
        // braces between neighbouring spokes, from the middle ring out to the rim
        float b = m_wheelAngle + 2.f * kPi * (i + 1) / m_spokes;
        line(at(mid, a), at(outerIn, b), rimDark);
    }
    ring(midIn, rimDark);
    ring(mid, inner);
    ring(outerIn, rimDark);
    ring(outer - 1.f, rimDark);
    ring(outer, rim);
    m_wheelTex.draw(lines);
    // The hub: a ring of bolts round a cap
    sf::CircleShape hub(hubR, 24);
    hub.setOrigin(hubR, hubR);
    hub.setPosition(c);
    hub.setFillColor(sf::Color(60, 190, 210));
    m_wheelTex.draw(hub);
    hub.setRadius(6.f);
    hub.setOrigin(6.f, 6.f);
    hub.setFillColor(sf::Color(74, 34, 104));
    m_wheelTex.draw(hub);
    sf::VertexArray bolts(sf::Points);
    for (int i = 0; i < 8; ++i) {
        sf::Vector2f p = at(6.f, m_wheelAngle + 2.f * kPi * i / 8);
        bolts.append(sf::Vertex(sf::Vector2f(std::floor(p.x) + 0.5f, std::floor(p.y) + 0.5f), sf::Color(255, 214, 246)));
    }
    m_wheelTex.draw(bolts);
    hub.setRadius(4.f);
    hub.setOrigin(4.f, 4.f);
    hub.setFillColor(sf::Color(226, 110, 214));
    m_wheelTex.draw(hub);
    hub.setRadius(1.5f);
    hub.setOrigin(1.5f, 1.5f);
    hub.setFillColor(sf::Color(255, 214, 246));
    m_wheelTex.draw(hub);
    m_wheelTex.display();

    sf::Sprite wheel(m_wheelTex.getTexture());
    wheel.setScale(2.f, 2.f);
    wheel.setPosition(std::round(m_wheelCenter.x - side), std::round(m_wheelCenter.y - side));
    target.draw(wheel);

    // Bulbs round the rim and at the ends of the spokes on the middle ring, turning with the wheel
    {
        std::vector<sf::Vector2f> rimBulbs, midBulbs;
        int n = m_spokes * 2;
        for (int i = 0; i < n; ++i) {
            float a = m_wheelAngle + 2.f * kPi * (i + 0.5f) / n;
            sf::Vector2f p = m_wheelCenter + (m_wheelRim - 4.f) * sf::Vector2f(std::cos(a), std::sin(a));
            rimBulbs.push_back({ std::round(p.x), std::round(p.y) });
        }
        for (int i = 0; i < m_spokes; ++i) {
            float a = m_wheelAngle + 2.f * kPi * i / m_spokes;
            sf::Vector2f p = m_wheelCenter + m_wheelRim * 0.62f * sf::Vector2f(std::cos(a), std::sin(a));
            midBulbs.push_back({ std::round(p.x), std::round(p.y) });
        }
        sf::VertexArray lit(sf::Quads), unlit(sf::Quads);
        drawBulbs(lit, unlit, rimBulbs, 0, sf::Color(255, 220, 130), sf::Color(255, 110, 210), 12.f);
        drawBulbs(lit, unlit, midBulbs, 1, sf::Color(90, 226, 255), sf::Color(90, 226, 255), 9.f);
        target.draw(unlit, &m_atlas);
        sf::RenderStates add(&m_atlas);
        add.blendMode = sf::BlendAdd;
        target.draw(lit, add);
    }

    // The gondolas hang from the rim and stay upright, swaying a little
    sf::VertexArray va(sf::Quads);
    for (int i = 0; i < m_gondolas; ++i) {
        float a = m_wheelAngle + 2.f * kPi * i / m_gondolas;
        sf::Vector2f pin = m_wheelCenter + m_wheelRim * sf::Vector2f(std::cos(a), std::sin(a));
        const sf::IntRect& src = m_sprites[i % 2 ? "gondola_pink" : "gondola_teal"];
        float sway = 4.f * std::sin(m_time * 1.3f + i * 0.9f);
        quad(va, src, { std::round(pin.x), std::round(pin.y) }, { kPixel, kPixel }, sway, { src.width * 0.5f, 0.f });
    }
    target.draw(va, &m_atlas);

    sf::Sprite legs(m_wheelFront);
    legs.setPosition(m_wheelFrontAt);
    target.draw(legs);
}

void LivingPark::draw(sf::RenderTarget& target) {
    if (!m_ready) return;
    sf::Sprite bg(m_background);
    bg.setScale(1920.f / m_background.getSize().x, 1080.f / m_background.getSize().y);
    target.draw(bg);

    sf::RenderStates add(&m_atlas);
    add.blendMode = sf::BlendAdd;
    const sf::IntRect& dot = m_sprites["dot"];
    const sf::IntRect& glowSrc = m_sprites["glow"];
    sf::Vector2f glowMid(glowSrc.width * 0.5f, glowSrc.height * 0.5f);

    // The sky: windows lighting up in the city, birds
    {
        sf::VertexArray va(sf::Quads);
        for (size_t i = 0; i < m_windows.size(); ++i) {
            float on = smoothstep(0.2f, 0.6f, std::sin(m_time * 0.11f + m_windowPhase[i]));
            if (on <= 0.f) continue;
            quad(va, dot, m_windows[i], { 2.f, 2.f }, 0.f, {}, withAlpha(sf::Color(255, 210, 120), on * 0.85f));
        }
        target.draw(va, &m_atlas);

        sf::VertexArray birds(sf::Quads);
        for (const auto& b : m_birds) {
            const sf::IntRect& src = m_sprites[(static_cast<int>(b.flap) % 2) ? "bird1" : "bird0"];
            quad(birds, src, b.pos, { kPixel, kPixel }, 0.f, { src.width * 0.5f, src.height * 0.5f }, sf::Color::White, b.speed < 0.f);
        }
        target.draw(birds, &m_atlas);
    }

    // The coaster's lights, then the atom in front of them
    {
        sf::VertexArray lit(sf::Quads), unlit(sf::Quads);
        drawBulbs(lit, unlit, m_coasterBulbs, 0, sf::Color(255, 96, 214), sf::Color(90, 226, 255), 10.f);
        target.draw(unlit, &m_atlas);
        target.draw(lit, add);
        sf::Sprite atom(m_atomFront);
        atom.setPosition(m_atomFrontAt);
        target.draw(atom);
    }

    drawWheel(target);

    // Light: lamps, the atom, the carousel's bulbs, the signs
    {
        sf::VertexArray va(sf::Quads);
        for (const auto& g : m_glows) {
            float a;
            if (g.rate >= 3.f) a = 0.35f + 0.65f * smoothstep(0.3f, 1.f, std::sin(m_time * g.rate - g.phase));    // chasing bulbs
            else a = 0.75f + 0.25f * std::sin(m_time * g.rate + g.phase);
            if (g.flicker > 0.f) a *= 0.15f;
            float size = g.radius * 2.f / glowSrc.width;
            quad(va, glowSrc, g.pos, { size, size }, 0.f, glowMid, withAlpha(g.color, a * g.strength));
        }
        // The neon signs' glow; when one flickers, its glow drops for a moment
        for (const auto& s : m_signs) {
            sf::Vector2f mid(s.box.left + s.box.width * 0.5f, s.box.top + s.box.height * 0.5f);
            float sx = s.box.width * 1.9f / glowSrc.width, sy = s.box.height * 2.4f / glowSrc.height;
            float a = 0.32f + 0.08f * std::sin(m_time * 2.1f + mid.x);
            if (s.off > 0.f) a *= 0.25f;
            quad(va, glowSrc, mid, { sx, sy }, 0.f, glowMid, withAlpha(s.color, a));
        }
        target.draw(va, add);
    }
}
