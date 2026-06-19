#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <string>

enum class BlendMode {
    Normal,
    Multiply,
    Additive,
    Screen,
    Overlay
};

enum class ToolType {
    Brush,
    Pencil,
    Eraser,
    Fill,
    Select
};

struct Layer {
    sf::RenderTexture* texture;
    std::string name;
    bool visible;
    bool locked;
    float opacity;
    BlendMode blendMode;

    Layer(std::string n = "Layer");
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

    std::vector<std::vector<Frame>> undoHistory;
    std::vector<std::vector<Frame>> redoHistory;

    sf::Texture deskTexture;
    sf::Sprite deskSprite;

    sf::Texture canvasTexture;
    sf::Sprite canvasSprite;

    sf::RenderTexture previewTexture;

    sf::FloatRect drawArea;
    sf::Vector2u canvasLogicalSize;

    bool isDrawing;
    sf::Vector2f startPos;
    sf::Vector2f lastPos;

    ToolType activeTool;
    float brushSize;
    float brushHardness;
    sf::Color primaryColor;
    sf::Color secondaryColor;
    bool brushSmoothing;
    float fillTolerance;
    bool fillContiguous;

    int activeLayer;
    bool onionSkinEnabled;
    float onionSkinOpacity;
    int onionSkinPrevCount;
    int onionSkinNextCount;

    sf::Vector2f viewOffset;
    float viewScale;
    sf::Vector2f targetOffset;
    float targetScale;

    // Selection Data
    bool hasSelection;
    sf::FloatRect selectionBounds;
    sf::Texture selectionTexture;
    sf::Vector2f selectionOffset;
    bool isMovingSelection;
    float selectionDashOffset;

    sf::RenderStates getSFMLBlendMode(BlendMode mode) const;
    void executeScanlineFill(sf::Vector2i startPoint, sf::Color targetColor, sf::Color replacementColor, sf::Image& image);
    void executeGlobalFill(sf::Color targetColor, sf::Color replacementColor, sf::Image& image);
    bool colorMatches(const sf::Color& a, const sf::Color& b) const;

public:
    Canvas();
    void init();
    void initCustom(int width, int height);

    void addFrame(int index);
    void duplicateFrame(int index);
    void deleteFrame(int index);
    void clearAllFrames();

    void addLayer(int frameIndex, const std::string& name = "New Layer");
    void deleteLayer(int frameIndex, int layerIndex);
    void duplicateLayer(int frameIndex, int layerIndex);
    void setLayerProperties(int frameIndex, int layerIndex, const std::string& name, bool visible, bool locked, float opacity, BlendMode mode);
    void moveLayer(int frameIndex, int fromIndex, int toIndex);
    void setActiveLayer(int index);
    int getActiveLayer() const;

    void setOnionSkin(bool enabled, float opacity, int prevCount = 1, int nextCount = 0);
    bool isOnionSkinEnabled() const;
    float getOnionSkinOpacity() const;

    void setActiveTool(ToolType tool);
    ToolType getActiveTool() const;
    void commitSelection(int currentFrame);

    void handleMousePressed(sf::Vector2f logicalPos, bool rightClick, int currentFrame);
    void handleMouseReleased(sf::Vector2f logicalPos, int currentFrame);
    void handleMouseMoved(sf::Vector2f logicalPos, int currentFrame);

    void setBrushSize(float size);
    float getBrushSize() const;
    void setBrushHardness(float hardness);
    void setPrimaryColor(sf::Color color);
    void setSecondaryColor(sf::Color color);
    sf::Color getPrimaryColor() const;
    void setBrushSmoothing(bool smoothing);
    void setFillSettings(float tolerance, bool contiguous);

    void undo();
    void redo();
    void saveUndoState();

    void updateTransform(float dt, sf::FloatRect availableSpace);
    sf::Transform getTransform() const;
    sf::Transform getInverseTransform() const;

    void draw(sf::RenderWindow& window, int currentFrame, bool isPlaying, const sf::RenderStates& states);
    void drawShadows(sf::RenderWindow& window, sf::Vector2f logicalSunPos, const std::vector<sf::FloatRect>& items, const std::vector<std::string>& categories, const sf::RenderStates& states);

    sf::FloatRect getDrawArea() const;
    sf::Vector2u getCanvasSize() const;
    sf::RenderTexture* getActiveRenderTexture(int currentFrame);
    Frame* getFrame(int index);
    const Frame* getFrameReadOnly(int index) const;
    size_t getFrameCount() const;
};