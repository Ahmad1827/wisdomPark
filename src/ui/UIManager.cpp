#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "UIManager.h"
#include "../core/NativeDialogs.h"
#include "../core/ExportManager.h"
#include "../ai/AIManager.h"
#include "../ui/AIPanel.h"
#include "../ui/AIReviewModal.h"
#include <iostream>
#include <algorithm>
#include <ctime>
#include <filesystem>
#include <random>
#include <fstream>
#include <sstream>
#include <windows.h>
#include "../tools/CanvasTool.h"
#include "../tools/SpriteSheetStudioTool.h"
#include "../tools/ShapeTool.h"
#include "../tools/MagicWandTool.h"
#include "../tools/PerspectiveTool.h"
#include "../tools/TextTool.h"
#include "../UI/UITheme.h"
#include "../UI/WorkspaceLayout.h"
#include "../UI/Panels/TopBar.h"
#include "../UI/Panels/ToolOptionsBar.h"
#include "../UI/Panels/ToolDock.h"
#include "../UI/Panels/StatusBar.h"

static int g_resW = 1920;
static int g_resH = 1080;
static bool g_resDropdownOpen = false;

#if defined(_WIN32)
#include <windows.h>
#include <shellapi.h>

static WNDPROC g_originalWndProc = nullptr;
static std::vector<std::pair<std::string, sf::Vector2i>> g_droppedFiles;

static LRESULT CALLBACK DropHookProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_DROPFILES) {
        HDROP hDrop = reinterpret_cast<HDROP>(wParam);
        POINT pt;
        DragQueryPoint(hDrop, &pt);

        UINT count = DragQueryFileA(hDrop, 0xFFFFFFFF, nullptr, 0);
        char filePath[MAX_PATH];
        for (UINT i = 0; i < count; ++i) {
            if (DragQueryFileA(hDrop, i, filePath, MAX_PATH)) {
                g_droppedFiles.push_back({ std::string(filePath), sf::Vector2i(pt.x, pt.y) });
            }
        }
        DragFinish(hDrop);
        return 0;
    }
    return CallWindowProc(g_originalWndProc, hwnd, msg, wParam, lParam);
}

static void SetupDragDrop(HWND hwnd) {
    if (!hwnd) return;

    LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle | WS_EX_ACCEPTFILES);
    DragAcceptFiles(hwnd, TRUE);

    g_originalWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)DropHookProc);

    HMODULE hUser32 = GetModuleHandleA("user32.dll");
    if (hUser32) {
        typedef BOOL(WINAPI* PFN_ChangeWindowMessageFilterEx)(HWND, UINT, DWORD, void*);
        PFN_ChangeWindowMessageFilterEx pFilterEx = (PFN_ChangeWindowMessageFilterEx)GetProcAddress(hUser32, "ChangeWindowMessageFilterEx");
        if (pFilterEx) {
            pFilterEx(hwnd, WM_DROPFILES, 1, NULL);
            pFilterEx(hwnd, WM_COPYDATA, 1, NULL);
            pFilterEx(hwnd, 0x0049, 1, NULL);
        }
    }
}
#endif

static sf::Image DownscaleIcon(const sf::Image& src, unsigned int targetSize = 32) {
    sf::Image dest;
    dest.create(targetSize, targetSize);
    unsigned int srcW = src.getSize().x;
    unsigned int srcH = src.getSize().y;

    for (unsigned int y = 0; y < targetSize; ++y) {
        for (unsigned int x = 0; x < targetSize; ++x) {
            unsigned int srcX = (x * srcW) / targetSize;
            unsigned int srcY = (y * srcH) / targetSize;
            dest.setPixel(x, y, src.getPixel(srcX, srcY));
        }
    }
    return dest;
}

static void ApplyWindowIcon(sf::RenderWindow& window) {
    sf::Image appIcon;
    bool loaded = appIcon.loadFromFile("wisdomParkicon.png") ||
        appIcon.loadFromFile("wisdomParkicon.jpg") ||
        appIcon.loadFromFile("Resources/wisdomParkicon.png") ||
        appIcon.loadFromFile("Resources/wisdomParkicon.jpg") ||
        appIcon.loadFromFile("assets/wisdomParkicon.png") ||
        appIcon.loadFromFile("assets/wisdomParkicon.jpg");

    if (loaded) {
        sf::Image safeIcon = DownscaleIcon(appIcon, 32);
        window.setIcon(safeIcon.getSize().x, safeIcon.getSize().y, safeIcon.getPixelsPtr());
    }
}

static AIPanel g_aiPanel;
static AIReviewModal g_aiReviewModal;
static bool g_typingApiKey = false;
static sf::RectangleShape loadingOverlay;
static sf::RectangleShape loadingBox;
static sf::Text loadingText;
static sf::CircleShape loadingSpinner;
static sf::RectangleShape loadingCancelBtn;
static sf::Text loadingCancelText;

UIManager::UIManager() : isTypingPrompt(false), showingText(false), textAlpha(255.0f), isLightingMode(false), promptQuantity(1), focusMode(false), projManager(nullptr), activeProjectName("Untitled_Project"), activeProjectPath(""), isDraggingSizeSlider(false), showUnsavedWarning(false), currentMenuState(MenuState::Main), startupTime(0.0f), activeTutorialIndex(-1), uiFullscreen(true), uiBorderless(false), uiVsync(true), uiAutoBackup(true), uiHwAccel(true), uiFpsLimit(60), uiAnimFps(12), uiHistorySize(15), easterEggClicks(0), m_debugUseSpriteStudio(false) {}

void UIManager::init(ProjectManager* pm, Canvas* baseCanvas) {
    projManager = pm;
    keybindManager.init();

    bgTexture.loadFromFile("assets/landofwisdompark2.jfif");
    bgSprite.setTexture(bgTexture);

    font.loadFromFile("assets/font.otf");

    projectBrowser.init(pm);
    keybindPanel.init(&keybindManager);
    exportModal.init();
    newProjectModal.init();

    leftToolbar.init();
    layerPanel.init();
    colorPalettePanel.init();
    rightProperties.init();
    bottomTimeline.init();
    audioPanel.init();
    initMinigame();

    AIManager::getInstance().init();
    g_aiPanel.init();
    g_aiReviewModal.init();

    handTrackerSocket.bind(5005);
    handTrackerSocket.setBlocking(false);

    uiText.setFont(font);
    uiText.setCharacterSize(30);
    uiText.setOutlineColor(sf::Color(0, 0, 0, 150));
    uiText.setOutlineThickness(2.0f);

    uiFullscreen = true;
    uiBorderless = false;

    promptBox.setSize(sf::Vector2f(600.f, 50.f));
    promptBox.setPosition(1920.f / 2.f - 300.f, 1080.f - 300.f);
    promptBox.setFillColor(sf::Color(15, 15, 18, 220));
    promptBox.setOutlineThickness(1.0f);
    promptBox.setOutlineColor(sf::Color(255, 255, 255, 30));

    promptDisplay.setFont(font);
    promptDisplay.setCharacterSize(20);
    promptDisplay.setFillColor(sf::Color::White);
    promptDisplay.setPosition(1920.f / 2.f - 290.f, 1080.f - 288.f);

    toolBg.setSize(sf::Vector2f(44.f, 180.f));
    toolBg.setFillColor(sf::Color(25, 25, 30, 240));
    toolBg.setOutlineThickness(1.f);
    toolBg.setOutlineColor(sf::Color(100, 100, 110));

    sizeSliderBg.setSize(sf::Vector2f(10.f, 100.f));
    sizeSliderBg.setFillColor(sf::Color(15, 15, 20));
    sizeSliderBg.setOutlineThickness(1.f);
    sizeSliderBg.setOutlineColor(sf::Color(60, 60, 70));

    sizeSliderHandle.setSize(sf::Vector2f(20.f, 10.f));
    sizeSliderHandle.setFillColor(sf::Color(0, 191, 255));

    sizeLabelText.setFont(font);
    sizeLabelText.setString("SIZE");
    sizeLabelText.setCharacterSize(10);
    sizeLabelText.setFillColor(sf::Color(180, 180, 180));

    sizeValueText.setFont(font);
    sizeValueText.setCharacterSize(12);
    sizeValueText.setFillColor(sf::Color::White);

    pixelPerfBtn.setSize(sf::Vector2f(30.f, 20.f));
    pixelPerfBtn.setFillColor(sf::Color(40, 40, 50));
    pixelPerfText.setFont(font);
    pixelPerfText.setString("PERF");
    pixelPerfText.setCharacterSize(10);
    pixelPerfText.setFillColor(sf::Color::White);

    warnOverlay.setSize(sf::Vector2f(1920.f, 1080.f));
    warnOverlay.setFillColor(sf::Color(0, 0, 0, 180));

    assetBrowser = std::make_unique<AssetBrowserPanel>(assetManager, font);
    assetBrowser->setProject("CurrentProject");
    assetBrowser->setBounds(sf::FloatRect(1440.f, 78.f, 390.f, 540.f));

    loadingOverlay.setSize(sf::Vector2f(1920.f, 1080.f));
    loadingOverlay.setFillColor(sf::Color(10, 10, 15, 200));

    loadingBox.setSize(sf::Vector2f(450.f, 220.f));
    loadingBox.setOrigin(225.f, 110.f);
    loadingBox.setPosition(960.f, 540.f);
    loadingBox.setFillColor(sf::Color(25, 25, 35));
    loadingBox.setOutlineThickness(2.f);
    loadingBox.setOutlineColor(sf::Color(100, 150, 255));

    loadingText.setFont(font);
    loadingText.setString("Wisdom Park AI is thinking...");
    loadingText.setCharacterSize(16);
    loadingText.setFillColor(sf::Color::White);
    loadingText.setOrigin(loadingText.getLocalBounds().width / 2.f, loadingText.getLocalBounds().height / 2.f);
    loadingText.setPosition(960.f, 480.f);

    loadingSpinner.setRadius(25.f);
    loadingSpinner.setPointCount(3);
    loadingSpinner.setFillColor(sf::Color(255, 200, 100));
    loadingSpinner.setOrigin(25.f, 25.f);
    loadingSpinner.setPosition(960.f, 540.f);

    loadingCancelBtn.setSize(sf::Vector2f(120.f, 35.f));
    loadingCancelBtn.setOrigin(60.f, 17.5f);
    loadingCancelBtn.setPosition(960.f, 610.f);
    loadingCancelBtn.setFillColor(sf::Color(180, 60, 60));

    loadingCancelText.setFont(font);
    loadingCancelText.setString("[ Cancel X ]");
    loadingCancelText.setCharacterSize(14);
    loadingCancelText.setFillColor(sf::Color::White);
    loadingCancelText.setOrigin(loadingCancelText.getLocalBounds().width / 2.f, loadingCancelText.getLocalBounds().height / 2.f);
    loadingCancelText.setPosition(960.f, 607.f);

    m_gradientPanel.init(&m_gradientConfig);
    m_topMenuBar.init();
    m_perspectiveManager.init();
    m_perspectivePanel.init(&m_perspectiveManager);
    m_textManager.init();
    m_textPanel.init(&m_textManager);
    baseCanvas->setPerspectiveManager(&m_perspectiveManager);
    baseCanvas->setTextManager(&m_textManager);

    m_topBar.Initialize(
        font,
        [this]() { newProjectModal.open(); },
        [this, baseCanvas]() {
            std::string file = NativeDialogs::openFileDialog("Wisdom Park Projects\0*.wpk\0All Files\0*.*\0");
            if (!file.empty() && projManager) {
                activeProjectPath = file;
                activeProjectName = std::filesystem::path(file).stem().string();
                int loadedFps = 12;
                bool isPix = false;
                if (projManager->loadProject(activeProjectPath, *baseCanvas, loadedFps, isPix)) {
                    baseCanvas->setPixelMode(isPix);
                    baseCanvas->clearIsDirty();
                    showMessage("Loaded Project: " + activeProjectName, sf::Color::Green);
                }
            }
        },
        [this, baseCanvas]() {
            if (activeProjectPath.empty()) {
                std::string file = NativeDialogs::saveFileDialog("Wisdom Park Projects\0*.wpk\0", "wpk", activeProjectName);
                if (!file.empty() && projManager) {
                    activeProjectPath = file;
                    projManager->saveProjectAs(activeProjectPath, activeProjectName, *baseCanvas, 12, baseCanvas->getPixelMode());
                    baseCanvas->clearIsDirty();
                    showMessage("Saved Project", sf::Color::Green);
                }
            }
            else if (projManager) {
                projManager->saveProjectAs(activeProjectPath, activeProjectName, *baseCanvas, 12, baseCanvas->getPixelMode());
                baseCanvas->clearIsDirty();
                showMessage("Saved Project", sf::Color::Green);
            }
        },
        [this, baseCanvas]() { exportModal.open(*baseCanvas, 0); },
        [this, baseCanvas]() { baseCanvas->undo(); },
        [this, baseCanvas]() { baseCanvas->redo(); },
        [this]() { m_fullscreenToggleRequested = true; },
        [this, baseCanvas]() {
            if (baseCanvas->getIsDirty()) {
                showUnsavedWarning = true;
            }
            else {
                currentMenuState = MenuState::Main;
            }
        }
    );

    m_toolOptionsBar.Initialize(font);
    m_toolDock.Initialize(font);

    m_rightDockTabs.Initialize(
        font,
        [this]() {
            m_activeRightTab = (m_activeRightTab == RightTabMode::Layers) ? RightTabMode::None : RightTabMode::Layers;
        },
        [this]() {
            m_activeRightTab = (m_activeRightTab == RightTabMode::Palette) ? RightTabMode::None : RightTabMode::Palette;
        },
        [this]() {
            m_activeRightTab = (m_activeRightTab == RightTabMode::Properties) ? RightTabMode::None : RightTabMode::Properties;
        },
        [this]() {
            if (assetBrowser) assetBrowser->toggle();
        },
        [this]() {
            audioPanel.toggle();
        }
    );

    m_statusBar.Initialize(font, [this]() {
        m_showTimeline = !m_showTimeline;
        });

    m_timelineHeader.Initialize(
        font,
        [this]() {},
        [this, baseCanvas]() {},
        [this, baseCanvas]() {},
        [this, baseCanvas]() {},
        [this, baseCanvas]() { baseCanvas->setOnionSkin(!baseCanvas->isOnionSkinEnabled(), baseCanvas->getOnionSkinPrevOpacity(), baseCanvas->getOnionSkinNextOpacity()); },
        [this]() { m_showTimeline = false; }
    );

    m_toolDock.AddTool("brush", "Brush Tool (B)", [this, baseCanvas]() { baseCanvas->setActiveTool(ToolType::Brush); m_toolDock.SetActiveTool("brush"); });
    m_toolDock.AddTool("pencil", "Pencil Tool (P)", [this, baseCanvas]() { baseCanvas->setActiveTool(ToolType::Pencil); m_toolDock.SetActiveTool("pencil"); });
    m_toolDock.AddTool("eraser", "Eraser Tool (E)", [this, baseCanvas]() { baseCanvas->setActiveTool(ToolType::Eraser); m_toolDock.SetActiveTool("eraser"); });
    m_toolDock.AddTool("fill", "Fill Bucket (F)", [this, baseCanvas]() { baseCanvas->setActiveTool(ToolType::Fill); m_toolDock.SetActiveTool("fill"); });
    m_toolDock.AddTool("select", "Selection Box (M)", [this, baseCanvas]() { baseCanvas->setActiveTool(ToolType::Select); m_toolDock.SetActiveTool("select"); });
    m_toolDock.AddTool("magic_wand", "Magic Wand (W)", [this, baseCanvas]() { baseCanvas->setActiveTool(ToolType::MagicWand); m_toolDock.SetActiveTool("magic_wand"); });
    m_toolDock.AddTool("shapes", "Shapes Tool (U)", [this, baseCanvas]() { baseCanvas->setActiveTool(ToolType::Shapes); m_toolDock.SetActiveTool("shapes"); });
    m_toolDock.AddTool("text", "Text Tool (T)", [this, baseCanvas]() { baseCanvas->setActiveTool(ToolType::Text); m_toolDock.SetActiveTool("text"); });
    m_toolDock.AddTool("gradient", "Gradient Tool (G)", [this, baseCanvas]() { baseCanvas->setActiveTool(ToolType::Gradient); m_toolDock.SetActiveTool("gradient"); });
    m_toolDock.AddTool("symmetry", "Symmetry Axis", [this, baseCanvas]() { baseCanvas->setActiveTool(ToolType::Symmetry); m_toolDock.SetActiveTool("symmetry"); });
    m_toolDock.AddTool("perspective", "Perspective Grid", [this, baseCanvas]() { baseCanvas->setActiveTool(ToolType::Perspective); m_toolDock.SetActiveTool("perspective"); });
    m_toolDock.AddTool("ai_gen", "AI Generator", [this]() { g_aiPanel.toggle(); m_toolDock.SetActiveTool("ai_gen"); });

    initStartMenu();
}

void UIManager::initStartMenu() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> disX(0.0f, 1920.0f);
    std::uniform_real_distribution<float> disY(0.0f, 1080.0f);
    std::uniform_real_distribution<float> disV(-10.0f, 10.0f);
    std::uniform_real_distribution<float> disLife(0.5f, 5.0f);
    std::uniform_real_distribution<float> disSize(1.0f, 4.0f);

    for (int i = 0; i < 150; ++i) {
        StartParticle p;
        p.x = disX(gen);
        p.y = disY(gen);
        p.vx = disV(gen);
        p.vy = -std::abs(disV(gen)) - 5.0f;
        p.maxLife = disLife(gen);
        p.life = static_cast<float>(i % 100) / 100.0f * p.maxLife;
        p.size = disSize(gen);
        int cR = 200 + rand() % 55;
        int cG = 150 + rand() % 100;
        int cB = 50 + rand() % 50;
        p.baseColor = sf::Color(static_cast<sf::Uint8>(cR), static_cast<sf::Uint8>(cG), static_cast<sf::Uint8>(cB));
        particles.push_back(p);
    }

    for (int i = 0; i < 4; ++i) {
        LightRay lr;
        lr.shape.setPointCount(4);
        float wTop = 150.f + static_cast<float>(rand() % 200);
        float wBot = 400.f + static_cast<float>(rand() % 400);
        float sx = 800.f + (static_cast<float>(i) * 300.f);
        lr.shape.setPoint(0, sf::Vector2f(sx, -100.f));
        lr.shape.setPoint(1, sf::Vector2f(sx + wTop, -100.f));
        lr.shape.setPoint(2, sf::Vector2f(sx - 500.f + wBot, 1200.f));
        lr.shape.setPoint(3, sf::Vector2f(sx - 500.f, 1200.f));
        lr.shape.setFillColor(sf::Color(255, 230, 180, 8));
        lr.speed = 10.f + static_cast<float>(rand() % 20);
        lr.offset = static_cast<float>(rand() % 1000);
        lightRays.push_back(lr);
    }
}

void UIManager::showMessage(const std::string& msg, sf::Color color) {
    uiText.setString(msg);
    uiText.setFillColor(color);
    sf::FloatRect textRect = uiText.getLocalBounds();
    uiText.setOrigin(textRect.left + textRect.width / 2.0f, textRect.top + textRect.height / 2.0f);
    uiText.setPosition(1920.0f / 2.0f, 80.0f);
    showingText = true;
    textAlpha = 255.0f;
    textClock.restart();
}

void UIManager::updateHoverValue(const std::string& key, bool isHovering, float dt, float speed) {
    if (hoverMap.find(key) == hoverMap.end()) hoverMap[key] = 0.0f;
    float target = isHovering ? 1.0f : 0.0f;
    hoverMap[key] += (target - hoverMap[key]) * speed * dt;
}

float UIManager::getHover(const std::string& key) {
    return hoverMap.count(key) ? hoverMap[key] : 0.0f;
}

void UIManager::drawGlassPanel(sf::RenderWindow& window, sf::FloatRect bounds, float hoverScale) {
    sf::RectangleShape panel(sf::Vector2f(bounds.width, bounds.height));
    panel.setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    panel.setPosition(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);

    float s = 1.0f + 0.02f * hoverScale;
    panel.setScale(s, s);

    panel.setFillColor(sf::Color(25, 28, 35, 190));
    panel.setOutlineThickness(1.5f);
    sf::Color outline = sf::Color(
        static_cast<sf::Uint8>(100.0f + hoverScale * 155.0f),
        static_cast<sf::Uint8>(100.0f + hoverScale * 100.0f),
        120,
        static_cast<sf::Uint8>(150.0f + hoverScale * 105.0f)
    );
    panel.setOutlineColor(outline);

    window.draw(panel);
}

void UIManager::drawPremiumText(sf::RenderWindow& window, const std::string& str, float x, float y, int size, sf::Color mainCol, sf::Color outlineCol, sf::Color shadowCol) {
    sf::Text t(str, font, size);
    sf::FloatRect b = t.getLocalBounds();
    t.setOrigin(b.left + b.width / 2.f, b.top + b.height / 2.f);

    t.setPosition(x + 6.f, y + 6.f);
    t.setFillColor(shadowCol);
    window.draw(t);

    t.setFillColor(outlineCol);
    const float offsets[8][2] = { {-2,-2}, {0,-2}, {2,-2}, {-2,0}, {2,0}, {-2,2}, {0,2}, {2,2} };
    for (int i = 0; i < 8; ++i) {
        t.setPosition(x + offsets[i][0], y + offsets[i][1]);
        window.draw(t);
    }

    t.setPosition(x, y);
    t.setFillColor(mainCol);
    window.draw(t);

    float shineOffset = static_cast<float>(std::fmod(startupTime * 400.0f, static_cast<double>(b.width) * 3.0)) - static_cast<float>(b.width);
    sf::RectangleShape shine(sf::Vector2f(10.f, static_cast<float>(size) * 1.5f));
    shine.setOrigin(5.f, static_cast<float>(size) * 0.75f);
    shine.setPosition(x - b.width / 2.f + shineOffset, y);
    shine.setRotation(25.f);
    shine.setFillColor(sf::Color(255, 255, 255, 60));
    window.draw(shine);
}

void UIManager::updateStartMenu(float dt, sf::Vector2f mousePos) {
    startupTime += dt;
    for (auto& p : particles) {
        p.life += dt;
        if (p.life > p.maxLife) {
            p.life = 0.0f;
            p.x = static_cast<float>(rand() % 1920);
            p.y = 1080.0f + static_cast<float>(rand() % 100);
        }
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.vx += std::sin(startupTime + p.y * 0.01f) * 2.0f * dt;
    }
}

void UIManager::drawStartMenu(sf::RenderWindow& window) {
    sf::RectangleShape overlay(sf::Vector2f(1920.f, 1080.f));
    overlay.setFillColor(sf::Color(10, 15, 25, 140));
    window.draw(overlay);

    for (const auto& lr : lightRays) {
        sf::ConvexShape r = lr.shape;
        float shift = std::sin(startupTime * 0.5f + lr.offset) * 50.f;
        r.move(shift, 0.f);
        window.draw(r, sf::RenderStates(sf::BlendAdd));
    }

    for (const auto& p : particles) {
        sf::CircleShape circ(p.size);
        circ.setOrigin(p.size, p.size);
        circ.setPosition(p.x, p.y);
        float fade = 1.0f;
        if (p.life < 0.5f) fade = p.life / 0.5f;
        else if (p.maxLife - p.life < 0.5f) fade = (p.maxLife - p.life) / 0.5f;
        sf::Color c = p.baseColor;
        c.a = static_cast<sf::Uint8>(std::clamp(180.0f * fade, 0.0f, 255.0f));
        circ.setFillColor(c);
        window.draw(circ, sf::RenderStates(sf::BlendAdd));
    }

    if (currentMenuState == MenuState::Main) drawMainMenu(window);
    else if (currentMenuState == MenuState::Settings) drawSettingsMenu(window);
    else if (currentMenuState == MenuState::Tutorials) drawTutorialsMenu(window);
    else if (currentMenuState == MenuState::Credits) drawCreditsMenu(window);
}

void UIManager::drawStandardMainMenu(sf::RenderWindow& window) {
    sf::Vector2i mousePosI = sf::Mouse::getPosition(window);
    sf::Vector2f mousePos = window.mapPixelToCoords(mousePosI);

    float floatAnim = std::sin(startupTime * 2.0f) * 4.0f;
    float titleY = 70.f + floatAnim;

    WisdomUI::Theme::DrawCrispText(window, font, "WISDOM PARK", 56, 960.f, titleY, WisdomUI::Theme::SunsetGold, sf::Color(12, 4, 18), true, true);
    WisdomUI::Theme::DrawCrispText(window, font, "CREATIVE ANIMATION STUDIO & PIXEL ART SUITE", 16, 960.f, titleY + 52.f, WisdomUI::Theme::SunsetPeach, sf::Color(12, 4, 18), true, true);

    sf::FloatRect menuPod(440.f, 145.f, 1040.f, 790.f);
    WisdomUI::Theme::DrawSunsetPanel(window, menuPod, 1.0f);

    sf::FloatRect bannerGrip(menuPod.left + 20.f, menuPod.top + 14.f, menuPod.width - 40.f, 38.f);
    sf::RectangleShape bannerBg(sf::Vector2f(bannerGrip.width, bannerGrip.height));
    bannerBg.setPosition(bannerGrip.left, bannerGrip.top);
    bannerBg.setFillColor(WisdomUI::Theme::SunsetDeepDark);
    bannerBg.setOutlineThickness(1.5f);
    bannerBg.setOutlineColor(WisdomUI::Theme::SunsetPlum);
    window.draw(bannerBg);

    WisdomUI::Theme::DrawCrispText(window, font, ":: MAIN LAUNCHPAD ::", 15, bannerGrip.left + bannerGrip.width / 2.f, bannerGrip.top + bannerGrip.height / 2.f, WisdomUI::Theme::SunsetAmber, sf::Color(14, 6, 20), true, true);

    std::vector<std::pair<std::string, std::pair<std::string, std::string>>> menuCards = {
        {"NEW PROJECT", {"Initialize dynamic canvas or pixel grid", "CTRL+N"}},
        {"PROJECT VAULT", {"Browse recent project files and disk archives", "CTRL+O"}},
        {"STUDIO SETTINGS", {"Configure display resolution, vsync and drivers", "ESC"}},
        {"KNOWLEDGE CODEX", {"Interactive guides, tutorials and technique manuals", "F1"}},
        {"KEYBIND MATRIX", {"Customize keyboard shortcuts and tool bindings", "K"}},
        {"STUDIO CREDITS", {"Engine architecture and contribution roll", "C"}},
        {"EXIT SOFTWARE", {"Close application safely", "ALT+F4"}}
    };

    float startX = menuPod.left + 24.f;
    float startY = menuPod.top + 66.f;
    float btnW = menuPod.width - 48.f;
    float btnH = 80.f;
    float spacing = 14.f;

    for (size_t i = 0; i < menuCards.size(); ++i) {
        sf::FloatRect cardRect(startX, startY + static_cast<float>(i) * (btnH + spacing), btnW, btnH);
        bool isHovered = cardRect.contains(mousePos);
        bool isAccent = (i == 0);
        bool isExit = (i == menuCards.size() - 1);

        sf::RectangleShape card(sf::Vector2f(cardRect.width, cardRect.height));
        card.setPosition(cardRect.left, cardRect.top);

        if (isHovered) {
            card.setFillColor(isExit ? sf::Color(150, 24, 44, 240) : WisdomUI::Theme::SunsetSkyMid);
            card.setOutlineThickness(2.0f);
            card.setOutlineColor(isAccent ? WisdomUI::Theme::SunsetGold : (isExit ? sf::Color(255, 70, 90) : WisdomUI::Theme::SunsetAmber));
        }
        else {
            card.setFillColor(WisdomUI::Theme::SunsetDeepDark);
            card.setOutlineThickness(1.2f);
            card.setOutlineColor(isAccent ? WisdomUI::Theme::SunsetCoral : WisdomUI::Theme::SunsetPlum);
        }
        window.draw(card);

        if (isHovered) {
            sf::RectangleShape accentStrip(sf::Vector2f(6.f, cardRect.height - 16.f));
            accentStrip.setPosition(cardRect.left + 8.f, cardRect.top + 8.f);
            accentStrip.setFillColor(isAccent ? WisdomUI::Theme::SunsetGold : (isExit ? sf::Color(255, 70, 90) : WisdomUI::Theme::SunsetAmber));
            window.draw(accentStrip);
        }

        sf::Color titleColor = isHovered ? WisdomUI::Theme::SunsetGold : (isAccent ? WisdomUI::Theme::SunsetAmber : WisdomUI::Theme::TextPrimary);
        WisdomUI::Theme::DrawCrispText(window, font, menuCards[i].first, 22, cardRect.left + 30.f, cardRect.top + 14.f, titleColor, sf::Color(14, 6, 20));
        WisdomUI::Theme::DrawCrispText(window, font, menuCards[i].second.first, 14, cardRect.left + 30.f, cardRect.top + 46.f, isHovered ? sf::Color::White : WisdomUI::Theme::TextSecondary);

        if (!menuCards[i].second.second.empty()) {
            sf::FloatRect badge(cardRect.left + cardRect.width - 130.f, cardRect.top + (cardRect.height - 40.f) / 2.f, 110.f, 40.f);
            sf::RectangleShape badgeBg(sf::Vector2f(badge.width, badge.height));
            badgeBg.setPosition(badge.left, badge.top);
            badgeBg.setFillColor(sf::Color(10, 4, 18));
            badgeBg.setOutlineThickness(1.5f);
            badgeBg.setOutlineColor(isHovered ? WisdomUI::Theme::SunsetGold : WisdomUI::Theme::SunsetPlum);
            window.draw(badgeBg);

            WisdomUI::Theme::DrawCrispText(window, font, menuCards[i].second.second, 14, badge.left + badge.width / 2.f, badge.top + badge.height / 2.f, isHovered ? WisdomUI::Theme::SunsetGold : WisdomUI::Theme::SunsetPeach, sf::Color::Transparent, true, true);
        }
    }

    sf::FloatRect tickerBounds(340.f, 950.f, 1240.f, 48.f);
    WisdomUI::Theme::DrawSunsetPanel(window, tickerBounds, 1.0f);
    WisdomUI::Theme::DrawCrispText(window, font, "WISDOM PARK STUDIO  |  v2.6.0 RETRO ENGINE  |  DIRECT HARDWARE ACCELERATION READY", 14, tickerBounds.left + tickerBounds.width / 2.f, tickerBounds.top + tickerBounds.height / 2.f, WisdomUI::Theme::SunsetGold, sf::Color(14, 6, 20), true, true);
}

void UIManager::drawMainMenu(sf::RenderWindow& window) {
    sf::Vector2i mousePosI = sf::Mouse::getPosition(window);
    sf::Vector2f mousePos = window.mapPixelToCoords(mousePosI);

    if (!m_useMinigameWelcome) {
        drawStandardMainMenu(window);
    }
    else {
        for (const auto& wall : m_arcadeMazeWalls) {
            sf::RectangleShape glow(sf::Vector2f(wall.width + 6.f, wall.height + 6.f));
            glow.setPosition(wall.left - 3.f, wall.top - 3.f);
            glow.setFillColor(sf::Color(0, 120, 255, 40));
            window.draw(glow);

            sf::RectangleShape w(sf::Vector2f(wall.width, wall.height));
            w.setPosition(wall.left, wall.top);
            w.setFillColor(sf::Color(25, 90, 255));
            window.draw(w);

            sf::RectangleShape core(sf::Vector2f(std::max(1.f, wall.width - 2.f), std::max(1.f, wall.height - 2.f)));
            core.setPosition(wall.left + 1.f, wall.top + 1.f);
            core.setFillColor(sf::Color(190, 225, 255));
            window.draw(core);
        }

        for (const auto& item : m_arcadeCollectibles) {
            if (!item.collected) {
                drawPixelItem(window, item.pos, item.type, item.animPhase);
            }
        }

        for (const auto& portal : m_arcadePortals) {
            drawStationPortal(window, portal, mousePos);
        }

        for (const auto& g : m_arcadeGhosts) {
            drawPixelGhost(window, g);
        }

        drawPixelHero(window);

        for (const auto& fx : m_arcadeFX) {
            sf::RectangleShape r(sf::Vector2f(fx.size, fx.size));
            r.setPosition(fx.pos);
            sf::Color c = fx.color;
            c.a = static_cast<sf::Uint8>((fx.life / fx.maxLife) * 255.f);
            r.setFillColor(c);
            window.draw(r);
        }

        for (const auto& fl : m_arcadeFloaters) {
            float a = (fl.life / fl.maxLife);
            sf::Color c = fl.color;
            c.a = static_cast<sf::Uint8>(a * 255.f);
            WisdomUI::Theme::DrawCrispText(window, font, fl.text, 16, fl.pos.x, fl.pos.y, c, sf::Color(14, 4, 20, static_cast<sf::Uint8>(a * 255.f)), true, true);
        }

        drawArcadeBezelOverlay(window);

        WisdomUI::Theme::DrawCrispText(window, font, "1UP", 15, 240.f, 52.f, sf::Color(255, 60, 90), sf::Color(14, 4, 20));
        WisdomUI::Theme::DrawCrispText(window, font, std::to_string(m_arcadeScore), 20, 240.f, 74.f, WisdomUI::Theme::SunsetGold, sf::Color(14, 4, 20));

        WisdomUI::Theme::DrawCrispText(window, font, "HIGH SCORE", 15, 960.f, 52.f, sf::Color(255, 60, 90), sf::Color(14, 4, 20), true, false);
        WisdomUI::Theme::DrawCrispText(window, font, std::to_string(m_arcadeHighScore), 20, 960.f, 74.f, WisdomUI::Theme::SunsetGold, sf::Color(14, 4, 20), true, false);

        WisdomUI::Theme::DrawCrispText(window, font, "LIVES", 14, 1640.f, 52.f, WisdomUI::Theme::SunsetAmber, sf::Color(14, 4, 20));
        for (int i = 0; i < m_arcadeHero.lives; ++i) {
            sf::CircleShape lifeIcon(7.f);
            lifeIcon.setPosition(1640.f + static_cast<float>(i) * 20.f, 76.f);
            lifeIcon.setFillColor(WisdomUI::Theme::SunsetGold);
            window.draw(lifeIcon);
        }

        WisdomUI::Theme::DrawCrispText(window, font, "NAVIGATE [WASD / ARROWS] TO ROOM ENTRANCES  -  OR CLICK BOOTHS DIRECTLY", 12, 960.f, 975.f, WisdomUI::Theme::SunsetAmber, sf::Color(14, 4, 20), true, true);
    }

    bool isToggleHov = m_welcomeModeToggleBounds.contains(mousePos);
    std::string toggleLabel = m_useMinigameWelcome ? "Mode: Arcade Minigame" : "Mode: Standard Menu";
    WisdomUI::Theme::DrawSunsetButton(window, m_welcomeModeToggleBounds, toggleLabel, font, 11, false, isToggleHov, m_useMinigameWelcome, 1.0f);

    bool isFsHov = m_startMenuFullscreenBtnBounds.contains(mousePos);
    std::string fsLabel = uiFullscreen ? "Fullscreen: ON" : "Fullscreen: OFF";
    WisdomUI::Theme::DrawSunsetButton(window, m_startMenuFullscreenBtnBounds, fsLabel, font, 11, uiFullscreen, isFsHov, uiFullscreen, 1.0f);
}

void UIManager::drawBackButton(sf::RenderWindow& window, const std::string& hoverKey, float x, float y) {
    sf::Vector2i mousePosI = sf::Mouse::getPosition(window);
    sf::Vector2f mousePos = window.mapPixelToCoords(mousePosI);

    sf::FloatRect bounds(x, y, 130.f, 38.f);
    bool isHov = bounds.contains(mousePos);
    WisdomUI::Theme::DrawSunsetButton(window, bounds, "< BACK", font, 12, false, isHov, false, 1.0f);
}

void UIManager::drawSettingsMenu(sf::RenderWindow& window) {
    sf::Vector2i mousePosI = sf::Mouse::getPosition(window);
    sf::Vector2f mousePos = window.mapPixelToCoords(mousePosI);

    sf::FloatRect container(200.f, 60.f, 1520.f, 960.f);
    WisdomUI::Theme::DrawSunsetPanel(window, container, 1.0f);

    sf::FloatRect backBtn(container.left + 28.f, container.top + 22.f, 140.f, 44.f);
    WisdomUI::Theme::DrawSunsetButton(window, backBtn, "< BACK", font, 16, false, backBtn.contains(mousePos), false, 1.0f);

    WisdomUI::Theme::DrawCrispText(window, font, "STUDIO CONFIGURATION & DRIVERS", 28, container.left + 190.f, container.top + 20.f, WisdomUI::Theme::SunsetGold, sf::Color(14, 6, 20));
    WisdomUI::Theme::DrawCrispText(window, font, "DISPLAY, ENGINE PERFORMANCE, BACKUPS & AI MATRIX", 14, container.left + 192.f, container.top + 54.f, WisdomUI::Theme::TextSecondary);

    sf::RectangleShape div(sf::Vector2f(container.width - 56.f, 2.f));
    div.setPosition(container.left + 28.f, container.top + 86.f);
    div.setFillColor(WisdomUI::Theme::SunsetPlum);
    window.draw(div);

    auto drawCard = [&](sf::FloatRect b, const std::string& title) {
        sf::RectangleShape box(sf::Vector2f(b.width, b.height));
        box.setPosition(b.left, b.top);
        box.setFillColor(WisdomUI::Theme::SunsetDeepDark);
        box.setOutlineThickness(1.5f);
        box.setOutlineColor(WisdomUI::Theme::SunsetPlum);
        window.draw(box);

        sf::FloatRect headerGrip(b.left + 14.f, b.top + 12.f, b.width - 28.f, 34.f);
        sf::RectangleShape hBg(sf::Vector2f(headerGrip.width, headerGrip.height));
        hBg.setPosition(headerGrip.left, headerGrip.top);
        hBg.setFillColor(WisdomUI::Theme::SunsetSkyTop);
        hBg.setOutlineThickness(1.f);
        hBg.setOutlineColor(WisdomUI::Theme::SunsetPlum);
        window.draw(hBg);

        WisdomUI::Theme::DrawCrispText(window, font, title, 14, headerGrip.left + 16.f, headerGrip.top + 8.f, WisdomUI::Theme::SunsetAmber);
        };

    auto drawToggle = [&](float x, float y, float w, const std::string& label, bool active) {
        sf::FloatRect rowRect(x, y, w, 48.f);
        bool isHov = rowRect.contains(mousePos);

        sf::RectangleShape checkBg(sf::Vector2f(28.f, 28.f));
        checkBg.setPosition(x + 16.f, y + 10.f);
        checkBg.setFillColor(active ? WisdomUI::Theme::SunsetGold : sf::Color(14, 6, 20));
        checkBg.setOutlineThickness(1.5f);
        checkBg.setOutlineColor(active ? WisdomUI::Theme::SunsetAmber : WisdomUI::Theme::SunsetPlum);
        window.draw(checkBg);

        if (active) {
            sf::RectangleShape checkMark(sf::Vector2f(14.f, 14.f));
            checkMark.setPosition(x + 23.f, y + 17.f);
            checkMark.setFillColor(sf::Color(14, 6, 20));
            window.draw(checkMark);
        }

        WisdomUI::Theme::DrawCrispText(window, font, label, 17, x + 58.f, y + 14.f, isHov ? WisdomUI::Theme::SunsetGold : WisdomUI::Theme::TextPrimary);

        sf::FloatRect badge(x + w - 120.f, y + 8.f, 104.f, 32.f);
        sf::RectangleShape badgeBg(sf::Vector2f(badge.width, badge.height));
        badgeBg.setPosition(badge.left, badge.top);
        badgeBg.setFillColor(sf::Color(10, 4, 18));
        badgeBg.setOutlineThickness(1.f);
        badgeBg.setOutlineColor(active ? WisdomUI::Theme::SunsetGold : WisdomUI::Theme::SunsetPlum);
        window.draw(badgeBg);

        std::string stateStr = active ? "ENABLED" : "DISABLED";
        sf::Color badgeCol = active ? WisdomUI::Theme::SunsetGold : WisdomUI::Theme::TextSecondary;
        WisdomUI::Theme::DrawCrispText(window, font, stateStr, 12, badge.left + badge.width / 2.f, badge.top + badge.height / 2.f, badgeCol, sf::Color::Transparent, true, true);
        };

    auto drawStepper = [&](float x, float y, float w, const std::string& label, const std::string& val) {
        sf::FloatRect rowRect(x, y, w, 48.f);
        WisdomUI::Theme::DrawCrispText(window, font, label, 17, x + 16.f, y + 14.f, WisdomUI::Theme::TextPrimary);

        sf::FloatRect btnL(x + w - 240.f, y + 6.f, 40.f, 36.f);
        sf::FloatRect btnR(x + w - 56.f, y + 6.f, 40.f, 36.f);

        WisdomUI::Theme::DrawSunsetButton(window, btnL, "<", font, 14, false, btnL.contains(mousePos), false, 1.0f);

        sf::FloatRect valBg(x + w - 192.f, y + 6.f, 128.f, 36.f);
        sf::RectangleShape vBox(sf::Vector2f(valBg.width, valBg.height));
        vBox.setPosition(valBg.left, valBg.top);
        vBox.setFillColor(sf::Color(14, 6, 20));
        vBox.setOutlineThickness(1.f);
        vBox.setOutlineColor(WisdomUI::Theme::SunsetPlum);
        window.draw(vBox);

        WisdomUI::Theme::DrawCrispText(window, font, val, 15, valBg.left + valBg.width / 2.f, valBg.top + valBg.height / 2.f, WisdomUI::Theme::SunsetGold, sf::Color::Transparent, true, true);

        WisdomUI::Theme::DrawSunsetButton(window, btnR, ">", font, 14, false, btnR.contains(mousePos), false, 1.0f);
        };

    auto drawDropdownButton = [&](float x, float y, float w, const std::string& label, const std::string& currentVal, bool isOpen) -> sf::FloatRect {
        sf::FloatRect rowRect(x, y, w, 48.f);
        WisdomUI::Theme::DrawCrispText(window, font, label, 17, x + 16.f, y + 14.f, WisdomUI::Theme::TextPrimary);

        sf::FloatRect dropBtn(x + w - 240.f, y + 6.f, 224.f, 36.f);
        WisdomUI::Theme::DrawSunsetButton(window, dropBtn, currentVal + "  " + (isOpen ? "^" : "v"), font, 14, false, dropBtn.contains(mousePos), isOpen, 1.0f);
        return dropBtn;
        };

    float cardW = 710.f;
    float cardH = 410.f;
    float rowW = cardW - 32.f;
    float c1X = container.left + 32.f;
    float c2X = container.left + 778.f;
    float r1Y = container.top + 104.f;
    float r2Y = container.top + 530.f;

    drawCard(sf::FloatRect(c1X, r1Y, cardW, cardH), ":: DISPLAY & GRAPHICS INTERFACE ::");
    drawToggle(c1X + 16.f, r1Y + 60.f, rowW, "Fullscreen Mode", uiFullscreen);
    drawToggle(c1X + 16.f, r1Y + 120.f, rowW, "Vertical Sync (VSync)", uiVsync);
    drawStepper(c1X + 16.f, r1Y + 180.f, rowW, "FPS Target Limit", std::to_string(uiFpsLimit));

    std::string resStr = std::to_string(g_resW) + " x " + std::to_string(g_resH);
    std::vector<std::string> resOpts = { "1280 x 720", "1600 x 900", "1920 x 1080" };
    sf::FloatRect resDropBtnRect = drawDropdownButton(c1X + 16.f, r1Y + 240.f, rowW, "Display Resolution", resStr, g_resDropdownOpen);
    drawStepper(c1X + 16.f, r1Y + 300.f, rowW, "UI Palette Theme", "Sunset Arcade");

    drawCard(sf::FloatRect(c2X, r1Y, cardW, cardH), ":: ARCHIVE PERSISTENCE & AUTO-SAVING ::");
    drawToggle(c2X + 16.f, r1Y + 60.f, rowW, "Enable Auto-Backup Vault", uiAutoBackup);
    drawStepper(c2X + 16.f, r1Y + 120.f, rowW, "Autosave Frequency", "5 Mins");
    drawStepper(c2X + 16.f, r1Y + 180.f, rowW, "Default Vault Directory", "/Projects");
    drawStepper(c2X + 16.f, r1Y + 240.f, rowW, "Export Output Format", "PNG / Sheet");

    drawCard(sf::FloatRect(c1X, r2Y, cardW, cardH), ":: CANVASES & TIMELINE MEMORY ::");
    drawToggle(c1X + 16.f, r2Y + 60.f, rowW, "Hardware GPU Acceleration", uiHwAccel);
    drawStepper(c1X + 16.f, r2Y + 120.f, rowW, "Animation Preview Rate", std::to_string(uiAnimFps) + " FPS");
    drawStepper(c1X + 16.f, r2Y + 180.f, rowW, "Undo Stack History Size", std::to_string(uiHistorySize) + " Steps");
    drawStepper(c1X + 16.f, r2Y + 240.f, rowW, "Pixel Grid Contrast", "High");

    drawCard(sf::FloatRect(c2X, r2Y, cardW, cardH), ":: AI GENERATION CORE MATRIX ::");
    drawStepper(c2X + 16.f, r2Y + 60.f, rowW, "Active AI Provider", AIManager::getInstance().getActiveProvider());

    std::string keyDisplay = AIManager::getInstance().getApiKey(AIManager::getInstance().getActiveProvider());
    if (keyDisplay.empty()) keyDisplay = "Click to enter token key...";
    else keyDisplay = std::string(std::min(static_cast<size_t>(16), keyDisplay.length()), '*');
    if (g_typingApiKey) keyDisplay += "_";

    sf::FloatRect keyRow(c2X + 16.f, r2Y + 120.f, rowW, 48.f);
    WisdomUI::Theme::DrawCrispText(window, font, "API Access Token", 17, keyRow.left + 16.f, keyRow.top + 14.f, WisdomUI::Theme::TextPrimary);

    sf::FloatRect keyBox(c2X + 16.f + rowW - 320.f, keyRow.top + 6.f, 304.f, 36.f);
    sf::RectangleShape kBox(sf::Vector2f(keyBox.width, keyBox.height));
    kBox.setPosition(keyBox.left, keyBox.top);
    kBox.setFillColor(sf::Color(14, 6, 20));
    kBox.setOutlineThickness(1.5f);
    kBox.setOutlineColor(g_typingApiKey ? WisdomUI::Theme::SunsetGold : WisdomUI::Theme::SunsetPlum);
    window.draw(kBox);

    WisdomUI::Theme::DrawCrispText(window, font, keyDisplay, 14, keyBox.left + 12.f, keyBox.top + 9.f, g_typingApiKey ? WisdomUI::Theme::SunsetGold : WisdomUI::Theme::SunsetPeach);

    if (g_resDropdownOpen) {
        sf::FloatRect menu(resDropBtnRect.left, resDropBtnRect.top + resDropBtnRect.height + 4.f, resDropBtnRect.width, static_cast<float>(resOpts.size()) * 36.f);

        sf::RectangleShape shadow(sf::Vector2f(menu.width + 6.f, menu.height + 6.f));
        shadow.setPosition(menu.left - 3.f, menu.top - 3.f);
        shadow.setFillColor(sf::Color(0, 0, 0, 160));
        window.draw(shadow);

        sf::RectangleShape mBg(sf::Vector2f(menu.width, menu.height));
        mBg.setPosition(menu.left, menu.top);
        mBg.setFillColor(sf::Color(14, 6, 20, 250));
        mBg.setOutlineThickness(1.5f);
        mBg.setOutlineColor(WisdomUI::Theme::SunsetGold);
        window.draw(mBg);

        for (size_t i = 0; i < resOpts.size(); ++i) {
            sf::FloatRect optRect(menu.left, menu.top + static_cast<float>(i) * 36.f, menu.width, 36.f);
            bool hovOpt = optRect.contains(mousePos);
            if (hovOpt) {
                sf::RectangleShape hBox(sf::Vector2f(optRect.width, optRect.height));
                hBox.setPosition(optRect.left, optRect.top);
                hBox.setFillColor(WisdomUI::Theme::SunsetSkyMid);
                window.draw(hBox);
            }
            WisdomUI::Theme::DrawCrispText(window, font, resOpts[i], 14, optRect.left + 14.f, optRect.top + 9.f, hovOpt ? WisdomUI::Theme::SunsetGold : WisdomUI::Theme::TextPrimary);
        }
    }
}

void UIManager::drawTutorialsMenu(sf::RenderWindow& window) {
    sf::Vector2i mousePosI = sf::Mouse::getPosition(window);
    sf::Vector2f mousePos = window.mapPixelToCoords(mousePosI);

    sf::FloatRect container(200.f, 60.f, 1520.f, 960.f);
    WisdomUI::Theme::DrawSunsetPanel(window, container, 1.0f);

    sf::FloatRect backBtn(container.left + 28.f, container.top + 22.f, 140.f, 44.f);
    WisdomUI::Theme::DrawSunsetButton(window, backBtn, "< BACK", font, 16, false, backBtn.contains(mousePos), false, 1.0f);

    WisdomUI::Theme::DrawCrispText(window, font, "KNOWLEDGE CODEX & STUDIO MANUAL", 28, container.left + 190.f, container.top + 20.f, WisdomUI::Theme::SunsetGold, sf::Color(14, 6, 20));
    WisdomUI::Theme::DrawCrispText(window, font, "INTERACTIVE ANIMATION TECHNIQUES, SHORTCUTS & TOOLS", 14, container.left + 192.f, container.top + 54.f, WisdomUI::Theme::TextSecondary);

    sf::RectangleShape div(sf::Vector2f(container.width - 56.f, 2.f));
    div.setPosition(container.left + 28.f, container.top + 86.f);
    div.setFillColor(WisdomUI::Theme::SunsetPlum);
    window.draw(div);

    std::vector<std::pair<std::string, std::string>> topics = {
        {"Getting Started", "Learn the basics of canvas initialization, tool pallets and navigation."},
        {"Drawing & Inking", "Master the smooth brush, retro pixel pencil, eraser and symmetry."},
        {"Timeline & Frames", "Understand frame duplication, timing onion skinning and loop playback."},
        {"Layer Management", "Organize scenes with blend modes, opacity sliders and frame locking."},
        {"Selection & Lasso", "Transform, isolate, rotate, scale, flip and duplicate selection art."},
        {"Pixel Perfect Mode", "Strip jagged pixel doublets and draw smooth authentic retro lines."},
        {"Keyboard Matrix", "Speed up your animation speed with custom hotkeys and tool bindings."},
        {"Exporting Studio", "Render sequential PNG animation sequences and packed sprite sheets."},
        {"AI Co-Pilot Suite", "Generate sprite variations, fill textures and inpaint via cloud AI."}
    };

    std::vector<std::string> fullText = {
        "Wisdom Park is designed for high-speed professional 2D animation and pixel art.\n\n"
        "Start by creating a new project from the main launchpad or press Ctrl+N.\n"
        "Use the left toolbar for drawing tools, and open the bottom timeline (Space/Tab)\n"
        "to manage and preview animation sequences seamlessly.",

        "Select the Brush or Pencil tool (B or P).\n\n"
        "- Brush: Smooth anti-aliased strokes with dynamic radius scaling.\n"
        "- Pencil: Strict grid-locked pixels ideal for retro sprite craft.\n"
        "- Right-Click / Middle-Click: Hold and drag to pan the viewport smoothly.",

        "The Timeline bar manages individual drawing frames.\n\n"
        "- Click '+' or press Shift+N to duplicate/add a new frame.\n"
        "- Press Space to toggle real-time animation playback.\n"
        "- Enable Onion Skinning (O) to render adjacent frames as reference silhouettes.",

        "Layers isolate independent animation components.\n\n"
        "- Use the Layer Panel on the right dock to add, reorder or merge layers.\n"
        "- Toggle layer visibility or lock layers to prevent accidental strokes.\n"
        "- Adjust individual opacity sliders for realistic shading and lighting.",

        "Use the Select Tool (S) or Magic Wand (W) to isolate artwork.\n\n"
        "- Once selected, drag pixels anywhere across the canvas.\n"
        "- Press H or V to flip selections horizontally or vertically.\n"
        "- Press Delete to wipe the selected pixels instantly.",

        "Pixel Mode disables smoothing and enforces strict tile alignments.\n\n"
        "- Enable 'Pixel Perfect' in the Tool Options bar to automatically clean\n"
        "  up redundant double-corner pixels on fast strokes.\n"
        "- Use Tile Mode to test seamlessly repeating environment textures.",

        "Every major studio action has a dedicated shortcut.\n\n"
        "- Press Tab or click 'Keybinds' on the main launchpad to rebind them.\n"
        "- Defaults: B (Brush), P (Pencil), E (Eraser), G (Gradient), Ctrl+Z (Undo),\n"
        "  Ctrl+S (Save), F8 (Sprite Sheet Studio).",

        "When your timeline animation is complete, open Export Studio (Ctrl+E).\n\n"
        "- Output transparent individual PNG sequences for video editors.\n"
        "- Pack the entire timeline into compact sprite sheets ready for Unity,\n"
        "  Godot, or Unreal Engine.",

        "AI Co-Pilot features require an active API key in Settings.\n\n"
        "- Isolate an area using the Selection tool.\n"
        "- Type a contextual prompt in the AI Generator panel.\n"
        "- The AI will populate textures or variations matched to your theme."
    };

    if (activeTutorialIndex == -1) {
        float startX = container.left + 36.f;
        float startY = container.top + 110.f;
        float cardW = (container.width - 108.f) / 3.f;
        float cardH = 260.f;

        for (size_t i = 0; i < topics.size(); ++i) {
            float col = static_cast<float>(i % 3);
            float row = static_cast<float>(i / 3);
            float cx = startX + col * (cardW + 18.f);
            float cy = startY + row * (cardH + 18.f);

            sf::FloatRect cardRect(cx, cy, cardW, cardH);
            bool isHov = cardRect.contains(mousePos);

            sf::RectangleShape card(sf::Vector2f(cardRect.width, cardRect.height));
            card.setPosition(cardRect.left, cardRect.top);
            card.setFillColor(isHov ? WisdomUI::Theme::SunsetSkyMid : WisdomUI::Theme::SunsetDeepDark);
            card.setOutlineThickness(1.5f);
            card.setOutlineColor(isHov ? WisdomUI::Theme::SunsetGold : WisdomUI::Theme::SunsetPlum);
            window.draw(card);

            sf::FloatRect badge(cx + 20.f, cy + 18.f, 96.f, 26.f);
            sf::RectangleShape bBg(sf::Vector2f(badge.width, badge.height));
            bBg.setPosition(badge.left, badge.top);
            bBg.setFillColor(sf::Color(14, 6, 20));
            bBg.setOutlineThickness(1.f);
            bBg.setOutlineColor(WisdomUI::Theme::SunsetAmber);
            window.draw(bBg);

            WisdomUI::Theme::DrawCrispText(window, font, "PART 0" + std::to_string(i + 1), 12, badge.left + badge.width / 2.f, badge.top + badge.height / 2.f, WisdomUI::Theme::SunsetAmber, sf::Color::Transparent, true, true);

            WisdomUI::Theme::DrawCrispText(window, font, topics[i].first, 19, cx + 20.f, cy + 56.f, isHov ? WisdomUI::Theme::SunsetGold : WisdomUI::Theme::TextPrimary, sf::Color(14, 6, 20));

            sf::Text descTxt(topics[i].second, font, 14);
            descTxt.setPosition(cx + 20.f, cy + 96.f);
            descTxt.setFillColor(WisdomUI::Theme::TextSecondary);
            descTxt.setLineSpacing(1.45f);
            window.draw(descTxt);

            WisdomUI::Theme::DrawCrispText(window, font, "READ GUIDE >>", 14, cx + 20.f, cy + cardH - 34.f, isHov ? WisdomUI::Theme::SunsetGold : WisdomUI::Theme::SunsetPeach);
        }
    }
    else {
        sf::FloatRect contentCard(container.left + 36.f, container.top + 110.f, container.width - 72.f, container.height - 146.f);
        sf::RectangleShape cBg(sf::Vector2f(contentCard.width, contentCard.height));
        cBg.setPosition(contentCard.left, contentCard.top);
        cBg.setFillColor(WisdomUI::Theme::SunsetDeepDark);
        cBg.setOutlineThickness(1.5f);
        cBg.setOutlineColor(WisdomUI::Theme::SunsetPlum);
        window.draw(cBg);

        WisdomUI::Theme::DrawCrispText(window, font, "CHAPTER 0" + std::to_string(activeTutorialIndex + 1) + " : " + topics[activeTutorialIndex].first, 26, contentCard.left + 36.f, contentCard.top + 32.f, WisdomUI::Theme::SunsetGold, sf::Color(14, 6, 20));

        sf::RectangleShape cDiv(sf::Vector2f(contentCard.width - 72.f, 2.f));
        cDiv.setPosition(contentCard.left + 36.f, contentCard.top + 78.f);
        cDiv.setFillColor(WisdomUI::Theme::SunsetPlum);
        window.draw(cDiv);

        sf::Text mainTxt(fullText[activeTutorialIndex], font, 18);
        mainTxt.setPosition(contentCard.left + 36.f, contentCard.top + 108.f);
        mainTxt.setFillColor(WisdomUI::Theme::TextPrimary);
        mainTxt.setLineSpacing(1.65f);
        window.draw(mainTxt);

        sf::FloatRect returnBtn(contentCard.left + contentCard.width - 260.f, contentCard.top + contentCard.height - 72.f, 224.f, 50.f);
        bool retHov = returnBtn.contains(mousePos);
        WisdomUI::Theme::DrawSunsetButton(window, returnBtn, "<< Topics Codex", font, 15, false, retHov, true, 1.0f);
    }
}

void UIManager::drawCreditsMenu(sf::RenderWindow& window) {
    sf::Vector2i mousePosI = sf::Mouse::getPosition(window);
    sf::Vector2f mousePos = window.mapPixelToCoords(mousePosI);

    sf::FloatRect container(320.f, 80.f, 1280.f, 920.f);
    WisdomUI::Theme::DrawSunsetPanel(window, container, 1.0f);

    sf::FloatRect backBtn(container.left + 28.f, container.top + 22.f, 140.f, 44.f);
    WisdomUI::Theme::DrawSunsetButton(window, backBtn, "< BACK", font, 16, false, backBtn.contains(mousePos), false, 1.0f);

    WisdomUI::Theme::DrawCrispText(window, font, "HALL OF FAME & STUDIO CREDITS", 28, container.left + 190.f, container.top + 20.f, WisdomUI::Theme::SunsetGold, sf::Color(14, 6, 20));
    WisdomUI::Theme::DrawCrispText(window, font, "CORE ARCHITECTURE, CONTRIBUTIONS & ACADEMIC ROLL", 14, container.left + 192.f, container.top + 54.f, WisdomUI::Theme::TextSecondary);

    sf::RectangleShape div(sf::Vector2f(container.width - 56.f, 2.f));
    div.setPosition(container.left + 28.f, container.top + 88.f);
    div.setFillColor(WisdomUI::Theme::SunsetPlum);
    window.draw(div);

    auto drawCreditBlock = [&](float x, float y, float w, float h, const std::string& title, const std::string& content) {
        sf::RectangleShape b(sf::Vector2f(w, h));
        b.setPosition(x, y);
        b.setFillColor(WisdomUI::Theme::SunsetDeepDark);
        b.setOutlineThickness(1.5f);
        b.setOutlineColor(WisdomUI::Theme::SunsetPlum);
        window.draw(b);

        sf::FloatRect headerGrip(x + 14.f, y + 12.f, w - 28.f, 34.f);
        sf::RectangleShape hBg(sf::Vector2f(headerGrip.width, headerGrip.height));
        hBg.setPosition(headerGrip.left, headerGrip.top);
        hBg.setFillColor(WisdomUI::Theme::SunsetSkyTop);
        hBg.setOutlineThickness(1.f);
        hBg.setOutlineColor(WisdomUI::Theme::SunsetPlum);
        window.draw(hBg);

        WisdomUI::Theme::DrawCrispText(window, font, title, 15, headerGrip.left + 14.f, headerGrip.top + 8.f, WisdomUI::Theme::SunsetAmber);

        sf::Text cTxt(content, font, 17);
        cTxt.setPosition(x + 22.f, y + 60.f);
        cTxt.setFillColor(WisdomUI::Theme::TextPrimary);
        cTxt.setLineSpacing(1.55f);
        window.draw(cTxt);
        };

    float bx = container.left + 32.f;
    float by = container.top + 106.f;
    float colW = 590.f;

    drawCreditBlock(bx, by, colW, 230.f, ":: LEAD DEVELOPER & ARCHITECT ::",
        "Ahmad Arnaoute (AtodDev)\n"
        "Role: Engine & UI Architecture, Tool Systems\n"
        "Specialization: High-Performance Systems & Pixel Pipeline");

    drawCreditBlock(bx + colW + 36.f, by, colW, 230.f, ":: ACADEMIC AFFILIATION ::",
        "POLITEHNICA University of Bucharest\n"
        "Faculty of Automatic Control and Computers\n"
        "Group: 324CD\n"
        "Bucharest, Romania");

    drawCreditBlock(bx, by + 252.f, colW, 310.f, ":: PROJECTS & OPEN SOURCE CONTRIBUTIONS ::",
        "- Oppia Foundation (Backend Testing Infrastructure)\n"
        "- safe-comment-stripper (Open Source Utility)\n"
        "- iMeditatii Education Platform\n"
        "- AtodDev's Progress & Progress Aggregator\n"
        "- Wisdom Park Retro Animation Suite");

    drawCreditBlock(bx + colW + 36.f, by + 252.f, colW, 310.f, ":: CORE TECHNOLOGY STACK ::",
        "- SFML 2.6.x (Graphics, Window, Systems)\n"
        "- nlohmann::json (Structured Serialization)\n"
        "- C++17 Standard Compliant Architecture\n"
        "- Commercial & Open Studio License");

    sf::FloatRect eggPod(bx, by + 584.f, container.width - 64.f, 90.f);
    bool eggHov = eggPod.contains(mousePos);

    sf::RectangleShape eggBg(sf::Vector2f(eggPod.width, eggPod.height));
    eggBg.setPosition(eggPod.left, eggPod.top);
    eggBg.setFillColor(eggHov ? WisdomUI::Theme::SunsetSkyMid : WisdomUI::Theme::SunsetDeepDark);
    eggBg.setOutlineThickness(1.5f);
    eggBg.setOutlineColor(easterEggClicks >= 5 ? WisdomUI::Theme::SunsetGold : (eggHov ? WisdomUI::Theme::SunsetAmber : WisdomUI::Theme::SunsetPlum));
    window.draw(eggBg);

    std::string coreStatus = (easterEggClicks >= 5) ? "ENGINE CORE MATRIX UNLOCKED (DEBUG MODE ACTIVE)" : "WISDOM PARK STUDIO SYSTEM CORE (CLICK TO CALIBRATE)";
    WisdomUI::Theme::DrawCrispText(window, font, coreStatus, 16, eggPod.left + eggPod.width / 2.f, eggPod.top + 26.f, easterEggClicks >= 5 ? WisdomUI::Theme::SunsetGold : WisdomUI::Theme::SunsetAmber, sf::Color(14, 6, 20), true, true);
    WisdomUI::Theme::DrawCrispText(window, font, "Build: 2026.8  |  Direct Hardware Render Pipeline Online", 13, eggPod.left + eggPod.width / 2.f, eggPod.top + 56.f, WisdomUI::Theme::TextSecondary, sf::Color::Transparent, true, true);
}

bool UIManager::triggerSave(Canvas& canvas, Timeline& timeline) {
    if (activeProjectPath.empty()) {
        std::string file = NativeDialogs::saveFileDialog("Wisdom Park Projects\0*.wpk\0", "wpk", activeProjectName);
        if (!file.empty()) {
            activeProjectPath = file;
            if (projManager->saveProjectAs(activeProjectPath, activeProjectName, canvas, static_cast<int>(timeline.getFps()), canvas.getPixelMode())) {
                canvas.clearIsDirty();
                return true;
            }
        }
        return false;
    }
    else {
        if (projManager->saveProjectAs(activeProjectPath, activeProjectName, canvas, static_cast<int>(timeline.getFps()), canvas.getPixelMode())) {
            canvas.clearIsDirty();
            return true;
        }
        return false;
    }
}

void UIManager::handleEvent(const sf::Event& event, sf::RenderWindow& window, AppState& currentState, AppSettings& settings, Canvas& canvas, Timeline& timeline, AIHelper& aiHelper, ProjectManager& pm) {
    if (event.type == sf::Event::Resized) {
        window.setView(WisdomUI::WorkspaceLayout::GetLetterboxView(sf::Vector2u(event.size.width, event.size.height)));
    }

    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::F11) {
        toggleFullscreen(window, settings);
        return;
    }

    if (AIManager::getInstance().isProcessingAsync()) {
        auto killAiProcess = [&]() {
            AIManager::getInstance().abortTask();
#if defined(_WIN32)
            std::system("taskkill /IM python.exe /F /T >nul 2>nul");
            std::system("taskkill /IM py.exe /F /T >nul 2>nul");
            std::system("taskkill /IM python3.exe /F /T >nul 2>nul");
#else
            std::system("pkill -f run_ai.py");
#endif
            std::ofstream f("temp_ai_output.png");
            f.close();
            };

        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
            killAiProcess();
            return;
        }

        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2i mPos = sf::Mouse::getPosition(window);
            sf::Vector2f viewPos = window.mapPixelToCoords(mPos, window.getDefaultView());

            if (loadingCancelBtn.getGlobalBounds().contains(viewPos) ||
                (viewPos.x > 700.f && viewPos.x < 1220.f && viewPos.y > 450.f && viewPos.y < 750.f)) {
                killAiProcess();
            }
            return;
        }
        return;
    }

    window.setView(WisdomUI::WorkspaceLayout::GetLetterboxView(window.getSize()));
    sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
    if (event.type == sf::Event::MouseButtonPressed || event.type == sf::Event::MouseButtonReleased) {
        pixelPos = sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
    }
    sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);
    sf::Vector2f logicalMousePos = canvas.getInverseTransform().transformPoint(mousePos);

    if (showUnsavedWarning) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            float boxWidth = 450.f;
            float boxHeight = 200.f;
            float cx = (1920.f - boxWidth) / 2.f;
            float cy = (1080.f - boxHeight) / 2.f;

            sf::FloatRect saveBounds(cx + 30.f, cy + 120.f, 110.f, 40.f);
            sf::FloatRect discardBounds(cx + 170.f, cy + 120.f, 110.f, 40.f);
            sf::FloatRect cancelBounds(cx + 310.f, cy + 120.f, 110.f, 40.f);

            if (saveBounds.contains(mousePos)) {
                if (triggerSave(canvas, timeline)) {
                    showUnsavedWarning = false;
                    currentState = AppState::Welcome;
                    currentMenuState = MenuState::Main;
                }
                else {
                    showMessage("Error Saving Project!", sf::Color::Red);
                }
            }
            else if (discardBounds.contains(mousePos)) {
                showUnsavedWarning = false;
                currentState = AppState::Welcome;
                currentMenuState = MenuState::Main;
            }
            else if (cancelBounds.contains(mousePos)) {
                showUnsavedWarning = false;
            }
        }
        return;
    }

    if (keybindPanel.isVisible()) {
        keybindPanel.handleEvent(event);
        return;
    }

    if (exportModal.getIsOpen()) {
        exportModal.handleEvent(event, window);
        return;
    }

    if (newProjectModal.getIsOpen()) {
        std::string res = newProjectModal.handleEvent(event, window);
        if (res == "create") {
            activeProjectName = newProjectModal.getProjectName();
            if (activeProjectName.empty()) {
                activeProjectName = (newProjectModal.getIsPixelMode() ? "Pixel_Art_" : "New_Project_") + std::to_string(static_cast<long long>(std::time(nullptr)));
            }
            activeProjectPath = "";
            canvas.setPixelMode(newProjectModal.getIsPixelMode());
            pm.createNewProject(activeProjectName, newProjectModal.getWidth(), newProjectModal.getHeight(), 12, newProjectModal.getIsPixelMode(), canvas);
            canvas.clearIsDirty();
            currentState = AppState::Painting;
            showMessage("Created Project: " + std::to_string(newProjectModal.getWidth()) + "x" + std::to_string(newProjectModal.getHeight()), sf::Color::Green);
        }
        else if (res == "cancel") {
            newProjectModal.close();
        }
        return;
    }

    if (currentState == AppState::Welcome) {
        if (m_showKeybinds) {
            handleKeybindModalEvent(event, window);
            return;
        }

        if (currentMenuState == MenuState::Main) {
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                if (m_welcomeModeToggleBounds.contains(mousePos)) {
                    m_useMinigameWelcome = !m_useMinigameWelcome;
                    if (m_useMinigameWelcome) {
                        initMinigame();
                    }
                    return;
                }

                if (m_startMenuFullscreenBtnBounds.contains(mousePos)) {
                    toggleFullscreen(window, settings);
                    return;
                }

                if (m_useMinigameWelcome) {
                    for (const auto& portal : m_arcadePortals) {
                        if (portal.bounds.contains(mousePos)) {
                            if (portal.id == "keybinds") {
                                m_showKeybinds = true;
                                return;
                            }
                            triggerArcadeStation(portal.id, window);
                            return;
                        }
                    }
                }
                else {
                    float startX = 440.f + 24.f;
                    float startY = 145.f + 66.f;
                    float btnW = 1040.f - 48.f;
                    float btnH = 80.f;
                    float spacing = 14.f;

                    for (int i = 0; i < 7; ++i) {
                        sf::FloatRect cardRect(startX, startY + static_cast<float>(i) * (btnH + spacing), btnW, btnH);
                        if (cardRect.contains(mousePos)) {
                            if (i == 0) newProjectModal.open();
                            else if (i == 1) currentMenuState = MenuState::Projects;
                            else if (i == 2) currentMenuState = MenuState::Settings;
                            else if (i == 3) { currentMenuState = MenuState::Tutorials; activeTutorialIndex = -1; }
                            else if (i == 4) m_showKeybinds = true;
                            else if (i == 5) { currentMenuState = MenuState::Credits; easterEggClicks = 0; }
                            else if (i == 6) window.close();
                            return;
                        }
                    }
                }
            }
            else if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::K) {
                    m_showKeybinds = true;
                    return;
                }
                if (m_useMinigameWelcome) {
                    if (event.key.code == sf::Keyboard::Space || (event.key.code == sf::Keyboard::N && event.key.control)) {
                        triggerArcadeStation("new_project", window);
                        return;
                    }
                    if (event.key.code == sf::Keyboard::O) {
                        triggerArcadeStation("projects", window);
                        return;
                    }
                    if (event.key.code == sf::Keyboard::Escape) {
                        triggerArcadeStation("settings", window);
                        return;
                    }
                    if (event.key.code == sf::Keyboard::F1) {
                        triggerArcadeStation("tutorials", window);
                        return;
                    }
                    if (event.key.code == sf::Keyboard::C) {
                        triggerArcadeStation("credits", window);
                        return;
                    }
                }
            }
        }
        else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            if (currentMenuState == MenuState::Projects) {
                ProjectMetadata meta;
                std::string action = projectBrowser.handleClick(mousePos, meta);
                if (action == "back") {
                    currentMenuState = MenuState::Main;
                    return;
                }
                else if (action == "new_project") {
                    newProjectModal.open();
                }
                else if (action == "load_project") {
                    activeProjectName = meta.name;
                    activeProjectPath = meta.path;
                    int loadedFps = 12;
                    bool isPix = false;
                    if (pm.loadProject(meta.path, canvas, loadedFps, isPix)) {
                        timeline.setFrame(0);
                        canvas.clearIsDirty();
                        currentState = AppState::Painting;
                        showMessage("Loaded Project: " + meta.name, sf::Color::Green);
                    }
                    else {
                        showMessage("Failed to load project files.", sf::Color::Red);
                    }
                }
                else if (action == "open_native") {
                    std::string file = NativeDialogs::openFileDialog("Wisdom Park Projects\0*.wpk\0All Files\0*.*\0");
                    if (!file.empty()) {
                        activeProjectPath = file;
                        activeProjectName = std::filesystem::path(file).stem().string();
                        int loadedFps = 12;
                        bool isPix = false;
                        if (pm.loadProject(activeProjectPath, canvas, loadedFps, isPix)) {
                            timeline.setFrame(0);
                            canvas.clearIsDirty();
                            currentState = AppState::Painting;
                            showMessage("Loaded Native Project", sf::Color::Green);
                        }
                        else {
                            showMessage("Failed to load native project.", sf::Color::Red);
                        }
                    }
                }
            }
            else if (currentMenuState == MenuState::Settings) {
                sf::FloatRect backBtn(228.f, 82.f, 140.f, 44.f);
                if (backBtn.contains(mousePos)) {
                    currentMenuState = MenuState::Main;
                    return;
                }

                float cardW = 710.f;
                float rowW = cardW - 32.f;
                float c1X = 232.f;
                float c2X = 978.f;
                float r1Y = 164.f;
                float r2Y = 590.f;

                auto checkToggle = [&](float x, float y) { return sf::FloatRect(x, y, rowW, 48.f).contains(mousePos); };
                auto checkStepperL = [&](float x, float y) { return sf::FloatRect(x + rowW - 240.f, y + 6.f, 40.f, 36.f).contains(mousePos); };
                auto checkStepperR = [&](float x, float y) { return sf::FloatRect(x + rowW - 56.f, y + 6.f, 40.f, 36.f).contains(mousePos); };

                bool displayChanged = false;

                sf::FloatRect dropBtn(c1X + 16.f + rowW - 240.f, r1Y + 240.f + 6.f, 224.f, 36.f);

                if (g_resDropdownOpen) {
                    sf::FloatRect dropMenu(dropBtn.left, dropBtn.top + dropBtn.height + 4.f, dropBtn.width, 3 * 36.f);
                    if (dropMenu.contains(mousePos)) {
                        int index = static_cast<int>(mousePos.y - dropMenu.top) / 36;
                        if (index == 0) { settings.resWidth = 1280; settings.resHeight = 720; }
                        else if (index == 1) { settings.resWidth = 1600; settings.resHeight = 900; }
                        else if (index == 2) { settings.resWidth = 1920; settings.resHeight = 1080; }

                        g_resW = settings.resWidth;
                        g_resH = settings.resHeight;
                        displayChanged = true;
                    }
                    g_resDropdownOpen = false;
                }
                else if (dropBtn.contains(mousePos)) {
                    g_resDropdownOpen = true;
                }
                else {
                    if (checkToggle(c1X + 16.f, r1Y + 60.f)) {
                        toggleFullscreen(window, settings);
                    }

                    if (checkToggle(c1X + 16.f, r1Y + 120.f)) {
                        uiVsync = !uiVsync;
                        settings.vsync = uiVsync;
                        window.setVerticalSyncEnabled(uiVsync);
                    }

                    if (checkStepperL(c1X + 16.f, r1Y + 180.f)) { uiFpsLimit = (uiFpsLimit == 60) ? 240 : ((uiFpsLimit == 144) ? 60 : 144); settings.fpsLimit = uiFpsLimit; window.setFramerateLimit(uiFpsLimit); }
                    if (checkStepperR(c1X + 16.f, r1Y + 180.f)) { uiFpsLimit = (uiFpsLimit == 60) ? 144 : ((uiFpsLimit == 144) ? 240 : 60); settings.fpsLimit = uiFpsLimit; window.setFramerateLimit(uiFpsLimit); }

                    if (checkToggle(c2X + 16.f, r1Y + 60.f)) { uiAutoBackup = !uiAutoBackup; settings.autoBackup = uiAutoBackup; }
                    if (checkStepperL(c2X + 16.f, r1Y + 120.f)) {}
                    if (checkStepperR(c2X + 16.f, r1Y + 120.f)) {}

                    if (checkToggle(c1X + 16.f, r2Y + 60.f)) { uiHwAccel = !uiHwAccel; settings.hwAccel = uiHwAccel; }

                    if (checkStepperL(c1X + 16.f, r2Y + 120.f)) { uiAnimFps = (uiAnimFps == 12) ? 60 : ((uiAnimFps == 24) ? 12 : 24); settings.animFps = uiAnimFps; }
                    if (checkStepperR(c1X + 16.f, r2Y + 120.f)) { uiAnimFps = (uiAnimFps == 12) ? 24 : ((uiAnimFps == 24) ? 60 : 12); settings.animFps = uiAnimFps; }

                    if (checkStepperL(c1X + 16.f, r2Y + 180.f)) { uiHistorySize = (uiHistorySize == 15) ? 50 : ((uiHistorySize == 30) ? 15 : 30); settings.historySize = uiHistorySize; }
                    if (checkStepperR(c1X + 16.f, r2Y + 180.f)) { uiHistorySize = (uiHistorySize == 15) ? 30 : ((uiHistorySize == 30) ? 50 : 15); settings.historySize = uiHistorySize; }

                    if (checkStepperL(c2X + 16.f, r2Y + 60.f)) AIManager::getInstance().cycleProvider(-1);
                    if (checkStepperR(c2X + 16.f, r2Y + 60.f)) AIManager::getInstance().cycleProvider(1);

                    sf::FloatRect keyInputBox(c2X + 16.f + rowW - 320.f, r2Y + 120.f + 6.f, 304.f, 36.f);
                    if (keyInputBox.contains(mousePos)) {
                        g_typingApiKey = true;
                    }
                    else if (g_typingApiKey) {
                        g_typingApiKey = false;
                        AIManager::getInstance().saveSettingsLocally();
                        showMessage("AI Configurations Applied and Saved", sf::Color::Green);
                    }
                }

                SettingsManager::saveSettings(settings);

                if (displayChanged) {
                    if (uiFullscreen) {
                        window.create(sf::VideoMode::getDesktopMode(), "Wisdom Park", sf::Style::Fullscreen);
                    }
                    else {
                        window.create(sf::VideoMode(settings.resWidth, settings.resHeight), "Wisdom Park", sf::Style::Default);
                        sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
                        window.setPosition(sf::Vector2i(
                            std::max(0, static_cast<int>((desktop.width - settings.resWidth) / 2)),
                            std::max(0, static_cast<int>((desktop.height - settings.resHeight) / 2))
                        ));
                    }

                    ApplyWindowIcon(window);

                    window.setFramerateLimit(uiFpsLimit);
                    window.setVerticalSyncEnabled(uiVsync);
                    window.setView(WisdomUI::WorkspaceLayout::GetLetterboxView(window.getSize()));
                }
            }
            else if (currentMenuState == MenuState::Tutorials) {
                sf::FloatRect backBtn(228.f, 82.f, 140.f, 44.f);
                if (backBtn.contains(mousePos)) {
                    if (activeTutorialIndex != -1) activeTutorialIndex = -1;
                    else currentMenuState = MenuState::Main;
                    return;
                }

                if (activeTutorialIndex == -1) {
                    float startX = 236.f;
                    float startY = 170.f;
                    float cardW = (1520.f - 108.f) / 3.f;
                    float cardH = 260.f;

                    for (int i = 0; i < 9; ++i) {
                        float col = static_cast<float>(i % 3);
                        float row = static_cast<float>(i / 3);
                        float cx = startX + col * (cardW + 18.f);
                        float cy = startY + row * (cardH + 18.f);

                        sf::FloatRect bounds(cx, cy, cardW, cardH);
                        if (bounds.contains(mousePos)) {
                            activeTutorialIndex = i;
                            return;
                        }
                    }
                }
                else {
                    sf::FloatRect returnBtn(1424.f, 912.f, 224.f, 50.f);
                    if (returnBtn.contains(mousePos)) {
                        activeTutorialIndex = -1;
                        return;
                    }
                }
            }
            else if (currentMenuState == MenuState::Credits) {
                sf::FloatRect backBtn(348.f, 102.f, 140.f, 44.f);
                if (backBtn.contains(mousePos)) {
                    currentMenuState = MenuState::Main;
                    return;
                }

                sf::FloatRect eggPod(352.f, 770.f, 1216.f, 90.f);
                if (eggPod.contains(mousePos)) {
                    easterEggClicks++;
                }
            }
        }

        if (currentMenuState == MenuState::Projects && event.type == sf::Event::MouseWheelScrolled) {
            projectBrowser.handleScroll(event.mouseWheelScroll.delta);
        }

        if (currentMenuState == MenuState::Settings && g_typingApiKey) {
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Enter) {
                    g_typingApiKey = false;
                    AIManager::getInstance().saveSettingsLocally();
                    showMessage("Key Applied Successfully!", sf::Color::Green);
                    return;
                }
                if (event.key.code == sf::Keyboard::V && event.key.control) {
                    std::string clipboardData = sf::Clipboard::getString().toAnsiString();
                    clipboardData.erase(std::remove(clipboardData.begin(), clipboardData.end(), '\n'), clipboardData.end());
                    clipboardData.erase(std::remove(clipboardData.begin(), clipboardData.end(), '\r'), clipboardData.end());
                    clipboardData.erase(std::remove(clipboardData.begin(), clipboardData.end(), ' '), clipboardData.end());

                    if (!clipboardData.empty()) {
                        std::string prov = AIManager::getInstance().getActiveProvider();
                        std::string existingKey = AIManager::getInstance().getApiKey(prov);
                        AIManager::getInstance().setApiKey(prov, existingKey + clipboardData);
                        showMessage("API Key Pasted From Clipboard", sf::Color::Green);
                    }
                    return;
                }
            }

            if (event.type == sf::Event::TextEntered) {
                std::string prov = AIManager::getInstance().getActiveProvider();
                std::string k = AIManager::getInstance().getApiKey(prov);
                if (event.text.unicode == '\b' && !k.empty()) {
                    k.pop_back();
                }
                else if (event.text.unicode >= 32 && event.text.unicode < 127 && event.text.unicode != 'v' && event.text.unicode != 'V') {
                    k += static_cast<char>(event.text.unicode);
                }
                AIManager::getInstance().setApiKey(prov, k);
                return;
            }
        }

        if (keybindManager.isActionTriggered("proj_new", event)) {
            newProjectModal.open();
        }
    }
    else if (currentState == AppState::Painting) {
        if (m_showEscapeMenu) {
            if (handleEscapeMenuEvent(event, window, currentState, settings, canvas, timeline)) return;
        }

        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
            if (g_aiPanel.getIsVisible()) {
                g_aiPanel.toggle();
                return;
            }
            if (assetBrowser && assetBrowser->getIsVisible()) {
                assetBrowser->toggle();
                return;
            }
            if (audioPanel.getIsVisible()) {
                audioPanel.toggle();
                return;
            }
            if (m_activeRightTab != RightTabMode::None) {
                m_activeRightTab = RightTabMode::None;
                return;
            }

            m_showEscapeMenu = !m_showEscapeMenu;
            return;
        }

        if (m_topBar.HandleEvent(event, window)) {
            if (m_fullscreenToggleRequested) {
                m_fullscreenToggleRequested = false;
                toggleFullscreen(window, settings);
            }
            return;
        }

        if (m_fullscreenToggleRequested) {
            m_fullscreenToggleRequested = false;
            toggleFullscreen(window, settings);
            return;
        }
        if (m_toolDock.HandleEvent(event, window)) return;
        if (m_rightDockTabs.HandleEvent(event, window)) return;
        if (m_statusBar.HandleEvent(event, window)) return;

        if (m_toolOptionsBar.HandleEvent(event, window,
            [&](float sz) {
                if (canvas.getPixelMode()) canvas.setPixelBrushSize(static_cast<int>(sz));
                else canvas.setBrushSize(sz);
            },
            [&]() {
                canvas.togglePixelPerfect();
            },
            [&](const std::string& action) {
                int curFrame = static_cast<int>(timeline.getCurrentFrame());
                if (action == "flip_h") canvas.flipSelectionHorizontal(curFrame);
                else if (action == "flip_v") canvas.flipSelectionVertical(curFrame);
                else if (action == "duplicate") canvas.duplicateSelection(curFrame);
                else if (action == "crop") canvas.cropSelection(curFrame);
                else if (action == "delete") canvas.deleteSelection(curFrame);
            },
            [&]() {
                canvas.makeOutline(timeline.getCurrentFrame(), canvas.getPrimaryColor());
                showMessage("Outline Created", sf::Color::Green);
            }
        )) return;

        if (m_activeRightTab == RightTabMode::Layers) {
            if (layerPanel.handleEvent(event, mousePos, canvas, timeline.getCurrentFrame())) return;
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                std::string lpAction = layerPanel.processClick(mousePos, canvas, timeline.getCurrentFrame());
                if (!lpAction.empty()) {
                    if (lpAction == "layer_close") { m_activeRightTab = RightTabMode::None; return; }
                    if (lpAction == "layer_push") {
                        int cur = timeline.getCurrentFrame();
                        if (cur == static_cast<int>(canvas.getFrameCount()) - 1) {
                            canvas.addFrame(cur); timeline.addFrameAfter(cur);
                        }
                        canvas.pushLayerToNextFrame(cur, canvas.getActiveLayer());
                        timeline.nextFrame();
                    }
                    return;
                }
            }
        }
        else if (m_activeRightTab == RightTabMode::Palette) {
            if (colorPalettePanel.handleEvent(event, mousePos, canvas)) return;
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                std::string cpAction = colorPalettePanel.processClick(mousePos, canvas);
                if (cpAction == "color_close") { m_activeRightTab = RightTabMode::None; return; }
            }
        }
        else if (m_activeRightTab == RightTabMode::Properties) {
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                std::string rpAction = rightProperties.handleClick(mousePos);
                if (!rpAction.empty()) {
                    if (rpAction == "prop_close") { m_activeRightTab = RightTabMode::None; return; }
                    if (rpAction == "fps_up") timeline.setFps(timeline.getFps() + 1.0f);
                    else if (rpAction == "fps_down") timeline.setFps(std::max(1.0f, timeline.getFps() - 1.0f));
                    else if (rpAction == "theme_all") aiHelper.setTheme("all");
                    else if (rpAction == "theme_struct") aiHelper.setTheme("structure");
                    else if (rpAction == "theme_clutter") aiHelper.setTheme("clutter");
                    else if (rpAction == "theme_custom") aiHelper.setTheme("custom");
                    else if (rpAction == "theme_wfc") aiHelper.setTheme("wfc");
                    else if (rpAction == "toggle_light") isLightingMode = !isLightingMode;
                    else if (rpAction == "toggle_terrain") aiHelper.toggleTerrain();
                    else if (rpAction == "onion_toggle") canvas.setOnionSkin(!canvas.isOnionSkinEnabled(), canvas.getOnionSkinPrevOpacity(), canvas.getOnionSkinNextOpacity());
                    else if (rpAction == "onion_op_up") canvas.setOnionSkin(canvas.isOnionSkinEnabled(), canvas.getOnionSkinPrevOpacity() + 25.f, canvas.getOnionSkinNextOpacity() + 25.f);
                    else if (rpAction == "onion_op_down") canvas.setOnionSkin(canvas.isOnionSkinEnabled(), canvas.getOnionSkinPrevOpacity() - 25.f, canvas.getOnionSkinNextOpacity() - 25.f);
                    return;
                }
            }
        }

        if (assetBrowser && assetBrowser->getIsVisible()) {
            assetBrowser->handleEvent(event, window, canvas, timeline.getCurrentFrame());
        }

        if (audioPanel.getIsVisible()) {
            if (audioPanel.handleEvent(event, mousePos)) {
                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    std::string action = audioPanel.handleClick(mousePos, timeline.getCurrentFrame());
                    if (action == "imported") {
                        showMessage("Audio Directory Scanned Successfully", sf::Color::Green);
                    }
                }
                return;
            }
        }

        if (m_showTimeline) {
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                float timelineY = 1080.0f - WisdomUI::Theme::StatusBarHeight - WisdomUI::Theme::TimelineHeight;
                sf::FloatRect headerBounds(0.0f, timelineY, 1920.0f, 28.0f);
                sf::FloatRect playBtn(120.0f, headerBounds.top + 3.0f, 65.0f, 22.0f);
                sf::FloatRect addBtn(192.0f, headerBounds.top + 3.0f, 55.0f, 22.0f);
                sf::FloatRect dupBtn(252.0f, headerBounds.top + 3.0f, 55.0f, 22.0f);
                sf::FloatRect delBtn(312.0f, headerBounds.top + 3.0f, 55.0f, 22.0f);
                sf::FloatRect onionBtn(372.0f, headerBounds.top + 3.0f, 75.0f, 22.0f);
                sf::FloatRect closeBtn(1920.0f - 32.0f, headerBounds.top + 3.0f, 22.0f, 22.0f);

                if (playBtn.contains(mousePos)) { timeline.togglePlayback(); return; }
                if (addBtn.contains(mousePos)) { canvas.addFrame(timeline.getCurrentFrame()); timeline.addFrameAfter(timeline.getCurrentFrame()); timeline.nextFrame(); return; }
                if (dupBtn.contains(mousePos)) { canvas.duplicateFrame(timeline.getCurrentFrame()); timeline.duplicateFrame(timeline.getCurrentFrame()); timeline.nextFrame(); return; }
                if (delBtn.contains(mousePos)) {
                    if (canvas.getFrameCount() > 1) {
                        int cur = timeline.getCurrentFrame();
                        canvas.deleteFrame(cur);
                        timeline.deleteFrame(cur);
                        if (timeline.getCurrentFrame() >= static_cast<int>(canvas.getFrameCount())) {
                            timeline.setFrame(static_cast<int>(canvas.getFrameCount()) - 1);
                        }
                    }
                    return;
                }
                if (onionBtn.contains(mousePos)) {
                    canvas.setOnionSkin(!canvas.isOnionSkinEnabled(), canvas.getOnionSkinPrevOpacity(), canvas.getOnionSkinNextOpacity());
                    return;
                }
                if (closeBtn.contains(mousePos)) {
                    m_showTimeline = false;
                    return;
                }

                float cardW = 90.0f;
                float cardH = 120.0f;
                float startX = 20.0f;
                float cardY = timelineY + 42.0f;
                int totalFrames = static_cast<int>(canvas.getFrameCount());

                for (int i = 0; i < totalFrames; ++i) {
                    sf::FloatRect cardBounds(startX, cardY, cardW, cardH);
                    if (cardBounds.contains(mousePos)) {
                        timeline.setFrame(i);
                        return;
                    }
                    startX += cardW + 12.0f;
                }

                if (headerBounds.contains(mousePos) || mousePos.y > timelineY) return;
            }
        }

        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::F8) {
            m_debugUseSpriteStudio = !m_debugUseSpriteStudio;

            if (m_debugUseSpriteStudio) {
                m_activeTool = std::make_unique<SpriteSheetStudioTool>();
                m_activeTool->Initialize();

                sf::FloatRect physicalSpace(0.f, 0.f, static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y));
                m_activeTool->SetBounds(physicalSpace);

                sf::Event resizeFix;
                resizeFix.type = sf::Event::Resized;
                resizeFix.size.width = window.getSize().x;
                resizeFix.size.height = window.getSize().y;
                m_activeTool->HandleEvent(resizeFix, window);
            }
            else {
                m_activeTool.reset();
                window.setView(WisdomUI::WorkspaceLayout::GetLetterboxView(window.getSize()));
            }

            showMessage(m_debugUseSpriteStudio ? "Debug: Embedded Sprite Sheet Studio" : "Debug: Native Canvas Workspace", sf::Color::Yellow);
            return;
        }

        if (m_debugUseSpriteStudio) {
            if (m_activeTool) {
                sf::View physicalView(sf::FloatRect(0.f, 0.f, static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y)));
                window.setView(physicalView);

                sf::FloatRect physicalSpace(0.f, 0.f, static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y));
                m_activeTool->SetBounds(physicalSpace);
                m_activeTool->HandleEvent(event, window);
            }
            return;
        }

        if (g_aiPanel.getIsVisible()) {
            if (g_aiPanel.handleEvent(event, mousePos)) {
                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    std::string action = g_aiPanel.handleClick(mousePos);
                    if (action == "execute") {
                        sf::Image currentCanvas = ExportManager::flattenFrame(canvas, timeline.getCurrentFrame());
                        AIRequest req = g_aiPanel.buildRequestFromCanvasContext(canvas.getCanvasSize().x, canvas.getCanvasSize().y, canvas.getPixelMode(), 1.0f, canvas.getActiveTool() == ToolType::Select);
                        req.baseImage = currentCanvas;

                        showMessage("AI Process Initialized Async...", sf::Color::Yellow);
                        AIManager::getInstance().executeRequest(req);
                    }
                    else if (action == "back") {
                        showMessage("AI Panel Closed.", sf::Color::Cyan);
                    }
                }
                return;
            }
        }

        if (event.type == sf::Event::TextEntered && isTypingPrompt) {
            if (event.text.unicode == '\b') {
                if (!currentPrompt.empty()) currentPrompt.pop_back();
            }
            else if (event.text.unicode < 128 && event.text.unicode != '\r' && event.text.unicode != '\n' && event.text.unicode != '\b') {
                currentPrompt += static_cast<char>(event.text.unicode);
            }
            promptDisplay.setString("> " + currentPrompt + "_");
        }

        if (event.type == sf::Event::KeyPressed) {
            if (m_textManager.getEditingText() == nullptr) {
                if (event.key.code == sf::Keyboard::Numpad6) {
                    isTypingPrompt = !isTypingPrompt;
                    if (isTypingPrompt) {
                        currentPrompt = "";
                        promptDisplay.setString("> _");
                        showMessage("Legacy Terminal (Use AI Panel on the left)", sf::Color(0, 191, 255));
                    }
                }

                if (isTypingPrompt) return;

                if (event.key.code == sf::Keyboard::G) {
                    canvas.commitSelection(timeline.getCurrentFrame());
                    canvas.setActiveTool(ToolType::Gradient);
                    m_toolDock.SetActiveTool("gradient");
                    showMessage("Gradient Tool Activated", sf::Color::Green);
                }

                if (keybindManager.isActionTriggered("ui_settings", event)) keybindPanel.toggle();
                if (keybindManager.isActionTriggered("export_png", event)) exportModal.open(canvas, timeline.getCurrentFrame());

                if (keybindManager.isActionTriggered("proj_save", event)) {
                    if (triggerSave(canvas, timeline)) showMessage("Project Saved Successfully!", sf::Color::Green);
                    else showMessage("Error Saving Project!", sf::Color::Red);
                }

                if (keybindManager.isActionTriggered("proj_save_as", event)) {
                    std::string file = NativeDialogs::saveFileDialog("Wisdom Park Projects\0*.wpk\0", "wpk", activeProjectName);
                    if (!file.empty()) {
                        activeProjectPath = file;
                        if (pm.saveProjectAs(activeProjectPath, activeProjectName, canvas, static_cast<int>(timeline.getFps()), canvas.getPixelMode())) {
                            canvas.clearIsDirty();
                            showMessage("Project Saved As Successfully!", sf::Color::Green);
                        }
                        else showMessage("Error Saving Project!", sf::Color::Red);
                    }
                }

                if (keybindManager.isActionTriggered("proj_open", event)) {
                    std::string file = NativeDialogs::openFileDialog("Wisdom Park Projects\0*.wpk\0All Files\0*.*\0");
                    if (!file.empty()) {
                        activeProjectPath = file;
                        activeProjectName = std::filesystem::path(file).stem().string();
                        int loadedFps = 12;
                        bool isPix = false;
                        if (pm.loadProject(activeProjectPath, canvas, loadedFps, isPix)) {
                            timeline.setFrame(0);
                            canvas.setPixelMode(isPix);
                            canvas.clearIsDirty();
                            showMessage("Loaded Native Project", sf::Color::Green);
                        }
                        else {
                            showMessage("Failed to load native project.", sf::Color::Red);
                        }
                    }
                }

                if (keybindManager.isActionTriggered("proj_new", event)) newProjectModal.open();

                if (keybindManager.isActionTriggered("time_next", event)) {
                    if (timeline.getCurrentFrame() < timeline.getFrameCount() - 1) {
                        timeline.nextFrame();
                    }
                    else {
                        canvas.addFrame(timeline.getCurrentFrame());
                        timeline.addFrameAfter(timeline.getCurrentFrame());
                        timeline.nextFrame();
                    }
                }

                if (keybindManager.isActionTriggered("time_prev", event)) {
                    if (timeline.getCurrentFrame() > 0) {
                        timeline.prevFrame();
                    }
                    else {
                        canvas.addFrame(-1);
                        timeline.addFrameAfter(-1);
                        timeline.setFrame(0);
                    }
                }

                if (keybindManager.isActionTriggered("time_play", event)) timeline.togglePlayback();
                if (keybindManager.isActionTriggered("time_start", event)) timeline.setFrame(0);
                if (keybindManager.isActionTriggered("time_end", event)) timeline.setFrame(static_cast<int>(canvas.getFrameCount()) - 1);

                if (keybindManager.isActionTriggered("time_add", event)) {
                    canvas.addFrame(timeline.getCurrentFrame());
                    timeline.addFrameAfter(timeline.getCurrentFrame());
                    timeline.nextFrame();
                }
                if (keybindManager.isActionTriggered("time_del", event)) {
                    if (canvas.getFrameCount() > 1) {
                        int cur = timeline.getCurrentFrame();
                        canvas.deleteFrame(cur);
                        timeline.deleteFrame(cur);
                        if (timeline.getCurrentFrame() >= static_cast<int>(canvas.getFrameCount())) {
                            timeline.setFrame(static_cast<int>(canvas.getFrameCount()) - 1);
                        }
                    }
                }

                if (keybindManager.isActionTriggered("layer_new", event)) canvas.addLayer(timeline.getCurrentFrame(), "New Layer");
                if (keybindManager.isActionTriggered("layer_dup", event)) canvas.duplicateLayer(timeline.getCurrentFrame(), canvas.getActiveLayer());
                if (keybindManager.isActionTriggered("layer_del", event)) canvas.deleteLayer(timeline.getCurrentFrame(), canvas.getActiveLayer());
                if (keybindManager.isActionTriggered("layer_merge_down", event)) canvas.mergeDown(timeline.getCurrentFrame());
                if (keybindManager.isActionTriggered("layer_merge_vis", event)) canvas.mergeVisible(timeline.getCurrentFrame());
                if (event.key.code == sf::Keyboard::PageUp) {
                    int curL = canvas.getActiveLayer();
                    if (curL < static_cast<int>(canvas.getFrameReadOnly(timeline.getCurrentFrame())->layers.size()) - 1) {
                        canvas.moveLayer(timeline.getCurrentFrame(), curL, curL + 1);
                        showMessage("Layer +1 (Up)", sf::Color::Cyan);
                    }
                }
                if (event.key.code == sf::Keyboard::PageDown) {
                    int curL = canvas.getActiveLayer();
                    if (curL > 0) {
                        canvas.moveLayer(timeline.getCurrentFrame(), curL, curL - 1);
                        showMessage("Layer -1 (Down)", sf::Color::Cyan);
                    }
                }
                if (event.key.code == sf::Keyboard::E && event.key.control) {
                    canvas.commitSelection(timeline.getCurrentFrame());
                    canvas.mergeDown(timeline.getCurrentFrame());
                    showMessage("Merged Object Down", sf::Color::Green);
                }
                if (event.key.code == sf::Keyboard::B && event.key.control) {
                    if (assetBrowser) assetBrowser->toggle();
                }
                if (keybindManager.isActionTriggered("edit_del_sel", event)) {
                    if (canvas.getActiveTool() == ToolType::Select) canvas.deleteSelection(timeline.getCurrentFrame());
                }
                if (keybindManager.isActionTriggered("edit_deselect", event)) {
                    canvas.commitSelection(timeline.getCurrentFrame());
                    canvas.setActiveTool(ToolType::Brush);
                    m_toolDock.SetActiveTool("brush");
                }

                if (keybindManager.isActionTriggered("edit_copy", event)) canvas.copySelection(timeline.getCurrentFrame());
                if (keybindManager.isActionTriggered("edit_paste", event)) canvas.pasteSelection(timeline.getCurrentFrame());
                if (keybindManager.isActionTriggered("edit_dup_sel", event)) canvas.duplicateSelection(timeline.getCurrentFrame());

                if (canvas.getActiveTool() == ToolType::Select) {
                    if (keybindManager.isActionTriggered("sel_flip_h", event)) canvas.flipSelectionHorizontal(timeline.getCurrentFrame());
                    if (keybindManager.isActionTriggered("sel_flip_v", event)) canvas.flipSelectionVertical(timeline.getCurrentFrame());
                }

                if (canvas.getPixelMode()) {
                    if (keybindManager.isActionTriggered("tool_brush", event)) {
                        canvas.cyclePixelBrushSize();
                        showMessage("Brush Size: " + std::to_string(canvas.getPixelBrushSize()) + "px", sf::Color::Green);
                    }
                    if (keybindManager.isActionTriggered("view_grid", event)) { canvas.togglePixelGrid(); }
                    if (keybindManager.isActionTriggered("tool_move", event)) { canvas.toggleTileMode(); }
                    if (keybindManager.isActionTriggered("layer_vis", event)) { canvas.resetView(); showMessage("View Reset", sf::Color::Green); }
                    if (keybindManager.isActionTriggered("tool_eraser", event)) { canvas.commitSelection(timeline.getCurrentFrame()); canvas.setActiveTool(ToolType::Eraser); m_toolDock.SetActiveTool("eraser"); }
                    if (keybindManager.isActionTriggered("tool_pencil", event)) { canvas.commitSelection(timeline.getCurrentFrame()); canvas.setActiveTool(ToolType::Pencil); m_toolDock.SetActiveTool("pencil"); }
                }
                else {
                    if (keybindManager.isActionTriggered("tool_brush", event)) { canvas.commitSelection(timeline.getCurrentFrame()); canvas.setActiveTool(ToolType::Brush); m_toolDock.SetActiveTool("brush"); }
                    if (keybindManager.isActionTriggered("tool_pencil", event)) { canvas.commitSelection(timeline.getCurrentFrame()); canvas.setActiveTool(ToolType::Pencil); m_toolDock.SetActiveTool("pencil"); }
                    if (keybindManager.isActionTriggered("tool_eraser", event)) { canvas.commitSelection(timeline.getCurrentFrame()); canvas.setActiveTool(ToolType::Eraser); m_toolDock.SetActiveTool("eraser"); }
                    if (keybindManager.isActionTriggered("tool_fill", event)) { canvas.commitSelection(timeline.getCurrentFrame()); canvas.setActiveTool(ToolType::Fill); m_toolDock.SetActiveTool("fill"); }
                    if (keybindManager.isActionTriggered("tool_select", event)) { canvas.setActiveTool(ToolType::Select); m_toolDock.SetActiveTool("select"); }
                }

                if (keybindManager.isActionTriggered("edit_undo", event)) canvas.undo();
                if (keybindManager.isActionTriggered("edit_redo", event)) canvas.redo();

                if (keybindManager.isActionTriggered("tool_eyedropper", event)) {
                    if (canvas.getDrawArea().contains(logicalMousePos)) {
                        sf::Image flat = ExportManager::flattenFrame(canvas, timeline.getCurrentFrame());
                        sf::Vector2f texScale(static_cast<float>(canvas.getCanvasSize().x) / canvas.getDrawArea().width, static_cast<float>(canvas.getCanvasSize().y) / canvas.getDrawArea().height);
                        int px = static_cast<int>((logicalMousePos.x - canvas.getDrawArea().left) * texScale.x);
                        int py = static_cast<int>((logicalMousePos.y - canvas.getDrawArea().top) * texScale.y);

                        if (px >= 0 && px < static_cast<int>(flat.getSize().x) && py >= 0 && py < static_cast<int>(flat.getSize().y)) {
                            sf::Color picked = flat.getPixel(px, py);
                            canvas.setPrimaryColor(picked);
                            colorPalettePanel.setColors(picked, canvas.getSecondaryColor());
                            colorPalettePanel.getColorManager().addRecentColor(picked);
                            showMessage("Color Picked", sf::Color::Green);
                        }
                    }
                }
            }
        }

        if (!timeline.isPlaying() && !keybindPanel.isVisible() && !exportModal.getIsOpen() && !newProjectModal.getIsOpen() && !g_aiPanel.getIsVisible() && !g_aiReviewModal.getIsOpen()) {
            sf::Vector2f shiftedMousePos = mousePos;
            shiftedMousePos.x -= WisdomUI::Theme::ToolDockWidth;

            if (canvas.getActiveTool() == ToolType::Perspective) {
                m_perspectivePanel.handleEvent(event, shiftedMousePos, canvas.getCanvasSize());
            }
            if (canvas.getActiveTool() == ToolType::Text) {
                if (m_textPanel.handleEvent(event, shiftedMousePos)) return;
            }
            if (canvas.getActiveTool() == ToolType::Gradient) {
                if (m_gradientPanel.handleEvent(event, shiftedMousePos)) return;
            }

            if (event.type == sf::Event::MouseWheelScrolled && event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
                canvas.zoom(event.mouseWheelScroll.delta);
            }

            if (event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Left && colorPalettePanel.getIsEyedropperActive()) {
                    if (canvas.getDrawArea().contains(logicalMousePos)) {
                        sf::Image flat = ExportManager::flattenFrame(canvas, timeline.getCurrentFrame());
                        sf::Vector2f texScale(static_cast<float>(canvas.getCanvasSize().x) / canvas.getDrawArea().width, static_cast<float>(canvas.getCanvasSize().y) / canvas.getDrawArea().height);
                        int px = static_cast<int>((logicalMousePos.x - canvas.getDrawArea().left) * texScale.x);
                        int py = static_cast<int>((logicalMousePos.y - canvas.getDrawArea().top) * texScale.y);

                        if (px >= 0 && px < static_cast<int>(flat.getSize().x) && py >= 0 && py < static_cast<int>(flat.getSize().y)) {
                            sf::Color picked = flat.getPixel(px, py);
                            canvas.setPrimaryColor(picked);
                            colorPalettePanel.setColors(picked, canvas.getSecondaryColor());
                            colorPalettePanel.getColorManager().addRecentColor(picked);
                            colorPalettePanel.setEyedropperActive(false);
                        }
                    }
                    return;
                }

                canvas.handleMousePressed(logicalMousePos, event.mouseButton.button == sf::Mouse::Right, timeline.getCurrentFrame());
            }

            if (m_activeTool) {
                m_activeTool->HandleEvent(event, window);
            }
        }
    }
}

void UIManager::update(sf::RenderWindow& window, AppState currentState, AppSettings& settings, float dt, Canvas& canvas, Timeline& timeline) {
    static bool s_syncedInitialSettings = false;
    if (!s_syncedInitialSettings) {
        uiFullscreen = settings.fullscreen;
        s_syncedInitialSettings = true;
    }
#if defined(_WIN32)
    static HWND s_lastHwnd = nullptr;
    HWND hwnd = window.getSystemHandle();
    if (hwnd != s_lastHwnd) {
        SetupDragDrop(hwnd);
        s_lastHwnd = hwnd;
    }

    if (!g_droppedFiles.empty()) {
        for (const auto& dropItem : g_droppedFiles) {
            const std::string& filePath = dropItem.first;
            sf::Vector2i dropPixel = dropItem.second;
            sf::Vector2f dropPos = window.mapPixelToCoords(dropPixel);

            std::string ext = std::filesystem::path(filePath).extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

            bool isImage = (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".jfif" || ext == ".bmp" || ext == ".tga" || ext == ".webp");
            bool isAudio = (ext == ".wav" || ext == ".ogg" || ext == ".mp3" || ext == ".flac");
            bool isProject = (ext == ".wpk");

            if (currentState == AppState::Painting) {
                sf::FloatRect assetBrowserRect(1440.f, 78.f, 390.f, 540.f);
                bool droppedInAssetBrowser = (assetBrowser && assetBrowser->getIsVisible() && assetBrowserRect.contains(dropPos));

                if (droppedInAssetBrowser) {
                    std::vector<std::string> fileList = { filePath };
                    assetManager.importAssets(fileList);
                    m_activeRightTab = RightTabMode::Assets;
                    showMessage("Loaded to Asset Vault: " + std::filesystem::path(filePath).filename().string(), sf::Color::Green);
                }
                else if (isProject) {
                    int loadedFps = 12;
                    bool isPix = false;
                    if (projManager && projManager->loadProject(filePath, canvas, loadedFps, isPix)) {
                        activeProjectPath = filePath;
                        activeProjectName = std::filesystem::path(filePath).stem().string();
                        timeline.setFrame(0);
                        canvas.clearIsDirty();
                        showMessage("Opened Project: " + activeProjectName, sf::Color::Green);
                    }
                }
                else if (isImage) {
                    int curFrame = static_cast<int>(timeline.getCurrentFrame());
                    canvas.importImageToActiveLayer(filePath, curFrame);
                    showMessage("Placed on Canvas: " + std::filesystem::path(filePath).filename().string(), sf::Color::Green);
                }
                else if (isAudio) {
                    std::vector<std::string> audioFile = { filePath };
                    assetManager.importAssets(audioFile);
                    m_activeRightTab = RightTabMode::Audio;
                    if (audioPanel.getIsVisible()) audioPanel.toggle();
                    audioPanel.toggle();
                    showMessage("Loaded Audio Track: " + std::filesystem::path(filePath).filename().string(), sf::Color::Green);
                }
                else {
                    std::vector<std::string> generalFile = { filePath };
                    assetManager.importAssets(generalFile);
                    m_activeRightTab = RightTabMode::Assets;
                    if (assetBrowser && !assetBrowser->getIsVisible()) assetBrowser->toggle();
                    showMessage("Imported File to Assets", sf::Color::Green);
                }
            }
        }
        g_droppedFiles.clear();
    }
#endif

    if (m_debugUseSpriteStudio) {
        if (m_activeTool) {
            sf::FloatRect physicalSpace(0.f, 0.f, static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y));
            m_activeTool->SetBounds(physicalSpace);
            m_activeTool->Update(dt, window);
        }
        return;
    }

    char buffer[1024];
    std::size_t received;
    sf::IpAddress sender;
    unsigned short port;

    if (handTrackerSocket.receive(buffer, sizeof(buffer) - 1, received, sender, port) == sf::Socket::Done) {
        buffer[received] = '\0';
        std::string dataString(buffer);
        std::stringstream ss(dataString);
        std::string item;

        float normalizedX = 0.0f;
        float normalizedY = 0.0f;
        int isLeftPinching = 0;
        int isRightPinching = 0;
        int isZoomPinching = 0;

        try {
            if (std::getline(ss, item, ',')) normalizedX = std::stof(item);
            if (std::getline(ss, item, ',')) normalizedY = std::stof(item);
            if (std::getline(ss, item, ',')) isLeftPinching = std::stoi(item);
            if (std::getline(ss, item, ',')) isRightPinching = std::stoi(item);
            if (std::getline(ss, item, ',')) isZoomPinching = std::stoi(item);
        }
        catch (...) {
            return;
        }

        int screenX = static_cast<int>(normalizedX * 1920.0f);
        int screenY = static_cast<int>(normalizedY * 1080.0f);

        sf::Mouse::setPosition(sf::Vector2i(screenX, screenY), window);

        if (isLeftPinching == 1 && lastLeftState == 0) {
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
        }
        else if (isLeftPinching == 0 && lastLeftState == 1) {
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
        }

        if (isRightPinching == 1 && lastRightState == 0) {
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
        }
        else if (isRightPinching == 0 && lastRightState == 1) {
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
        }

        if (isZoomPinching == 1) {
            if (lastZoomState == 0) {
                zoomOriginY = normalizedY;
            }
            float deltaY = normalizedY - zoomOriginY;
            if (std::abs(deltaY) > 0.015f) {
                int scrollAmt = (deltaY < 0.0f) ? 120 : -120;
                keybd_event(VK_CONTROL, 0, 0, 0);
                mouse_event(MOUSEEVENTF_WHEEL, 0, 0, scrollAmt, 0);
                keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
                zoomOriginY = normalizedY;
            }
        }

        lastLeftState = isLeftPinching;
        lastRightState = isRightPinching;
        lastZoomState = isZoomPinching;
    }

    sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
    sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);
    sf::Vector2f logicalMousePos = canvas.getInverseTransform().transformPoint(mousePos);

    m_topMenuBar.update(mousePos, static_cast<float>(window.getSize().x));
    if (keybindPanel.isVisible()) keybindPanel.updateHover(mousePos);
    if (exportModal.getIsOpen()) exportModal.updateHover(mousePos);
    if (newProjectModal.getIsOpen()) newProjectModal.updateHover(mousePos);

    if (AIManager::getInstance().isProcessingAsync()) {
        loadingSpinner.rotate(150.f * dt);
    }

    if (currentState == AppState::Welcome) {
        if (!keybindPanel.isVisible()) {
            projectBrowser.updateHover(mousePos);
            updateStartMenu(dt, mousePos);
            if (m_useMinigameWelcome) {
                updateMinigame(dt, mousePos, window);
            }

            if (currentMenuState == MenuState::Main) {
                std::vector<std::string> buttons = { "Projects", "Settings", "Tutorials", "Keybinds", "Credits", "Exit" };
                float by = 420.f;
                for (const auto& btn : buttons) {
                    sf::FloatRect bBounds(150.f, by, 400.f, 60.f);
                    updateHoverValue("mm_" + btn, bBounds.contains(mousePos), dt);
                    by += 80.f;
                }
                float ry = 280.f;
                for (int i = 0; i < 4; ++i) {
                    sf::FloatRect rBounds(900.f, ry, 800.f, 100.f);
                    updateHoverValue("rec_" + std::to_string(i), rBounds.contains(mousePos), dt);
                    ry += 125.f;
                }
            }
            else if (currentMenuState == MenuState::Settings) {
                sf::FloatRect backBounds(100.f, 100.f, 120.f, 50.f);
                updateHoverValue("btn_back", backBounds.contains(mousePos), dt);

                auto checkToggle = [&](float x, float y) { return sf::FloatRect(x, y, 300.f, 30.f).contains(mousePos); };
                auto checkStepperL = [&](float x, float y) { return sf::FloatRect(x + 190.f, y - 10.f, 40.f, 40.f).contains(mousePos); };
                auto checkStepperR = [&](float x, float y) { return sf::FloatRect(x + 260.f, y - 10.f, 40.f, 40.f).contains(mousePos); };

                updateHoverValue("set_t_fs", checkToggle(380.f, 310.f), dt);
                updateHoverValue("set_t_bl", checkToggle(380.f, 360.f), dt);
                updateHoverValue("set_t_vs", checkToggle(380.f, 410.f), dt);
                updateHoverValue("set_s_fpsL", checkStepperL(380.f, 460.f), dt);
                updateHoverValue("set_s_fpsR", checkStepperR(380.f, 460.f), dt);
                updateHoverValue("set_t_ab", checkToggle(1050.f, 310.f), dt);
                updateHoverValue("set_t_hw", checkToggle(380.f, 710.f), dt);
                updateHoverValue("set_s_afpsL", checkStepperL(380.f, 760.f), dt);
                updateHoverValue("set_s_afpsR", checkStepperR(380.f, 760.f), dt);
                updateHoverValue("set_s_undoL", checkStepperL(380.f, 810.f), dt);
                updateHoverValue("set_s_undoR", checkStepperR(380.f, 810.f), dt);
                updateHoverValue("set_s_aiL", checkStepperL(1050.f, 710.f), dt);
                updateHoverValue("set_s_aiR", checkStepperR(1050.f, 710.f), dt);
            }
            else if (currentMenuState == MenuState::Tutorials) {
                sf::FloatRect backBounds(100.f, 100.f, 120.f, 50.f);
                updateHoverValue("btn_back", backBounds.contains(mousePos), dt);

                if (activeTutorialIndex == -1) {
                    int c = 0, r = 0;
                    for (int i = 0; i < 9; ++i) {
                        float x = 300.f + static_cast<float>(c) * 450.f;
                        float y = 250.f + static_cast<float>(r) * 220.f;
                        sf::FloatRect bounds(x, y, 400.f, 180.f);
                        updateHoverValue("tut_" + std::to_string(i), bounds.contains(mousePos), dt);
                        c++;
                        if (c >= 3) { c = 0; r++; }
                    }
                }
                else {
                    sf::FloatRect bBounds(860.f, 750.f, 200.f, 60.f);
                    updateHoverValue("tut_back_btn", bBounds.contains(mousePos), dt);
                }
            }
            else if (currentMenuState == MenuState::Credits) {
                sf::FloatRect backBounds(100.f, 100.f, 120.f, 50.f);
                updateHoverValue("btn_back", backBounds.contains(mousePos), dt);

                sf::Text dummyText("WISDOM PARK STUDIO", font, 26);
                sf::FloatRect sb = dummyText.getLocalBounds();
                sf::FloatRect titleBounds(960.f - sb.width / 2.f, 300.f - sb.height / 2.f, sb.width, sb.height);
                if (titleBounds.contains(mousePos)) {
                    easterEggClicks++;
                }
            }
            else {
                sf::FloatRect backBounds(100.f, 100.f, 120.f, 50.f);
                updateHoverValue("btn_back", backBounds.contains(mousePos), dt);
            }
        }
    }
    else if (currentState == AppState::Painting) {
        timeline.update(dt);

        bool rightDockOpen = (m_activeRightTab != RightTabMode::None);
        auto regions = m_workspaceLayout.Update(rightDockOpen, m_showTimeline);

        m_topBar.SetBounds(regions.topBar);
        m_topBar.SetProjectName(activeProjectName, canvas.getIsDirty());
        m_topBar.Update(dt, mousePos);

        m_toolOptionsBar.SetBounds(regions.optionsBar);
        std::string toolName = "Brush";
        if (canvas.getActiveTool() == ToolType::Pencil) toolName = "Pencil";
        else if (canvas.getActiveTool() == ToolType::Eraser) toolName = "Eraser";
        else if (canvas.getActiveTool() == ToolType::Fill) toolName = "Fill";
        else if (canvas.getActiveTool() == ToolType::Select) toolName = "Select";
        else if (canvas.getActiveTool() == ToolType::MagicWand) toolName = "Magic Wand";
        else if (canvas.getActiveTool() == ToolType::Shapes) toolName = "Shapes";
        else if (canvas.getActiveTool() == ToolType::Text) toolName = "Text";
        else if (canvas.getActiveTool() == ToolType::Gradient) toolName = "Gradient";
        else if (canvas.getActiveTool() == ToolType::Perspective) toolName = "Perspective";
        else if (canvas.getActiveTool() == ToolType::Symmetry) toolName = "Symmetry";

        float curSize = canvas.getPixelMode() ? static_cast<float>(canvas.getPixelBrushSize()) : canvas.getBrushSize();
        m_toolOptionsBar.SyncState(toolName, curSize, canvas.getPixelMode(), canvas.isPixelPerfectEnabled());
        m_toolOptionsBar.Update(dt, mousePos);

        m_toolDock.SetBounds(regions.toolDock);
        m_toolDock.Update(dt, mousePos);

        m_rightDockTabs.SetBounds(regions.rightDockTabs);
        m_rightDockTabs.SetTabState("layers", m_activeRightTab == RightTabMode::Layers);
        m_rightDockTabs.SetTabState("palette", m_activeRightTab == RightTabMode::Palette);
        m_rightDockTabs.SetTabState("properties", m_activeRightTab == RightTabMode::Properties);
        m_rightDockTabs.SetTabState("assets", assetBrowser && assetBrowser->getIsVisible());
        m_rightDockTabs.SetTabState("audio", audioPanel.getIsVisible());
        m_rightDockTabs.Update(dt, mousePos);
        if (m_activeRightTab == RightTabMode::Layers) {
            layerPanel.update(dt, focusMode, true);
        }
        else {
            layerPanel.update(dt, focusMode, false);
        }

        if (m_activeRightTab == RightTabMode::Palette) {
            colorPalettePanel.update(dt, focusMode, canvas, true);
        }
        else {
            colorPalettePanel.update(dt, focusMode, canvas, false);
        }

        if (m_activeRightTab == RightTabMode::Properties) {
            rightProperties.update(dt, focusMode, true);
        }
        else {
            rightProperties.update(dt, focusMode, false);
        }

        if (assetBrowser && assetBrowser->getIsVisible()) {
            assetBrowser->update(dt);
        }

        if (audioPanel.getIsVisible()) {
            audioPanel.update(dt);
        }

        if (m_showTimeline) {
            sf::FloatRect headerBounds(0.0f, regions.timeline.top, 1920.0f, 28.0f);
            m_timelineHeader.SetBounds(headerBounds);
            m_timelineHeader.SyncState(timeline.isPlaying(), timeline.getCurrentFrame(), static_cast<int>(canvas.getFrameCount()), timeline.getFps(), canvas.isOnionSkinEnabled());
            m_timelineHeader.Update(dt, mousePos);
            bottomTimeline.update(dt, focusMode);
        }

        m_statusBar.SetBounds(regions.statusBar);
        m_statusBar.UpdateData(canvas.getCanvasSize(), logicalMousePos, 1.0f, canvas.getActiveLayer(), timeline.getCurrentFrame(), m_showTimeline);
        m_statusBar.Update(dt, mousePos);

        g_aiPanel.update(dt);
        audioPanel.updatePlayback(timeline.getCurrentFrame(), timeline.getFps(), timeline.isPlaying());

        if (AIManager::getInstance().hasAsyncFinished()) {
            sf::Image originalImage;
            AIResult asyncRes = AIManager::getInstance().getAsyncResult(originalImage);
            if (asyncRes.success) {
                g_aiReviewModal.open(originalImage, asyncRes.resultImage);
                showMessage("AI Generation Complete!", sf::Color::Green);
            }
            else {
                showMessage("AI Process Error: " + asyncRes.errorMessage, sf::Color::Red);
            }
        }

        bool needsShapeTool = (canvas.getActiveTool() == ToolType::Shapes);
        bool needsWandTool = (canvas.getActiveTool() == ToolType::MagicWand);
        bool needsPerspectiveTool = (canvas.getActiveTool() == ToolType::Perspective);
        bool needsTextTool = (canvas.getActiveTool() == ToolType::Text);
        bool needsGradientTool = (canvas.getActiveTool() == ToolType::Gradient);

        bool hasShapeTool = dynamic_cast<ShapeTool*>(m_activeTool.get()) != nullptr;
        bool hasWandTool = dynamic_cast<MagicWandTool*>(m_activeTool.get()) != nullptr;
        bool hasPerspectiveTool = dynamic_cast<PerspectiveTool*>(m_activeTool.get()) != nullptr;
        bool hasTextTool = dynamic_cast<TextTool*>(m_activeTool.get()) != nullptr;
        bool hasGradientTool = dynamic_cast<GradientTool*>(m_activeTool.get()) != nullptr;

        if (!m_activeTool || (needsShapeTool && !hasShapeTool) || (needsWandTool && !hasWandTool) || (needsPerspectiveTool && !hasPerspectiveTool) || (needsTextTool && !hasTextTool) || (needsGradientTool && !hasGradientTool) || (!needsShapeTool && !needsWandTool && !needsPerspectiveTool && !needsTextTool && !needsGradientTool && (hasShapeTool || hasWandTool || hasPerspectiveTool || hasTextTool || hasGradientTool) && !m_debugUseSpriteStudio)) {
            if (needsShapeTool) {
                m_activeTool = std::make_unique<ShapeTool>(canvas, timeline);
            }
            else if (needsWandTool) {
                m_activeTool = std::make_unique<MagicWandTool>(canvas, timeline);
            }
            else if (needsPerspectiveTool) {
                m_activeTool = std::make_unique<PerspectiveTool>(canvas, timeline, m_perspectiveManager);
            }
            else if (needsTextTool) {
                m_activeTool = std::make_unique<TextTool>(canvas, timeline, m_textManager);
            }
            else if (needsGradientTool) {
                m_activeTool = std::make_unique<GradientTool>(canvas, timeline, m_gradientConfig);
            }
            else if (m_debugUseSpriteStudio) {
                m_activeTool = std::make_unique<SpriteSheetStudioTool>();
            }
            else {
                m_activeTool = std::make_unique<CanvasTool>(canvas, timeline, isLightingMode);
            }
            m_activeTool->Initialize();
        }

        if (auto* wand = dynamic_cast<MagicWandTool*>(m_activeTool.get())) {
            if (wand->wantsColorPanelOpen()) {
                m_activeRightTab = RightTabMode::Palette;
                wand->clearColorPanelRequest();
            }
        }
        if (canvas.getActiveTool() == ToolType::Text) {
            if (m_textPanel.wantsColorPanelOpen()) {
                m_activeRightTab = RightTabMode::Palette;
                m_textPanel.clearColorPanelRequest();
            }
        }
        if (canvas.getActiveTool() == ToolType::Gradient) {
            m_gradientPanel.setSelectedColor(canvas.getPrimaryColor());
            if (m_gradientPanel.wantsColorPanelOpen()) {
                m_activeRightTab = RightTabMode::Palette;
                m_gradientPanel.clearColorPanelRequest();
            }
        }

        m_activeTool->SetBounds(regions.canvas);
        m_activeTool->Update(dt, window);

        if (showingText && textClock.getElapsedTime().asSeconds() > 2.0f) showingText = false;
        else if (showingText && textClock.getElapsedTime().asSeconds() > 1.5f) {
            textAlpha = std::max(0.0f, textAlpha - 255.0f * (1.0f / 60.0f));
            sf::Color fc = uiText.getFillColor(); fc.a = static_cast<sf::Uint8>(textAlpha);
            sf::Color oc = uiText.getOutlineColor(); oc.a = static_cast<sf::Uint8>(textAlpha);
            uiText.setFillColor(fc); uiText.setOutlineColor(oc);
        }

        if (showUnsavedWarning) {
            if (warnSaveBtn.getGlobalBounds().contains(mousePos)) warnSaveBtn.setFillColor(sf::Color(70, 200, 70));
            else warnSaveBtn.setFillColor(sf::Color(50, 180, 50));

            if (warnDiscardBtn.getGlobalBounds().contains(mousePos)) warnDiscardBtn.setFillColor(sf::Color(200, 70, 70));
            else warnDiscardBtn.setFillColor(sf::Color(180, 50, 50));

            if (warnCancelBtn.getGlobalBounds().contains(mousePos)) warnCancelBtn.setFillColor(sf::Color(100, 100, 110));
            else warnCancelBtn.setFillColor(sf::Color(80, 80, 90));
        }
    }
}

void UIManager::draw(sf::RenderWindow& window, AppState currentState, Canvas& canvas, AIHelper& aiHelper, Timeline& timeline) {
    window.setView(WisdomUI::WorkspaceLayout::GetLetterboxView(window.getSize()));

    bgSprite.setPosition(0.f, 0.f);
    bgSprite.setScale(1920.f / bgTexture.getSize().x, 1080.f / bgTexture.getSize().y);
    bgSprite.setColor(sf::Color::White);
    window.draw(bgSprite);

    if (currentState == AppState::Welcome) {
        if (currentMenuState == MenuState::Main) drawMainMenu(window);
        else if (currentMenuState == MenuState::Projects) projectBrowser.draw(window);
        else if (currentMenuState == MenuState::Settings) drawSettingsMenu(window);
        else if (currentMenuState == MenuState::Tutorials) drawTutorialsMenu(window);
        else if (currentMenuState == MenuState::Credits) drawCreditsMenu(window);

        if (m_showKeybinds) {
            drawKeybindModal(window);
        }
        if (newProjectModal.getIsOpen()) {
            newProjectModal.draw(window);
        }
    }
    else if (currentState == AppState::Painting) {
        if (m_debugUseSpriteStudio) {
            if (m_activeTool) m_activeTool->Render(window);
            return;
        }

        rightProperties.syncState(aiHelper.getTheme(), isLightingMode, aiHelper.isTerrainEnabled(), canvas.isOnionSkinEnabled(), canvas.getOnionSkinPrevOpacity(), timeline.getFps());

        if (m_activeTool) {
            m_activeTool->Render(window);
            if (auto* canvasTool = dynamic_cast<CanvasTool*>(m_activeTool.get())) {
                canvasTool->RenderShadows(window, aiHelper);
            }
        }

        aiHelper.draw(window);

        if (m_activeRightTab == RightTabMode::Layers) {
            layerPanel.draw(window, canvas, timeline.getCurrentFrame());
        }
        else if (m_activeRightTab == RightTabMode::Palette) {
            colorPalettePanel.draw(window);
        }
        else if (m_activeRightTab == RightTabMode::Properties) {
            rightProperties.draw(window);
        }

        if (assetBrowser && assetBrowser->getIsVisible()) {
            assetBrowser->draw(window);
        }

        if (audioPanel.getIsVisible()) {
            audioPanel.draw(window);
        }

        if (canvas.getActiveTool() == ToolType::Perspective) m_perspectivePanel.draw(window);
        if (canvas.getActiveTool() == ToolType::Text) m_textPanel.draw(window);
        if (canvas.getActiveTool() == ToolType::Gradient) m_gradientPanel.draw(window);

        if (m_showTimeline) {
            float timelineY = 1080.0f - WisdomUI::Theme::StatusBarHeight - WisdomUI::Theme::TimelineHeight;
            sf::FloatRect timelineBounds(0.0f, timelineY, 1920.0f, WisdomUI::Theme::TimelineHeight);

            WisdomUI::Theme::DrawSunsetPanel(window, timelineBounds, 1.0f);

            sf::FloatRect trayBounds(12.0f, timelineY + 32.0f, 1920.0f - 24.0f, WisdomUI::Theme::TimelineHeight - 40.0f);
            sf::RectangleShape trayBg(sf::Vector2f(trayBounds.width, trayBounds.height));
            trayBg.setPosition(trayBounds.left, trayBounds.top);
            trayBg.setFillColor(WisdomUI::Theme::SunsetDeepDark);
            trayBg.setOutlineThickness(1.0f);
            trayBg.setOutlineColor(WisdomUI::Theme::SunsetPlum);
            window.draw(trayBg);

            m_timelineHeader.Render(window);

            float cardW = 90.0f;
            float cardH = 120.0f;
            float startX = 24.0f;
            float cardY = timelineY + 42.0f;
            int totalFrames = static_cast<int>(canvas.getFrameCount());
            int curFrame = timeline.getCurrentFrame();

            float canvasW = static_cast<float>(canvas.getCanvasSize().x);
            float canvasH = static_cast<float>(canvas.getCanvasSize().y);

            for (int i = 0; i < totalFrames; ++i) {
                bool isSelected = (i == curFrame);

                sf::FloatRect cardBounds(startX, cardY, cardW, cardH);
                sf::RectangleShape card(sf::Vector2f(cardW, cardH));
                card.setPosition(startX, cardY);
                card.setFillColor(isSelected ? WisdomUI::Theme::SunsetSkyMid : WisdomUI::Theme::SunsetSkyTop);
                card.setOutlineThickness(isSelected ? 2.0f : 1.0f);
                card.setOutlineColor(isSelected ? WisdomUI::Theme::SunsetAmber : WisdomUI::Theme::SunsetPlum);
                window.draw(card);

                float boxW = cardW - 14.0f;
                float boxH = cardH - 36.0f;
                float boxX = startX + 7.0f;
                float boxY = cardY + 7.0f;

                sf::RectangleShape thumbBase(sf::Vector2f(boxW, boxH));
                thumbBase.setPosition(boxX, boxY);
                thumbBase.setFillColor(sf::Color(210, 210, 210));
                thumbBase.setOutlineThickness(1.f);
                thumbBase.setOutlineColor(WisdomUI::Theme::SunsetCoralDark);
                window.draw(thumbBase);

                int gridCols = 8;
                int gridRows = 8;
                float cellW = boxW / static_cast<float>(gridCols);
                float cellH = boxH / static_cast<float>(gridRows);

                for (int r = 0; r < gridRows; ++r) {
                    for (int c = 0; c < gridCols; ++c) {
                        if ((r + c) % 2 == 1) {
                            sf::RectangleShape cell(sf::Vector2f(cellW, cellH));
                            cell.setPosition(boxX + c * cellW, boxY + r * cellH);
                            cell.setFillColor(sf::Color(180, 180, 180));
                            window.draw(cell);
                        }
                    }
                }

                const Frame* frameData = canvas.getFrameReadOnly(i);
                if (frameData && canvasW > 0.0f && canvasH > 0.0f) {
                    float scale = std::min((boxW - 4.0f) / canvasW, (boxH - 4.0f) / canvasH);
                    float previewW = canvasW * scale;
                    float previewH = canvasH * scale;
                    float previewX = boxX + (boxW - previewW) / 2.0f;
                    float previewY = boxY + (boxH - previewH) / 2.0f;

                    for (const auto& layer : frameData->layers) {
                        if (layer.visible && layer.texture) {
                            const sf::Texture& tex = layer.texture->getTexture();
                            sf::Sprite layerSprite(tex);
                            layerSprite.setScale(scale, scale);
                            layerSprite.setPosition(previewX, previewY);

                            float rawOp = layer.opacity;
                            if (rawOp <= 1.0f && rawOp > 0.0f) {
                                rawOp *= 255.0f;
                            }
                            else if (rawOp > 1.0f && rawOp <= 100.0f) {
                                rawOp = (rawOp / 100.0f) * 255.0f;
                            }
                            sf::Uint8 alpha = static_cast<sf::Uint8>(std::clamp(rawOp, 0.0f, 255.0f));

                            layerSprite.setColor(sf::Color(255, 255, 255, alpha));
                            window.draw(layerSprite);
                        }
                    }
                }

                WisdomUI::Theme::DrawCrispText(window, font, std::to_string(i + 1), 12, startX + cardW / 2.0f, cardY + cardH - 14.0f, isSelected ? WisdomUI::Theme::SunsetAmber : WisdomUI::Theme::TextSecondary, sf::Color::Transparent, true, true);

                if (isSelected) {
                    sf::RectangleShape selTag(sf::Vector2f(cardW - 12.0f, 2.0f));
                    selTag.setPosition(startX + 6.0f, cardY + 3.0f);
                    selTag.setFillColor(WisdomUI::Theme::SunsetCoral);
                    window.draw(selTag);
                }

                startX += cardW + 12.0f;
            }

            bottomTimeline.syncOnionState(canvas.isOnionSkinEnabled(), canvas.getOnionSkinPrevCount(), canvas.getOnionSkinNextCount());
        }

        m_topBar.Render(window);
        m_toolOptionsBar.Render(window);
        m_toolDock.Render(window);
        m_rightDockTabs.Render(window);
        m_statusBar.Render(window);

        g_aiPanel.draw(window);
        g_aiReviewModal.draw(window);

        if (showingText) window.draw(uiText);
        if (isTypingPrompt) {
            window.draw(promptBox);
            window.draw(promptDisplay);
        }

        keybindPanel.draw(window);
        if (exportModal.getIsOpen()) exportModal.draw(window);

        if (AIManager::getInstance().isProcessingAsync()) {
            window.draw(loadingOverlay);
            window.draw(loadingBox);
            window.draw(loadingText);
            window.draw(loadingSpinner);
            window.draw(loadingCancelBtn);
            window.draw(loadingCancelText);
        }

        if (showUnsavedWarning) {
            sf::RectangleShape overlay(sf::Vector2f(1920.f, 1080.f));
            overlay.setFillColor(sf::Color(0, 0, 0, 160));
            window.draw(overlay);

            float boxWidth = 450.f;
            float boxHeight = 200.f;
            float cx = (1920.f - boxWidth) / 2.f;
            float cy = (1080.f - boxHeight) / 2.f;

            sf::FloatRect warnBounds(cx, cy, boxWidth, boxHeight);
            WisdomUI::Theme::DrawSunsetPanel(window, warnBounds, 1.0f);

            WisdomUI::Theme::DrawCrispText(window, font, "UNSAVED CHANGES", 20, cx + boxWidth / 2.0f, cy + 30.f, WisdomUI::Theme::SunsetAmber, sf::Color(10, 4, 16), true, true);
            WisdomUI::Theme::DrawCrispText(window, font, "Would you like to save before leaving?", 14, cx + boxWidth / 2.0f, cy + 70.f, WisdomUI::Theme::TextSecondary, sf::Color(10, 4, 16), true, true);

            sf::Vector2i mousePosI = sf::Mouse::getPosition(window);
            sf::Vector2f mousePos = window.mapPixelToCoords(mousePosI);

            sf::FloatRect saveBounds(cx + 30.f, cy + 120.f, 110.f, 40.f);
            sf::FloatRect discardBounds(cx + 170.f, cy + 120.f, 110.f, 40.f);
            sf::FloatRect cancelBounds(cx + 310.f, cy + 120.f, 110.f, 40.f);

            WisdomUI::Theme::DrawThemedButton(window, saveBounds, "Save", font, 14, false, saveBounds.contains(mousePos), false, 1.0f);
            WisdomUI::Theme::DrawThemedButton(window, discardBounds, "Discard", font, 14, false, discardBounds.contains(mousePos), true, 1.0f);
            WisdomUI::Theme::DrawThemedButton(window, cancelBounds, "Cancel", font, 14, false, cancelBounds.contains(mousePos), false, 1.0f);
        }

        if (m_showEscapeMenu) {
            drawEscapeMenu(window, canvas, timeline);
        }

        m_toolDock.RenderTooltip(window);
    }
}

bool loadStudioFont(sf::Font& font) {
    if (font.loadFromFile("Resources/font.ttf")) return true;
    if (font.loadFromFile("../Resources/font.ttf")) return true;
    if (font.loadFromFile("../../Resources/font.ttf")) return true;
    if (font.loadFromFile("C:/Windows/Fonts/arial.ttf")) return true;
    return false;
}

void UIManager::initMinigame() {
    m_arcadeHero.pos = sf::Vector2f(960.f, 660.f);
    m_arcadeHero.vel = sf::Vector2f(0.f, 0.f);
    m_arcadeHero.dir = 0;
    m_arcadeHero.nextDir = 0;
    m_arcadeHero.speed = 260.f;
    m_arcadeHero.mouthAnim = 0.f;
    m_arcadeHero.deathAnim = 0.f;
    m_arcadeHero.isDying = false;
    m_arcadeHero.lives = 3;
    m_arcadeHero.invulnTimer = 0.f;

    m_arcadeScore = 0;
    m_arcadeGlobalTime = 0.0f;
    m_arcadeMasterTimer = 0.0f;
    m_arcadePendingAction = "";
    m_arcadeActionDelay = 0.0f;

    m_arcadeGhosts.clear();
    m_arcadeGhosts.push_back({ sf::Vector2f(920.f, 525.f), sf::Vector2f(0.f, -190.f), sf::Color(255, 45, 85), GhostPersonality::Shadow, 1, 190.f, 0.f, false, 0.f, false, sf::Vector2f(920.f, 525.f) });
    m_arcadeGhosts.push_back({ sf::Vector2f(950.f, 525.f), sf::Vector2f(0.f, -190.f), sf::Color(255, 140, 210), GhostPersonality::Speedy, 1, 200.f, 0.5f, false, 0.f, false, sf::Vector2f(950.f, 525.f) });
    m_arcadeGhosts.push_back({ sf::Vector2f(970.f, 525.f), sf::Vector2f(0.f, -190.f), sf::Color(50, 220, 255), GhostPersonality::Bashful, 1, 185.f, 1.0f, false, 0.f, false, sf::Vector2f(970.f, 525.f) });
    m_arcadeGhosts.push_back({ sf::Vector2f(1000.f, 525.f), sf::Vector2f(0.f, -190.f), sf::Color(255, 175, 45), GhostPersonality::Pokey, 1, 180.f, 1.5f, false, 0.f, false, sf::Vector2f(1000.f, 525.f) });

    m_arcadePortals.clear();
    m_arcadePortals.push_back({ "new_project", "1P START", "NEW CANVAS", "SPACE / CLICK", sf::FloatRect(200.f, 180.f, 220.f, 80.f), sf::Color(0, 255, 200), 0.f, 0.f, 0.f, true });
    m_arcadePortals.push_back({ "projects", "ARCHIVES", "PROJECT VAULT", "O", sf::FloatRect(1500.f, 180.f, 220.f, 80.f), sf::Color(255, 110, 255), 1.2f, 0.f, 0.f, true });
    m_arcadePortals.push_back({ "keybinds", "KEYS", "SHORTCUTS", "K", sf::FloatRect(680.f, 180.f, 210.f, 75.f), sf::Color(255, 215, 60), 0.8f, 0.f, 0.f, true });
    m_arcadePortals.push_back({ "credits", "FAMOUS", "HALL OF FAME", "C", sf::FloatRect(1030.f, 180.f, 210.f, 75.f), sf::Color(255, 160, 40), 1.9f, 0.f, 0.f, true });
    m_arcadePortals.push_back({ "settings", "CONFIG", "SETTINGS", "ESC", sf::FloatRect(200.f, 780.f, 220.f, 80.f), sf::Color(80, 255, 120), 2.4f, 0.f, 0.f, true });
    m_arcadePortals.push_back({ "tutorials", "HOW TO PLAY", "CODEX", "F1", sf::FloatRect(1500.f, 780.f, 220.f, 80.f), sf::Color(255, 140, 60), 3.6f, 0.f, 0.f, true });
    m_arcadePortals.push_back({ "exit", "POWER OFF", "TERMINATE", "ALT+F4", sf::FloatRect(850.f, 780.f, 220.f, 80.f), sf::Color(255, 50, 70), 4.2f, 0.f, 0.f, true });

    m_arcadeMazeWalls.clear();

    m_arcadeMazeWalls.push_back(sf::FloatRect(120.f, 100.f, 1680.f, 10.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(120.f, 950.f, 1680.f, 10.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(120.f, 100.f, 10.f, 860.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(1790.f, 100.f, 10.f, 860.f));

    m_arcadeMazeWalls.push_back(sf::FloatRect(190.f, 170.f, 240.f, 8.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(190.f, 170.f, 8.f, 100.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(422.f, 170.f, 8.f, 100.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(190.f, 270.f, 75.f, 8.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(355.f, 270.f, 75.f, 8.f));

    m_arcadeMazeWalls.push_back(sf::FloatRect(1490.f, 170.f, 240.f, 8.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(1490.f, 170.f, 8.f, 100.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(1722.f, 170.f, 8.f, 100.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(1490.f, 270.f, 75.f, 8.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(1655.f, 270.f, 75.f, 8.f));

    m_arcadeMazeWalls.push_back(sf::FloatRect(670.f, 170.f, 230.f, 8.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(670.f, 170.f, 8.f, 95.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(892.f, 170.f, 8.f, 95.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(670.f, 265.f, 75.f, 8.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(825.f, 265.f, 75.f, 8.f));

    m_arcadeMazeWalls.push_back(sf::FloatRect(1020.f, 170.f, 230.f, 8.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(1020.f, 170.f, 8.f, 95.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(1242.f, 170.f, 8.f, 95.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(1020.f, 265.f, 75.f, 8.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(1175.f, 265.f, 75.f, 8.f));

    m_arcadeMazeWalls.push_back(sf::FloatRect(190.f, 870.f, 240.f, 8.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(190.f, 770.f, 8.f, 108.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(422.f, 770.f, 8.f, 108.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(190.f, 770.f, 75.f, 8.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(355.f, 770.f, 75.f, 8.f));

    m_arcadeMazeWalls.push_back(sf::FloatRect(1490.f, 870.f, 240.f, 8.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(1490.f, 770.f, 8.f, 108.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(1722.f, 770.f, 8.f, 108.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(1490.f, 770.f, 75.f, 8.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(1655.f, 770.f, 75.f, 8.f));

    m_arcadeMazeWalls.push_back(sf::FloatRect(840.f, 870.f, 240.f, 8.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(840.f, 770.f, 8.f, 108.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(1072.f, 770.f, 8.f, 108.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(840.f, 770.f, 75.f, 8.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(1005.f, 770.f, 75.f, 8.f));

    m_arcadeMazeWalls.push_back(sf::FloatRect(870.f, 470.f, 60.f, 8.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(990.f, 470.f, 60.f, 8.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(870.f, 570.f, 180.f, 8.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(870.f, 470.f, 8.f, 108.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(1042.f, 470.f, 8.f, 108.f));

    m_arcadeMazeWalls.push_back(sf::FloatRect(530.f, 340.f, 8.f, 140.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(530.f, 560.f, 8.f, 140.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(1382.f, 340.f, 8.f, 140.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(1382.f, 560.f, 8.f, 140.f));

    m_arcadeMazeWalls.push_back(sf::FloatRect(210.f, 520.f, 210.f, 8.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(1500.f, 520.f, 210.f, 8.f));

    m_arcadeMazeWalls.push_back(sf::FloatRect(650.f, 370.f, 170.f, 8.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(1100.f, 370.f, 170.f, 8.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(650.f, 650.f, 170.f, 8.f));
    m_arcadeMazeWalls.push_back(sf::FloatRect(1100.f, 650.f, 170.f, 8.f));

    m_arcadeCollectibles.clear();
    auto spawnDotsAlongLine = [this](float x1, float y1, float x2, float y2, float step) {
        float len = std::hypot(x2 - x1, y2 - y1);
        int count = static_cast<int>(len / step);
        for (int i = 0; i <= count; ++i) {
            float t = static_cast<float>(i) / std::max(1, count);
            m_arcadeCollectibles.push_back({ sf::Vector2f(x1 + (x2 - x1) * t, y1 + (y2 - y1) * t), CollectibleType::Dot, 10, false, 0.f, static_cast<float>(rand() % 100) * 0.1f });
        }
        };

    spawnDotsAlongLine(155.f, 135.f, 1765.f, 135.f, 40.f);
    spawnDotsAlongLine(155.f, 915.f, 1765.f, 915.f, 40.f);
    spawnDotsAlongLine(155.f, 140.f, 155.f, 910.f, 40.f);
    spawnDotsAlongLine(1765.f, 140.f, 1765.f, 910.f, 40.f);

    spawnDotsAlongLine(475.f, 140.f, 475.f, 910.f, 40.f);
    spawnDotsAlongLine(1445.f, 140.f, 1445.f, 910.f, 40.f);

    spawnDotsAlongLine(590.f, 310.f, 1330.f, 310.f, 42.f);
    spawnDotsAlongLine(590.f, 720.f, 1330.f, 720.f, 42.f);

    spawnDotsAlongLine(590.f, 440.f, 770.f, 440.f, 38.f);
    spawnDotsAlongLine(1150.f, 440.f, 1330.f, 440.f, 38.f);
    spawnDotsAlongLine(590.f, 590.f, 770.f, 590.f, 38.f);
    spawnDotsAlongLine(1150.f, 590.f, 1330.f, 590.f, 38.f);

    m_arcadeCollectibles.push_back({ sf::Vector2f(155.f, 135.f), CollectibleType::PowerPellet, 50, false, 0.f, 0.f });
    m_arcadeCollectibles.push_back({ sf::Vector2f(1765.f, 135.f), CollectibleType::PowerPellet, 50, false, 0.f, 1.f });
    m_arcadeCollectibles.push_back({ sf::Vector2f(155.f, 915.f), CollectibleType::PowerPellet, 50, false, 0.f, 2.f });
    m_arcadeCollectibles.push_back({ sf::Vector2f(1765.f, 915.f), CollectibleType::PowerPellet, 50, false, 0.f, 3.f });

    m_arcadeCollectibles.push_back({ sf::Vector2f(475.f, 520.f), CollectibleType::Cherry, 100, false, 0.f, 0.5f });
    m_arcadeCollectibles.push_back({ sf::Vector2f(1445.f, 520.f), CollectibleType::Orange, 500, false, 0.f, 1.5f });
    m_arcadeCollectibles.push_back({ sf::Vector2f(960.f, 380.f), CollectibleType::Grape, 1000, false, 0.f, 2.5f });
    m_arcadeCollectibles.push_back({ sf::Vector2f(960.f, 650.f), CollectibleType::Key, 3000, false, 0.f, 3.5f });

    m_arcadeFX.clear();
    m_arcadeFloaters.clear();
}

void UIManager::spawnParticleBurst(sf::Vector2f pos, sf::Color col, int count, float spd) {
    for (int i = 0; i < count; ++i) {
        ArcadeParticleFX p;
        p.pos = pos;
        float ang = static_cast<float>(rand() % 360) * 3.14159f / 180.f;
        float s = (spd * 0.4f) + static_cast<float>(rand() % static_cast<int>(spd * 0.8f + 1.f));
        p.vel = sf::Vector2f(std::cos(ang) * s, std::sin(ang) * s);
        p.maxLife = 0.35f + static_cast<float>(rand() % 10) * 0.03f;
        p.life = p.maxLife;
        p.size = 2.f + static_cast<float>(rand() % 4);
        p.color = col;
        m_arcadeFX.push_back(p);
    }
}

void UIManager::addFloatingText(const std::string& str, sf::Vector2f pos, sf::Color col) {
    ArcadeScoreFloater f;
    f.text = str;
    f.pos = pos;
    f.vel = sf::Vector2f(0.f, -70.f);
    f.maxLife = 0.85f;
    f.life = f.maxLife;
    f.color = col;
    m_arcadeFloaters.push_back(f);
}

void UIManager::triggerArcadeStation(const std::string& id, sf::RenderWindow& window) {
    m_isArcadePaused = true;

    for (auto& p : m_arcadePortals) {
        if (p.id == id) {
            p.triggerFlash = 1.0f;
            spawnParticleBurst(sf::Vector2f(p.bounds.left + p.bounds.width / 2.f, p.bounds.top + p.bounds.height / 2.f), p.marqueeColor, 32, 340.f);
        }
    }

    if (id == "exit") {
        window.close();
    }
    else if (id == "new_project") newProjectModal.open();
    else if (id == "projects") currentMenuState = MenuState::Projects;
    else if (id == "settings") currentMenuState = MenuState::Settings;
    else if (id == "tutorials") { currentMenuState = MenuState::Tutorials; activeTutorialIndex = -1; }
    else if (id == "keybinds") keybindPanel.toggle();
    else if (id == "credits") { currentMenuState = MenuState::Credits; easterEggClicks = 0; }
}

void UIManager::updateMinigame(float dt, sf::Vector2f mousePos, sf::RenderWindow& window) {
    bool isAnyScreenOpen = (currentMenuState != MenuState::Main)
        || newProjectModal.getIsOpen()
        || keybindPanel.isVisible()
        || exportModal.getIsOpen();

    if (isAnyScreenOpen) {
        m_isArcadePaused = true;
    }
    else {
        m_isArcadePaused = false;
    }

    if (m_isArcadePaused) {
        return;
    }

    m_arcadeGlobalTime += dt;
    m_arcadeMasterTimer += dt;

    if (m_arcadeHero.invulnTimer > 0.f) m_arcadeHero.invulnTimer -= dt;

    if (m_arcadeHero.isDying) {
        m_arcadeHero.deathAnim += dt * 3.f;
        if (m_arcadeHero.deathAnim >= 3.14159f) {
            m_arcadeHero.isDying = false;
            m_arcadeHero.deathAnim = 0.f;

            if (m_arcadeHero.lives <= 0) {
                window.close();
                return;
            }

            m_arcadeHero.pos = sf::Vector2f(960.f, 660.f);
            m_arcadeHero.vel = sf::Vector2f(0.f, 0.f);
            m_arcadeHero.invulnTimer = 2.0f;
        }
    }
    else {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) || sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) m_arcadeHero.nextDir = 0;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) || sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) m_arcadeHero.nextDir = 1;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) || sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) m_arcadeHero.nextDir = 2;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) || sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) m_arcadeHero.nextDir = 3;

        auto getDirVector = [](int d) -> sf::Vector2f {
            if (d == 0) return sf::Vector2f(1.f, 0.f);
            if (d == 1) return sf::Vector2f(0.f, -1.f);
            if (d == 2) return sf::Vector2f(-1.f, 0.f);
            return sf::Vector2f(0.f, 1.f);
            };

        sf::Vector2f desiredVel = getDirVector(m_arcadeHero.nextDir) * m_arcadeHero.speed;
        sf::FloatRect checkNext(m_arcadeHero.pos.x + desiredVel.x * dt - 10.f, m_arcadeHero.pos.y + desiredVel.y * dt - 10.f, 20.f, 20.f);
        bool nextBlocked = false;
        for (const auto& w : m_arcadeMazeWalls) {
            if (w.intersects(checkNext)) { nextBlocked = true; break; }
        }

        if (!nextBlocked) {
            m_arcadeHero.dir = m_arcadeHero.nextDir;
            m_arcadeHero.vel = desiredVel;
        }

        sf::FloatRect checkCurrent(m_arcadeHero.pos.x + m_arcadeHero.vel.x * dt - 10.f, m_arcadeHero.pos.y + m_arcadeHero.vel.y * dt - 10.f, 20.f, 20.f);
        bool curBlocked = false;
        for (const auto& w : m_arcadeMazeWalls) {
            if (w.intersects(checkCurrent)) { curBlocked = true; break; }
        }

        if (!curBlocked) {
            m_arcadeHero.pos += m_arcadeHero.vel * dt;
            m_arcadeHero.mouthAnim += dt * 14.f;
        }
        else {
            m_arcadeHero.vel = sf::Vector2f(0.f, 0.f);
        }

        m_arcadeHero.pos.x = std::clamp(m_arcadeHero.pos.x, 155.f, 1755.f);
        m_arcadeHero.pos.y = std::clamp(m_arcadeHero.pos.y, 145.f, 905.f);
    }

    sf::FloatRect heroBounds(m_arcadeHero.pos.x - 10.f, m_arcadeHero.pos.y - 10.f, 20.f, 20.f);

    for (auto& item : m_arcadeCollectibles) {
        if (item.collected) {
            item.respawnTimer -= dt;
            if (item.respawnTimer <= 0.f) item.collected = false;
        }
        else {
            item.animPhase += dt * 4.f;
            float pickRadius = (item.type == CollectibleType::Dot) ? 14.f : 24.f;
            sf::FloatRect itemRect(item.pos.x - pickRadius / 2.f, item.pos.y - pickRadius / 2.f, pickRadius, pickRadius);

            if (heroBounds.intersects(itemRect) && !m_arcadeHero.isDying) {
                item.collected = true;
                item.respawnTimer = (item.type == CollectibleType::Dot) ? 12.f : 25.f;
                m_arcadeScore += item.points;
                if (m_arcadeScore > m_arcadeHighScore) m_arcadeHighScore = m_arcadeScore;

                if (item.type == CollectibleType::PowerPellet) {
                    for (auto& g : m_arcadeGhosts) {
                        g.isScared = true;
                        g.scaredTimer = 8.0f;
                    }
                    spawnParticleBurst(item.pos, WisdomUI::Theme::SunsetGold, 20, 240.f);
                    addFloatingText("POWER UP!", item.pos, WisdomUI::Theme::SunsetGold);
                }
                else if (item.type != CollectibleType::Dot) {
                    spawnParticleBurst(item.pos, WisdomUI::Theme::SunsetAmber, 16, 180.f);
                    addFloatingText("+" + std::to_string(item.points), item.pos, WisdomUI::Theme::SunsetAmber);
                }
            }
        }
    }

    for (auto& g : m_arcadeGhosts) {
        g.animTimer += dt * 8.f;
        if (g.isScared) {
            g.scaredTimer -= dt;
            if (g.scaredTimer <= 0.f) g.isScared = false;
        }

        if (g.isEaten) {
            sf::Vector2f toSpawn = g.spawnPos - g.pos;
            float dist = std::hypot(toSpawn.x, toSpawn.y);
            if (dist < 10.f) {
                g.isEaten = false;
                g.isScared = false;
                g.pos = g.spawnPos;
            }
            else {
                g.pos += (toSpawn / dist) * 450.f * dt;
            }
            continue;
        }

        auto tryGhostDirection = [&](int dirChoice) -> bool {
            sf::Vector2f dirV(0.f, 0.f);
            if (dirChoice == 0) dirV.x = 1.f;
            else if (dirChoice == 1) dirV.y = -1.f;
            else if (dirChoice == 2) dirV.x = -1.f;
            else dirV.y = 1.f;

            float curSpd = g.isScared ? (g.speed * 0.65f) : g.speed;
            sf::FloatRect checkStep(g.pos.x + dirV.x * curSpd * dt - 10.f, g.pos.y + dirV.y * curSpd * dt - 10.f, 20.f, 20.f);
            for (const auto& w : m_arcadeMazeWalls) {
                if (w.intersects(checkStep)) return false;
            }
            g.dir = dirChoice;
            g.vel = dirV * curSpd;
            return true;
            };

        if (!tryGhostDirection(g.dir) || (rand() % 80 == 0)) {
            std::vector<int> candidates = { 0, 1, 2, 3 };
            int opposite = (g.dir + 2) % 4;
            candidates.erase(std::remove(candidates.begin(), candidates.end(), opposite), candidates.end());
            static std::mt19937 rng(std::random_device{}());
            std::shuffle(candidates.begin(), candidates.end(), rng);

            bool found = false;
            for (int d : candidates) {
                if (tryGhostDirection(d)) { found = true; break; }
            }
            if (!found) tryGhostDirection(opposite);
        }

        g.pos += g.vel * dt;
        g.pos.x = std::clamp(g.pos.x, 155.f, 1755.f);
        g.pos.y = std::clamp(g.pos.y, 145.f, 905.f);

        sf::FloatRect ghostBounds(g.pos.x - 10.f, g.pos.y - 10.f, 20.f, 20.f);
        if (ghostBounds.intersects(heroBounds) && !m_arcadeHero.isDying && m_arcadeHero.invulnTimer <= 0.f) {
            if (g.isScared) {
                g.isEaten = true;
                m_arcadeScore += 400;
                spawnParticleBurst(g.pos, sf::Color(80, 200, 255), 24, 280.f);
                addFloatingText("+400", g.pos, sf::Color(80, 200, 255));
            }
            else {
                m_arcadeHero.isDying = true;
                m_arcadeHero.deathAnim = 0.f;
                m_arcadeHero.lives--;
                spawnParticleBurst(m_arcadeHero.pos, WisdomUI::Theme::SunsetGold, 36, 320.f);
                addFloatingText("OUCH!", m_arcadeHero.pos, sf::Color(255, 60, 60));
            }
        }
    }

    for (auto& portal : m_arcadePortals) {
        bool hov = portal.bounds.contains(mousePos);
        bool heroInside = portal.bounds.intersects(heroBounds);
        portal.hoverAlpha += (((hov || heroInside) ? 1.0f : 0.0f) - portal.hoverAlpha) * 16.0f * dt;
        portal.triggerFlash = std::max(0.0f, portal.triggerFlash - 3.0f * dt);
        portal.pulse += dt * 3.f;

        if (heroInside) {
            if (portal.isArmed && !m_isArcadePaused) {
                portal.isArmed = false;
                triggerArcadeStation(portal.id, window);
            }
        }
        else {
            portal.isArmed = true;
        }
    }

    for (auto& p : m_arcadeFX) {
        p.life -= dt;
        p.pos += p.vel * dt;
    }
    m_arcadeFX.erase(std::remove_if(m_arcadeFX.begin(), m_arcadeFX.end(), [](const ArcadeParticleFX& p) { return p.life <= 0.f; }), m_arcadeFX.end());

    for (auto& f : m_arcadeFloaters) {
        f.life -= dt;
        f.pos += f.vel * dt;
    }
    m_arcadeFloaters.erase(std::remove_if(m_arcadeFloaters.begin(), m_arcadeFloaters.end(), [](const ArcadeScoreFloater& f) { return f.life <= 0.f; }), m_arcadeFloaters.end());
}

void UIManager::drawPixelHero(sf::RenderWindow& window) {
    if (m_arcadeHero.isDying) {
        float angle = m_arcadeHero.deathAnim * 180.f;
        sf::CircleShape deathShape(18.f);
        deathShape.setOrigin(18.f, 18.f);
        deathShape.setPosition(m_arcadeHero.pos);
        deathShape.setFillColor(WisdomUI::Theme::SunsetGold);
        deathShape.setScale(std::max(0.05f, 1.0f - m_arcadeHero.deathAnim / 3.14159f), std::max(0.05f, 1.0f - m_arcadeHero.deathAnim / 3.14159f));
        deathShape.setRotation(angle);
        window.draw(deathShape);
        return;
    }

    if (m_arcadeHero.invulnTimer > 0.f && static_cast<int>(m_arcadeMasterTimer * 15.f) % 2 == 0) return;

    float px = std::floor(m_arcadeHero.pos.x);
    float py = std::floor(m_arcadeHero.pos.y);
    float mouth = std::abs(std::sin(m_arcadeHero.mouthAnim)) * 12.f;

    sf::CircleShape body(18.f);
    body.setOrigin(18.f, 18.f);
    body.setPosition(px, py);
    body.setFillColor(WisdomUI::Theme::SunsetGold);
    body.setOutlineThickness(2.f);
    body.setOutlineColor(WisdomUI::Theme::SunsetCoralDark);
    window.draw(body);

    sf::ConvexShape wedge(3);
    wedge.setPoint(0, sf::Vector2f(0.f, 0.f));
    wedge.setPoint(1, sf::Vector2f(24.f, -mouth));
    wedge.setPoint(2, sf::Vector2f(24.f, mouth));
    wedge.setPosition(px, py);
    wedge.setFillColor(WisdomUI::Theme::SunsetDeepDark);

    float rot = 0.f;
    if (m_arcadeHero.dir == 1) rot = 270.f;
    else if (m_arcadeHero.dir == 2) rot = 180.f;
    else if (m_arcadeHero.dir == 3) rot = 90.f;
    wedge.setRotation(rot);
    window.draw(wedge);

    sf::CircleShape eye(3.5f);
    eye.setOrigin(1.75f, 1.75f);
    float eyeX = px + (m_arcadeHero.dir == 2 ? -4.f : 4.f);
    float eyeY = py - 8.f;
    eye.setPosition(eyeX, eyeY);
    eye.setFillColor(sf::Color(14, 4, 20));
    window.draw(eye);
}

void UIManager::drawPixelGhost(sf::RenderWindow& window, const ArcadeGhost& g) {
    float gx = std::floor(g.pos.x);
    float gy = std::floor(g.pos.y);

    if (g.isEaten) {
        sf::CircleShape eyeL(5.f); eyeL.setPosition(gx - 10.f, gy - 6.f); eyeL.setFillColor(sf::Color::White); window.draw(eyeL);
        sf::CircleShape eyeR(5.f); eyeR.setPosition(gx + 2.f, gy - 6.f); eyeR.setFillColor(sf::Color::White); window.draw(eyeR);
        sf::CircleShape pupL(2.5f); pupL.setPosition(gx - 8.f, gy - 4.f); pupL.setFillColor(sf::Color(20, 60, 220)); window.draw(pupL);
        sf::CircleShape pupR(2.5f); pupR.setPosition(gx + 4.f, gy - 4.f); pupR.setFillColor(sf::Color(20, 60, 220)); window.draw(pupR);
        return;
    }

    sf::Color bodyCol = g.baseColor;
    if (g.isScared) {
        bool flash = (g.scaredTimer < 2.5f) && (static_cast<int>(m_arcadeMasterTimer * 8.f) % 2 == 0);
        bodyCol = flash ? sf::Color(255, 255, 255) : sf::Color(40, 80, 230);
    }

    sf::CircleShape head(16.f);
    head.setOrigin(16.f, 16.f);
    head.setPosition(gx, gy - 4.f);
    head.setFillColor(bodyCol);
    window.draw(head);

    sf::RectangleShape torso(sf::Vector2f(32.f, 18.f));
    torso.setOrigin(16.f, 0.f);
    torso.setPosition(gx, gy - 4.f);
    torso.setFillColor(bodyCol);
    window.draw(torso);

    float skirtWave = std::sin(g.animTimer) * 3.f;
    for (int i = 0; i < 3; ++i) {
        sf::ConvexShape tentacle(3);
        float tx = gx - 16.f + static_cast<float>(i) * 11.f;
        tentacle.setPoint(0, sf::Vector2f(tx, gy + 14.f));
        tentacle.setPoint(1, sf::Vector2f(tx + 10.f, gy + 14.f));
        tentacle.setPoint(2, sf::Vector2f(tx + 5.f, gy + 20.f + (i % 2 == 0 ? skirtWave : -skirtWave)));
        tentacle.setFillColor(bodyCol);
        window.draw(tentacle);
    }

    if (!g.isScared) {
        float lookX = (g.dir == 0) ? 3.f : ((g.dir == 2) ? -3.f : 0.f);
        float lookY = (g.dir == 3) ? 3.f : ((g.dir == 1) ? -3.f : 0.f);

        sf::CircleShape scleraL(5.f); scleraL.setPosition(gx - 10.f, gy - 8.f); scleraL.setFillColor(sf::Color::White); window.draw(scleraL);
        sf::CircleShape scleraR(5.f); scleraR.setPosition(gx + 2.f, gy - 8.f); scleraR.setFillColor(sf::Color::White); window.draw(scleraR);
        sf::CircleShape pupL(2.5f); pupL.setPosition(gx - 8.f + lookX, gy - 6.f + lookY); pupL.setFillColor(sf::Color(20, 30, 90)); window.draw(pupL);
        sf::CircleShape pupR(2.5f); pupR.setPosition(gx + 4.f + lookX, gy - 6.f + lookY); pupR.setFillColor(sf::Color(20, 30, 90)); window.draw(pupR);
    }
    else {
        sf::CircleShape scaredEyeL(3.f); scaredEyeL.setPosition(gx - 8.f, gy - 6.f); scaredEyeL.setFillColor(sf::Color(255, 180, 180)); window.draw(scaredEyeL);
        sf::CircleShape scaredEyeR(3.f); scaredEyeR.setPosition(gx + 4.f, gy - 6.f); scaredEyeR.setFillColor(sf::Color(255, 180, 180)); window.draw(scaredEyeR);
    }
}

void UIManager::drawPixelItem(sf::RenderWindow& window, sf::Vector2f pos, CollectibleType type, float anim) {
    float px = std::floor(pos.x);
    float py = std::floor(pos.y);

    if (type == CollectibleType::Dot) {
        sf::RectangleShape dot(sf::Vector2f(6.f, 6.f));
        dot.setOrigin(3.f, 3.f);
        dot.setPosition(px, py);
        dot.setFillColor(sf::Color(255, 215, 160));
        window.draw(dot);
        return;
    }

    if (type == CollectibleType::PowerPellet) {
        float pulse = std::sin(anim) * 3.f;
        sf::CircleShape pellet(9.f + pulse);
        pellet.setOrigin(pellet.getRadius(), pellet.getRadius());
        pellet.setPosition(px, py);
        pellet.setFillColor(WisdomUI::Theme::SunsetGold);
        pellet.setOutlineThickness(1.5f);
        pellet.setOutlineColor(WisdomUI::Theme::SunsetAmber);
        window.draw(pellet);
        return;
    }

    float bob = std::sin(anim) * 3.f;
    sf::Vector2f drawPos(px, py + bob);

    if (type == CollectibleType::Cherry) {
        sf::CircleShape c1(7.f); c1.setPosition(drawPos.x - 9.f, drawPos.y - 2.f); c1.setFillColor(sf::Color(255, 30, 70)); window.draw(c1);
        sf::CircleShape c2(7.f); c2.setPosition(drawPos.x + 1.f, drawPos.y); c2.setFillColor(sf::Color(255, 30, 70)); window.draw(c2);
        sf::RectangleShape stem(sf::Vector2f(2.f, 10.f)); stem.setPosition(drawPos.x - 2.f, drawPos.y - 10.f); stem.setFillColor(sf::Color(80, 220, 90)); stem.setRotation(18.f); window.draw(stem);
    }
    else if (type == CollectibleType::Orange) {
        sf::CircleShape o(9.f); o.setPosition(drawPos.x - 9.f, drawPos.y - 7.f); o.setFillColor(sf::Color(255, 140, 30)); window.draw(o);
        sf::RectangleShape leaf(sf::Vector2f(5.f, 4.f)); leaf.setPosition(drawPos.x - 2.f, drawPos.y - 11.f); leaf.setFillColor(sf::Color(90, 230, 90)); window.draw(leaf);
    }
    else if (type == CollectibleType::Grape) {
        sf::CircleShape g1(6.f); g1.setPosition(drawPos.x - 7.f, drawPos.y - 8.f); g1.setFillColor(sf::Color(180, 60, 255)); window.draw(g1);
        sf::CircleShape g2(6.f); g2.setPosition(drawPos.x + 1.f, drawPos.y - 8.f); g2.setFillColor(sf::Color(180, 60, 255)); window.draw(g2);
        sf::CircleShape g3(6.f); g3.setPosition(drawPos.x - 3.f, drawPos.y - 1.f); g3.setFillColor(sf::Color(150, 40, 235)); window.draw(g3);
    }
    else if (type == CollectibleType::Key) {
        sf::CircleShape kHead(7.f); kHead.setPosition(drawPos.x - 7.f, drawPos.y - 10.f); kHead.setFillColor(WisdomUI::Theme::SunsetGold); window.draw(kHead);
        sf::RectangleShape kShaft(sf::Vector2f(4.f, 14.f)); kShaft.setPosition(drawPos.x - 2.f, drawPos.y - 2.f); kShaft.setFillColor(WisdomUI::Theme::SunsetGold); window.draw(kShaft);
        sf::RectangleShape kTooth(sf::Vector2f(6.f, 3.f)); kTooth.setPosition(drawPos.x + 2.f, drawPos.y + 4.f); kTooth.setFillColor(WisdomUI::Theme::SunsetGold); window.draw(kTooth);
    }
}

void UIManager::drawStationPortal(sf::RenderWindow& window, const ArcadeStationPortal& p, sf::Vector2f mousePos) {
    sf::FloatRect b = p.bounds;
    bool isLit = p.triggerFlash > 0.05f;
    bool isHov = p.hoverAlpha > 0.2f;

    sf::RectangleShape shadow(sf::Vector2f(b.width, b.height));
    shadow.setPosition(b.left + 4.f, b.top + 4.f);
    shadow.setFillColor(sf::Color(0, 0, 0, 150));
    window.draw(shadow);

    sf::RectangleShape base(sf::Vector2f(b.width, b.height));
    base.setPosition(b.left, b.top);
    if (isLit) {
        base.setFillColor(sf::Color(255, 245, 210));
        base.setOutlineThickness(3.0f);
        base.setOutlineColor(sf::Color::White);
    }
    else if (isHov) {
        base.setFillColor(p.id == "exit" ? sf::Color(140, 20, 40, 240) : sf::Color(48, 16, 62, 240));
        base.setOutlineThickness(2.0f);
        base.setOutlineColor(p.marqueeColor);
    }
    else {
        base.setFillColor(sf::Color(16, 8, 24, 210));
        base.setOutlineThickness(1.5f);
        base.setOutlineColor(p.marqueeColor);
    }
    window.draw(base);

    sf::Color txtColor = isLit ? sf::Color(16, 4, 22) : (isHov ? sf::Color::White : p.marqueeColor);
    WisdomUI::Theme::DrawCrispText(window, font, p.title, 15, b.left + b.width / 2.f, b.top + 18.f, txtColor, sf::Color(14, 4, 20), true, true);
    WisdomUI::Theme::DrawCrispText(window, font, p.subtitle, 10, b.left + b.width / 2.f, b.top + 42.f, isLit ? sf::Color(50, 10, 60) : sf::Color(190, 190, 190), sf::Color::Transparent, true, true);

    sf::FloatRect badge(b.left + b.width / 2.f - 50.f, b.top + b.height - 20.f, 100.f, 15.f);
    sf::RectangleShape badgeBg(sf::Vector2f(badge.width, badge.height));
    badgeBg.setPosition(badge.left, badge.top);
    badgeBg.setFillColor(sf::Color(10, 4, 16));
    badgeBg.setOutlineThickness(1.f);
    badgeBg.setOutlineColor(p.marqueeColor);
    window.draw(badgeBg);

    WisdomUI::Theme::DrawCrispText(window, font, p.keyShortcut, 9, badge.left + badge.width / 2.f, badge.top + badge.height / 2.f, isHov ? sf::Color::White : WisdomUI::Theme::SunsetGold, sf::Color::Transparent, true, true);
}

void UIManager::drawArcadeBezelOverlay(sf::RenderWindow& window) {
    sf::RectangleShape topBezel(sf::Vector2f(1920.f, 60.f));
    topBezel.setPosition(0.f, 0.f);
    topBezel.setFillColor(sf::Color(14, 8, 12));
    window.draw(topBezel);

    sf::RectangleShape botBezel(sf::Vector2f(1920.f, 80.f));
    botBezel.setPosition(0.f, 1000.f);
    botBezel.setFillColor(sf::Color(14, 8, 12));
    window.draw(botBezel);

    sf::RectangleShape leftBezel(sf::Vector2f(80.f, 1080.f));
    leftBezel.setPosition(0.f, 0.f);
    leftBezel.setFillColor(sf::Color(14, 8, 12));
    window.draw(leftBezel);

    sf::RectangleShape rightBezel(sf::Vector2f(80.f, 1080.f));
    rightBezel.setPosition(1840.f, 0.f);
    rightBezel.setFillColor(sf::Color(14, 8, 12));
    window.draw(rightBezel);

    sf::RectangleShape outerGold(sf::Vector2f(1780.f, 960.f));
    outerGold.setPosition(70.f, 50.f);
    outerGold.setFillColor(sf::Color::Transparent);
    outerGold.setOutlineThickness(10.f);
    outerGold.setOutlineColor(sf::Color(190, 130, 45));
    window.draw(outerGold);

    sf::RectangleShape innerGold(sf::Vector2f(1764.f, 944.f));
    innerGold.setPosition(78.f, 58.f);
    innerGold.setFillColor(sf::Color::Transparent);
    innerGold.setOutlineThickness(3.f);
    innerGold.setOutlineColor(sf::Color(255, 215, 110));
    window.draw(innerGold);

    sf::RectangleShape scanline(sf::Vector2f(1760.f, 1.5f));
    scanline.setFillColor(sf::Color(0, 0, 0, 45));
    for (float y = 60.f; y < 1000.f; y += 4.f) {
        scanline.setPosition(80.f, y);
        window.draw(scanline);
    }
}

void UIManager::handleKeybindModalEvent(const sf::Event& event, sf::RenderWindow& window) {
    if (!m_showKeybinds) return;

    sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
    sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);

    if (!m_listeningKeyActionId.empty()) {
        if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Escape) {
                m_listeningKeyActionId = "";
                return;
            }
            bool ctrl = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::RControl);
            bool shift = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);
            bool alt = sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt) || sf::Keyboard::isKeyPressed(sf::Keyboard::RAlt);

            if (event.key.code != sf::Keyboard::LControl && event.key.code != sf::Keyboard::RControl &&
                event.key.code != sf::Keyboard::LShift && event.key.code != sf::Keyboard::RShift &&
                event.key.code != sf::Keyboard::LAlt && event.key.code != sf::Keyboard::RAlt) {

                Keybind newKb{ event.key.code, ctrl, shift, alt };
                keybindManager.setKeybind(m_listeningKeyActionId, newKb);
                m_listeningKeyActionId = "";
            }
            return;
        }
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::FloatRect modalBounds(280.f, 70.f, 1360.f, 940.f);
        sf::FloatRect closeBtn(modalBounds.left + modalBounds.width - 130.f, modalBounds.top + 20.f, 106.f, 44.f);
        sf::FloatRect restoreBtn(modalBounds.left + modalBounds.width - 320.f, modalBounds.top + 20.f, 174.f, 44.f);
        sf::FloatRect searchBox(modalBounds.left + 32.f, modalBounds.top + 84.f, 320.f, 44.f);

        if (closeBtn.contains(mousePos)) {
            m_showKeybinds = false;
            m_listeningKeyActionId = "";
            return;
        }
        if (restoreBtn.contains(mousePos)) {
            keybindManager.restoreDefaults();
            return;
        }
        if (searchBox.contains(mousePos)) {
            m_isTypingKeybindSearch = true;
            return;
        }
        else {
            m_isTypingKeybindSearch = false;
        }

        std::vector<std::string> cats = { "All", "Tools", "Project", "Edit", "Selection", "Timeline", "Layers", "View", "UI" };
        float tabX = modalBounds.left + 372.f;
        for (const auto& cat : cats) {
            sf::FloatRect tRect(tabX, modalBounds.top + 84.f, 98.f, 44.f);
            if (tRect.contains(mousePos)) {
                m_selectedKeybindCategory = cat;
                m_keybindScrollOffset = 0.0f;
                return;
            }
            tabX += 106.f;
        }

        sf::FloatRect listArea(modalBounds.left + 32.f, modalBounds.top + 156.f, modalBounds.width - 64.f, modalBounds.height - 188.f);
        float rowY = listArea.top - m_keybindScrollOffset;
        float rowH = 56.f;

        for (const auto& id : keybindManager.getActionOrder()) {
            const auto& act = keybindManager.getAction(id);
            if (m_selectedKeybindCategory != "All" && act.category != m_selectedKeybindCategory) continue;

            if (!m_keybindSearchQuery.empty()) {
                std::string q = m_keybindSearchQuery;
                std::string n = act.name;
                std::transform(q.begin(), q.end(), q.begin(), ::tolower);
                std::transform(n.begin(), n.end(), n.begin(), ::tolower);
                if (n.find(q) == std::string::npos) continue;
            }

            if (rowY + rowH >= listArea.top && rowY <= listArea.top + listArea.height) {
                sf::FloatRect bindBtn(listArea.left + listArea.width - 240.f, rowY + 8.f, 220.f, 40.f);
                if (bindBtn.contains(mousePos)) {
                    m_listeningKeyActionId = id;
                    return;
                }
            }
            rowY += rowH + 10.f;
        }
    }

    if (event.type == sf::Event::MouseWheelScrolled) {
        m_keybindScrollOffset = std::clamp(m_keybindScrollOffset - event.mouseWheelScroll.delta * 55.0f, 0.0f, m_keybindMaxScroll);
    }

    if (event.type == sf::Event::TextEntered && m_isTypingKeybindSearch) {
        if (event.text.unicode == '\b') {
            if (!m_keybindSearchQuery.empty()) m_keybindSearchQuery.pop_back();
        }
        else if (event.text.unicode >= 32 && event.text.unicode < 127 && m_keybindSearchQuery.length() < 24) {
            m_keybindSearchQuery += static_cast<char>(event.text.unicode);
        }
    }
}

void UIManager::drawKeybindModal(sf::RenderWindow& window) {
    if (!m_showKeybinds) return;

    sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
    sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);

    sf::RectangleShape overlay(sf::Vector2f(1920.f, 1080.f));
    overlay.setFillColor(sf::Color(10, 4, 16, 225));
    window.draw(overlay);

    sf::FloatRect modalBounds(280.f, 70.f, 1360.f, 940.f);
    WisdomUI::Theme::DrawSunsetPanel(window, modalBounds, 1.0f);

    WisdomUI::Theme::DrawCrispText(window, font, "STUDIO KEYBIND MATRIX", 28, modalBounds.left + 32.f, modalBounds.top + 20.f, WisdomUI::Theme::SunsetGold, sf::Color(14, 6, 20));
    WisdomUI::Theme::DrawCrispText(window, font, "CUSTOMIZE & REBIND WORKSPACE HOTKEYS", 14, modalBounds.left + 34.f, modalBounds.top + 54.f, WisdomUI::Theme::TextSecondary);

    sf::FloatRect closeBtn(modalBounds.left + modalBounds.width - 130.f, modalBounds.top + 20.f, 106.f, 44.f);
    sf::FloatRect restoreBtn(modalBounds.left + modalBounds.width - 320.f, modalBounds.top + 20.f, 174.f, 44.f);

    WisdomUI::Theme::DrawSunsetButton(window, restoreBtn, "Reset Defaults", font, 15, false, restoreBtn.contains(mousePos), false, 1.0f);
    WisdomUI::Theme::DrawSunsetButton(window, closeBtn, "Close", font, 15, false, closeBtn.contains(mousePos), false, 1.0f);

    sf::FloatRect searchBox(modalBounds.left + 32.f, modalBounds.top + 84.f, 320.f, 44.f);
    sf::RectangleShape sBox(sf::Vector2f(searchBox.width, searchBox.height));
    sBox.setPosition(searchBox.left, searchBox.top);
    sBox.setFillColor(WisdomUI::Theme::SunsetDeepDark);
    sBox.setOutlineThickness(1.5f);
    sBox.setOutlineColor(m_isTypingKeybindSearch ? WisdomUI::Theme::SunsetGold : WisdomUI::Theme::SunsetPlum);
    window.draw(sBox);

    std::string searchDisplay = m_keybindSearchQuery.empty() ? (m_isTypingKeybindSearch ? "_" : "Search shortcuts...") : (m_keybindSearchQuery + (m_isTypingKeybindSearch ? "_" : ""));
    sf::Color searchColor = m_keybindSearchQuery.empty() && !m_isTypingKeybindSearch ? WisdomUI::Theme::SunsetPlum : WisdomUI::Theme::TextPrimary;
    WisdomUI::Theme::DrawCrispText(window, font, searchDisplay, 15, searchBox.left + 14.f, searchBox.top + 12.f, searchColor);

    std::vector<std::string> cats = { "All", "Tools", "Project", "Edit", "Selection", "Timeline", "Layers", "View", "UI" };
    float tabX = modalBounds.left + 372.f;
    for (const auto& cat : cats) {
        sf::FloatRect tRect(tabX, modalBounds.top + 84.f, 98.f, 44.f);
        bool isSel = (m_selectedKeybindCategory == cat);
        WisdomUI::Theme::DrawSunsetButton(window, tRect, cat, font, 14, isSel, tRect.contains(mousePos), isSel, 1.0f);
        tabX += 106.f;
    }

    sf::RectangleShape div(sf::Vector2f(modalBounds.width - 64.f, 2.f));
    div.setPosition(modalBounds.left + 32.f, modalBounds.top + 142.f);
    div.setFillColor(WisdomUI::Theme::SunsetPlum);
    window.draw(div);

    sf::FloatRect listArea(modalBounds.left + 32.f, modalBounds.top + 156.f, modalBounds.width - 64.f, modalBounds.height - 188.f);

    float rowY = listArea.top - m_keybindScrollOffset;
    float rowH = 56.f;
    float totalH = 0.0f;

    for (const auto& id : keybindManager.getActionOrder()) {
        const auto& act = keybindManager.getAction(id);

        if (m_selectedKeybindCategory != "All" && act.category != m_selectedKeybindCategory) continue;

        if (!m_keybindSearchQuery.empty()) {
            std::string q = m_keybindSearchQuery;
            std::string n = act.name;
            std::transform(q.begin(), q.end(), q.begin(), ::tolower);
            std::transform(n.begin(), n.end(), n.begin(), ::tolower);
            if (n.find(q) == std::string::npos) continue;
        }

        totalH += rowH + 10.f;

        if (rowY + rowH >= listArea.top && rowY <= listArea.top + listArea.height) {
            sf::FloatRect rowRect(listArea.left, rowY, listArea.width, rowH);
            bool isListening = (m_listeningKeyActionId == id);
            bool isHov = rowRect.contains(mousePos);

            sf::RectangleShape rBg(sf::Vector2f(rowRect.width, rowRect.height));
            rBg.setPosition(rowRect.left, rowRect.top);
            rBg.setFillColor(isListening ? WisdomUI::Theme::SunsetSkyMid : (isHov ? WisdomUI::Theme::SunsetSkyTop : WisdomUI::Theme::SunsetDeepDark));
            rBg.setOutlineThickness(1.5f);
            rBg.setOutlineColor(isListening ? WisdomUI::Theme::SunsetGold : (isHov ? WisdomUI::Theme::SunsetAmber : WisdomUI::Theme::SunsetPlum));
            window.draw(rBg);

            WisdomUI::Theme::DrawCrispText(window, font, act.name, 18, rowRect.left + 22.f, rowRect.top + 16.f, isListening ? WisdomUI::Theme::SunsetGold : WisdomUI::Theme::TextPrimary);

            sf::FloatRect catTag(rowRect.left + 420.f, rowRect.top + 14.f, 110.f, 28.f);
            sf::RectangleShape cBg(sf::Vector2f(catTag.width, catTag.height));
            cBg.setPosition(catTag.left, catTag.top);
            cBg.setFillColor(sf::Color(14, 6, 20));
            cBg.setOutlineThickness(1.f);
            cBg.setOutlineColor(WisdomUI::Theme::SunsetPlum);
            window.draw(cBg);

            WisdomUI::Theme::DrawCrispText(window, font, act.category, 12, catTag.left + catTag.width / 2.f, catTag.top + catTag.height / 2.f, WisdomUI::Theme::SunsetPeach, sf::Color::Transparent, true, true);

            sf::FloatRect bindBtn(rowRect.left + rowRect.width - 240.f, rowRect.top + 8.f, 220.f, 40.f);
            std::string keyStr = isListening ? "Press Key..." : keybindManager.getActionString(id);
            if (keyStr.empty()) keyStr = "[Unbound]";

            WisdomUI::Theme::DrawSunsetButton(window, bindBtn, keyStr, font, 15, isListening, bindBtn.contains(mousePos), isListening, 1.0f);
        }

        rowY += rowH + 10.f;
    }

    m_keybindMaxScroll = std::max(0.0f, totalH - listArea.height);
}

void UIManager::toggleFullscreen(sf::RenderWindow& window, AppSettings& settings) {
    uiFullscreen = !uiFullscreen;
    settings.fullscreen = uiFullscreen;
    settings.borderless = false;

    if (uiFullscreen) {
        window.create(sf::VideoMode::getDesktopMode(), "Wisdom Park", sf::Style::Fullscreen);
    }
    else {
        window.create(sf::VideoMode(settings.resWidth, settings.resHeight), "Wisdom Park", sf::Style::Default);
        sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
        window.setPosition(sf::Vector2i(
            std::max(0, static_cast<int>((desktop.width - settings.resWidth) / 2)),
            std::max(0, static_cast<int>((desktop.height - settings.resHeight) / 2))
        ));
    }

    ApplyWindowIcon(window);

    window.setFramerateLimit(uiFpsLimit);
    window.setVerticalSyncEnabled(uiVsync);
    window.setView(WisdomUI::WorkspaceLayout::GetLetterboxView(window.getSize()));
    SettingsManager::saveSettings(settings);
    showMessage(uiFullscreen ? "Fullscreen: ON" : "Fullscreen: OFF", sf::Color::Cyan);
}

void UIManager::drawEscapeMenu(sf::RenderWindow& window, Canvas& canvas, Timeline& timeline) {
    sf::RectangleShape overlay(sf::Vector2f(1920.f, 1080.f));
    overlay.setFillColor(sf::Color(10, 4, 16, 225));
    window.draw(overlay);

    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    float modalW = 480.f;
    float modalH = 580.f;
    float modalX = (1920.f - modalW) / 2.f;
    float modalY = (1080.f - modalH) / 2.f;

    sf::FloatRect menuBounds(modalX, modalY, modalW, modalH);
    WisdomUI::Theme::DrawSunsetPanel(window, menuBounds, 1.0f);

    WisdomUI::Theme::DrawCrispText(window, font, "STUDIO PAUSED", 24, modalX + modalW / 2.f, modalY + 36.f, WisdomUI::Theme::SunsetGold, sf::Color(14, 6, 20), true, true);
    WisdomUI::Theme::DrawCrispText(window, font, activeProjectName + (canvas.getIsDirty() ? " *" : ""), 13, modalX + modalW / 2.f, modalY + 66.f, WisdomUI::Theme::SunsetPeach, sf::Color(14, 6, 20), true, true);

    sf::RectangleShape div(sf::Vector2f(modalW - 56.f, 2.f));
    div.setPosition(modalX + 28.f, modalY + 90.f);
    div.setFillColor(WisdomUI::Theme::SunsetPlum);
    window.draw(div);

    std::vector<std::pair<std::string, std::string>> menuItems = {
        { "resume", "Resume Studio" },
        { "save", "Save Project (Ctrl+S)" },
        { "save_as", "Save Project As..." },
        { "export", "Export Sequence (Ctrl+E)" },
        { "fullscreen", std::string("Fullscreen: ") + (uiFullscreen ? "ON" : "OFF") },
        { "main_menu", "Return to Main Menu" },
        { "exit", "Exit Application" }
    };

    float btnX = modalX + 36.f;
    float btnY = modalY + 112.f;
    float btnW = modalW - 72.f;
    float btnH = 46.f;
    float spacing = 12.f;

    for (size_t i = 0; i < menuItems.size(); ++i) {
        sf::FloatRect bRect(btnX, btnY + static_cast<float>(i) * (btnH + spacing), btnW, btnH);
        bool isHov = bRect.contains(mousePos);
        bool isExit = (menuItems[i].first == "exit" || menuItems[i].first == "main_menu");
        bool isResume = (menuItems[i].first == "resume");

        WisdomUI::Theme::DrawSunsetButton(window, bRect, menuItems[i].second, font, 14, isResume, isHov, isExit, 1.0f);
    }
}

bool UIManager::handleEscapeMenuEvent(const sf::Event& event, sf::RenderWindow& window, AppState& currentState, AppSettings& settings, Canvas& canvas, Timeline& timeline) {
    if (!m_showEscapeMenu) return false;

    if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
        m_showEscapeMenu = false;
        return true;
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

        float modalW = 480.f;
        float modalH = 580.f;
        float modalX = (1920.f - modalW) / 2.f;
        float modalY = (1080.f - modalH) / 2.f;

        float btnX = modalX + 36.f;
        float btnY = modalY + 112.f;
        float btnW = modalW - 72.f;
        float btnH = 46.f;
        float spacing = 12.f;

        std::vector<std::string> actions = { "resume", "save", "save_as", "export", "fullscreen", "main_menu", "exit" };

        for (size_t i = 0; i < actions.size(); ++i) {
            sf::FloatRect bRect(btnX, btnY + static_cast<float>(i) * (btnH + spacing), btnW, btnH);
            if (bRect.contains(mousePos)) {
                std::string action = actions[i];

                if (action == "resume") {
                    m_showEscapeMenu = false;
                }
                else if (action == "save") {
                    if (triggerSave(canvas, timeline)) {
                        showMessage("Project Saved Successfully!", sf::Color::Green);
                    }
                    else {
                        showMessage("Error Saving Project!", sf::Color::Red);
                    }
                }
                else if (action == "save_as") {
                    std::string file = NativeDialogs::saveFileDialog("Wisdom Park Projects\0*.wpk\0", "wpk", activeProjectName);
                    if (!file.empty() && projManager) {
                        activeProjectPath = file;
                        if (projManager->saveProjectAs(activeProjectPath, activeProjectName, canvas, static_cast<int>(timeline.getFps()), canvas.getPixelMode())) {
                            canvas.clearIsDirty();
                            showMessage("Project Saved As Successfully!", sf::Color::Green);
                        }
                    }
                }
                else if (action == "export") {
                    m_showEscapeMenu = false;
                    exportModal.open(canvas, timeline.getCurrentFrame());
                }
                else if (action == "fullscreen") {
                    toggleFullscreen(window, settings);
                }
                else if (action == "main_menu") {
                    m_showEscapeMenu = false;
                    if (canvas.getIsDirty()) {
                        showUnsavedWarning = true;
                    }
                    else {
                        currentState = AppState::Welcome;
                        currentMenuState = MenuState::Main;
                    }
                }
                else if (action == "exit") {
                    window.close();
                }
                return true;
            }
        }
    }
    return true;
}