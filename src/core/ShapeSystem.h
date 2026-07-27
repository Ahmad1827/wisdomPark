#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <cmath>

enum class ShapeId {
    Line, Rectangle, FilledRectangle, RoundedRectangle, FilledRoundedRectangle,
    Circle, FilledCircle, Ellipse, FilledEllipse, Triangle, FilledTriangle,
    Polygon, Polyline, BezierCurve, Arrow, FilledArrow, Star, FilledStar,
    Diamond, FilledDiamond, Hexagon, FilledHexagon
};

enum class StrokeStyle { Solid, Dashed, Dotted };
enum class FillType { Solid, Gradient, Pattern, Dither, Transparent };

class BaseShape {
public:
    virtual ~BaseShape() = default;
    virtual void draw(sf::RenderTarget& target, bool isPixelMode, sf::RenderStates states = sf::RenderStates::Default) = 0;
    virtual void setBounds(const sf::Vector2f& start, const sf::Vector2f& end, bool lockProportions, bool fromCenter) = 0;
    virtual void rasterize(sf::RenderTexture& target, bool isPixelMode) = 0;
    virtual void applySymmetry(const sf::Vector2f& symStart, const sf::Vector2f& symEnd) = 0;

    sf::Color strokeColor = sf::Color::Black;
    sf::Color fillColor = sf::Color::Transparent;
    float strokeWidth = 1.0f;
    float cornerRadius = 0.0f;
    int numSides = 5;
    int starPoints = 5;
    float innerRadius = 0.5f;
    float arrowHeadSize = 20.0f;
    float arrowHeadAngle = 30.0f;
    bool isFilled = false;
    StrokeStyle strokeStyle = StrokeStyle::Solid;
    FillType fillType = FillType::Solid;
    bool useAntiAliasing = true;
    bool pixelPerfect = false;

    sf::Vector2f originalStart;
    sf::Vector2f originalEnd;
    sf::Vector2f center;
    float rotationAngle = 0.0f;
    sf::FloatRect bounds;
    ShapeId shapeId;
};

class PathShape : public BaseShape {
public:
    std::vector<sf::Vector2f> points;
    PathShape(ShapeId t);
    void draw(sf::RenderTarget& target, bool isPixelMode, sf::RenderStates states = sf::RenderStates::Default) override;
    void setBounds(const sf::Vector2f& start, const sf::Vector2f& end, bool lockProportions, bool fromCenter) override;
    void rasterize(sf::RenderTexture& target, bool isPixelMode) override;
    void applySymmetry(const sf::Vector2f& symStart, const sf::Vector2f& symEnd) override;

private:
    void generatePoints();
    void drawBresenham(sf::RenderTarget& target, sf::RenderStates states);
    void drawSmooth(sf::RenderTarget& target, sf::RenderStates states);
    sf::Vector2f reflectPoint(const sf::Vector2f& p, const sf::Vector2f& sA, const sf::Vector2f& sB) const;
};

class ShapeFactory {
public:
    static std::unique_ptr<BaseShape> createShape(ShapeId id);
};

class ShapeManager {
public:
    std::unique_ptr<BaseShape> activeShape;

    void beginShape(ShapeId id, const sf::Vector2f& pos);
    void updateShape(const sf::Vector2f& pos, bool lockProportions, bool fromCenter);
    void rasterizeActive(sf::RenderTexture& target, bool isPixelMode);
    void clearActive();
    void drawActive(sf::RenderTarget& target, bool isPixelMode, sf::RenderStates states = sf::RenderStates::Default);
};