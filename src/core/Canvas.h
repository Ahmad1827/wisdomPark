#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <string>

struct Layer {
    std::unique_ptr<sf::RenderTexture> texture;
    bool visible;
    float opacity;

    Layer();
    ~Layer();
    Layer(const Layer& other);
    Layer& operator=(const Layer& other);
    Layer(Layer&& other) noexcept;
    Layer& operator=(Layer&& other) noexcept;
};

struct Frame {
    std::vector<Layer> layers;

    Frame();
    ~Frame();
    Frame(const Frame& other);
    Frame& operator=(const Frame& other);
    Frame(Frame&& other) noexcept;
    Frame& operator=(Frame&& other) noexcept;
};

class Canvas {
private:
    std::vector<Frame> frames;
    std::vector<sf::Image> undoHistory;
    std::vector<sf::Image> redoHistory;

    sf::Sprite canvasSprite;
    sf::Texture canvasTexture;
    sf::FloatRect drawArea;

    bool isDrawing;
    sf::Vector2f lastPos;
    float brushSize;
    sf::Color brushColor;

    int activeLayer;
    bool onionSkinEnabled;
    float onionSkinOpacity;

public:
    Canvas();
    void init();

    void addFrame(int index);
    void duplicateFrame(int index);
    void deleteFrame(int index);

    void setOnionSkin(bool enabled, float opacity);
    bool isOnionSkinEnabled() const;
    float getOnionSkinOpacity() const;

    void handleMousePressed(sf::Vector2f mousePos, bool middleClick, int currentFrame);
    void handleMouseReleased();
    void handleMouseMoved(sf::Vector2f mousePos, int currentFrame);

    void setBrushSize(float size);
    float getBrushSize() const;
    void setBrushColor(sf::Color color);
    sf::Color getBrushColor() const;

    void undo(int currentFrame);
    void redo(int currentFrame);
    void saveUndoState(int currentFrame);

    void draw(sf::RenderWindow& window, int currentFrame, bool isPlaying);
    void drawShadows(sf::RenderWindow& window, sf::Vector2f sunPos, const std::vector<sf::FloatRect>& items, const std::vector<std::string>& categories);

    sf::FloatRect getDrawArea() const;
    sf::RenderTexture* getFrame(int index);
    size_t getFrameCount() const;
};