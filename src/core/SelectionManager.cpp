#include "SelectionManager.h"
#include <cmath>
#include <algorithm>

SelectionManager::SelectionManager() : state(SelectionState::Inactive), dashOffset(0.f), hasClipboard(false), isDragging(false),
showHandles(false), handleVisualSize(6.f), isResizingFlag(false), activeHandle(-1),
resizeAnchorWorld(0.f, 0.f), resizeAnchorLocal(0.f, 0.f), resizeDraggedLocal(0.f, 0.f) {
    sf::Image dashImg;
    dashImg.create(8, 2, sf::Color::Transparent);
    for (int i = 0; i < 4; i++) {
        dashImg.setPixel(i, 0, sf::Color::White);
        dashImg.setPixel(i, 1, sf::Color::Black);
    }
    dashTexture.loadFromImage(dashImg);
    dashTexture.setRepeated(true);
}

void SelectionManager::update(float dt) {
    if (state != SelectionState::Inactive) {
        dashOffset -= 30.f * dt;
    }
}

void SelectionManager::draw(sf::RenderWindow& window, const sf::RenderStates& baseStates) {
    if (state == SelectionState::Inactive) return;

    if (state == SelectionState::Floating) {
        window.draw(floatingSprite, baseStates);
    }

    const std::vector<sf::Vector2f>& pts = (state == SelectionState::Floating) ? localPoints : pathPoints;

    if (pts.size() > 1) {
        sf::VertexArray ants(sf::LineStrip, pts.size());
        float dist = 0.f;
        for (size_t i = 0; i < pts.size(); ++i) {
            if (i > 0) {
                sf::Vector2f diff = pts[i] - pts[i - 1];
                dist += std::sqrt(diff.x * diff.x + diff.y * diff.y);
            }
            ants[i].position = pts[i];
            ants[i].texCoords = sf::Vector2f(dist + dashOffset, 0.f);
            ants[i].color = sf::Color(255, 255, 255, 200);
        }

        sf::RenderStates states = baseStates;
        states.texture = &dashTexture;
        if (state == SelectionState::Floating) {
            states.transform *= floatingSprite.getTransform();
        }

        window.draw(ants, states);
    }

    // Corner resize handles - already in world/canvas-space (from
    // getHandlePositions), so draw with baseStates as-is, not multiplied by
    // the floating sprite's transform like the dashed outline above.
    if (state == SelectionState::Floating && showHandles) {
        auto corners = getHandlePositions();
        for (const auto& c : corners) {
            sf::RectangleShape h(sf::Vector2f(handleVisualSize, handleVisualSize));
            h.setOrigin(handleVisualSize / 2.f, handleVisualSize / 2.f);
            h.setPosition(c);
            h.setFillColor(sf::Color(0, 191, 255));
            h.setOutlineThickness(std::max(0.5f, handleVisualSize * 0.15f));
            h.setOutlineColor(sf::Color::White);
            window.draw(h, baseStates);
        }
    }
}

void SelectionManager::startLasso(sf::Vector2f pos, sf::Vector2u canvasSize) {
    pos.x = std::clamp(pos.x, 0.f, static_cast<float>(canvasSize.x));
    pos.y = std::clamp(pos.y, 0.f, static_cast<float>(canvasSize.y));
    state = SelectionState::Drawing;
    pathPoints.clear();
    pathPoints.push_back(pos);
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
            state = SelectionState::Selected;
        }
        else {
            state = SelectionState::Inactive;
        }
    }
}

void SelectionManager::calculateBoundingBox() {
    if (pathPoints.empty()) return;
    float minX = pathPoints[0].x, maxX = pathPoints[0].x;
    float minY = pathPoints[0].y, maxY = pathPoints[0].y;
    for (const auto& p : pathPoints) {
        if (p.x < minX) minX = p.x;
        if (p.x > maxX) maxX = p.x;
        if (p.y < minY) minY = p.y;
        if (p.y > maxY) maxY = p.y;
    }
    boundingBox = sf::FloatRect(minX, minY, maxX - minX, maxY - minY);
}

void SelectionManager::clampToCanvas(sf::Vector2u canvasSize, bool skip) {
    if (skip) return;
    if (state != SelectionState::Floating) return;

    sf::Vector2f pos = floatingSprite.getPosition();
    sf::Vector2f origin = floatingSprite.getOrigin();
    sf::Vector2f scale = floatingSprite.getScale();

    float width = static_cast<float>(floatingTexture.getSize().x) * std::abs(scale.x);
    float height = static_cast<float>(floatingTexture.getSize().y) * std::abs(scale.y);

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
    if (state == SelectionState::Selected) {
        if (!boundingBox.contains(pos)) return false;
        return isInsidePolygon(pos, pathPoints);
    }
    else if (state == SelectionState::Floating) {
        sf::Vector2f localPos = floatingSprite.getInverseTransform().transformPoint(pos);
        return isInsidePolygon(localPos, localPoints);
    }
    return false;
}

void SelectionManager::extractFromLayer(sf::RenderTexture* layerTexture, bool removeOriginal) {
    if (state != SelectionState::Selected) return;

    int w = static_cast<int>(boundingBox.width);
    int h = static_cast<int>(boundingBox.height);
    if (w <= 0 || h <= 0) return;

    sf::Image sourceImg = layerTexture->getTexture().copyToImage();
    sf::Image extractImg;
    extractImg.create(static_cast<unsigned int>(w), static_cast<unsigned int>(h), sf::Color::Transparent);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            sf::Vector2f globalPt(boundingBox.left + x, boundingBox.top + y);
            if (isInsidePolygon(globalPt, pathPoints)) {
                if (globalPt.x >= 0 && globalPt.x < static_cast<float>(sourceImg.getSize().x) && globalPt.y >= 0 && globalPt.y < static_cast<float>(sourceImg.getSize().y)) {
                    extractImg.setPixel(static_cast<unsigned int>(x), static_cast<unsigned int>(y), sourceImg.getPixel(static_cast<unsigned int>(globalPt.x), static_cast<unsigned int>(globalPt.y)));
                    if (removeOriginal) {
                        sourceImg.setPixel(static_cast<unsigned int>(globalPt.x), static_cast<unsigned int>(globalPt.y), sf::Color::Transparent);
                    }
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
    floatingSprite.setTexture(floatingTexture, true);

    // Keep origin strictly top-left so it stays exactly where it was extracted
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
    if (state == SelectionState::Floating) {
        layerTexture->draw(floatingSprite);
        layerTexture->display();
    }
    state = SelectionState::Inactive;
    isDragging = false;
    showHandles = false;
    isResizingFlag = false;
    activeHandle = -1;
}

void SelectionManager::discardFloating() {
    state = SelectionState::Inactive;
    isDragging = false;
    showHandles = false;
    isResizingFlag = false;
    activeHandle = -1;
}

void SelectionManager::clearSelection() {
    state = SelectionState::Inactive;
    isDragging = false;
    showHandles = false;
    isResizingFlag = false;
    activeHandle = -1;
}

void SelectionManager::startDrag(sf::Vector2f pos) {
    if (state == SelectionState::Floating || state == SelectionState::Selected) {
        dragStartPos = pos;
        isDragging = true;
    }
}

void SelectionManager::drag(sf::Vector2f pos, sf::Vector2u canvasSize, bool allowOutsideCanvas) {
    if (state == SelectionState::Floating && isDragging) {
        sf::Vector2f delta = pos - dragStartPos;
        floatingSprite.move(delta);
        clampToCanvas(canvasSize, allowOutsideCanvas);
        dragStartPos = pos;
    }
}

void SelectionManager::endDrag() {
    isDragging = false;
}

void SelectionManager::copy() {
    if (state == SelectionState::Floating) {
        clipboardTexture = floatingTexture;
        hasClipboard = true;
    }
    else if (state == SelectionState::Selected) {
        int w = static_cast<int>(boundingBox.width);
        int h = static_cast<int>(boundingBox.height);
        if (w <= 0 || h <= 0) return;

        sf::Image tempImg;
        tempImg.create(static_cast<unsigned int>(w), static_cast<unsigned int>(h), sf::Color::Transparent);
        clipboardTexture.loadFromImage(tempImg);
        hasClipboard = true;
    }
}

void SelectionManager::paste(sf::Vector2u canvasSize) {
    if (!hasClipboard) return;

    floatingTexture = clipboardTexture;
    floatingSprite.setTexture(floatingTexture, true);
    int w = static_cast<int>(floatingTexture.getSize().x);
    int h = static_cast<int>(floatingTexture.getSize().y);

    // Set origin to top-left to avoid coordinate drift when pasting
    floatingSprite.setOrigin(0.f, 0.f);
    floatingSprite.setPosition(boundingBox.left, boundingBox.top);
    floatingSprite.setScale(1.f, 1.f);

    localPoints.clear();
    localPoints.push_back(sf::Vector2f(0.f, 0.f));
    localPoints.push_back(sf::Vector2f(static_cast<float>(w), 0.f));
    localPoints.push_back(sf::Vector2f(static_cast<float>(w), static_cast<float>(h)));
    localPoints.push_back(sf::Vector2f(0.f, static_cast<float>(h)));
    localPoints.push_back(sf::Vector2f(0.f, 0.f));

    state = SelectionState::Floating;
}

void SelectionManager::deleteSelection(sf::RenderTexture* layerTexture) {
    if (state == SelectionState::Selected) extractFromLayer(layerTexture, true);
    discardFloating();
}

void SelectionManager::flipHorizontal() {
    if (state == SelectionState::Floating) {
        sf::Image img = floatingTexture.copyToImage();
        int w = static_cast<int>(img.getSize().x);
        int h = static_cast<int>(img.getSize().y);
        sf::Image flipped;
        flipped.create(static_cast<unsigned int>(w), static_cast<unsigned int>(h), sf::Color::Transparent);

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                flipped.setPixel(static_cast<unsigned int>(w - 1 - x), static_cast<unsigned int>(y), img.getPixel(static_cast<unsigned int>(x), static_cast<unsigned int>(y)));
            }
        }

        floatingTexture.loadFromImage(flipped);
        floatingSprite.setTexture(floatingTexture, true);
    }
}

void SelectionManager::flipVertical() {
    if (state == SelectionState::Floating) {
        sf::Image img = floatingTexture.copyToImage();
        int w = static_cast<int>(img.getSize().x);
        int h = static_cast<int>(img.getSize().y);
        sf::Image flipped;
        flipped.create(static_cast<unsigned int>(w), static_cast<unsigned int>(h), sf::Color::Transparent);

        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                flipped.setPixel(static_cast<unsigned int>(x), static_cast<unsigned int>(h - 1 - y), img.getPixel(static_cast<unsigned int>(x), static_cast<unsigned int>(y)));
            }
        }

        floatingTexture.loadFromImage(flipped);
        floatingSprite.setTexture(floatingTexture, true);
    }
}

void SelectionManager::duplicate(sf::RenderTexture* layerTexture, sf::Vector2u canvasSize) {
    if (state == SelectionState::Selected) {
        extractFromLayer(layerTexture, false);
        floatingSprite.move(20.f, 20.f);
        clampToCanvas(canvasSize);
    }
    else if (state == SelectionState::Floating) {
        layerTexture->draw(floatingSprite);
        layerTexture->display();
        floatingSprite.move(20.f, 20.f);
        clampToCanvas(canvasSize);
    }
}

SelectionState SelectionManager::getState() const { return state; }
bool SelectionManager::isActive() const { return state != SelectionState::Inactive; }

// ---------------------------------------------------------------------------
// Resize / free-transform via corner handles
// ---------------------------------------------------------------------------

void SelectionManager::setShowHandles(bool show) {
    showHandles = show;
    if (!show) {
        isResizingFlag = false;
        activeHandle = -1;
    }
}

bool SelectionManager::isShowingHandles() const { return showHandles; }

void SelectionManager::setHandleVisualSize(float localSize) {
    handleVisualSize = std::max(1.0f, localSize);
}

std::array<sf::Vector2f, 4> SelectionManager::getHandlePositions() const {
    float w = static_cast<float>(floatingTexture.getSize().x);
    float h = static_cast<float>(floatingTexture.getSize().y);
    sf::Transform t = floatingSprite.getTransform();
    return {
        t.transformPoint(0.f, 0.f), // TL
        t.transformPoint(w, 0.f),   // TR
        t.transformPoint(w, h),     // BR
        t.transformPoint(0.f, h)    // BL
    };
}

int SelectionManager::hitTestHandle(sf::Vector2f pos, float handleRadius) const {
    if (state != SelectionState::Floating) return -1;
    auto corners = getHandlePositions();
    for (size_t i = 0; i < corners.size(); ++i) {
        sf::Vector2f d = pos - corners[i];
        if (std::sqrt(d.x * d.x + d.y * d.y) <= handleRadius) return static_cast<int>(i);
    }
    return -1;
}

bool SelectionManager::startResize(sf::Vector2f pos, float handleRadius) {
    if (state != SelectionState::Floating) return false;
    int idx = hitTestHandle(pos, handleRadius);
    if (idx == -1) return false;

    activeHandle = idx;
    int anchorIdx = (idx + 2) % 4; // opposite corner stays fixed

    auto worldCorners = getHandlePositions();
    resizeAnchorWorld = worldCorners[anchorIdx];

    float w = static_cast<float>(floatingTexture.getSize().x);
    float h = static_cast<float>(floatingTexture.getSize().y);
    sf::Vector2f origin = floatingSprite.getOrigin();

    std::array<sf::Vector2f, 4> localFull = {
        sf::Vector2f(0.f, 0.f), sf::Vector2f(w, 0.f), sf::Vector2f(w, h), sf::Vector2f(0.f, h)
    };
    resizeAnchorLocal = localFull[anchorIdx] - origin;
    resizeDraggedLocal = localFull[idx] - origin;

    isResizingFlag = true;
    return true;
}

void SelectionManager::resize(sf::Vector2f pos, sf::Vector2u canvasSize, bool allowOutsideCanvas) {
    if (!isResizingFlag || state != SelectionState::Floating) return;

    sf::Vector2f diffLocal = resizeDraggedLocal - resizeAnchorLocal;

    float newScaleX = floatingSprite.getScale().x;
    float newScaleY = floatingSprite.getScale().y;
    if (std::abs(diffLocal.x) > 0.0001f) newScaleX = (pos.x - resizeAnchorWorld.x) / diffLocal.x;
    if (std::abs(diffLocal.y) > 0.0001f) newScaleY = (pos.y - resizeAnchorWorld.y) / diffLocal.y;

    // Prevent inverting/vanishing the selection - clamp to a sane minimum
    // size in canvas-logical pixels regardless of the source texture size.
    float w = static_cast<float>(floatingTexture.getSize().x);
    float h = static_cast<float>(floatingTexture.getSize().y);
    const float minDim = 4.0f;
    float minScaleX = minDim / std::max(1.f, w);
    float minScaleY = minDim / std::max(1.f, h);
    if (newScaleX < minScaleX) newScaleX = minScaleX;
    if (newScaleY < minScaleY) newScaleY = minScaleY;

    sf::Vector2f newPos;
    newPos.x = resizeAnchorWorld.x - newScaleX * resizeAnchorLocal.x;
    newPos.y = resizeAnchorWorld.y - newScaleY * resizeAnchorLocal.y;

    floatingSprite.setScale(newScaleX, newScaleY);
    floatingSprite.setPosition(newPos);

    clampToCanvas(canvasSize, allowOutsideCanvas);
}

void SelectionManager::endResize() {
    isResizingFlag = false;
    activeHandle = -1;
}

bool SelectionManager::isResizing() const { return isResizingFlag; }