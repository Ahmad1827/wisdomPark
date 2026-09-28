#include "PencilStroke.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace {
    const float kMinSpacing = 0.5f;
    const float kTilt = 0.8f;   // lean of the pencil (right-handed writer), radians
    const int   kLanes = 7;     // vertices across the stroke width (graphite striations)

    float len(sf::Vector2f v) {
        return std::hypot(v.x, v.y);
    }

    sf::Vector2f normalize(sf::Vector2f v) {
        float l = len(v);
        if (l < 0.0001f) return sf::Vector2f(0.f, 0.f);
        return v / l;
    }

    float sstep(float e0, float e1, float x) {
        float t = std::clamp((x - e0) / (e1 - e0), 0.f, 1.f);
        return t * t * (3.f - 2.f * t);
    }

    float hash2(int x, int y) {
        uint32_t h = static_cast<uint32_t>(x) * 374761393u + static_cast<uint32_t>(y) * 668265263u;
        h = (h ^ (h >> 13)) * 1274126177u;
        h ^= h >> 16;
        return static_cast<float>(h & 0xFFFFu) / 65535.f;
    }

    float valueNoise(float x, float y) {
        float fx = std::floor(x);
        float fy = std::floor(y);
        int ix = static_cast<int>(fx);
        int iy = static_cast<int>(fy);
        float tx = x - fx;
        float ty = y - fy;
        tx = tx * tx * (3.f - 2.f * tx);
        ty = ty * ty * (3.f - 2.f * ty);
        float a = hash2(ix, iy);
        float b = hash2(ix + 1, iy);
        float c = hash2(ix, iy + 1);
        float d = hash2(ix + 1, iy + 1);
        return a + (b - a) * tx + (c - a) * ty + (a - b - c + d) * tx * ty;
    }

    // Paper tooth, fixed in canvas space. Fibers slightly stretched horizontally like notebook paper.
    float paperGrain(sf::Vector2f p, float seed) {
        return 0.55f * valueNoise(p.x * 0.55f + seed, p.y * 1.25f - seed)
            + 0.30f * valueNoise(p.x * 1.60f - seed * 1.7f, p.y * 1.90f + seed * 0.6f)
            + 0.15f * valueNoise(p.x * 4.30f + seed * 2.3f, p.y * 4.10f - seed * 0.9f);
    }

    sf::Vector2f catmull(sf::Vector2f p0, sf::Vector2f p1, sf::Vector2f p2, sf::Vector2f p3, float t) {
        float t2 = t * t;
        float t3 = t2 * t;
        return 0.5f * ((2.f * p1)
            + (p2 - p0) * t
            + (2.f * p0 - 5.f * p1 + 4.f * p2 - p3) * t2
            + (3.f * p1 - p0 - 3.f * p2 + p3) * t3);
    }

    void appendSoftCap(sf::VertexArray& va, sf::Vector2f center, sf::Vector2f dir, float r, sf::Color coreCol, sf::Color edgeCol, bool forward) {
        const int steps = 8;
        float baseAngle = std::atan2(dir.y, dir.x) + (forward ? -1.5707963f : 1.5707963f);
        for (int i = 0; i < steps; ++i) {
            float a1 = baseAngle + (static_cast<float>(i) / steps) * 3.14159265f;
            float a2 = baseAngle + (static_cast<float>(i + 1) / steps) * 3.14159265f;
            va.append(sf::Vertex(center, coreCol));
            va.append(sf::Vertex(center + sf::Vector2f(std::cos(a1) * r, std::sin(a1) * r), edgeCol));
            va.append(sf::Vertex(center + sf::Vector2f(std::cos(a2) * r, std::sin(a2) * r), edgeCol));
        }
    }
}

void PencilStroke::begin(sf::Vector2f pos, float baseRadius, sf::Color color) {
    m_pts.clear();
    m_baseRadius = std::clamp(baseRadius * 0.45f, 0.6f, 3.0f);

    if (color.r < 35 && color.g < 35 && color.b < 35) {
        m_graphite = sf::Color(46, 48, 54);
        m_maxAlpha = 190.f;
    }
    else {
        m_graphite = sf::Color(
            static_cast<sf::Uint8>(color.r * 0.88f + 12.f),
            static_cast<sf::Uint8>(color.g * 0.88f + 12.f),
            static_cast<sf::Uint8>(color.b * 0.88f + 14.f));
        m_maxAlpha = 175.f;
    }

    m_clock.restart();
    m_lastT = 0.f;
    m_accDist = 0.f;
    m_speed = 0.f;
    m_endSpeed = 0.f;
    m_pressure = 0.8f;
    m_active = true;
    m_finished = false;
    m_pts.push_back({ pos, m_pressure });
}

void PencilStroke::addPoint(sf::Vector2f pos) {
    if (!m_active || m_pts.empty()) return;

    sf::Vector2f last = m_pts.back().pos;
    float d = len(pos - last);
    if (d < kMinSpacing) return;

    // Speed measured over small time windows so bursts of points don't fake high speed
    float now = m_clock.getElapsedTime().asSeconds();
    m_accDist += d;
    float elapsed = now - m_lastT;
    if (elapsed >= 0.008f) {
        float inst = m_accDist / elapsed;
        m_speed += (inst - m_speed) * 0.35f;
        m_accDist = 0.f;
        m_lastT = now;
    }

    // Fast = lighter touch, slow = heavier; people also press harder in turns
    float target = 1.1f - m_speed / 2400.f;
    if (m_pts.size() >= 2) {
        sf::Vector2f a = normalize(last - m_pts[m_pts.size() - 2].pos);
        sf::Vector2f b = normalize(pos - last);
        float turn = 1.f - (a.x * b.x + a.y * b.y);
        target += std::min(turn, 1.f) * 0.2f;
    }
    target = std::clamp(target, 0.5f, 1.2f);
    m_pressure += (target - m_pressure) * 0.18f;

    m_pts.push_back({ pos, m_pressure });
}

void PencilStroke::clear() {
    m_pts.clear();
    m_active = false;
    m_finished = false;
}

void PencilStroke::appendMesh(sf::VertexArray& out, const sf::Vector2f* preview,
    const std::function<sf::Vector2f(sf::Vector2f)>& xf) const {
    if (m_pts.empty()) return;

    std::vector<Sample> src = m_pts;
    if (preview && len(*preview - src.back().pos) >= 0.15f) {
        src.push_back({ *preview, src.back().pressure });
    }
    if (xf) {
        for (auto& s : src) s.pos = xf(s.pos);
    }

    const sf::Color clearEdge(m_graphite.r, m_graphite.g, m_graphite.b, 0);

    auto dot = [&](sf::Vector2f c, float p) {
        float r = m_baseRadius * (0.75f + 0.2f * p);
        float g = paperGrain(c, 3.1f);
        sf::Uint8 a = static_cast<sf::Uint8>(std::clamp(m_maxAlpha * 0.75f * (0.35f + 0.65f * g), 20.f, 220.f));
        sf::Color core(m_graphite.r, m_graphite.g, m_graphite.b, a);
        appendSoftCap(out, c, sf::Vector2f(1.f, 0.f), r, core, clearEdge, true);
        appendSoftCap(out, c, sf::Vector2f(-1.f, 0.f), r, core, clearEdge, true);
        };

    if (src.size() == 1) {
        dot(src[0].pos, src[0].pressure);
        return;
    }

    // Smooth + dense resample so the paper grain has enough vertices to show
    std::vector<sf::Vector2f> P;
    std::vector<float> PR;
    P.push_back(src[0].pos);
    PR.push_back(src[0].pressure);

    for (size_t i = 0; i + 1 < src.size(); ++i) {
        sf::Vector2f p0 = src[i > 0 ? i - 1 : i].pos;
        sf::Vector2f p1 = src[i].pos;
        sf::Vector2f p2 = src[i + 1].pos;
        sf::Vector2f p3 = src[(i + 2 < src.size()) ? i + 2 : i + 1].pos;

        int steps = std::clamp(static_cast<int>(len(p2 - p1) / 0.7f), 1, 24);
        for (int k = 1; k <= steps; ++k) {
            float t = static_cast<float>(k) / static_cast<float>(steps);
            P.push_back(catmull(p0, p1, p2, p3, t));
            PR.push_back(src[i].pressure + (src[i + 1].pressure - src[i].pressure) * t);
        }
    }

    size_t count = P.size();
    std::vector<float> S(count, 0.f);
    for (size_t i = 1; i < count; ++i) S[i] = S[i - 1] + len(P[i] - P[i - 1]);
    float total = S.back();

    if (count < 2 || total < 0.2f) {
        dot(P.back(), PR.back());
        return;
    }

    std::vector<sf::Vector2f> D(count);
    std::vector<sf::Vector2f> N(count);
    for (size_t i = 0; i < count; ++i) {
        sf::Vector2f dir(0.f, 0.f);
        if (i == 0) {
            dir = normalize(P[1] - P[0]);
        }
        else if (i + 1 == count) {
            dir = normalize(P[count - 1] - P[count - 2]);
        }
        else {
            sf::Vector2f d1 = normalize(P[i] - P[i - 1]);
            sf::Vector2f d2 = normalize(P[i + 1] - P[i]);
            dir = normalize(d1 + d2);
            if (len(dir) < 0.001f) dir = d2;
        }
        D[i] = dir;
        N[i] = sf::Vector2f(-dir.y, dir.x);
    }

    // Press-in at the start, lift-off taper at the end (longer when the hand leaves quickly)
    float startLen = std::min(1.5f + 3.f * m_baseRadius, total * 0.35f);
    float endLen = 0.f;
    if (m_finished) {
        endLen = std::min(std::clamp(2.f + m_endSpeed * 0.004f, 2.f, 10.f), total * 0.45f);
    }

    std::vector<sf::Vertex> grid(count * kLanes);
    std::vector<float> radii(count);

    for (size_t i = 0; i < count; ++i) {
        float p = PR[i];

        float taper = 0.4f + 0.6f * sstep(0.f, 1.f, S[i] / startLen);
        if (endLen > 0.f) taper *= 0.12f + 0.88f * sstep(0.f, 1.f, (total - S[i]) / endLen);

        // Tilted tip: strokes across the lean are wider than strokes along it
        float ang = std::atan2(D[i].y, D[i].x);
        float tilt = 0.78f + 0.34f * std::abs(std::sin(ang - kTilt));

        float r = m_baseRadius * (0.82f + 0.18f * p) * tilt * taper;
        radii[i] = r;
        float rL = std::max(0.3f, r * (0.8f + 0.4f * paperGrain(P[i] + N[i] * r, 11.3f)));
        float rR = std::max(0.3f, r * (0.8f + 0.4f * paperGrain(P[i] - N[i] * r, 27.9f)));

        // Pressure mostly changes darkness; more pressure fills the valleys of the paper
        float density = std::clamp((0.45f + 0.6f * p) * (0.3f + 0.7f * taper), 0.f, 1.1f);
        float threshold = 0.64f - 0.36f * std::clamp(p * taper, 0.f, 1.2f);

        for (int j = 0; j < kLanes; ++j) {
            float u = -1.f + 2.f * static_cast<float>(j) / static_cast<float>(kLanes - 1);
            float off = (u < 0.f) ? u * rR : u * rL;
            sf::Vector2f vp = P[i] + N[i] * off;

            float profile = std::pow(std::max(0.f, 1.f - u * u), 0.6f);
            float deposit = sstep(threshold - 0.22f, threshold + 0.22f, paperGrain(vp, 3.1f));
            float streak = 0.72f + 0.28f * valueNoise(static_cast<float>(j) * 1.9f + 7.3f, S[i] * 0.12f);

            float a = m_maxAlpha * density * profile * (0.18f + 0.82f * deposit) * streak;
            grid[i * kLanes + j] = sf::Vertex(vp,
                sf::Color(m_graphite.r, m_graphite.g, m_graphite.b,
                    static_cast<sf::Uint8>(std::clamp(a, 0.f, 235.f))));
        }
    }

    const int mid = kLanes / 2;
    appendSoftCap(out, P[0], D[0], std::max(0.3f, radii[0]), grid[mid].color, clearEdge, false);

    for (size_t i = 0; i + 1 < count; ++i) {
        for (int j = 0; j + 1 < kLanes; ++j) {
            const sf::Vertex& a = grid[i * kLanes + j];
            const sf::Vertex& b = grid[i * kLanes + j + 1];
            const sf::Vertex& c = grid[(i + 1) * kLanes + j];
            const sf::Vertex& d = grid[(i + 1) * kLanes + j + 1];
            out.append(a); out.append(b); out.append(c);
            out.append(b); out.append(d); out.append(c);
        }
    }

    appendSoftCap(out, P[count - 1], D[count - 1], std::max(0.3f, radii[count - 1]),
        grid[(count - 1) * kLanes + mid].color, clearEdge, true);
}