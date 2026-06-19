#include "SelectionManager.h"
#include <cmath>

SelectionManager::SelectionManager() : state(SelectionState::Inactive), dashOffset(0.f), hasClipboard(false) {
    // Generate a 8x2 alternating black/white texture for marching ants
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
        dashOffset -= 30.f * dt; // Animate marching ants
    }
}

void SelectionManager::draw(sf::RenderWindow& window, const sf::RenderStates& baseStates) {
    if (state == SelectionState::Inactive) return;

    // Draw the floating pixels if we have cut them from the canvas
    if (state == SelectionState::Floating) {
        window.draw(floatingSprite, baseStates);
    }

    // Draw the selection outline (Marching Ants)
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
}

void SelectionManager::startLasso(sf::Vector2f pos) {
    state = SelectionState::Drawing;
    pathPoints.clear();
    pathPoints.push_back(pos);
}

void SelectionManager::addLassoPoint(sf::Vector2f pos) {
    if (state == SelectionState::Drawing) {
        // Prevent stacking duplicate points
        if (pathPoints.empty() || pathPoints.back() != pos) {
            pathPoints.push_back(pos);
        }
    }
}

void SelectionManager::endLasso() {
    if (state == SelectionState::Drawing) {
        if (pathPoints.size() > 2) {
            // Close the polygon
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

void SelectionManager::cutFromLayer(sf::RenderTexture* layerTexture) {
    if (state != SelectionState::Selected) return;

    int w = static_cast<int>(boundingBox.width);
    int h = static_cast<int>(boundingBox.height);
    if (w <= 0 || h <= 0) return;

    sf::Image sourceImg = layerTexture->getTexture().copyToImage();
    sf::Image extractImg;
    extractImg.create(w, h, sf::Color::Transparent);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            sf::Vector2f globalPt(boundingBox.left + x, boundingBox.top + y);
            if (isInsidePolygon(globalPt, pathPoints)) {
                if (globalPt.x >= 0 && globalPt.x < sourceImg.getSize().x && globalPt.y >= 0 && globalPt.y < sourceImg.getSize().y) {
                    extractImg.setPixel(x, y, sourceImg.getPixel(globalPt.x, globalPt.y));
                    sourceImg.setPixel(globalPt.x, globalPt.y, sf::Color::Transparent);
                }
            }
        }
    }

    // Apply the "hole" back to the canvas
    layerTexture->clear(sf::Color::Transparent);
    sf::Texture tempTex; tempTex.loadFromImage(sourceImg);
    layerTexture->draw(sf::Sprite(tempTex), sf::RenderStates(sf::BlendNone));
    layerTexture->display();

    // Create the floating sprite
    floatingTexture.loadFromImage(extractImg);
    floatingSprite.setTexture(floatingTexture, true);

    // Set origin to center for easy rotation/scaling
    floatingSprite.setOrigin(w / 2.f, h / 2.f);
    floatingSprite.setPosition(boundingBox.left + w / 2.f, boundingBox.top + h / 2.f);
    floatingSprite.setScale(1.f, 1.f);
    floatingSprite.setRotation(0.f);

    // Convert lasso points to local space relative to the sprite origin
    localPoints.clear();
    for (const auto& p : pathPoints) {
        localPoints.push_back(p - floatingSprite.getPosition());
    }

    state = SelectionState::Floating;
}

void SelectionManager::commitToLayer(sf::RenderTexture* layerTexture) {
    if (state == SelectionState::Floating) {
        layerTexture->draw(floatingSprite);
        layerTexture->display();
    }
    state = SelectionState::Inactive;
}

void SelectionManager::discardFloating() {
    state = SelectionState::Inactive;
}

void SelectionManager::clearSelection() {
    state = SelectionState::Inactive;
}

void SelectionManager::startDrag(sf::Vector2f pos) {
    if (state == SelectionState::Floating || state == SelectionState::Selected) {
        dragStartPos = pos;
    }
}

void SelectionManager::drag(sf::Vector2f pos) {
    if (state == SelectionState::Floating) {
        sf::Vector2f delta = pos - dragStartPos;
        floatingSprite.move(delta);
        dragStartPos = pos;
    }
}

void SelectionManager::copy() {
    if (state == SelectionState::Floating) {
        clipboardTexture = floatingTexture;
        hasClipboard = true;
    }
    else if (state == SelectionState::Selected) {
        // Selection hasn't been cut yet, but we can copy it via bounding box bounding
        // (Handled directly by canvas passing it to us, or we just rely on cutting first for simplicity)
    }
}

void SelectionManager::paste() {
    if (!hasClipboard) return;

    floatingTexture = clipboardTexture;
    floatingSprite.setTexture(floatingTexture, true);
    int w = floatingTexture.getSize().x;
    int h = floatingTexture.getSize().y;
    floatingSprite.setOrigin(w / 2.f, h / 2.f);
    floatingSprite.setPosition(1920.f / 2.f, 1080.f / 2.f); // Paste in center of logic canvas

    // Generate a simple rectangle bounding box for pasted items if we didn't save the exact polygon
    localPoints.clear();
    localPoints.push_back(sf::Vector2f(-w / 2.f, -h / 2.f));
    localPoints.push_back(sf::Vector2f(w / 2.f, -h / 2.f));
    localPoints.push_back(sf::Vector2f(w / 2.f, h / 2.f));
    localPoints.push_back(sf::Vector2f(-w / 2.f, h / 2.f));
    localPoints.push_back(sf::Vector2f(-w / 2.f, -h / 2.f));

    state = SelectionState::Floating;
}

void SelectionManager::deleteSelection(sf::RenderTexture* layerTexture) {
    if (state == SelectionState::Selected) cutFromLayer(layerTexture);
    discardFloating();
}

SelectionState SelectionManager::getState() const { return state; }
bool SelectionManager::isActive() const { return state != SelectionState::Inactive; }