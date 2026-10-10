#include "LoadingScene.h"
#include "UITheme.h"
#include <algorithm>
#include <cmath>
#include <string>

using WisdomUI::Theme;

namespace {
    const sf::FloatRect kPanel(580.f, 240.f, 760.f, 560.f);
    const sf::FloatRect kScene(600.f, 300.f, 720.f, 380.f);
    const sf::FloatRect kCancel(880.f, 740.f, 160.f, 44.f);
    const float kHorizon = kScene.top + 210.f;
    const float kSceneRight = kScene.left + kScene.width;
    const float kSceneBottom = kScene.top + kScene.height;
    const float kSunX = kScene.left + 520.f;

    const char* kTips[] = {
        "Steer the boat with your mouse",
        "Click the sky to drop a star, then sail over it",
        "Click the water to wake a fish",
        "Esc or Cancel stops the request"
    };

    sf::Color alpha(sf::Color c, float a) {
        c.a = static_cast<sf::Uint8>(std::clamp(a, 0.f, 1.f) * static_cast<float>(c.a));
        return c;
    }
}

void LoadingScene::reset() {
    time = 0.f;
    boatX = kScene.left + kScene.width * 0.5f;
    boatVel = 0.f;
    boatDir = 1.f;
    starTimer = 2.f;
    wakeTimer = 0.f;
    starsCaught = 0;
    stars.clear();
    fish.clear();
    ripples.clear();
    particles.clear();
}

float LoadingScene::rand01() {
    seed = seed * 1664525u + 1013904223u;
    return static_cast<float>((seed >> 8) & 0xFFFF) / 65535.f;
}

float LoadingScene::waveY(float x, float base, float amp, float speed, float phase) const {
    return base + amp * std::sin(x * 0.018f + time * speed + phase) + amp * 0.45f * std::sin(x * 0.041f - time * speed * 0.7f + phase * 2.f);
}

float LoadingScene::boatSurface(float x) const {
    return waveY(x, kHorizon + 62.f, 6.f, 1.4f, 1.f);
}

void LoadingScene::splash(float x, float y, int count, sf::Color color) {
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.pos = sf::Vector2f(x + (rand01() - 0.5f) * 10.f, y);
        p.vel = sf::Vector2f((rand01() - 0.5f) * 140.f, -60.f - rand01() * 120.f);
        p.maxLife = p.life = 0.5f + rand01() * 0.4f;
        p.size = 2.f + rand01() * 2.f;
        p.color = color;
        particles.push_back(p);
    }
}

void LoadingScene::update(float dt, sf::Vector2f mouse) {
    dt = std::min(dt, 0.05f);
    time += dt;

    // The boat chases the mouse while it is over the scene, otherwise it drifts on its own
    float target = kScene.left + kScene.width * 0.5f + std::sin(time * 0.35f) * 230.f;
    if (kScene.contains(mouse)) target = mouse.x;
    target = std::clamp(target, kScene.left + 50.f, kSceneRight - 50.f);
    boatVel = std::clamp((target - boatX) * 2.2f, -240.f, 240.f);
    boatX += boatVel * dt;
    if (boatVel > 10.f) boatDir = 1.f;
    else if (boatVel < -10.f) boatDir = -1.f;

    wakeTimer -= dt;
    if (std::abs(boatVel) > 50.f && wakeTimer <= 0.f) {
        wakeTimer = 0.06f;
        Particle p;
        p.pos = sf::Vector2f(boatX - boatDir * 30.f, boatSurface(boatX - boatDir * 30.f) + 2.f);
        p.vel = sf::Vector2f(-boatDir * 30.f, -14.f - rand01() * 16.f);
        p.maxLife = p.life = 0.5f;
        p.size = 2.5f;
        p.color = sf::Color(255, 230, 220, 200);
        particles.push_back(p);
    }

    starTimer -= dt;
    if (starTimer <= 0.f) {
        starTimer = 3.5f + rand01() * 2.5f;
        if (stars.size() < 4) stars.push_back({ kScene.left + 40.f + rand01() * (kScene.width - 80.f), kScene.top + 20.f, 40.f, false });
    }

    for (Star& s : stars) {
        float surface = boatSurface(s.x);
        if (!s.floating) {
            s.vy += 320.f * dt;
            s.y += s.vy * dt;
            if (s.y >= surface) {
                s.floating = true;
                ripples.push_back({ s.x, surface, 0.f });
                splash(s.x, surface, 6, sf::Color(255, 240, 200, 220));
            }
        }
        else {
            s.x = std::clamp(s.x - 5.f * dt, kScene.left + 20.f, kSceneRight - 20.f);
            s.y = boatSurface(s.x) - 5.f;
        }
    }
    for (size_t i = 0; i < stars.size();) {
        if (stars[i].floating && std::abs(stars[i].x - boatX) < 34.f) {
            splash(stars[i].x, stars[i].y, 12, Theme::SunsetGold);
            ++starsCaught;
            stars.erase(stars.begin() + i);
        }
        else ++i;
    }

    for (size_t i = 0; i < fish.size();) {
        Fish& f = fish[i];
        f.vel.y += 520.f * dt;
        f.pos += f.vel * dt;
        float surface = boatSurface(f.pos.x);
        if (f.vel.y > 0.f && f.pos.y > surface) {
            ripples.push_back({ f.pos.x, surface, 0.f });
            splash(f.pos.x, surface, 6, sf::Color(255, 230, 220, 200));
            fish.erase(fish.begin() + i);
        }
        else ++i;
    }

    for (Ripple& r : ripples) r.age += dt;
    ripples.erase(std::remove_if(ripples.begin(), ripples.end(), [](const Ripple& r) { return r.age > 1.2f; }), ripples.end());

    for (Particle& p : particles) {
        p.vel.y += 260.f * dt;
        p.pos += p.vel * dt;
        p.life -= dt;
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& p) { return p.life <= 0.f; }), particles.end());
}

bool LoadingScene::handleClick(sf::Vector2f mouse) {
    if (kCancel.contains(mouse)) return true;
    if (!kScene.contains(mouse)) return false;

    float surface = boatSurface(mouse.x);
    if (mouse.y < surface - 6.f) {
        if (stars.size() < 8) stars.push_back({ mouse.x, mouse.y, 0.f, false });
    }
    else {
        ripples.push_back({ mouse.x, surface, 0.f });
        splash(mouse.x, surface, 8, sf::Color(255, 230, 220, 200));
        if (fish.size() < 6) fish.push_back({ sf::Vector2f(mouse.x, surface), sf::Vector2f((rand01() - 0.5f) * 160.f, -250.f - rand01() * 60.f) });
    }
    return false;
}

void LoadingScene::drawWaveLayer(sf::RenderWindow& window, float base, float amp, float speed, float phase, sf::Color top, sf::Color bottom, sf::Color foam) const {
    sf::VertexArray body(sf::TriangleStrip);
    sf::VertexArray crest(sf::LineStrip);
    for (float x = kScene.left; x <= kSceneRight + 0.5f; x += 8.f) {
        float y = waveY(x, base, amp, speed, phase);
        body.append(sf::Vertex(sf::Vector2f(x, y), top));
        body.append(sf::Vertex(sf::Vector2f(x, kSceneBottom), bottom));
        crest.append(sf::Vertex(sf::Vector2f(x, y), foam));
    }
    window.draw(body);
    window.draw(crest);
}

void LoadingScene::drawBoat(sf::RenderWindow& window) const {
    float y = boatSurface(boatX);
    float slope = (boatSurface(boatX + 12.f) - boatSurface(boatX - 12.f)) / 24.f;

    sf::RenderStates states;
    states.transform.translate(boatX, y + 2.f);
    states.transform.rotate(std::atan(slope) * 57.2958f * 0.8f);
    states.transform.scale(boatDir, 1.f);

    sf::RectangleShape mast(sf::Vector2f(3.f, 62.f));
    mast.setPosition(-1.5f, -72.f);
    mast.setFillColor(sf::Color(70, 40, 44));
    window.draw(mast, states);

    // Sails lean a little with speed
    float billow = 4.f + std::abs(boatVel) * 0.03f + std::sin(time * 3.f) * 1.5f;
    sf::ConvexShape mainSail(3);
    mainSail.setPoint(0, sf::Vector2f(-3.f, -68.f));
    mainSail.setPoint(1, sf::Vector2f(-3.f, -16.f));
    mainSail.setPoint(2, sf::Vector2f(-34.f - billow, -18.f));
    mainSail.setFillColor(sf::Color(255, 244, 226));
    window.draw(mainSail, states);

    sf::ConvexShape jib(3);
    jib.setPoint(0, sf::Vector2f(3.f, -60.f));
    jib.setPoint(1, sf::Vector2f(3.f, -18.f));
    jib.setPoint(2, sf::Vector2f(26.f + billow * 0.5f, -18.f));
    jib.setFillColor(Theme::SunsetPeach);
    window.draw(jib, states);

    sf::ConvexShape flag(3);
    flag.setPoint(0, sf::Vector2f(0.f, -72.f));
    flag.setPoint(1, sf::Vector2f(0.f, -64.f));
    flag.setPoint(2, sf::Vector2f(-12.f, -68.f + std::sin(time * 8.f) * 2.f));
    flag.setFillColor(Theme::SunsetCoral);
    window.draw(flag, states);

    sf::ConvexShape hull(4);
    hull.setPoint(0, sf::Vector2f(-34.f, -12.f));
    hull.setPoint(1, sf::Vector2f(40.f, -12.f));
    hull.setPoint(2, sf::Vector2f(26.f, 6.f));
    hull.setPoint(3, sf::Vector2f(-26.f, 6.f));
    hull.setFillColor(sf::Color(124, 70, 58));
    window.draw(hull, states);

    sf::RectangleShape trim(sf::Vector2f(74.f, 3.f));
    trim.setPosition(-34.f, -12.f);
    trim.setFillColor(Theme::SunsetAmber);
    window.draw(trim, states);
}

void LoadingScene::draw(sf::RenderWindow& window, const sf::Font& font, sf::Vector2f mouse) {
    sf::RectangleShape dim(sf::Vector2f(1920.f, 1080.f));
    dim.setFillColor(sf::Color(10, 4, 16, 200));
    window.draw(dim);

    Theme::DrawSunsetPanel(window, kPanel, 1.0f);

    std::string title = "Sketching it out";
    title.append(static_cast<size_t>(time * 2.f) % 4, '.');
    float titleW = Theme::MeasureText(font, "Sketching it out", 26);
    Theme::DrawCrispText(window, font, title, 26, 960.f - titleW * 0.5f, 270.f, Theme::SunsetGold, Theme::SunsetDeepDark, false, true);

    // Sky
    const sf::Color skyTop(29, 19, 46), skyMid(128, 58, 112), skyLow(255, 160, 122);
    float midY = kScene.top + 120.f;
    sf::VertexArray sky(sf::Quads, 8);
    sky[0] = sf::Vertex(sf::Vector2f(kScene.left, kScene.top), skyTop);
    sky[1] = sf::Vertex(sf::Vector2f(kSceneRight, kScene.top), skyTop);
    sky[2] = sf::Vertex(sf::Vector2f(kSceneRight, midY), skyMid);
    sky[3] = sf::Vertex(sf::Vector2f(kScene.left, midY), skyMid);
    sky[4] = sf::Vertex(sf::Vector2f(kScene.left, midY), skyMid);
    sky[5] = sf::Vertex(sf::Vector2f(kSceneRight, midY), skyMid);
    sky[6] = sf::Vertex(sf::Vector2f(kSceneRight, kHorizon + 20.f), skyLow);
    sky[7] = sf::Vertex(sf::Vector2f(kScene.left, kHorizon + 20.f), skyLow);
    window.draw(sky);

    // Fixed twinkling stars in the upper sky
    unsigned int s = 12345u;
    auto next = [&s]() { s = s * 1664525u + 1013904223u; return static_cast<float>((s >> 8) & 0xFFFF) / 65535.f; };
    for (int i = 0; i < 34; ++i) {
        float x = kScene.left + 10.f + next() * (kScene.width - 20.f);
        float y = kScene.top + 8.f + next() * 100.f;
        float tw = 0.45f + 0.55f * std::sin(time * (1.5f + next() * 2.5f) + next() * 6.28f);
        sf::RectangleShape dot(sf::Vector2f(2.f, 2.f));
        dot.setPosition(std::floor(x), std::floor(y));
        dot.setFillColor(alpha(sf::Color(255, 246, 220, 220), tw * (1.f - (y - kScene.top) / 140.f)));
        window.draw(dot);
    }

    // Setting sun with a soft halo
    for (int i = 3; i >= 0; --i) {
        float r = 44.f + static_cast<float>(i) * 16.f + std::sin(time * 1.2f) * 2.f;
        sf::CircleShape halo(r, 48);
        halo.setOrigin(r, r);
        halo.setPosition(kSunX, kHorizon + 4.f);
        halo.setFillColor(i == 0 ? Theme::SunsetGlow : sf::Color(255, 210, 140, static_cast<sf::Uint8>(40 - i * 9)));
        window.draw(halo);
    }

    // Clouds drift and fade out at the edges of the scene
    for (int i = 0; i < 3; ++i) {
        float w = 110.f + static_cast<float>(i) * 30.f;
        float span = kScene.width - w;
        float x = kScene.left + std::fmod(static_cast<float>(i) * 260.f + time * (9.f + static_cast<float>(i) * 4.f), span);
        float y = kScene.top + 40.f + static_cast<float>(i) * 34.f;
        float edge = std::min(x - kScene.left, kScene.left + span - x) / 70.f;
        sf::Color c = alpha(sf::Color(255, 206, 196, 110), edge);
        sf::RectangleShape puff(sf::Vector2f(w, 10.f));
        puff.setPosition(x, y);
        puff.setFillColor(c);
        window.draw(puff);
        puff.setSize(sf::Vector2f(w * 0.6f, 10.f));
        puff.setPosition(x + w * 0.15f, y - 9.f);
        window.draw(puff);
        puff.setSize(sf::Vector2f(w * 0.5f, 8.f));
        puff.setPosition(x + w * 0.4f, y + 9.f);
        window.draw(puff);
    }

    // Stars still falling are drawn against the sky
    auto drawStar = [&](float x, float y) {
        float pulse = 1.f + 0.2f * std::sin(time * 6.f + x);
        sf::CircleShape glow(11.f * pulse);
        glow.setOrigin(11.f * pulse, 11.f * pulse);
        glow.setPosition(x, y);
        glow.setFillColor(sf::Color(255, 226, 110, 60));
        window.draw(glow);
        sf::CircleShape core(5.f, 4);
        core.setOrigin(5.f, 5.f);
        core.setPosition(x, y);
        core.setFillColor(Theme::SunsetGlow);
        window.draw(core);
    };
    for (const Star& st : stars) if (!st.floating) drawStar(st.x, st.y);

    // Sea: far swell, sun glitter, the boat on the middle swell, near swell in front
    drawWaveLayer(window, kHorizon + 14.f, 3.f, 0.9f, 0.f, sf::Color(104, 62, 132), sf::Color(60, 40, 104), sf::Color(255, 190, 160, 120));

    for (int i = 0; i < 9; ++i) {
        float y = kHorizon + 22.f + static_cast<float>(i) * 17.f;
        float w = 30.f + static_cast<float>(i) * 9.f + std::sin(time * 2.4f + static_cast<float>(i) * 1.7f) * 14.f;
        sf::RectangleShape glint(sf::Vector2f(w, 3.f));
        glint.setPosition(kSunX - w * 0.5f + std::sin(time * 1.3f + static_cast<float>(i)) * 6.f, y);
        glint.setFillColor(sf::Color(255, 226, 150, static_cast<sf::Uint8>(150 - i * 12)));
        window.draw(glint);
    }

    for (const Fish& f : fish) {
        sf::RenderStates states;
        states.transform.translate(f.pos);
        states.transform.rotate(std::atan2(f.vel.y, f.vel.x) * 57.2958f);
        sf::CircleShape body(9.f, 16);
        body.setOrigin(9.f, 9.f);
        body.setScale(1.f, 0.5f);
        body.setFillColor(Theme::SunsetCoral);
        window.draw(body, states);
        sf::ConvexShape tail(3);
        tail.setPoint(0, sf::Vector2f(-7.f, 0.f));
        tail.setPoint(1, sf::Vector2f(-16.f, -6.f));
        tail.setPoint(2, sf::Vector2f(-16.f, 6.f));
        tail.setFillColor(Theme::SunsetPeach);
        window.draw(tail, states);
    }

    drawBoat(window);
    for (const Star& st : stars) if (st.floating) drawStar(st.x, st.y);

    drawWaveLayer(window, kHorizon + 66.f, 6.f, 1.4f, 1.f, sf::Color(72, 46, 118, 235), sf::Color(40, 28, 84), sf::Color(255, 210, 190, 150));

    for (const Ripple& r : ripples) {
        float radius = 6.f + r.age * 36.f;
        sf::CircleShape ring(radius, 32);
        ring.setOrigin(radius, radius);
        ring.setPosition(r.x, r.y + 4.f);
        ring.setScale(1.f, 0.3f);
        ring.setFillColor(sf::Color::Transparent);
        ring.setOutlineThickness(1.5f);
        ring.setOutlineColor(alpha(sf::Color(255, 230, 220, 200), 1.f - r.age / 1.2f));
        window.draw(ring);
    }

    drawWaveLayer(window, kHorizon + 124.f, 8.f, 1.9f, 2.4f, sf::Color(44, 30, 86), sf::Color(22, 14, 44), sf::Color(200, 150, 190, 130));

    for (const Particle& p : particles) {
        if (p.pos.x < kScene.left || p.pos.x > kSceneRight || p.pos.y < kScene.top || p.pos.y > kSceneBottom) continue;
        sf::RectangleShape bit(sf::Vector2f(p.size, p.size));
        bit.setPosition(p.pos);
        bit.setFillColor(alpha(p.color, p.life / p.maxLife));
        window.draw(bit);
    }

    sf::RectangleShape frame(sf::Vector2f(kScene.width, kScene.height));
    frame.setPosition(kScene.left, kScene.top);
    frame.setFillColor(sf::Color::Transparent);
    frame.setOutlineThickness(2.f);
    frame.setOutlineColor(Theme::SunsetPlum);
    window.draw(frame);

    // Footer: stars caught, a rotating tip, time waited
    float footY = kSceneBottom + 26.f;
    Theme::DrawCrispText(window, font, "Stars caught: " + std::to_string(starsCaught), 16, kScene.left + 4.f, footY, Theme::SunsetGold, sf::Color::Transparent, false, true);

    int secs = static_cast<int>(time);
    std::string clock = std::to_string(secs / 60) + ":" + (secs % 60 < 10 ? "0" : "") + std::to_string(secs % 60);
    Theme::DrawCrispText(window, font, clock, 16, kSceneRight - 4.f - Theme::MeasureText(font, clock, 16), footY, Theme::TextMuted, sf::Color::Transparent, false, true);

    const char* tip = kTips[static_cast<int>(time / 5.f) % 4];
    Theme::DrawCrispText(window, font, tip, 16, 960.f, footY, Theme::TextSecondary, sf::Color::Transparent, true, true);

    Theme::DrawSunsetButton(window, kCancel, "Cancel", font, 16, false, kCancel.contains(mouse), true, 1.0f);
}
