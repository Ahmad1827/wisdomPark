#include "ShapeSystem.h"

PathShape::PathShape(ShapeId t) {
    shapeId = t;
    if (t == ShapeId::FilledRectangle || t == ShapeId::FilledRoundedRectangle || t == ShapeId::FilledCircle ||
        t == ShapeId::FilledEllipse || t == ShapeId::FilledTriangle || t == ShapeId::FilledArrow ||
        t == ShapeId::FilledStar || t == ShapeId::FilledDiamond || t == ShapeId::FilledHexagon) {
        isFilled = true;
    }
}

void PathShape::generatePoints() {
    points.clear();
    float w = bounds.width;
    float h = bounds.height;
    float cx = bounds.left + w / 2.f;
    float cy = bounds.top + h / 2.f;

    if (shapeId == ShapeId::Line || shapeId == ShapeId::Polyline) {
        points.push_back(originalStart);
        points.push_back(originalEnd);
    }
    else if (shapeId == ShapeId::Arrow || shapeId == ShapeId::FilledArrow) {
        sf::Vector2f dir = originalEnd - originalStart;
        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (len > 0) {
            dir /= len;
            sf::Vector2f perp(-dir.y, dir.x);
            float headLen = std::min(arrowHeadSize, len * 0.4f);
            float shaftWidth = std::min(arrowHeadSize * 0.5f, len * 0.15f);

            points.push_back(originalStart - perp * shaftWidth);
            points.push_back(originalStart + perp * shaftWidth);
            points.push_back(originalEnd - dir * headLen + perp * shaftWidth);
            points.push_back(originalEnd - dir * headLen + perp * (shaftWidth * 2.f));
            points.push_back(originalEnd);
            points.push_back(originalEnd - dir * headLen - perp * (shaftWidth * 2.f));
            points.push_back(originalEnd - dir * headLen - perp * shaftWidth);
        }
    }
    else if (shapeId == ShapeId::Rectangle || shapeId == ShapeId::FilledRectangle || shapeId == ShapeId::RoundedRectangle || shapeId == ShapeId::FilledRoundedRectangle) {
        points.push_back({ bounds.left, bounds.top });
        points.push_back({ bounds.left + w, bounds.top });
        points.push_back({ bounds.left + w, bounds.top + h });
        points.push_back({ bounds.left, bounds.top + h });
    }
    else if (shapeId == ShapeId::Ellipse || shapeId == ShapeId::FilledEllipse || shapeId == ShapeId::Circle || shapeId == ShapeId::FilledCircle) {
        int segs = 60;
        for (int i = 0; i < segs; ++i) {
            float a = i * 3.14159265f * 2.0f / segs;
            points.push_back({ cx + (w / 2.f) * std::cos(a), cy + (h / 2.f) * std::sin(a) });
        }
    }
    else if (shapeId == ShapeId::Triangle || shapeId == ShapeId::FilledTriangle) {
        points.push_back({ cx, bounds.top });
        points.push_back({ bounds.left + w, bounds.top + h });
        points.push_back({ bounds.left, bounds.top + h });
    }
    else if (shapeId == ShapeId::Polygon || shapeId == ShapeId::Hexagon || shapeId == ShapeId::FilledHexagon) {
        int s = (shapeId == ShapeId::Hexagon || shapeId == ShapeId::FilledHexagon) ? 6 : numSides;
        for (int i = 0; i < s; ++i) {
            float a = i * 3.14159265f * 2.0f / s - 3.14159265f / 2.0f;
            points.push_back({ cx + (w / 2.f) * std::cos(a), cy + (h / 2.f) * std::sin(a) });
        }
    }
    else if (shapeId == ShapeId::Star || shapeId == ShapeId::FilledStar || shapeId == ShapeId::Diamond || shapeId == ShapeId::FilledDiamond) {
        int s = (shapeId == ShapeId::Diamond || shapeId == ShapeId::FilledDiamond) ? 4 : starPoints;
        float ir = (shapeId == ShapeId::Diamond || shapeId == ShapeId::FilledDiamond) ? 0.5f : innerRadius;
        for (int i = 0; i < s * 2; ++i) {
            float r = (i % 2 == 0) ? 1.0f : ir;
            float a = i * 3.14159265f / s - 3.14159265f / 2.0f;
            points.push_back({ cx + r * (w / 2.f) * std::cos(a), cy + r * (h / 2.f) * std::sin(a) });
        }
    }
    else if (shapeId == ShapeId::BezierCurve) {
        points.push_back({ bounds.left, bounds.top });
        points.push_back({ cx, bounds.top });
        points.push_back({ bounds.left + w, bounds.top + h });
    }

    if (rotationAngle != 0.0f) {
        float rad = rotationAngle * 3.14159265f / 180.f;
        float cosA = std::cos(rad);
        float sinA = std::sin(rad);
        for (auto& p : points) {
            float dx = p.x - cx;
            float dy = p.y - cy;
            p.x = cx + dx * cosA - dy * sinA;
            p.y = cy + dx * sinA + dy * cosA;
        }
    }
}

void PathShape::draw(sf::RenderTarget& target, bool isPixelMode, sf::RenderStates states) {
    generatePoints();
    if (isPixelMode) {
        drawBresenham(target, states);
    }
    else {
        drawSmooth(target, states);
    }
}

void PathShape::setBounds(const sf::Vector2f& start, const sf::Vector2f& end, bool lockProportions, bool fromCenter) {
    originalStart = start;
    originalEnd = end;

    if (lockProportions) {
        float dx = end.x - start.x;
        float dy = end.y - start.y;
        float size = std::max(std::abs(dx), std::abs(dy));
        originalEnd.x = start.x + (dx >= 0 ? size : -size);
        originalEnd.y = start.y + (dy >= 0 ? size : -size);
    }

    if (fromCenter) {
        sf::Vector2f radius = originalEnd - start;
        originalStart = start - radius;
        originalEnd = start + radius;
    }

    float x = std::min(originalStart.x, originalEnd.x);
    float y = std::min(originalStart.y, originalEnd.y);
    float w = std::abs(originalEnd.x - originalStart.x);
    float h = std::abs(originalEnd.y - originalStart.y);

    bounds = sf::FloatRect(x, y, w, h);
    center = sf::Vector2f(x + w / 2.f, y + h / 2.f);
}

void PathShape::rasterize(sf::RenderTexture& target, bool isPixelMode) {
    draw(target, isPixelMode, sf::RenderStates::Default);
}

sf::Vector2f PathShape::reflectPoint(const sf::Vector2f& p, const sf::Vector2f& sA, const sf::Vector2f& sB) const {
    sf::Vector2f dir = sB - sA;
    float lenSq = dir.x * dir.x + dir.y * dir.y;
    if (lenSq < 0.0001f) return p;
    sf::Vector2f v = p - sA;
    float t = (v.x * dir.x + v.y * dir.y) / lenSq;
    sf::Vector2f proj = sA + dir * t;
    return p + 2.0f * (proj - p);
}

void PathShape::applySymmetry(const sf::Vector2f& symStart, const sf::Vector2f& symEnd) {
    std::vector<sf::Vector2f> mirrored;
    for (const auto& p : points) {
        mirrored.push_back(reflectPoint(p, symStart, symEnd));
    }
    points.insert(points.end(), mirrored.begin(), mirrored.end());
}

void PathShape::drawBresenham(sf::RenderTarget& target, sf::RenderStates states) {
    if (points.empty()) return;

    sf::VertexArray lines(shapeId == ShapeId::Line || shapeId == ShapeId::Arrow || shapeId == ShapeId::FilledArrow ? sf::Lines : sf::LineStrip, points.size() + (isFilled ? 1 : 0));
    for (size_t i = 0; i < points.size(); ++i) {
        lines[i].position = sf::Vector2f(std::round(points[i].x), std::round(points[i].y));
        lines[i].color = strokeColor;
    }

    if (isFilled && points.size() > 2) {
        lines[points.size()].position = sf::Vector2f(std::round(points[0].x), std::round(points[0].y));
        lines[points.size()].color = strokeColor;

        sf::ConvexShape convex;
        convex.setPointCount(points.size());
        for (size_t i = 0; i < points.size(); ++i) {
            convex.setPoint(i, sf::Vector2f(std::round(points[i].x), std::round(points[i].y)));
        }
        convex.setFillColor(fillColor);
        target.draw(convex, states);
    }
    target.draw(lines, states);
}

void PathShape::drawSmooth(sf::RenderTarget& target, sf::RenderStates states) {
    if (points.empty()) return;

    if (isFilled) {
        sf::ConvexShape convex;
        convex.setPointCount(points.size());
        for (size_t i = 0; i < points.size(); ++i) {
            convex.setPoint(i, points[i]);
        }
        convex.setFillColor(fillColor);
        convex.setOutlineColor(strokeColor);
        convex.setOutlineThickness(strokeWidth);
        target.draw(convex, states);
    }
    else {
        sf::VertexArray lines(shapeId == ShapeId::Line || shapeId == ShapeId::Arrow ? sf::Lines : sf::LineStrip, points.size() + 1);
        for (size_t i = 0; i < points.size(); ++i) {
            lines[i].position = points[i];
            lines[i].color = strokeColor;
        }
        lines[points.size()].position = points[0];
        lines[points.size()].color = strokeColor;
        target.draw(lines, states);
    }
}

std::unique_ptr<BaseShape> ShapeFactory::createShape(ShapeId id) {
    return std::make_unique<PathShape>(id);
}

void ShapeManager::beginShape(ShapeId id, const sf::Vector2f& pos) {
    activeShape = ShapeFactory::createShape(id);
    activeShape->setBounds(pos, pos, false, false);
}

void ShapeManager::updateShape(const sf::Vector2f& pos, bool lockProportions, bool fromCenter) {
    if (activeShape) {
        activeShape->setBounds(activeShape->originalStart, pos, lockProportions, fromCenter);
    }
}

void ShapeManager::rasterizeActive(sf::RenderTexture& target, bool isPixelMode) {
    if (activeShape) {
        activeShape->rasterize(target, isPixelMode);
    }
}

void ShapeManager::clearActive() {
    activeShape = nullptr;
}

void ShapeManager::drawActive(sf::RenderTarget& target, bool isPixelMode, sf::RenderStates states) {
    if (activeShape) {
        activeShape->draw(target, isPixelMode, states);
    }
}