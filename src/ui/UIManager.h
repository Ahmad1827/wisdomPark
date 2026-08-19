#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <map>
#include <vector>
#include "../core/TextSystem.h"
#include "../ui/TextPanel.h"
#include "../core/PerspectiveSystem.h"
#include "../ui/PerspectivePanel.h"
#include "../core/AppState.h"
#include "../core/SettingsManager.h"
#include "../core/ProjectManager.h"
#include "../core/KeybindManager.h"
#include "../core/Canvas.h"
#include "../core/Timeline.h"
#include "../core/ExportManager.h"
#include "../ai/AIHelper.h"
#include "Screens/ProjectBrowser.h"
#include "Screens/ExportModal.h"
#include "Screens/NewProjectModal.h"
#include "KeybindSettingsPanel.h"
#include "LeftToolbar.h"
#include "BottomTimeline.h"
#include "LayerPanel.h"
#include "ColorPalettePanel.h"
#include "RightProperties.h"
#include "AudioPanel.h"
#include <SFML/Network.hpp>
#include <chrono>
#include "../core/ITool.h"
#include <memory>
#include "../ui/TopMenuBar.h"
#include "../ui/GradientPanel.h"
#include "../tools/GradientTool.h"
#include "../core/GradientSystem.h"
#include "../core/AssetManager.h"
#include "AssetBrowserPanel.h"
#include "../UI/WorkspaceLayout.h"
#include "../UI/Panels/ToolDock.h"

enum class MenuState {
    Main,
    Projects,
    Settings,
    Tutorials,
    Credits
};

struct StartParticle {
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float size = 0.0f;
    float life = 0.0f;
    float maxLife = 0.0f;
    sf::Color baseColor = sf::Color::White;
};

struct LightRay {
    sf::ConvexShape shape;
    float speed = 0.0f;
    float offset = 0.0f;
};

class UIManager {
private:
    WisdomUI::WorkspaceLayout m_workspaceLayout;
    WisdomUI::ToolDock m_toolDock;
    AssetManager assetManager;
    std::unique_ptr<AssetBrowserPanel> assetBrowser;
    GradientConfig m_gradientConfig;
    GradientPanel m_gradientPanel;
    TopMenuBar m_topMenuBar;
    TextManager m_textManager;
    TextPanel m_textPanel;
    std::unique_ptr<ITool> m_activeTool;
    bool m_debugUseSpriteStudio;
    int lastLeftState = 0;
    int lastRightState = 0;
    int lastZoomState = 0;
    std::chrono::steady_clock::time_point lastLeftDown;
    std::chrono::steady_clock::time_point lastRightDown;
    int doubleClickThreshold = 250;
    bool isScrolling = false;
    float dragOriginY = 0.0f;
    float zoomOriginY = 0.0f;
    sf::UdpSocket handTrackerSocket;
    bool isTypingPrompt;
    bool showingText;
    float textAlpha;
    bool isLightingMode;
    int promptQuantity;
    bool focusMode;

    PerspectiveManager m_perspectiveManager;
    PerspectivePanel m_perspectivePanel;

    ProjectManager* projManager;
    std::string activeProjectName;
    std::string activeProjectPath;

    KeybindManager keybindManager;

    sf::Texture bgTexture;
    sf::Sprite bgSprite;
    sf::Font font;

    ProjectBrowser projectBrowser;
    KeybindSettingsPanel keybindPanel;
    ExportModal exportModal;
    NewProjectModal newProjectModal;

    LeftToolbar leftToolbar;
    BottomTimeline bottomTimeline;
    AudioPanel audioPanel;

    LayerPanel layerPanel;
    ColorPalettePanel colorPalettePanel;
    RightProperties rightProperties;

    sf::Text uiText;
    sf::Clock textClock;

    sf::RectangleShape promptBox;
    sf::Text promptDisplay;
    std::string currentPrompt;

  /*  bool isPanning;
    sf::Vector2f lastPanMousePos;*/

    sf::RectangleShape toolBg;
    sf::RectangleShape sizeSliderBg;
    sf::RectangleShape sizeSliderHandle;
    sf::Text sizeLabelText;
    sf::Text sizeValueText;
    bool isDraggingSizeSlider;

    sf::RectangleShape pixelPerfBtn;
    sf::Text pixelPerfText;

    bool showUnsavedWarning;

    sf::RectangleShape warnOverlay;
    sf::RectangleShape warnBox;
    sf::Text warnTitle;
    sf::RectangleShape warnSaveBtn;
    sf::Text warnSaveText;
    sf::RectangleShape warnDiscardBtn;
    sf::Text warnDiscardText;
    sf::RectangleShape warnCancelBtn;
    sf::Text warnCancelText;

    MenuState currentMenuState;
    std::map<std::string, float> hoverMap;
    std::vector<StartParticle> particles;
    std::vector<LightRay> lightRays;
    float startupTime;

    int activeTutorialIndex;
    bool uiFullscreen;
    bool uiBorderless;
    bool uiVsync;
    bool uiAutoBackup;
    bool uiHwAccel;
    int uiFpsLimit;
    int uiAnimFps;
    int uiHistorySize;
    int easterEggClicks;

    void initStartMenu();
    void updateStartMenu(float dt, sf::Vector2f mousePos);
    void drawStartMenu(sf::RenderWindow& window);

    void updateHoverValue(const std::string& key, bool isHovering, float dt, float speed = 10.0f);
    float getHover(const std::string& key);

    void drawMainMenu(sf::RenderWindow& window);
    void drawSettingsMenu(sf::RenderWindow& window);
    void drawTutorialsMenu(sf::RenderWindow& window);
    void drawCreditsMenu(sf::RenderWindow& window);
    void drawBackButton(sf::RenderWindow& window, const std::string& hoverKey, float x, float y);

    void drawPremiumText(sf::RenderWindow& window, const std::string& str, float x, float y, int size, sf::Color mainCol, sf::Color outlineCol, sf::Color shadowCol);
    void drawGlassPanel(sf::RenderWindow& window, sf::FloatRect bounds, float hoverScale = 1.0f);

    bool triggerSave(Canvas& canvas, Timeline& timeline);
public:
    UIManager();
    void init(ProjectManager* pm, Canvas* baseCanvas);
    void showMessage(const std::string& msg, sf::Color color);
    void handleEvent(const sf::Event& event, sf::RenderWindow& window, AppState& currentState, AppSettings& settings, Canvas& canvas, Timeline& timeline, AIHelper& aiHelper, ProjectManager& pm);
    void update(sf::RenderWindow& window, AppState currentState, AppSettings& settings, float dt, Canvas& canvas, Timeline& timeline);
    void draw(sf::RenderWindow& window, AppState currentState, Canvas& canvas, AIHelper& aiHelper, Timeline& timeline);
    bool isFullscreen() const { return uiFullscreen; }
    int getFpsLimit() const { return uiFpsLimit; }
};