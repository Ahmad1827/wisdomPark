#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <chrono>
#include <SFML/Network.hpp>

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
#include "../core/ITool.h"
#include "../ui/TopMenuBar.h"
#include "../ui/GradientPanel.h"
#include "../tools/GradientTool.h"
#include "../core/GradientSystem.h"
#include "../core/AssetManager.h"
#include "AssetBrowserPanel.h"

#include "../UI/WorkspaceLayout.h"
#include "../UI/Panels/TopBar.h"
#include "../UI/Panels/ToolOptionsBar.h"
#include "../UI/Panels/ToolDock.h"
#include "../UI/Panels/RightDockTabs.h"
#include "../UI/Panels/TimelineHeader.h"
#include "../UI/Panels/StatusBar.h"


enum class GhostPersonality { Shadow, Speedy, Bashful, Pokey };
enum class CollectibleType { Dot, PowerPellet, Cherry, Orange, Grape, Key };

struct ArcadeGhost {
    sf::Vector2f pos = { 0.f, 0.f };
    sf::Vector2f vel = { 0.f, 0.f };
    sf::Color baseColor = sf::Color::White;
    GhostPersonality personality = GhostPersonality::Shadow;
    int dir = 1;
    float speed = 210.f;
    float animTimer = 0.f;
    bool isScared = false;
    float scaredTimer = 0.f;
    bool isEaten = false;
    sf::Vector2f spawnPos = { 0.f, 0.f };
};

struct ArcadeCollectible {
    sf::Vector2f pos = { 0.f, 0.f };
    CollectibleType type = CollectibleType::Dot;
    int points = 10;
    bool collected = false;
    float respawnTimer = 0.f;
    float animPhase = 0.f;
};

struct ArcadeStationPortal {
    std::string id = "";
    std::string title = "";
    std::string subtitle = "";
    std::string keyShortcut = "";
    sf::FloatRect bounds = { 0.f, 0.f, 0.f, 0.f };
    sf::Color marqueeColor = sf::Color::White;
    float pulse = 0.f;
    float hoverAlpha = 0.f;
    float triggerFlash = 0.f;
    bool isArmed = true;
};

struct ArcadePacHero {
    sf::Vector2f pos = { 960.f, 680.f };
    sf::Vector2f vel = { 0.f, 0.f };
    int dir = 0;
    int nextDir = 0;
    float speed = 280.f;
    float mouthAnim = 0.f;
    float deathAnim = 0.f;
    bool isDying = false;
    int lives = 3;
    float invulnTimer = 0.f;
};

struct ArcadeParticleFX {
    sf::Vector2f pos = { 0.f, 0.f };
    sf::Vector2f vel = { 0.f, 0.f };
    float life = 0.f;
    float maxLife = 1.0f;
    float size = 3.0f;
    sf::Color color = sf::Color::White;
};

struct ArcadeScoreFloater {
    std::string text = "";
    sf::Vector2f pos = { 0.f, 0.f };
    sf::Vector2f vel = { 0.f, -70.f };
    float life = 0.85f;
    float maxLife = 0.85f;
    sf::Color color = sf::Color::White;
};


enum class MenuState {
    Main,
    Projects,
    Settings,
    Tutorials,
    Credits
};

enum class RightTabMode {
    None,
    Layers,
    Palette,
    Properties,
    Assets,
    Audio
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
    bool m_isArcadePaused = false;
    ArcadePacHero m_arcadeHero;
    std::vector<ArcadeGhost> m_arcadeGhosts;
    std::vector<ArcadeCollectible> m_arcadeCollectibles;
    std::vector<ArcadeStationPortal> m_arcadePortals;
    std::vector<sf::FloatRect> m_arcadeMazeWalls;
    std::vector<ArcadeParticleFX> m_arcadeFX;
    std::vector<ArcadeScoreFloater> m_arcadeFloaters;

    float m_arcadeGlobalTime = 0.0f;
    float m_arcadeMasterTimer = 0.0f;
    int m_arcadeScore = 0;
    int m_arcadeHighScore = 25000;
    std::string m_arcadePendingAction = "";
    float m_arcadeActionDelay = 0.0f;

    void initMinigame();
    void updateMinigame(float dt, sf::Vector2f mousePos, sf::RenderWindow& window);
    void triggerArcadeStation(const std::string& id, sf::RenderWindow& window);
    void spawnParticleBurst(sf::Vector2f pos, sf::Color col, int count, float spd);
    void addFloatingText(const std::string& str, sf::Vector2f pos, sf::Color col);

    void drawPixelHero(sf::RenderWindow& window);
    void drawPixelGhost(sf::RenderWindow& window, const ArcadeGhost& g);
    void drawPixelItem(sf::RenderWindow& window, sf::Vector2f pos, CollectibleType type, float anim);
    void drawStationPortal(sf::RenderWindow& window, const ArcadeStationPortal& p, sf::Vector2f mousePos);
    void drawArcadeBezelOverlay(sf::RenderWindow& window);
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

    bool m_showTimeline{ false };
    RightTabMode m_activeRightTab{ RightTabMode::None };

    WisdomUI::WorkspaceLayout m_workspaceLayout;
    WisdomUI::TopBar m_topBar;
    WisdomUI::ToolOptionsBar m_toolOptionsBar;
    WisdomUI::ToolDock m_toolDock;
    WisdomUI::RightDockTabs m_rightDockTabs;
    WisdomUI::TimelineHeader m_timelineHeader;
    WisdomUI::StatusBar m_statusBar;

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