#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include "../core/AppState.h"
#include "../core/SettingsManager.h"
#include "../core/ProjectManager.h"
#include "../core/KeybindManager.h"
#include "../core/Canvas.h"
#include "../core/Timeline.h"
#include "../core/ExportManager.h"
#include "../ai/AIHelper.h"
#include "Screens/ProjectBrowser.h"
#include "Screens/AISettingsModal.h"
#include "Screens/ExportModal.h"
#include "Screens/NewProjectModal.h"
#include "KeybindSettingsPanel.h"
#include "LeftToolbar.h"
#include "BottomTimeline.h"
#include "LayerPanel.h"
#include "ColorPalettePanel.h"
#include "RightProperties.h"

class UIManager {
private:
    bool isTypingPrompt;
    bool showingText;
    float textAlpha;
    bool isLightingMode;
    int promptQuantity;
    bool focusMode;

    ProjectManager* projManager;
    std::string activeProjectName;
    std::string activeProjectPath;

    KeybindManager keybindManager;

    sf::Texture bgTexture;
    sf::Sprite bgSprite;
    sf::Font font;

    ProjectBrowser projectBrowser;
    AISettingsModal settingsModal;
    KeybindSettingsPanel keybindPanel;
    ExportModal exportModal;
    NewProjectModal newProjectModal;

    LeftToolbar leftToolbar;
    BottomTimeline bottomTimeline;

    LayerPanel layerPanel;
    ColorPalettePanel colorPalettePanel;
    RightProperties rightProperties;

    sf::Text uiText;
    sf::Clock textClock;

    sf::RectangleShape promptBox;
    sf::Text promptDisplay;
    std::string currentPrompt;

    bool isPanning;
    sf::Vector2f lastPanMousePos;

    sf::RectangleShape toolBg;
    sf::RectangleShape sizeSliderBg;
    sf::RectangleShape sizeSliderHandle;
    sf::Text sizeLabelText;
    sf::Text sizeValueText;
    bool isDraggingSizeSlider;

    sf::RectangleShape pixelPerfBtn;
    sf::Text pixelPerfText;

    bool showUnsavedWarning;
    sf::RectangleShape topBackBtn;
    sf::Text topBackText;
    sf::RectangleShape topSaveBtn;
    sf::Text topSaveText;

    sf::RectangleShape warnOverlay;
    sf::RectangleShape warnBox;
    sf::Text warnTitle;
    sf::RectangleShape warnSaveBtn;
    sf::Text warnSaveText;
    sf::RectangleShape warnDiscardBtn;
    sf::Text warnDiscardText;
    sf::RectangleShape warnCancelBtn;
    sf::Text warnCancelText;

    bool triggerSave(Canvas& canvas, Timeline& timeline);

public:
    UIManager();
    void init(ProjectManager* pm, Canvas* baseCanvas);
    void showMessage(const std::string& msg, sf::Color color);
    void handleEvent(const sf::Event& event, sf::RenderWindow& window, AppState& currentState, AppSettings& settings, Canvas& canvas, Timeline& timeline, AIHelper& aiHelper, ProjectManager& pm);
    void update(sf::RenderWindow& window, AppState currentState, AppSettings& settings, float dt, Canvas& canvas);
    void draw(sf::RenderWindow& window, AppState currentState, Canvas& canvas, AIHelper& aiHelper, Timeline& timeline);
};