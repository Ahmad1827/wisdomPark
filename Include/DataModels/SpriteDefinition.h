#pragma once
#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include <optional>

namespace StudioCore {

struct Rect {
    float x{0.0f}, y{0.0f}, width{0.0f}, height{0.0f};
};

struct Point {
    float x{0.0f}, y{0.0f};
};



class SpriteDefinition {
public:
    SpriteDefinition(const std::string& id, const Rect& sourceRect);

    const std::string& GetId() const;

    const Rect& GetSourceRect() const;
    void SetSourceRect(const Rect& rect);

    const Point& GetPivot() const;
    void SetPivot(const Point& pivot);

    float GetBaseline() const;
    void SetBaseline(float baseline);

    int GetPixelCount() const;
    void SetPixelCount(int count);

    const Point& GetCenter() const;
    void SetCenter(const Point& center);

    bool HasCustomPixels() const;
    void ClearCustomPixels();

private:
    std::string m_id;
    Rect m_sourceRect;
    Point m_pivot{0.0f, 0.0f};
    float m_baseline{0.0f};
    int m_pixelCount{0};
    Point m_center{0.0f, 0.0f};
};

}