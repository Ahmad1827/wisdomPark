#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include "../core/Canvas.h"
#include "../core/ColorManager.h"

enum class PalettePanelState { Hidden, Visible, Pinned };
enum class PaletteResizeDir { None, Left, Right, Top, Bottom, TopLeft, TopRight, BottomLeft, BottomRight };

class ColorPalettePanel {
private:
    sf::RectangleShape background;
    sf::RectangleShape headerBg;
    sf::Text headerText;
    sf::RectangleShape closeBtn;
    sf::Text closeText;

    sf::RectangleShape pinBtn;
    sf::Text pinLabel;

    sf::RectangleShape detachBtn;
    sf::Text detachLabel;

    sf::RectangleShape primaryBox;
    sf::RectangleShape secondaryBox;

    sf::RectangleShape eyedropperBtn;
    sf::Text eyedropperLabel;
    bool isEyedropperActive;

    sf::Image svImage;
    sf::Texture svTexture;
    sf::Sprite svSprite;
    sf::RectangleShape svSelector;

    sf::Image hueImage;
    sf::Texture hueTexture;
    sf::Sprite hueSprite;
    sf::RectangleShape hueSelector;

    sf::Image alphaImage;
    sf::Texture alphaTexture;
    sf::Sprite alphaSprite;
    sf::RectangleShape alphaSelector;

    sf::Font font;

    float width;
    float currentX;
    float targetX;
    PalettePanelState state;
    bool hovered;

    bool isDetached;
    sf::Vector2f detachedPos;
    sf::Vector2f detachedSize;

    bool isDraggingWindow;
    sf::Vector2f windowDragOffset;

    bool isResizing;
    PaletteResizeDir activeResizeDir;
    sf::Vector2f resizeStartMouse;
    sf::FloatRect resizeStartBounds;

    float pickerSize;
    float pickerX;
    float pickerY;

    ColorManager colorManager;

    float currentHue;
    float currentSat;
    float currentVal;
    float currentAlpha;

    bool isDraggingSV;
    bool isDraggingHue;
    bool isDraggingAlpha;

    int activeInputIndex;
    std::string inputBuffer;

    struct AdvicePalette {
        std::vector<sf::Color> colors;
        int useCount = 0;
        bool isPinned = false;
        uint32_t order = 0;
    };

    std::vector<AdvicePalette> m_advicePalettes;
    std::vector<std::pair<sf::FloatRect, std::pair<sf::Color, size_t>>> m_adviceSwatchBounds;
    std::vector<std::pair<sf::FloatRect, size_t>> m_advicePinBtnBounds;
    std::vector<std::pair<sf::FloatRect, size_t>> m_adviceDeleteBtnBounds;
    sf::FloatRect m_adviceClearAllBounds;

    std::vector<std::pair<sf::FloatRect, size_t>> m_customSwatchBounds;
    std::vector<size_t> m_selectedSwatchIndices;
    bool m_isBoxSelectingSwatches = false;
    sf::Vector2f m_swatchSelectStart;
    sf::Vector2f m_swatchSelectEnd;
    bool m_showSwatchContextMenu = false;
    sf::Vector2f m_swatchContextMenuPos;
    sf::FloatRect m_swatchDeleteBtnBounds;
    sf::FloatRect m_swatchAreaBounds;

    void saveAdvicePalettes() const;
    void loadAdvicePalettes();

    void updatePickerImages();
    void updateFromRGB(sf::Color c);
    std::string colorToHex(sf::Color c) const;
    PaletteResizeDir getResizeDirection(sf::Vector2f mousePos) const;

public:
    ColorPalettePanel();
    void init();
    void update(float dt, bool focusMode, Canvas& canvas, bool isOpen = true);
    void updateHover(sf::Vector2f mousePos, bool canOpen);
    void draw(sf::RenderWindow& window);

    std::string processClick(sf::Vector2f mousePos, Canvas& canvas);
    bool handleClick(sf::Vector2f mousePos, Canvas& canvas);
    bool handlePaletteClick(sf::Vector2f mousePos, sf::Color& outPrimary, sf::Color& outSecondary);
    bool handleEvent(const sf::Event& event, sf::Vector2f mousePos, Canvas& canvas);

    void setColors(sf::Color primary, sf::Color secondary);
    bool addAdvicePalette(const std::vector<sf::Color>& ramp);
    void removeAdvicePalette(size_t index);
    void clearAdvicePalettes();

    float getCurrentX() const;
    void forceClose();
    bool isHovered() const;
    bool isPanelPinned() const;
    sf::FloatRect getHandleBounds() const;
    ColorManager& getColorManager();

    bool getIsEyedropperActive() const;
    void setEyedropperActive(bool active);

    bool getIsDetached() const;
    void setIsDetached(bool detached);
};