#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <memory>
#include "SelectionManager.h"
#include "BrushManager.h"
#include "FrameRenderer.h"
#include "SymmetryManager.h"
#include "DitherManager.h"

enum class ToolType { Brush, Pencil, Eraser, Fill, Select, Symmetry, Shapes, MagicWand };
enum class BlendMode { Normal, Multiply, Additive, Screen, Overlay };
enum class TransformState { None, Scaling };

struct Layer {
    std::string name;
    bool visible;
    bool locked;
    float opacity;
    BlendMode blendMode;
    bool persistent;
    int colorTag;
    bool isImageResource;
    std::shared_ptr<sf::RenderTexture> texture;
    std::shared_ptr<sf::Texture> staticTexture;

    Layer(std::string n = "Layer");
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
    SymmetryManager symmetryManager;
    DitherManager ditherManager;
    bool useDithering = false;

    FrameRenderer frameRenderer;
    sf::Vector2u canvasLogicalSize;
    std::vector<Frame> frames;

    int activeLayer;
    ToolType activeTool;
    sf::Color primaryColor;
    sf::Color secondaryColor;

    float fillTolerance;
    bool fillContiguous;

    bool onionSkinEnabled;
    float onionSkinPrevOpacity;
    float onionSkinNextOpacity;
    int onionSkinPrevCount;
    int onionSkinNextCount;

    sf::FloatRect drawArea;
    float viewScale;
    float targetScale;
    sf::Vector2f viewOffset;
    sf::Vector2f targetOffset;

    float zoomMultiplier;
    sf::Vector2f panOffset;

    bool isDrawing;
    sf::Vector2f startPos;
    sf::Vector2f lastPos;
    sf::Vector2f lastHoverLocalPos;
    sf::Vector2f rawMousePos;
    bool isHoveringCanvas;

    std::vector<std::vector<Frame>> undoHistory;
    std::vector<std::vector<Frame>> redoHistory;

    sf::Texture deskTexture;
    sf::Sprite deskSprite;
    sf::Texture canvasTexture;
    sf::Sprite canvasSprite;

    BrushManager brushEngine;
    SelectionManager selection;

    bool isPixelMode;
    int pixelBrushSize;
    bool pixelGridEnabled;
    bool pixelSnapEnabled;
    bool tileModeX;
    bool tileModeY;
    bool pixelPerfectEnabled;

    bool isDirty;

    TransformState transformMode;
    bool pendingTransform;
    float currentRotation;
    sf::Vector2f currentScale;

    sf::Image layerSnapshot;
    std::vector<sf::Vector2i> activeStroke;
    sf::Texture frameTex[8];
    bool hasFrameAssets;

    bool colorMatches(const sf::Color& a, const sf::Color& b) const;
    void executeGlobalFill(sf::Color targetColor, sf::Color replacementColor, sf::Image& image);
    void executeQueueFill(sf::Vector2i startPoint, sf::Color targetColor, sf::Color replacementColor, sf::Image& image);

    void drawPixelExact(int x, int y, sf::Color c, int frameIdx);
    std::vector<sf::Vector2i> getBresenhamPoints(int x0, int y0, int x1, int y1);
    void drawBresenhamLine(int x0, int y0, int x1, int y1, sf::Color c, int frameIdx);

    float computeHandleHitRadius() const;
    bool isImageResourceActive(int currentFrame) const;

public:
    Canvas();
    void init();
    void initCustom(int width, int height);

    void updateTransform(float dt, sf::FloatRect space);
    sf::Transform getTransform() const;
    sf::Transform getInverseTransform() const;
    sf::FloatRect getDrawArea() const;
    sf::Vector2u getCanvasSize() const;

    void zoom(float delta);
    void pan(sf::Vector2f delta);
    void resetView();

    void addFrame(int index = -1);
    void duplicateFrame(int index);
    void deleteFrame(int index);
    void clearAllFrames();
    size_t getFrameCount() const;
    Frame* getFrame(int index);
    const Frame* getFrameReadOnly(int index) const;
    sf::RenderTexture* getActiveRenderTexture(int currentFrame);

    void addLayer(int frameIndex, const std::string& name = "Layer");
    void deleteLayer(int frameIndex, int layerIndex);
    void duplicateLayer(int frameIndex, int layerIndex);
    void setLayerProperties(int frameIndex, int layerIndex, const std::string& name, bool visible, bool locked, float opacity, BlendMode mode);
    void toggleLayerPersistence(int frameIndex, int layerIndex);
    void cycleLayerColorTag(int frameIndex, int layerIndex);
    void pushLayerToNextFrame(int currentFrame, int layerIndex);
    void mergeDown(int frameIndex);
    void mergeVisible(int frameIndex);
    void moveLayer(int frameIndex, int fromIndex, int toIndex);

    void setActiveLayer(int index);
    int getActiveLayer() const;

    void setOnionSkin(bool enabled, float prevOpac, float nextOpac);
    void setOnionSkinCounts(int prevCount, int nextCount);
    bool isOnionSkinEnabled() const;
    float getOnionSkinPrevOpacity() const;
    float getOnionSkinNextOpacity() const;
    int getOnionSkinPrevCount() const;
    int getOnionSkinNextCount() const;

    void commitSelection(int currentFrame);
    void copySelection();
    void pasteSelection(int currentFrame);
    void deleteSelection(int currentFrame);
    void flipSelectionHorizontal(int currentFrame);
    void flipSelectionVertical(int currentFrame);
    void duplicateSelection(int currentFrame);
    void cropSelection(int currentFrame);

    void setActiveTool(ToolType tool);
    ToolType getActiveTool() const;
    BrushManager& getBrushEngine();
    void setBrushSize(float size);
    float getBrushSize() const;

    void setPrimaryColor(sf::Color color);
    void setSecondaryColor(sf::Color color);
    sf::Color getPrimaryColor() const;
    sf::Color getSecondaryColor() const;
    void setFillSettings(float tolerance, bool contiguous);

    void saveUndoState();
    void undo();
    void redo();

    sf::RenderStates getSFMLBlendMode(BlendMode mode) const;

    void handleMousePressed(sf::Vector2f logicalPos, bool rightClick, int currentFrame);
    void handleMouseReleased(sf::Vector2f logicalPos, int currentFrame);
    void handleMouseMoved(sf::Vector2f logicalPos, sf::Vector2f rawPos, int currentFrame);

    void draw(sf::RenderWindow& window, int currentFrame, bool isPlaying, const sf::RenderStates& states);
    void drawShadows(sf::RenderWindow& window, sf::Vector2f logicalSunPos, const std::vector<sf::FloatRect>& items, const std::vector<std::string>& categories, const sf::RenderStates& states);

    void setPixelMode(bool enabled);
    bool getPixelMode() const;
    void setPixelBrushSize(int size);
    int getPixelBrushSize() const;
    void cyclePixelBrushSize();
    void togglePixelGrid();
    bool isPixelGridEnabled() const;
    void togglePixelSnap();
    bool isPixelSnapEnabled() const;
    void toggleTileMode();
    void togglePixelPerfect();
    bool isPixelPerfectEnabled() const;
    void fillSelection(sf::Color color, int currentFrame);
    bool getIsDirty() const;
    void clearIsDirty();

    void importImageToActiveLayer(const std::string& filepath, int currentFrame);

    void enterTransformMode(int currentFrame);
    void applyTransform(int currentFrame);
    void cancelTransform();
    bool isTransforming() const;

    SymmetryManager& getSymmetryManager() { return symmetryManager; }
    DitherManager& getDitherManager() { return ditherManager; }
    SelectionManager& getSelectionManager() { return selection; }
    void toggleDithering() { useDithering = !useDithering; }
};