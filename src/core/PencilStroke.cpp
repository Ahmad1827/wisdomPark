#include "PencilStroke.h"
#include <algorithm>
#include <cmath>

namespace {
    const float kMinSpacing = 0.6f;

    float len(sf::Vector2f v) {
        return std::hypot(v.x, v.y);
    }

    sf::Vector2f normalize(sf::Vector2f v) {
        float l = len(v);
        if (l < 0.0001f) return sf::Vector2f(0.f, 0.f);
        return v / l;
    }

    float paperTooth(sf::Vector2f pt, float seed) {
        float g1 = std::sin(pt.x * 0.85f + pt.y * 1.42f + seed);
        float g2 = std::cos(pt.x * 1.73f - pt.y * 0.91f + seed * 1.3f);
        float g3 = std::sin((pt.x + pt.y) * 2.6f + seed * 0.7f);
        return 0.5f + 0.28f * g1 + 0.14f * g2 + 0.08f * g3;
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
    m_baseRadius = std::clamp(baseRadius * 0.45f, 0.55f, 2.4f);

    if (color.r < 35 && color.g < 35 && color.b < 35) {
        m_coreColor = sf::Color(48, 51, 56, 130);
        m_edgeColor = sf::Color(62, 65, 72, 35);
    }
    else {
        sf::Uint8 grR = static_cast<sf::Uint8>(color.r * 0.82f + 15);
        sf::Uint8 grG = static_cast<sf::Uint8>(color.g * 0.82f + 15);
        sf::Uint8 grB = static_cast<sf::Uint8>(color.b * 0.82f + 18);
        m_coreColor = sf::Color(grR, grG, grB, 130);
        m_edgeColor = sf::Color(grR, grG, grB, 35);
    }

    m_clock.restart();
    m_lastT = 0.f;
    m_pressure = 1.f;
    m_active = true;
    m_finished = false;
    m_pts.push_back({ pos, 1.f });
}

void PencilStroke::addPoint(sf::Vector2f pos) {
    if (!m_active || m_pts.empty()) return;

    float dist = len(pos - m_pts.back().pos);
    if (dist < kMinSpacing) return;

    float now = m_clock.getElapsedTime().asSeconds();
    float dt = std::max(now - m_lastT, 0.001f);
    m_lastT = now;

    float speed = dist / dt;
    float target = std::clamp(1.05f - (speed / 3800.f) * 0.22f, 0.82f, 1.05f);
    m_pressure += (target - m_pressure) * 0.22f;

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

    if (src.size() == 1) {
        float r = m_baseRadius * 0.85f;
        appendSoftCap(out, src[0].pos, sf::Vector2f(1.f, 0.f), r, m_coreColor, m_edgeColor, true);
        appendSoftCap(out, src[0].pos, sf::Vector2f(-1.f, 0.f), r, m_coreColor, m_edgeColor, true);
        return;
    }

    std::vector<sf::Vector2f> P;
    std::vector<float> PR;
    P.push_back(src[0].pos);
    PR.push_back(src[0].pressure);

    for (size_t i = 0; i + 1 < src.size(); ++i) {
        sf::Vector2f p0 = src[i > 0 ? i - 1 : i].pos;
        sf::Vector2f p1 = src[i].pos;
        sf::Vector2f p2 = src[i + 1].pos;
        sf::Vector2f p3 = src[(i + 2 < src.size()) ? i + 2 : i + 1].pos;

        int steps = std::clamp(static_cast<int>(len(p2 - p1) / 1.2f), 1, 14);
        for (int k = 1; k <= steps; ++k) {
            float t = static_cast<float>(k) / static_cast<float>(steps);
            P.push_back(catmull(p0, p1, p2, p3, t));
            PR.push_back(src[i].pressure + (src[i + 1].pressure - src[i].pressure) * t);
        }
    }

    size_t count = P.size();
    if (count < 2) return;

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
        N[i] = sf::Vector2f(-dir.y, dir.x);
    }

    std::vector<sf::Vector2f> leftPts(count);
    std::vector<sf::Vector2f> rightPts(count);
    std::vector<sf::Color> centerCols(count);
    std::vector<sf::Color> leftCols(count);
    std::vector<sf::Color> rightCols(count);

    for (size_t i = 0; i < count; ++i) {
        float toothL = paperTooth(P[i], 1.15f);
        float toothR = paperTooth(P[i], 3.73f);
        float toothC = paperTooth(P[i], 5.41f);

        float r = m_baseRadius * PR[i];
        float rL = std::max(0.35f, r * (0.8f + 0.35f * toothL));
        float rR = std::max(0.35f, r * (0.8f + 0.35f * toothR));

        leftPts[i] = P[i] + N[i] * rL;
        rightPts[i] = P[i] - N[i] * rR;

        sf::Uint8 cAlpha = static_cast<sf::Uint8>(std::clamp(m_coreColor.a * (0.85f + 0.3f * toothC), 40.f, 210.f));
        sf::Uint8 lAlpha = static_cast<sf::Uint8>(std::clamp(m_edgeColor.a * (0.55f + 0.7f * toothL), 10.f, 110.f));
        sf::Uint8 rAlpha = static_cast<sf::Uint8>(std::clamp(m_edgeColor.a * (0.55f + 0.7f * toothR), 10.f, 110.f));

        centerCols[i] = sf::Color(m_coreColor.r, m_coreColor.g, m_coreColor.b, cAlpha);
        leftCols[i] = sf::Color(m_edgeColor.r, m_edgeColor.g, m_edgeColor.b, lAlpha);
        rightCols[i] = sf::Color(m_edgeColor.r, m_edgeColor.g, m_edgeColor.b, rAlpha);
    }

    sf::Vector2f startDir = normalize(P[1] - P[0]);
    appendSoftCap(out, P[0], startDir, m_baseRadius * 0.85f, centerCols[0], leftCols[0], false);

    for (size_t i = 0; i + 1 < count; ++i) {
        out.append(sf::Vertex(leftPts[i], leftCols[i]));
        out.append(sf::Vertex(P[i], centerCols[i]));
        out.append(sf::Vertex(leftPts[i + 1], leftCols[i + 1]));

        out.append(sf::Vertex(P[i], centerCols[i]));
        out.append(sf::Vertex(P[i + 1], centerCols[i + 1]));
        out.append(sf::Vertex(leftPts[i + 1], leftCols[i + 1]));

        out.append(sf::Vertex(P[i], centerCols[i]));
        out.append(sf::Vertex(rightPts[i], rightCols[i]));
        out.append(sf::Vertex(P[i + 1], centerCols[i + 1]));

        out.append(sf::Vertex(rightPts[i], rightCols[i]));
        out.append(sf::Vertex(rightPts[i + 1], rightCols[i + 1]));
        out.append(sf::Vertex(P[i + 1], centerCols[i + 1]));
    }

    sf::Vector2f endDir = normalize(P[count - 1] - P[count - 2]);
    appendSoftCap(out, P[count - 1], endDir, m_baseRadius * 0.85f, centerCols.back(), rightCols.back(), true);
}