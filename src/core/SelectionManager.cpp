#include "SelectionManager.h"
#include <cmath>
#include <algorithm>

SelectionManager::SelectionManager() : state(SelectionState::Inactive), isLassoSelection(true), dashOffset(0.f), hasClipboard(false), m_isDragging(false),
showHandles(false), handleVisualSize(6.f), isResizingFlag(false), activeHandle(-1),
resizeAnchorWorld(0.f, 0.f), resizeAnchorLocal(0.f, 0.f), resizeDraggedLocal(0.f, 0.f), resizeStartBox(0.f, 0.f, 0.f, 0.f) {
    sf::Image dashImg;
    dashImg.create(12, 2, sf::Color::Transparent);
    for (int i = 0; i < 6; i++) {
        dashImg.setPixel(i, 0, sf::Color(15, 10, 25));
        dashImg.setPixel(i, 1, sf::Color(15, 10, 25));
    }
    for (int i = 6; i < 12; i++) {
        dashImg.setPixel(i, 0, sf::Color(255, 215, 60));
        dashImg.setPixel(i, 1, sf::Color(255, 215, 60));
    }
    dashTexture.loadFromImage(dashImg);
    dashTexture.setRepeated(true);
}

void SelectionManager::update(float dt) {
    if (state != SelectionState::Inactive) {
        dashOffset -= 30.f * dt;
    }
}

void SelectionManager::drawPixels(sf::RenderWindow& window, const sf::RenderStates& baseStates) {
    if (state == SelectionState::Floating && floatingTexture.getSize().x > 0 && floatingTexture.getSize().y > 0) {
        window.draw(floatingSprite, baseStates);
    }
}

void SelectionManager::draw(sf::RenderWindow& window, const sf::RenderStates& baseStates) {
    if (state == SelectionState::Inactive) return;

    sf::RenderStates states = baseStates;
    float borderThickness = std::max(0.02f, handleVisualSize / 8.0f);

    if (isLassoSelection) {
        if (pathPoints.size() > 1) {
            sf::VertexArray darkUnder(sf::LineStrip, pathPoints.size());
            for (size_t i = 0; i < pathPoints.size(); ++i) {
                darkUnder[i].position = pathPoints[i];
                darkUnder[i].color = sf::Color(15, 10, 25, 220);
            }
            window.draw(darkUnder, states);

            sf::VertexArray ants(sf::LineStrip, pathPoints.size());
            float dist = 0.f;
            for (size_t i = 0; i < pathPoints.size(); ++i) {
                if (i > 0) {
                    sf::Vector2f diff = pathPoints[i] - pathPoints[i - 1];
                    dist += std::hypot(diff.x, diff.y);
                }
                ants[i].position = pathPoints[i];
                ants[i].texCoords = sf::Vector2f(dist + dashOffset, 0.5f);
                ants[i].color = sf::Color::White;
            }
            sf::RenderStates dashStates = states;
            dashStates.texture = &dashTexture;
            window.draw(ants, dashStates);
        }
    }
    else {
        float borderThickness = std::max(0.02f, handleVisualSize / 8.0f);
        for (const auto& box : subItemBoxes) {
            sf::RectangleShape r(sf::Vector2f(box.width, box.height));
            r.setPosition(box.left, box.top);
            r.setFillColor(sf::Color(0, 191, 255, 14));
            r.setOutlineThickness(borderThickness);
            r.setOutlineColor(sf::Color(0, 191, 255, 100));
            window.draw(r, states);
        }
    }

    if (showHandles && (state == SelectionState::Selected || state == SelectionState::Floating)) {
        sf::RectangleShape frameBox(sf::Vector2f(boundingBox.width, boundingBox.height));
        frameBox.setPosition(boundingBox.left, boundingBox.top);
        frameBox.setFillColor(sf::Color::Transparent);
        frameBox.setOutlineThickness(1.0f);
        frameBox.setOutlineColor(sf::Color(0, 191, 255, 180));
        window.draw(frameBox, baseStates);

        auto corners = getHandlePositions();
        for (const auto& c : corners) {
            sf::RectangleShape h(sf::Vector2f(handleVisualSize, handleVisualSize));
            h.setOrigin(handleVisualSize / 2.f, handleVisualSize / 2.f);
            h.setPosition(c);
            h.setFillColor(sf::Color(0, 191, 255));
            h.setOutlineThickness(std::max(0.05f, handleVisualSize * 0.15f));
            h.setOutlineColor(sf::Color(15, 10, 25));
            window.draw(h, baseStates);
        }
    }
}

void SelectionManager::setSelectionBoxes(const sf::FloatRect& masterBox, const std::vector<sf::FloatRect>& itemBoxes) {
    isLassoSelection = false;
    boundingBox = masterBox;
    subItemBoxes = itemBoxes;
    pathPoints.clear();
    state = SelectionState::Selected;
    showHandles = false;
}

void SelectionManager::moveSelection(sf::Vector2f delta) {
    boundingBox.left += delta.x;
    boundingBox.top += delta.y;
    for (auto& box : subItemBoxes) {
        box.left += delta.x;
        box.top += delta.y;
    }
    for (auto& p : pathPoints) {
        p += delta;
    }
}

void SelectionManager::flipPathHorizontal(float midX) {
    for (auto& p : pathPoints) {
        p.x = 2.0f * midX - p.x;
    }
}

void SelectionManager::flipPathVertical(float midY) {
    for (auto& p : pathPoints) {
        p.y = 2.0f * midY - p.y;
    }
}


void SelectionManager::startLasso(sf::Vector2f pos, sf::Vector2u canvasSize) {
    pos.x = std::clamp(pos.x, 0.f, static_cast<float>(canvasSize.x));
    pos.y = std::clamp(pos.y, 0.f, static_cast<float>(canvasSize.y));
    isLassoSelection = true;
    state = SelectionState::Drawing;
    subItemBoxes.clear();
    pathPoints.clear();
    pathPoints.push_back(pos);
    showHandles = false;
}

void SelectionManager::addLassoPoint(sf::Vector2f pos, sf::Vector2u canvasSize) {
    if (state == SelectionState::Drawing) {
        pos.x = std::clamp(pos.x, 0.f, static_cast<float>(canvasSize.x));
        pos.y = std::clamp(pos.y, 0.f, static_cast<float>(canvasSize.y));
        if (pathPoints.empty() || pathPoints.back() != pos) {
            pathPoints.push_back(pos);
        }
    }
}

void SelectionManager::endLasso() {
    if (state == SelectionState::Drawing) {
        if (pathPoints.size() > 2) {
            if (pathPoints.front() != pathPoints.back()) {
                pathPoints.push_back(pathPoints.front());
            }
            calculateBoundingBox();
            subItemBoxes.clear();
            state = SelectionState::Selected;
            showHandles = false;
        }
        else {
            state = SelectionState::Inactive;
            showHandles = false;
        }
    }
}

void SelectionManager::calculateBoundingBox() {
    if (pathPoints.empty()) return;
    float minX = pathPoints[0].x, maxX = pathPoints[0].x;
    float minY = pathPoints[0].y, maxY = pathPoints[0].y;
    for (const auto& p : pathPoints) {
        minX = std::min(minX, p.x);
        maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y);
        maxY = std::max(maxY, p.y);
    }
    boundingBox = sf::FloatRect(minX, minY, maxX - minX, maxY - minY);
}

void SelectionManager::clampToCanvas(sf::Vector2u canvasSize, bool skip) {
    if (skip || state != SelectionState::Floating) return;

    sf::Vector2f pos = floatingSprite.getPosition();
    sf::Vector2f origin = floatingSprite.getOrigin();
    sf::Vector2f scale = floatingSprite.getScale();

    float width = boundingBox.width * std::abs(scale.x);
    float height = boundingBox.height * std::abs(scale.y);

    float left = pos.x - (origin.x * std::abs(scale.x));
    float top = pos.y - (origin.y * std::abs(scale.y));

    if (left < 0.f) pos.x -= left;
    if (top < 0.f) pos.y -= top;

    float right = left + width;
    float bottom = top + height;

    if (right > static_cast<float>(canvasSize.x)) pos.x -= (right - static_cast<float>(canvasSize.x));
    if (bottom > static_cast<float>(canvasSize.y)) pos.y -= (bottom - static_cast<float>(canvasSize.y));

    floatingSprite.setPosition(pos);
}

bool SelectionManager::isInsidePolygon(sf::Vector2f point, const std::vector<sf::Vector2f>& polygon) const {
    bool inside = false;
    for (size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
        if (((polygon[i].y > point.y) != (polygon[j].y > point.y)) &&
            (point.x < (polygon[j].x - polygon[i].x) * (point.y - polygon[i].y) / (polygon[j].y - polygon[i].y) + polygon[i].x)) {
            inside = !inside;
        }
    }
    return inside;
}

bool SelectionManager::isPointInsideSelection(sf::Vector2f pos) const {
    if (state == SelectionState::Selected || state == SelectionState::Floating) {
        if (!boundingBox.contains(pos)) return false;
        if (isLassoSelection && pathPoints.size() > 2) {
            return isInsidePolygon(pos, pathPoints);
        }
        for (const auto& b : subItemBoxes) {
            if (b.contains(pos)) return true;
        }
        return true;
    }
    return false;
}

void SelectionManager::extractFromLayer(sf::RenderTexture* layerTexture, bool removeOriginal) {
    if (state != SelectionState::Selected || !layerTexture) return;

    int w = static_cast<int>(boundingBox.width);
    int h = static_cast<int>(boundingBox.height);
    if (w <= 0 || h <= 0) return;

    sf::Image sourceImg = layerTexture->getTexture().copyToImage();
    sf::Image extractImg;
    extractImg.create(static_cast<unsigned int>(w), static_cast<unsigned int>(h), sf::Color::Transparent);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            sf::Vector2f globalPt(boundingBox.left + static_cast<float>(x) + 0.5f, boundingBox.top + static_cast<float>(y) + 0.5f);
            bool inside = isLassoSelection ? isInsidePolygon(globalPt, pathPoints) : true;
            if (inside) {
                unsigned int sx = static_cast<unsigned int>(boundingBox.left + x);
                unsigned int sy = static_cast<unsigned int>(boundingBox.top + y);
                if (sx < sourceImg.getSize().x && sy < sourceImg.getSize().y) {
                    extractImg.setPixel(static_cast<unsigned int>(x), static_cast<unsigned int>(y), sourceImg.getPixel(sx, sy));
                    if (removeOriginal) sourceImg.setPixel(sx, sy, sf::Color::Transparent);
                }
            }
        }
    }

    if (removeOriginal) {
        layerTexture->clear(sf::Color::Transparent);
        sf::Texture tempTex; tempTex.loadFromImage(sourceImg);
        layerTexture->draw(sf::Sprite(tempTex), sf::RenderStates(sf::BlendNone));
        layerTexture->display();
    }

    floatingTexture.loadFromImage(extractImg);
    floatingTexture.setSmooth(layerTexture->isSmooth());
    floatingSprite.setTexture(floatingTexture, true);
    floatingSprite.setOrigin(0.f, 0.f);
    floatingSprite.setPosition(boundingBox.left, boundingBox.top);
    floatingSprite.setScale(1.f, 1.f);

    localPoints.clear();
    for (const auto& p : pathPoints) {
        localPoints.push_back(p - sf::Vector2f(boundingBox.left, boundingBox.top));
    }

    state = SelectionState::Floating;
}

void SelectionManager::commitToLayer(sf::RenderTexture* layerTexture) {
    if (state == SelectionState::Floating && layerTexture && floatingTexture.getSize().x > 0 && floatingTexture.getSize().y > 0) {
        layerTexture->draw(floatingSprite);
        layerTexture->display();
    }
    state = SelectionState::Inactive;
    m_isDragging = false;
    showHandles = false;
    isResizingFlag = false;
    activeHandle = -1;
    subItemBoxes.clear();
    pathPoints.clear();
    localPoints.clear();
}

void SelectionManager::discardFloating() {
    state = SelectionState::Inactive;
    m_isDragging = false;
    showHandles = false;
    isResizingFlag = false;
    activeHandle = -1;
    subItemBoxes.clear();
    pathPoints.clear();
    localPoints.clear();
}

void SelectionManager::clearSelection() {
    state = SelectionState::Inactive;
    m_isDragging = false;
    showHandles = false;
    isResizingFlag = false;
    activeHandle = -1;
    subItemBoxes.clear();
    pathPoints.clear();
    localPoints.clear();
    boundingBox = sf::FloatRect(0.f, 0.f, 0.f, 0.f);
}

void SelectionManager::startDrag(sf::Vector2f pos) {
    if (state == SelectionState::Floating || state == SelectionState::Selected) {
        dragStartPos = pos;
        m_isDragging = true;
    }
}

void SelectionManager::drag(sf::Vector2f pos, sf::Vector2u canvasSize, bool allowOutsideCanvas) {
    if ((state == SelectionState::Floating || state == SelectionState::Selected) && m_isDragging) {
        sf::Vector2f delta = pos - dragStartPos;
        moveSelection(delta);
        if (state == SelectionState::Floating) {
            floatingSprite.move(delta);
        }
        dragStartPos = pos;
    }
}

void SelectionManager::endDrag() {
    m_isDragging = false;
}

void SelectionManager::copy(sf::RenderTexture* layerTexture) {
    if (state == SelectionState::Floating) {
        clipboardTexture = floatingTexture;
        clipboardPos = floatingSprite.getPosition() - floatingSprite.getOrigin();
        hasClipboard = true;
        pasteCount = 0;
    }
    else if (state == SelectionState::Selected && layerTexture) {
        int w = static_cast<int>(boundingBox.width);
        int h = static_cast<int>(boundingBox.height);
        if (w <= 0 || h <= 0) return;

        sf::Image sourceImg = layerTexture->getTexture().copyToImage();
        sf::Image tempImg;
        tempImg.create(static_cast<unsigned int>(w), static_cast<unsigned int>(h), sf::Color::Transparent);

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                sf::Vector2f globalPt(boundingBox.left + static_cast<float>(x) + 0.5f, boundingBox.top + static_cast<float>(y) + 0.5f);
                if (!isLassoSelection || isInsidePolygon(globalPt, pathPoints)) {
                    unsigned int sx = static_cast<unsigned int>(boundingBox.left + x);
                    unsigned int sy = static_cast<unsigned int>(boundingBox.top + y);
                    if (sx < sourceImg.getSize().x && sy < sourceImg.getSize().y) {
                        tempImg.setPixel(x, y, sourceImg.getPixel(sx, sy));
                    }
                }
            }
        }

        clipboardTexture.loadFromImage(tempImg);
        clipboardPos = sf::Vector2f(boundingBox.left, boundingBox.top);
        hasClipboard = true;
        pasteCount = 0;
    }
}

void SelectionManager::paste(sf::Vector2u canvasSize) {
    if (!hasClipboard) return;

    floatingTexture = clipboardTexture;
    floatingSprite.setTexture(floatingTexture, true);
    int w = static_cast<int>(floatingTexture.getSize().x);
    int h = static_cast<int>(floatingTexture.getSize().y);

    pasteCount++;
    sf::Vector2f offset(pasteCount * 20.f, pasteCount * 20.f);

    floatingSprite.setOrigin(0.f, 0.f);
    floatingSprite.setPosition(clipboardPos + offset);
    floatingSprite.setScale(1.f, 1.f);

    clampToCanvas(canvasSize, false);

    isLassoSelection = false;
    boundingBox = sf::FloatRect(floatingSprite.getPosition().x, floatingSprite.getPosition().y, static_cast<float>(w), static_cast<float>(h));
    subItemBoxes.clear();
    subItemBoxes.push_back(boundingBox);

    state = SelectionState::Floating;
    showHandles = true;
}

void SelectionManager::deleteSelection(sf::RenderTexture* layerTexture) {
    if (state == SelectionState::Selected) extractFromLayer(layerTexture, true);
    discardFloating();
}

void SelectionManager::flipHorizontal() {
    if (state == SelectionState::Floating) {
        sf::Vector2f sc = floatingSprite.getScale();
        floatingSprite.setScale(-sc.x, sc.y);
    }
}

void SelectionManager::flipVertical() {
    if (state == SelectionState::Floating) {
        sf::Vector2f sc = floatingSprite.getScale();
        floatingSprite.setScale(sc.x, -sc.y);
    }
}

void SelectionManager::duplicate(sf::RenderTexture* layerTexture, sf::Vector2u canvasSize) {
    if (state == SelectionState::Selected) {
        extractFromLayer(layerTexture, false);
        floatingSprite.move(20.f, 20.f);
        clampToCanvas(canvasSize);
    }
    else if (state == SelectionState::Floating && layerTexture) {
        layerTexture->draw(floatingSprite);
        layerTexture->display();
        floatingSprite.move(20.f, 20.f);
        clampToCanvas(canvasSize);
    }
}

void SelectionManager::setShowHandles(bool show) {
    showHandles = show;
    if (!show) {
        isResizingFlag = false;
        activeHandle = -1;
    }
}

void SelectionManager::setHandleVisualSize(float localSize) {
    handleVisualSize = std::max(0.05f, localSize);
}

std::array<sf::Vector2f, 4> SelectionManager::getHandlePositions() const {
    float w = boundingBox.width;
    float h = boundingBox.height;
    sf::Transform t = floatingSprite.getTransform();
    if (state == SelectionState::Floating) {
        return {
            t.transformPoint(0.f, 0.f),
            t.transformPoint(w, 0.f),
            t.transformPoint(w, h),
            t.transformPoint(0.f, h)
        };
    }
    return {
        sf::Vector2f(boundingBox.left, boundingBox.top),
        sf::Vector2f(boundingBox.left + w, boundingBox.top),
        sf::Vector2f(boundingBox.left + w, boundingBox.top + h),
        sf::Vector2f(boundingBox.left, boundingBox.top + h)
    };
}

int SelectionManager::hitTestHandle(sf::Vector2f pos, float handleRadius) const {
    if (state != SelectionState::Selected && state != SelectionState::Floating) return -1;
    auto corners = getHandlePositions();
    for (size_t i = 0; i < corners.size(); ++i) {
        sf::Vector2f d = pos - corners[i];
        if (std::hypot(d.x, d.y) <= handleRadius) return static_cast<int>(i);
    }
    return -1;
}

bool SelectionManager::startResize(sf::Vector2f pos, float handleRadius) {
    int idx = hitTestHandle(pos, handleRadius);
    if (idx == -1) return false;

    activeHandle = idx;
    int anchorIdx = (idx + 2) % 4;

    auto corners = getHandlePositions();
    resizeAnchorWorld = corners[anchorIdx];
    resizeStartBox = boundingBox;
    isResizingFlag = true;
    return true;
}

void SelectionManager::resize(sf::Vector2f pos, sf::Vector2u canvasSize, bool allowOutsideCanvas) {
    if (!isResizingFlag) return;

    float newW = std::max(2.0f, std::abs(pos.x - resizeAnchorWorld.x));
    float newH = std::max(2.0f, std::abs(pos.y - resizeAnchorWorld.y));

    float newLeft = std::min(resizeAnchorWorld.x, pos.x);
    float newTop = std::min(resizeAnchorWorld.y, pos.y);

    if (boundingBox.width > 0.001f && boundingBox.height > 0.001f) {
        float sx = newW / boundingBox.width;
        float sy = newH / boundingBox.height;

        for (auto& p : pathPoints) {
            p.x = newLeft + (p.x - boundingBox.left) * sx;
            p.y = newTop + (p.y - boundingBox.top) * sy;
        }
    }

    boundingBox = sf::FloatRect(newLeft, newTop, newW, newH);
}

void SelectionManager::endResize() {
    isResizingFlag = false;
    activeHandle = -1;
}

void SelectionManager::moveFloating(sf::Vector2f offset, sf::Vector2u canvasSize) {
    if (state == SelectionState::Floating) {
        floatingSprite.move(offset);
        clampToCanvas(canvasSize, false);
    }
}