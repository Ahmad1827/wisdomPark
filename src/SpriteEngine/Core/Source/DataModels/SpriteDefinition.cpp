#include "DataModels/SpriteDefinition.h"

namespace StudioCore {

SpriteDefinition::SpriteDefinition(const std::string& id, const Rect& sourceRect)
    : m_id(id), m_sourceRect(sourceRect) {
    m_pivot = {sourceRect.width / 2.0f, sourceRect.height / 2.0f};
    m_center = {sourceRect.x + sourceRect.width / 2.0f, sourceRect.y + sourceRect.height / 2.0f};
}

const std::string& SpriteDefinition::GetId() const {
    return m_id;
}

const Rect& SpriteDefinition::GetSourceRect() const {
    return m_sourceRect;
}

void SpriteDefinition::SetSourceRect(const Rect& rect) {
    m_sourceRect = rect;
    m_center = {m_sourceRect.x + m_sourceRect.width / 2.0f, m_sourceRect.y + m_sourceRect.height / 2.0f};
}

const Point& SpriteDefinition::GetPivot() const {
    return m_pivot;
}

void SpriteDefinition::SetPivot(const Point& pivot) {
    m_pivot = pivot;
}

float SpriteDefinition::GetBaseline() const {
    return m_baseline;
}

void SpriteDefinition::SetBaseline(float baseline) {
    m_baseline = baseline;
}

int SpriteDefinition::GetPixelCount() const {
    return m_pixelCount;
}

void SpriteDefinition::SetPixelCount(int count) {
    m_pixelCount = count;
}

const Point& SpriteDefinition::GetCenter() const {
    return m_center;
}

void SpriteDefinition::SetCenter(const Point& center) {
    m_center = center;
}



}