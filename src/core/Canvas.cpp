#include "Canvas.h"
#include <cmath>
#include <algorithm>
#include <iostream>

Layer::Layer() : visible(true), opacity(255.0f) {
    texture = std::make_unique<sf::RenderTexture>();
    texture->create(1920, 1080);
    texture->clear(sf::Color::Transparent);
    texture->display();
}

Layer::~Layer() = default;

Layer::Layer(const Layer& other) : visible(other.visible), opacity(other.opacity) {
    texture = std::make_unique<sf::RenderTexture>();
    texture->create(1920, 1080);
    texture->clear(sf::Color::Transparent);
    sf::Sprite spr(other.texture->getTexture());
    texture->draw(spr);
    texture->display();
}

Layer& Layer::operator=(const Layer& other) {
    if (this != &other) {
        visible = other.visible;
        opacity = other.opacity;
        if (!texture) {
            texture = std::make_unique<sf::RenderTexture>();
            texture->create(1920, 1080);
        }
        texture->clear(sf::Color::Transparent);
        sf::Sprite spr(other.texture->getTexture());
        texture->draw(spr);
        texture->display();
    }
    return *this;
}

Layer::Layer(Layer&& other) noexcept = default;
Layer& Layer::operator=(Layer&& other) noexcept = default;

Frame::Frame() {
    layers.emplace_back();
}

Frame::~Frame() = default;

Frame::Frame(const Frame& other) {
    for (const auto& l : other.layers) {
        layers.emplace_back(l);
    }
}

Frame& Frame::operator=(const Frame& other) {
    if (this != &other) {
        layers.clear();
        for (const auto& l : other.layers) {
            layers.emplace_back(l);
        }
    }
    return *this;
}

Frame::Frame(Frame&& other) noexcept = default;
Frame& Frame::operator=(Frame&& other) noexcept = default;

Canvas::Canvas() : isDrawing(false), brushSize(5.0f), brushColor(sf::Color::Black), activeLayer(0), onionSkinEnabled(false), onionSkinOpacity(85.0f) {}

void Canvas::init() {
    canvasTexture.loadFromFile("assets/canvas.png");
    canvasSprite.setTexture(canvasTexture);
    float canvasScale = 700.0f / canvasSprite.getLocalBounds().height;
    canvasSprite.setScale(canvasScale, canvasScale);

    float canvasWidth = canvasSprite.getLocalBounds().width * canvasScale;
    float canvasHeight = canvasSprite.getLocalBounds().height * canvasScale;
    float canvasX = (1920.0f - canvasWidth) / 2.0f;
    float canvasY = 20.0f;
    canvasSprite.setPosition(canvasX, canvasY);

    float frameOffsetX = canvasWidth * 0.08f;
    float frameOffsetYTop = canvasHeight * 0.16f;
    float frameOffsetYBot = canvasHeight * 0.16f;
    drawArea = sf::FloatRect(
        canvasX + frameOffsetX,
        canvasY + frameOffsetYTop,
        canvasWidth - (frameOffsetX * 2.0f),
        canvasHeight - frameOffsetYTop - frameOffsetYBot
    );

    frames.emplace_back();
}

void Canvas::addFrame(int index) {
    frames.insert(frames.begin() + index + 1, Frame());
}

void Canvas::duplicateFrame(int index) {
    if (index >= 0 && index < frames.size()) {
        frames.insert(frames.begin() + index + 1, Frame(frames[index]));
    }
}

void Canvas::deleteFrame(int index) {
    if (frames.size() > 1 && index >= 0 && index < frames.size()) {
        frames.erase(frames.begin() + index);
    }
}

void Canvas::setOnionSkin(bool enabled, float opacity) {
    onionSkinEnabled = enabled;
    onionSkinOpacity = std::clamp(opacity, 0.0f, 255.0f);
}

bool Canvas::isOnionSkinEnabled() const { return onionSkinEnabled; }
float Canvas::getOnionSkinOpacity() const { return onionSkinOpacity; }

void Canvas::handleMousePressed(sf::Vector2f mousePos, bool middleClick, int currentFrame) {
    if (drawArea.contains(mousePos)) {
        if (!middleClick) {
            isDrawing = true;
            lastPos = mousePos;
        }
    }
}

void Canvas::handleMouseReleased() {
    isDrawing = false;
}

void Canvas::handleMouseMoved(sf::Vector2f mousePos, int currentFrame) {
    if (isDrawing && drawArea.contains(mousePos) && currentFrame >= 0 && currentFrame < frames.size()) {
        sf::Vector2f d = mousePos - lastPos;
        float length = std::sqrt(d.x * d.x + d.y * d.y);

        sf::RectangleShape line(sf::Vector2f(length, brushSize));
        line.setOrigin(0, brushSize / 2.f);
        line.setPosition(lastPos);
        line.setRotation(std::atan2(d.y, d.x) * 180.f / 3.14159265f);
        line.setFillColor(brushColor);

        sf::CircleShape circle(brushSize / 2.f);
        circle.setOrigin(brushSize / 2.f, brushSize / 2.f);
        circle.setPosition(mousePos);
        circle.setFillColor(brushColor);

        auto& tex = frames[currentFrame].layers[activeLayer].texture;

        if (brushColor == sf::Color::Transparent) {
            tex->draw(line, sf::RenderStates(sf::BlendNone));
            tex->draw(circle, sf::RenderStates(sf::BlendNone));
        }
        else {
            tex->draw(line);
            tex->draw(circle);
        }

        tex->display();
        lastPos = mousePos;
    }
    else if (!drawArea.contains(mousePos)) {
        isDrawing = false;
    }
}

void Canvas::setBrushSize(float size) {
    brushSize = std::clamp(size, 1.0f, 100.0f);
}

float Canvas::getBrushSize() const { return brushSize; }
void Canvas::setBrushColor(sf::Color color) { brushColor = color; }
sf::Color Canvas::getBrushColor() const { return brushColor; }

void Canvas::saveUndoState(int currentFrame) {
    if (currentFrame >= 0 && currentFrame < frames.size()) {
        undoHistory.push_back(frames[currentFrame].layers[activeLayer].texture->getTexture().copyToImage());
        redoHistory.clear();
    }
}

void Canvas::undo(int currentFrame) {
    if (!undoHistory.empty() && currentFrame >= 0 && currentFrame < frames.size()) {
        redoHistory.push_back(frames[currentFrame].layers[activeLayer].texture->getTexture().copyToImage());
        sf::Texture tex;
        tex.loadFromImage(undoHistory.back());
        sf::Sprite spr(tex);
        frames[currentFrame].layers[activeLayer].texture->clear(sf::Color::Transparent);
        frames[currentFrame].layers[activeLayer].texture->draw(spr);
        frames[currentFrame].layers[activeLayer].texture->display();
        undoHistory.pop_back();
    }
}

void Canvas::redo(int currentFrame) {
    if (!redoHistory.empty() && currentFrame >= 0 && currentFrame < frames.size()) {
        undoHistory.push_back(frames[currentFrame].layers[activeLayer].texture->getTexture().copyToImage());
        sf::Texture tex;
        tex.loadFromImage(redoHistory.back());
        sf::Sprite spr(tex);
        frames[currentFrame].layers[activeLayer].texture->clear(sf::Color::Transparent);
        frames[currentFrame].layers[activeLayer].texture->draw(spr);
        frames[currentFrame].layers[activeLayer].texture->display();
        redoHistory.pop_back();
    }
}

void Canvas::draw(sf::RenderWindow& window, int currentFrame, bool isPlaying) {
    window.draw(canvasSprite);

    if (!isPlaying && onionSkinEnabled && currentFrame > 0 && currentFrame < frames.size()) {
        for (const auto& layer : frames[currentFrame - 1].layers) {
            if (layer.visible) {
                sf::Sprite onionSpr(layer.texture->getTexture());
                onionSpr.setColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(onionSkinOpacity)));
                window.draw(onionSpr, sf::BlendAlpha);
            }
        }
    }

    if (currentFrame >= 0 && currentFrame < frames.size()) {
        for (const auto& layer : frames[currentFrame].layers) {
            if (layer.visible) {
                sf::Sprite spr(layer.texture->getTexture());
                spr.setColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(layer.opacity)));
                window.draw(spr);
            }
        }
    }
}

void Canvas::drawShadows(sf::RenderWindow& window, sf::Vector2f sunPos, const std::vector<sf::FloatRect>& items, const std::vector<std::string>& categories) {
    for (size_t i = 0; i < items.size(); ++i) {
        bool isClutter = (categories[i] == "healing" || categories[i] == "status-cures" || categories[i] == "vitamins" || categories[i] == "clutter");
        float shadowLen = isClutter ? 30.0f : 150.0f;

        sf::Vector2f baseCenter(items[i].left + items[i].width / 2.0f, items[i].top + items[i].height);
        sf::Vector2f dir = baseCenter - sunPos;
        float dist = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        if (dist > 0) { dir.x /= dist; dir.y /= dist; }

        sf::ConvexShape shadow;
        shadow.setPointCount(4);
        shadow.setPoint(0, sf::Vector2f(items[i].left, items[i].top + items[i].height));
        shadow.setPoint(1, sf::Vector2f(items[i].left + items[i].width, items[i].top + items[i].height));
        shadow.setPoint(2, sf::Vector2f(items[i].left + items[i].width + dir.x * shadowLen, items[i].top + items[i].height + dir.y * shadowLen));
        shadow.setPoint(3, sf::Vector2f(items[i].left + dir.x * shadowLen, items[i].top + items[i].height + dir.y * shadowLen));

        shadow.setFillColor(sf::Color(0, 0, 0, 100));
        window.draw(shadow);
    }

    sf::CircleShape sunShape(15);
    sunShape.setOrigin(15, 15);
    sunShape.setPosition(sunPos);
    sunShape.setFillColor(sf::Color(255, 255, 200, 200));
    sunShape.setOutlineThickness(2);
    sunShape.setOutlineColor(sf::Color::Yellow);
    window.draw(sunShape);
}

sf::FloatRect Canvas::getDrawArea() const { return drawArea; }

sf::RenderTexture* Canvas::getFrame(int index) {
    if (index >= 0 && index < frames.size()) {
        return frames[index].layers[activeLayer].texture.get();
    }
    return nullptr;
}

size_t Canvas::getFrameCount() const { return frames.size(); }