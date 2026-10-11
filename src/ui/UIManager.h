#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <chrono>
#include <SFML/Network.hpp>
#include "../core/DragDropHandler.h"
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
#include "LayerPanel.h"
#include "ColorPalettePanel.h"
#include "RightProperties.h"
#include "AudioPanel.h"
#include "../core/ITool.h"
#include "../ui/GradientPanel.h"
#include "../tools/GradientTool.h"
#include "../core/GradientSystem.h"
#include "../core/AssetManager.h"
#include "AssetBrowserPanel.h"
#include "ArcadeGame.h"

#include "../UI/WorkspaceLayout.h"
#include "../UI/Panels/TopBar.h"
#include "../UI/Panels/ToolOptionsBar.h"
#include "../UI/Panels/ToolDock.h"
#include "../UI/Panels/RightDockTabs.h"
#include "../UI/Panels/TimelineHeader.h"
#include "../UI/Panels/StatusBar.h"
#include "../core/GitImgClient.h"
#include "../core/HandTracker.h"

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

class UIManager {
private:
    bool m_showKeybinds = false;
    std::string m_listeningKeyActionId = "";
    std::string m_keybindSearchQuery = "";
    bool m_isTypingKeybindSearch = false;
    std::string m_selectedKeybindCategory = "All";
    float m_keybindScrollOffset = 0.0f;
    float m_keybindMaxScroll = 0.0f;

    void drawKeybindModal(sf::RenderWindow& window);
    void handleKeybindModalEvent(const sf::Event& event, sf::RenderWindow& window);
    bool m_useMinigameWelcome = false;
    sf::FloatRect m_welcomeModeToggleBounds = sf::FloatRect(1600.f, 35.f, 240.f, 42.f);
    void drawStandardMainMenu(sf::RenderWindow& window);
    void drawMainMenuBackdrop(sf::RenderWindow& window);
    sf::FloatRect getMainMenuItemBounds(int index) const;
    sf::FloatRect getRecentCardBounds(int index) const;
    void refreshRecentProjects();
    void loadProjectFromMenu(const ProjectMetadata& meta, AppState& currentState, Canvas& canvas, Timeline& timeline, ProjectManager& pm);
    std::vector<ProjectMetadata> m_recentProjects;
    bool m_wasOnMainMenu = false;

    void drawFireflies(sf::RenderWindow& window);
    void drawSubScreenHeader(sf::RenderWindow& window, const std::string& title, const std::string& subtitle, float time);
    float m_keybindTime = 0.0f;
    bool m_keybindsWereOpen = false;
    MenuState m_lastMenuState = MenuState::Main;
    int m_lastTutorialIndex = -1;
    float m_screenTime = 0.0f;
    float m_tutorialTime = 0.0f;
    ArcadeGame m_arcade;

    bool m_showEscapeMenu = false;
    sf::FloatRect m_startMenuFullscreenBtnBounds = sf::FloatRect(1440.f, 35.f, 150.f, 42.f);

    void toggleFullscreen(sf::RenderWindow& window, AppSettings& settings);
    void drawEscapeMenu(sf::RenderWindow& window, Canvas& canvas, Timeline& timeline);
    bool handleEscapeMenuEvent(const sf::Event& event, sf::RenderWindow& window, AppState& currentState, AppSettings& settings, Canvas& canvas, Timeline& timeline);
    bool m_fullscreenToggleRequested = false;

    bool m_showResizeModal = false;
    std::string m_resizeWBuf = "";
    std::string m_resizeHBuf = "";
    int m_activeResizeField = 0;

    void drawResizeModal(sf::RenderWindow& window, Canvas& canvas);
    bool handleResizeModalEvent(const sf::Event& event, sf::RenderWindow& window, Canvas& canvas, Timeline& timeline);

    bool m_showPasteResolutionModal = false;
    sf::Image m_pendingPasteImage;

    void drawPasteResolutionModal(sf::RenderWindow& window, Canvas& canvas);

    // What to carry on with once the unsaved-changes warning is answered.
    enum class UnsavedThen { MainMenu, NewProject };
    UnsavedThen m_unsavedThen = UnsavedThen::MainMenu;
    void requestNewProject(Canvas& canvas);
    void drawUnsavedWarning(sf::RenderWindow& window);
    void handleUnsavedWarningEvent(const sf::Event& event, sf::RenderWindow& window, AppState& currentState, Canvas& canvas, Timeline& timeline);
    bool handlePasteResolutionModalEvent(const sf::Event& event, sf::RenderWindow& window, Canvas& canvas, Timeline& timeline);

    void triggerArcadeStation(const std::string& id, sf::RenderWindow& window);
    AssetManager assetManager;
    std::unique_ptr<AssetBrowserPanel> assetBrowser;
    GradientConfig m_gradientConfig;
    GradientPanel m_gradientPanel;
    TextManager m_textManager;
    TextPanel m_textPanel;
    std::unique_ptr<ITool> m_activeTool;
    bool m_debugUseSpriteStudio;
    int lastLeftState = 0;
    int lastRightState = 0;
    int lastZoomState = 0;
    float zoomOriginY = 0.0f;
    sf::UdpSocket handTrackerSocket;
    GitImgClient m_gitImgClient;
    bool m_isPushingGitImg{ false };
    sf::Texture m_camTexture;
    sf::Sprite m_camSprite;
    bool m_camMinimized{ false };
    sf::FloatRect m_camWidgetBounds;
    sf::FloatRect m_camToggleBtnBounds;
    sf::FloatRect m_camHeaderBounds;
    sf::FloatRect m_camResizeBounds;
    sf::Vector2f m_camPos{ -1.0f, -1.0f };
    sf::Vector2f m_camSize{ 180.0f, 135.0f };
    bool m_isDraggingCam{ false };
    bool m_isResizingCam{ false };
    sf::Vector2f m_camDragOffset{ 0.0f, 0.0f };
    sf::Vector2f m_resizeStartPos{ 0.0f, 0.0f };
    sf::Vector2f m_resizeStartSize{ 0.0f, 0.0f };

    void drawHandCamWidget(sf::RenderWindow& window);
    bool handleHandCamWidgetEvents(const sf::Event& event);
    void pushToGitImg(Canvas& canvas, int frameIndex, bool opaqueBg = false);
    void pullCommitToCanvas(const std::string& commitHash, Canvas& canvas, Timeline& timeline);
    std::mutex m_pullMutex;
    bool m_hasPendingPulledImage = false;
    sf::Image m_pendingPulledImage;
    void pushSpriteSheetToGitImg(Canvas& canvas, bool opaqueBg);
    bool isTypingPrompt;
    bool showingText;
    float textAlpha;
    bool isLightingMode;
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

    AudioPanel audioPanel;

    LayerPanel layerPanel;
    ColorPalettePanel colorPalettePanel;
    RightProperties rightProperties;

    sf::Text uiText;
    sf::Clock textClock;

    sf::RectangleShape promptBox;
    sf::Text promptDisplay;
    std::string currentPrompt;

    bool showUnsavedWarning;

    MenuState currentMenuState;
    std::map<std::string, float> hoverMap;
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

    void updateStartMenu(float dt, sf::Vector2f mousePos);

    void updateHoverValue(const std::string& key, bool isHovering, float dt, float speed = 10.0f);
    float getHover(const std::string& key);

    void drawMainMenu(sf::RenderWindow& window);
    void drawSettingsMenu(sf::RenderWindow& window);
    void drawTutorialsMenu(sf::RenderWindow& window);
    void drawCreditsMenu(sf::RenderWindow& window);
    void drawBackButton(sf::RenderWindow& window, const std::string& hoverKey, float x, float y);

    bool triggerSave(Canvas& canvas, Timeline& timeline);
    void startAIAutocomplete(Canvas& canvas, Timeline& timeline, const std::string& hint, bool nextFrame = false);
    void openAIPrompt(bool contour, bool nextFrame = false);

    std::string m_pendingCheckoutHash;

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