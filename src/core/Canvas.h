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
#include "../core/PerspectiveSystem.h"

enum class ToolType { None, Brush, Pencil, Eraser, Fill, Select, Symmetry, Shapes, MagicWand, Perspective, Text, Gradient, Curve, FilledContour, Grid };
enum class BlendMode { Normal, Multiply, Additive, Screen, Overlay };
enum class TransformState { None, Scaling };
enum class PixelBrushShape { Square, Circle, Slash, Rectangle };

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

struct DeformPixel {
    int x;
    int y;
    sf::Color color;
};

struct VectorStroke {
    sf::VertexArray mesh;
    int layer{ 0 };
    int frame{ 0 };
    bool isErase = false;
};

struct CanvasImage {
    int id = 0;
    int frame = 0;
    int layer = 0;
    sf::FloatRect bounds;
    std::shared_ptr<sf::Texture> texture;
    sf::Image image;
};

class Canvas {
private:
    std::vector<CanvasImage> m_canvasImages;
    std::vector<CanvasImage> m_floatingImages;
    std::vector<int> m_selectedStrokes;
    std::vector<int> m_selectedImages;
    bool m_isMultiSelectionGroup{ false };
    sf::Vector2f m_dragStartMousePos{ 0.f, 0.f };
    sf::Vector2f m_lastDragPos{ 0.f, 0.f };
    sf::Vector2f m_lastClickPos{ 0.f, 0.f };
    int m_nextImageId{ 0 };

    struct ImageResizeSnapshot {
        int index;
        sf::FloatRect bounds;
        std::shared_ptr<sf::Texture> texture;
    };
    struct StrokeResizeSnapshot {
        int index;
        sf::VertexArray mesh;
    };
    std::vector<ImageResizeSnapshot> m_resizeImageSnapshots;
    std::vector<StrokeResizeSnapshot> m_resizeStrokeSnapshots;
    sf::FloatRect m_resizeStartBox{ 0.f, 0.f, 0.f, 0.f };

    struct UndoState {
        std::vector<Frame> frames;
        std::vector<VectorStroke> vectorStrokes;
        std::vector<CanvasImage> canvasImages;
        SelectionState selectionState{ SelectionState::Inactive };
        sf::FloatRect selectionBoundingBox{ 0.f, 0.f, 0.f, 0.f };
        std::vector<sf::FloatRect> selectionSubItemBoxes;
        std::vector<sf::Vector2f> selectionPathPoints;
        bool isLassoSelection{ false };
        bool showHandles{ false };
        std::vector<int> selectedStrokes;
        std::vector<int> selectedImages;
        bool isMultiSelectionGroup{ false };
        bool pendingTransform{ false };
        TransformState transformMode{ TransformState::None };
        bool symmetryEnabled{ false };
        bool symmetryVisible{ false };
        sf::Vector2f symmetryStartPoint{ 0.f, 0.f };
        sf::Vector2f symmetryEndPoint{ 0.f, 0.f };
        bool isMagicWandStyle{ false };
        std::vector<sf::Vector2i> magicWandPixels;
    };
    PerspectiveManager* m_perspectiveManager = nullptr;
    SymmetryManager symmetryManager;
    DitherManager ditherManager;
    bool useDithering = false;

    class TextManager* m_textManager = nullptr;

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

    sf::Vector2f shiftAnchor;
    bool hasShiftAnchor;

    bool isDeforming{ false };
    sf::Vector2f deformClickPos{ 0.f, 0.f };
    sf::Vector2f deformCurrentPos{ 0.f, 0.f };
    int deformMinX{ 0 };
    int deformMaxX{ 0 };
    int deformMinY{ 0 };
    int deformMaxY{ 0 };
    float deformT0{ 0.5f };
    bool deformIsHorizontal{ true };
    int deformMode{ 0 };
    std::vector<DeformPixel> deformPixels;
    std::vector<DeformPixel> currentDeformedPixels;
    int m_deformStrokeIndex{ -1 };
    sf::VertexArray m_originalDeformMesh;
    float m_vDeformMinX{ 0.f };
    float m_vDeformMaxX{ 0.f };
    float m_vDeformMinY{ 0.f };
    float m_vDeformMaxY{ 0.f };

    std::vector<sf::Vector2f> m_contourPoints;
    enum class SymmetryDragMode { None, NewAxis, StartHandle, EndHandle, MoveEntire };
    SymmetryDragMode m_symmetryDragMode{ SymmetryDragMode::None };
    sf::Vector2f m_symmetryDragOffsetStart{ 0.f, 0.f };
    sf::Vector2f m_symmetryDragOffsetEnd{ 0.f, 0.f };


    std::vector<UndoState> undoHistory;
    std::vector<UndoState> redoHistory;

    std::vector<VectorStroke> m_vectorStrokes;
    std::vector<VectorStroke> m_floatingVectorStrokes;
    sf::Vector2f m_floatingLocalSize{ 0.f, 0.f };

    sf::VertexArray m_activeVectorMesh;
    sf::Vector2f m_vPrevPoint;
    sf::Vector2f m_vPrevMidPoint;
    sf::Vector2f m_stabilizedPos;
    bool m_isVectorStrokeActive{ false };
    bool m_activeStrokeIsErase{ false };

    sf::RenderTexture m_layerCache;

    sf::Texture deskTexture;
    sf::Sprite deskSprite;
    sf::Texture canvasTexture;
    sf::Sprite canvasSprite;

    BrushManager brushEngine;
    SelectionManager selection;

    bool isPixelMode;
    int pixelBrushSize;
    PixelBrushShape m_pixelBrushShape{ PixelBrushShape::Square };
    struct PixelStrokePoint {
        int x;
        int y;
        sf::Color color;
    };
    bool m_recolorUndoSaved{ false };
    int m_currentFrame{ 0 };
    std::vector<PixelStrokePoint> m_pixelStrokePoints;
    sf::Clock m_selectClickClock;
    int m_lastClickedImageIdx{ -1 };
    int m_lastClickedEntityIdx{ -1 };
    std::vector<sf::Vector2i> m_pixelBrushMask{ {0, 0} };
    void rebuildPixelBrushMask();
    bool pixelGridEnabled{ false };
    bool customGridEnabled{ false };
    int customGridSize{ 16 };
    sf::Color customGridColor{ sf::Color(70, 130, 210, 180) };
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
    bool hasFrameAssets{ false };

    bool colorMatches(const sf::Color& a, const sf::Color& b) const;
    void executeGlobalFill(sf::Color targetColor, sf::Color replacementColor, sf::Image& image);
    void executeQueueFill(sf::Vector2i startPoint, sf::Color targetColor, sf::Color replacementColor, sf::Image& image);

    void drawPixelExact(int x, int y, sf::Color c, int frameIdx);
    std::vector<sf::Vector2i> getBresenhamPoints(int x0, int y0, int x1, int y1);
    void drawBresenhamLine(int x0, int y0, int x1, int y1, sf::Color c, int frameIdx);
    void drawContinuousLine(sf::Vector2f from, sf::Vector2f to, sf::Color col, int currentFrame);

    float computeDeformWeight(float t) const;
    void updateDeformPixels(sf::Vector2f delta);

    void fillPolygonContour(const std::vector<sf::Vector2f>& points, sf::Color color, int currentFrame);

    float computeHandleHitRadius() const;
    bool isImageResourceActive(int currentFrame) const;

    size_t maxUndoHistory{ 100 };

    void eraseVectorStrokesAt(sf::Vector2f p1, sf::Vector2f p2, float radius, int currentFrame);

    bool layerHasErase(int frameIndex, int layerIndex) const;
    void drawLayerContent(sf::RenderTarget& target, int frameIndex, int layerIndex,
        const sf::RenderStates& layerStates, bool isActiveLayerForPreview);
    void bakeLayerStrokes(int frameIndex, int layerIndex);

    void extractFloatingStrokes(int currentFrame);
    void flipFloatingStrokes(bool horizontal);

public:
    Canvas();
    void init();
    void initCustom(int width, int height);

    const std::vector<CanvasImage>& getCanvasImages() const { return m_canvasImages; }
    void setCanvasImages(const std::vector<CanvasImage>& images) {
        m_canvasImages = images;
        for (const auto& ci : m_canvasImages) {
            if (ci.id > m_nextImageId) {
                m_nextImageId = ci.id;
            }
        }
    }
    void clearCanvasImages() { m_canvasImages.clear(); }
    void clearObjectSelection() {
        m_selectedStrokes.clear();
        m_selectedImages.clear();
        m_isMultiSelectionGroup = false;
        m_lastClickedImageIdx = -1;
        m_lastClickedEntityIdx = -1;
        selection.clearSelection();
    }
    const std::vector<VectorStroke>& getVectorStrokes() const { return m_vectorStrokes; }
    void clearVectorStrokes() { m_vectorStrokes.clear(); }
    void setVectorStrokes(const std::vector<VectorStroke>& strokes) { m_vectorStrokes = strokes; }

    void cleanVectorLayers();
    void bakeAllStrokes();
    void updateTransform(float dt, sf::FloatRect space);
    sf::Transform getTransform() const;
    sf::Transform getInverseTransform() const;
    sf::FloatRect getDrawArea() const;
    sf::Vector2u getCanvasSize() const;
    void autoSelectObject(sf::Vector2f pos, int currentFrame);
    void setTextManager(class TextManager* tm) { m_textManager = tm; }
    class TextManager* getTextManager() { return m_textManager; }

    void zoom(float delta);
    void pan(sf::Vector2f delta);
    void resetView();


    void addFrame(int index);
    void addFrameAt(int targetIndex);
    void duplicateFrame(int index);
    void deleteFrame(int index);
    bool isFrameEmpty(int frameIndex) const;
    void clearAllFrames();
    size_t getFrameCount() const;
    Frame* getFrame(int index);
    const Frame* getFrameReadOnly(int index) const;
    sf::RenderTexture* getActiveRenderTexture(int currentFrame);
    void addVectorMesh(const sf::VertexArray& mesh, int frame, int layer);
    sf::Image flattenFrameToImage(int frameIndex, unsigned int scaleFactor = 1);

    void setPerspectiveManager(PerspectiveManager* pm) { m_perspectiveManager = pm; }

    void addLayer(int frameIndex, const std::string& name = "Layer");
    void deleteLayer(int frameIndex, int layerIndex);
    void duplicateLayer(int frameIndex, int layerIndex);
    void setLayerProperties(int frameIndex, int layerIndex, const std::string& name, bool visible, bool locked, float opacity, BlendMode mode, bool recordUndo = true);
    void toggleLayerPersistence(int frameIndex, int layerIndex);
    void cycleLayerColorTag(int frameIndex, int layerIndex);
    void pushLayerToNextFrame(int currentFrame, int layerIndex);
    void extendLayerToNextFrame(int currentFrame, int layerIndex);
    void mergeDown(int frameIndex);
    void mergeVisible(int frameIndex);
    void moveLayer(int frameIndex, int fromIndex, int toIndex);

    void setActiveLayer(int index, int currentFrame = -1);
    int getActiveLayer() const;

    void setOnionSkin(bool enabled, float prevOpac, float nextOpac);
    void setOnionSkinCounts(int prevCount, int nextCount);
    bool isOnionSkinEnabled() const;
    float getOnionSkinPrevOpacity() const;
    float getOnionSkinNextOpacity() const;
    int getOnionSkinPrevCount() const;
    int getOnionSkinNextCount() const;

    void commitSelection(int currentFrame);
    void pasteSelection(int currentFrame);
    void deleteSelection(int currentFrame);
    void flipSelectionHorizontal(int currentFrame);
    void flipSelectionVertical(int currentFrame);
    void duplicateSelection(int currentFrame);
    void mergeSelectedObjects(int currentFrame);
    void cropSelection(int currentFrame);
    void moveSelectionZOrder(int delta, int currentFrame);
    void bringSelectionToFront(int currentFrame);
    void sendSelectionToBack(int currentFrame);
    int getSelectionZOrder(int currentFrame) const;
    int getMaxZOrder(int currentFrame) const;
    void setSelectionZOrder(int newZ, int currentFrame);
    void recolorActiveSelection(sf::Color newColor);

    void setActiveTool(ToolType tool, int currentFrame = 0);
    ToolType getActiveTool() const;
    BrushManager& getBrushEngine();
    void setBrushSize(float size);
    float getBrushSize() const;

    void setPrimaryColor(sf::Color color);
    void setSecondaryColor(sf::Color color);
    sf::Color getPrimaryColor() const;
    sf::Color getSecondaryColor() const;
    void setFillSettings(float tolerance, bool contiguous);

    bool renderLayerToTexture(int frameIndex, int layerIndex, sf::RenderTexture& out);

    void saveUndoState();
    void undo();
    void redo();

    void copySelection(int currentFrame);
    void resizeCanvas(unsigned int newWidth, unsigned int newHeight);
    bool hasGlobalClipboard() const;
    bool pasteGlobalClipboard(int currentFrame);
    void pasteVectorStrokes(const std::vector<VectorStroke>& strokes, sf::Vector2f offset, int currentFrame);
    sf::RenderStates getSFMLBlendMode(BlendMode mode) const;

    void handleMousePressed(sf::Vector2f logicalPos, bool rightClick, int currentFrame);
    void handleMouseReleased(sf::Vector2f logicalPos, int currentFrame);
    void handleMouseMoved(sf::Vector2f logicalPos, sf::Vector2f rawPos, int currentFrame);

    void makeOutline(int currentFrame, sf::Color outlineColor);

    void draw(sf::RenderWindow& window, int currentFrame, bool isPlaying, const sf::RenderStates& states);
    void drawShadows(sf::RenderWindow& window, sf::Vector2f logicalSunPos, const std::vector<sf::FloatRect>& items, const std::vector<std::string>& categories, const sf::RenderStates& states);

    void setPixelMode(bool enabled);
    bool getPixelMode() const;
    void setPixelBrushSize(int size);
    int getPixelBrushSize() const;
    void cyclePixelBrushSize();
    void setPixelBrushShape(PixelBrushShape shape);
    PixelBrushShape getPixelBrushShape() const { return m_pixelBrushShape; }
    const std::vector<sf::Vector2i>& getPixelBrushMask() const { return m_pixelBrushMask; }
    void togglePixelGrid() { toggleCustomGrid(); }
    bool isPixelGridEnabled() const { return customGridEnabled; }
    void setCustomGridEnabled(bool enabled) { customGridEnabled = enabled; }
    bool isCustomGridEnabled() const { return customGridEnabled; }
    void toggleCustomGrid() { customGridEnabled = !customGridEnabled; }
    void setCustomGridSize(int size) { customGridSize = std::max(1, size); }
    int getCustomGridSize() const { return customGridSize; }
    void setCustomGridColor(sf::Color col) { customGridColor = col; }
    sf::Color getCustomGridColor() const { return customGridColor; }
    void togglePixelSnap();
    bool isPixelSnapEnabled() const;
    void toggleTileMode();
    void togglePixelPerfect();
    bool isPixelPerfectEnabled() const;
    void fillSelection(sf::Color color, int currentFrame);
    bool getIsDirty() const;
    void clearIsDirty();
    sf::Image& getCanvasImageCPU(CanvasImage& ci);
    void resetRecolorLatch() { m_recolorUndoSaved = false; }
    void clearHistory() {
        undoHistory.clear();
        redoHistory.clear();
        m_pixelStrokePoints.clear();
        m_recolorUndoSaved = false;
        m_lastClickedImageIdx = -1;
        m_lastClickedEntityIdx = -1;
    }

    void drawLayerThumbnail(sf::RenderTarget& target, int frameIndex, int layerIndex, sf::FloatRect bounds);
    void drawFrameThumbnail(sf::RenderTarget& target, int frameIndex, sf::FloatRect bounds);

    void importImageToActiveLayer(const std::string& filepath, int currentFrame);
    void replaceFrameImage(int frameIndex, const sf::Image& img);

    void enterTransformMode(int currentFrame);
    void applyTransform(int currentFrame);
    void cancelTransform();
    bool isTransforming() const;

    SymmetryManager& getSymmetryManager() { return symmetryManager; }
    void clearSymmetry() {
        saveUndoState();
        symmetryManager.enabled = false;
        symmetryManager.visible = false;
        symmetryManager.direction = { 0.f, 0.f };
        symmetryManager.startPoint = { 0.f, 0.f };
        symmetryManager.endPoint = { 0.f, 0.f };
        m_symmetryDragMode = SymmetryDragMode::None;
        isDirty = true;
    }
    DitherManager& getDitherManager() { return ditherManager; }
    SelectionManager& getSelectionManager() { return selection; }
    void toggleDithering() { useDithering = !useDithering; }

    SelectionManager& getSelection() { return selection; }
    const SelectionManager& getSelection() const { return selection; }

    void setMaxUndoHistory(size_t limit) { maxUndoHistory = limit; }
    size_t getMaxUndoHistory() const { return maxUndoHistory; }

    void pasteImage(const sf::Image& img, int currentFrame, bool originalResolution = false);
    const sf::Image& getGlobalClipboardImage() const;
    bool isClipboardVector() const;

    void setStabilizer(float val) { brushEngine.setStabilization(val); }
    float getStabilizer() const { return brushEngine.getActivePreset().stabilization; }
};