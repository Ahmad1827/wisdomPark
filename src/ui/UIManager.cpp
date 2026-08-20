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
#if defined(_WIN32)
#include <windows.h>
#include <shellapi.h>


static bool g_resDropdownOpen = false;
static int g_resW = 1280;
static int g_resH = 720;
static WNDPROC g_originalWndProc = nullptr;
static std::vector<std::string> g_droppedFiles;

static LRESULT CALLBACK DropHookProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_DROPFILES) {
        HDROP hDrop = (HDROP)wParam;
        UINT count = DragQueryFileA(hDrop, 0xFFFFFFFF, NULL, 0);
        for (UINT i = 0; i < count; ++i) {
            char path[MAX_PATH];
            if (DragQueryFileA(hDrop, i, path, MAX_PATH)) {
                g_droppedFiles.push_back(std::string(path));
            }
        }
        DragFinish(hDrop);
        return 0;
    }
    return CallWindowProc(g_originalWndProc, hwnd, uMsg, wParam, lParam);
}
#endif

static AIPanel g_aiPanel;
static AIReviewModal g_aiReviewModal;
static bool g_typingApiKey = false;
static sf::RectangleShape loadingOverlay;
static sf::RectangleShape loadingBox;
static sf::Text loadingText;
static sf::CircleShape loadingSpinner;
static sf::RectangleShape loadingCancelBtn;
static sf::Text loadingCancelText;

UIManager::UIManager() : isTypingPrompt(false), showingText(false), textAlpha(255.0f), isLightingMode(false), promptQuantity(1), focusMode(false), projManager(nullptr), activeProjectName("Untitled_Project"), activeProjectPath(""), isDraggingSizeSlider(false), showUnsavedWarning(false), currentMenuState(MenuState::Main), startupTime(0.0f), activeTutorialIndex(-1), uiFullscreen(false), uiBorderless(false), uiVsync(true), uiAutoBackup(true), uiHwAccel(true), uiFpsLimit(60), uiAnimFps(12), uiHistorySize(15), easterEggClicks(0), m_debugUseSpriteStudio(false) {}

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

    AIManager::getInstance().init();
    g_aiPanel.init();
    g_aiReviewModal.init();

    handTrackerSocket.bind(5005);
    handTrackerSocket.setBlocking(false);

    uiText.setFont(font);
    uiText.setCharacterSize(30);
    uiText.setOutlineColor(sf::Color(0, 0, 0, 150));
    uiText.setOutlineThickness(2.0f);

    uiFullscreen = false;
    uiBorderless = true;

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
    assetBrowser->setBounds(sf::FloatRect(1920.f - 44.f - 300.f, 68.f, 300.f, 1080.f - 68.f - 24.f));

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
        [this]() { uiFullscreen = !uiFullscreen; },
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
            if (m_activeRightTab == RightTabMode::Assets) {
                m_activeRightTab = RightTabMode::None;
                if (assetBrowser && assetBrowser->getIsVisible()) assetBrowser->toggle();
            }
            else {
                m_activeRightTab = RightTabMode::Assets;
                if (assetBrowser && !assetBrowser->getIsVisible()) assetBrowser->toggle();
            }
        },
        [this]() {
            if (m_activeRightTab == RightTabMode::Audio) {
                m_activeRightTab = RightTabMode::None;
                if (audioPanel.getIsVisible()) audioPanel.toggle();
            }
            else {
                m_activeRightTab = RightTabMode::Audio;
                if (!audioPanel.getIsVisible()) audioPanel.toggle();
            }
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

void UIManager::drawMainMenu(sf::RenderWindow& window) {
    sf::CircleShape glow(400.f);
    glow.setOrigin(400.f, 400.f);
    glow.setPosition(350.f, 250.f);
    glow.setFillColor(sf::Color(255, 150, 50, static_cast<sf::Uint8>(15.f + std::sin(startupTime * 2.f) * 5.f)));
    window.draw(glow, sf::RenderStates(sf::BlendAdd));

    drawPremiumText(window, "WISDOM PARK", 350.f, 250.f, 75, sf::Color(255, 220, 100), sf::Color(100, 50, 10), sf::Color(0, 0, 0, 150));

    sf::Text subTitle("CREATIVE STUDIO", font, 18);
    subTitle.setFillColor(sf::Color(200, 200, 200));
    subTitle.setLetterSpacing(4.0f);
    subTitle.setPosition(350.f - subTitle.getLocalBounds().width / 2.f, 310.f);
    window.draw(subTitle);

    std::vector<std::string> buttons = { "Projects", "Settings", "Tutorials", "Keybinds", "Credits", "Exit" };
    float by = 420.f;
    for (const auto& btn : buttons) {
        float hov = getHover("mm_" + btn);
        sf::FloatRect bounds(150.f - hov * 10.f, by, 400.f + hov * 20.f, 60.f);
        drawGlassPanel(window, bounds, hov);

        sf::Text t(btn, font, 24);
        sf::Uint8 cVal = static_cast<sf::Uint8>(230.f + hov * 25.f);
        t.setFillColor(sf::Color(cVal, cVal, cVal));
        t.setPosition(bounds.left + 30.f + hov * 15.f, bounds.top + 15.f);
        window.draw(t);

        by += 80.f;
    }

    sf::Text recTitle("Recent Projects", font, 24);
    recTitle.setFillColor(sf::Color::White);
    recTitle.setPosition(900.f, 200.f);
    window.draw(recTitle);

    sf::RectangleShape line(sf::Vector2f(800.f, 2.f));
    line.setPosition(900.f, 240.f);
    line.setFillColor(sf::Color(255, 255, 255, 40));
    window.draw(line);

    std::vector<std::string> recentDummies = { "Character Animation", "Walking Cycle", "Forest Scene", "Pixel RPG" };
    std::vector<std::string> recentDetails = { "Normal  |  1920x1080  |  24 Frames  |  Modified Today",
                                              "Pixel Art  |  64x64  |  8 Frames  |  Modified Yesterday",
                                              "Normal  |  3840x2160  |  1 Frame  |  Modified 3 days ago",
                                              "Pixel Art  |  128x128  |  12 Frames  |  Modified Last Week" };
    float ry = 280.f;
    for (size_t i = 0; i < recentDummies.size(); ++i) {
        float hov = getHover("rec_" + std::to_string(i));
        sf::FloatRect bounds(900.f, ry, 800.f, 100.f);
        drawGlassPanel(window, bounds, hov);

        sf::RectangleShape thumb(sf::Vector2f(140.f, 80.f));
        thumb.setPosition(bounds.left + 10.f, bounds.top + 10.f);
        thumb.setFillColor(sf::Color(15, 15, 20));
        thumb.setOutlineThickness(1.f);
        thumb.setOutlineColor(sf::Color(255, 255, 255, 50));
        window.draw(thumb);

        sf::Text pName(recentDummies[i], font, 24);
        pName.setFillColor(sf::Color(255, 200, 100));
        pName.setPosition(bounds.left + 170.f, bounds.top + 20.f);
        window.draw(pName);

        sf::Text pDet(recentDetails[i], font, 14);
        pDet.setFillColor(sf::Color(180, 180, 180));
        pDet.setPosition(bounds.left + 170.f, bounds.top + 60.f);
        window.draw(pDet);

        ry += 125.f;
    }
}

void UIManager::drawBackButton(sf::RenderWindow& window, const std::string& hoverKey, float x, float y) {
    float hov = getHover(hoverKey);
    sf::FloatRect bounds(x, y, 120.f, 50.f);
    drawGlassPanel(window, bounds, hov);
    sf::Text t("< BACK", font, 18);
    t.setFillColor(sf::Color::White);
    t.setOrigin(t.getLocalBounds().width / 2.f, t.getLocalBounds().height / 2.f);
    t.setPosition(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
    window.draw(t);
}

void UIManager::drawSettingsMenu(sf::RenderWindow& window) {
    drawBackButton(window, "btn_back", 100.f, 100.f);
    drawPremiumText(window, "SETTINGS", 960.f, 120.f, 40, sf::Color::White, sf::Color(50, 50, 50), sf::Color::Black);

    auto drawCard = [&](sf::FloatRect bounds, const std::string& title) {
        drawGlassPanel(window, bounds, 0.0f);
        sf::Text t(title, font, 24);
        t.setFillColor(sf::Color(255, 200, 50));
        t.setPosition(bounds.left + 30.f, bounds.top + 25.f);
        window.draw(t);
        sf::RectangleShape line(sf::Vector2f(bounds.width - 60.f, 2.f));
        line.setPosition(bounds.left + 30.f, bounds.top + 65.f);
        line.setFillColor(sf::Color(255, 255, 255, 40));
        window.draw(line);
        };

    auto drawToggle = [&](float x, float y, const std::string& label, bool active, const std::string& hKey) {
        float hov = getHover(hKey);
        sf::RectangleShape box(sf::Vector2f(24.f, 24.f));
        box.setPosition(x, y);
        box.setFillColor(active ? sf::Color(80, 180, 80) : sf::Color(40, 40, 45));
        box.setOutlineThickness(1.f);
        box.setOutlineColor(sf::Color(static_cast<sf::Uint8>(100.f + hov * 50.f), static_cast<sf::Uint8>(100.f + hov * 50.f), static_cast<sf::Uint8>(120.f + hov * 50.f)));
        window.draw(box);
        if (active) {
            sf::RectangleShape check(sf::Vector2f(12.f, 12.f));
            check.setPosition(x + 6.f, y + 6.f);
            check.setFillColor(sf::Color::White);
            window.draw(check);
        }
        sf::Text t(label, font, 20);
        t.setFillColor(sf::Color(static_cast<sf::Uint8>(220.f + hov * 35.f), static_cast<sf::Uint8>(220.f + hov * 35.f), static_cast<sf::Uint8>(220.f + hov * 35.f)));
        t.setPosition(x + 40.f, y);
        window.draw(t);
        };

    auto drawStepper = [&](float x, float y, const std::string& label, const std::string& val, const std::string& hKeyL, const std::string& hKeyR) {
        sf::Text t(label, font, 20);
        t.setFillColor(sf::Color(220, 220, 220));
        t.setPosition(x, y);
        window.draw(t);
        float hovL = getHover(hKeyL);
        sf::Text btnL("<", font, 22);
        btnL.setFillColor(sf::Color(static_cast<sf::Uint8>(150.f + hovL * 105.f), static_cast<sf::Uint8>(150.f + hovL * 105.f), static_cast<sf::Uint8>(150.f + hovL * 105.f)));
        btnL.setPosition(x + 200.f, y - 2.f);
        window.draw(btnL);
        sf::Text v(val, font, 20);
        v.setFillColor(sf::Color(255, 200, 100));
        sf::FloatRect vb = v.getLocalBounds();
        v.setPosition(x + 245.f - vb.width / 2.f, y);
        window.draw(v);
        float hovR = getHover(hKeyR);
        sf::Text btnR(">", font, 22);
        btnR.setFillColor(sf::Color(static_cast<sf::Uint8>(150.f + hovR * 105.f), static_cast<sf::Uint8>(150.f + hovR * 105.f), static_cast<sf::Uint8>(150.f + hovR * 105.f)));
        btnR.setPosition(x + 270.f, y - 2.f);
        window.draw(btnR);
        };

    auto drawDropdown = [&](float x, float y, const std::string& label, const std::string& currentVal, bool isOpen, const std::vector<std::string>& options) {
        sf::Text t(label, font, 20);
        t.setFillColor(sf::Color(220, 220, 220));
        t.setPosition(x, y);
        window.draw(t);

        sf::RectangleShape box(sf::Vector2f(160.f, 30.f));
        box.setPosition(x + 180.f, y - 5.f);
        box.setFillColor(sf::Color(30, 30, 35));
        box.setOutlineThickness(1.f);
        box.setOutlineColor(sf::Color(100, 100, 110));
        window.draw(box);

        sf::Text v(currentVal, font, 16);
        v.setFillColor(sf::Color(255, 200, 100));
        v.setPosition(x + 190.f, y + 1.f);
        window.draw(v);

        sf::Text arrow(isOpen ? "^" : "v", font, 16);
        arrow.setFillColor(sf::Color(200, 200, 200));
        arrow.setPosition(x + 320.f, y + 1.f);
        window.draw(arrow);

        if (isOpen) {
            sf::RectangleShape dropBg(sf::Vector2f(160.f, static_cast<float>(options.size()) * 30.f));
            dropBg.setPosition(x + 180.f, y + 25.f);
            dropBg.setFillColor(sf::Color(40, 40, 45));
            dropBg.setOutlineThickness(1.f);
            dropBg.setOutlineColor(sf::Color(100, 100, 110));
            window.draw(dropBg);

            for (size_t i = 0; i < options.size(); ++i) {
                sf::Text optT(options[i], font, 16);
                optT.setFillColor(sf::Color(200, 200, 200));
                optT.setPosition(x + 190.f, y + 25.f + (static_cast<float>(i) * 30.f) + 3.f);
                window.draw(optT);
            }
        }
        };

    // Height expanded to 380.f to fit dropdown perfectly
    drawCard(sf::FloatRect(350.f, 220.f, 550.f, 380.f), "Display & Interface");
    drawToggle(380.f, 310.f, "Fullscreen Mode", uiFullscreen, "set_t_fs");
    drawToggle(380.f, 360.f, "Borderless Window", uiBorderless, "set_t_bl");
    drawToggle(380.f, 410.f, "Vertical Sync", uiVsync, "set_t_vs");
    drawStepper(380.f, 460.f, "Frame Limit", std::to_string(uiFpsLimit), "set_s_fpsL", "set_s_fpsR");

    std::string resStr = std::to_string(g_resW) + " x " + std::to_string(g_resH);
    std::vector<std::string> resOpts = { "1280 x 720", "1600 x 900", "1920 x 1080" };
    drawDropdown(380.f, 510.f, "Resolution", resStr, g_resDropdownOpen, resOpts);

    drawStepper(380.f, 560.f, "UI Theme", "Modern Dark", "set_s_thmL", "set_s_thmR");

    drawCard(sf::FloatRect(1020.f, 220.f, 550.f, 380.f), "Saving & Exporting");
    drawToggle(1050.f, 310.f, "Enable Auto-Backup", uiAutoBackup, "set_t_ab");
    drawStepper(1050.f, 360.f, "Autosave Interval", "5 Mins", "set_s_asL", "set_s_asR");
    drawStepper(1050.f, 410.f, "Default Directory", "/Projects", "set_s_dirL", "set_s_dirR");

    drawCard(sf::FloatRect(350.f, 620.f, 550.f, 350.f), "Canvas & Memory");
    drawToggle(380.f, 710.f, "Hardware Acceleration", uiHwAccel, "set_t_hw");
    drawStepper(380.f, 760.f, "Anim Preview FPS", std::to_string(uiAnimFps), "set_s_afpsL", "set_s_afpsR");
    drawStepper(380.f, 810.f, "Undo History Size", std::to_string(uiHistorySize), "set_s_undoL", "set_s_undoR");

    drawCard(sf::FloatRect(1020.f, 620.f, 550.f, 350.f), "AI Generation");
    drawStepper(1050.f, 710.f, "Provider", AIManager::getInstance().getActiveProvider(), "set_s_aiL", "set_s_aiR");

    std::string keyDisplay = AIManager::getInstance().getApiKey(AIManager::getInstance().getActiveProvider());
    if (keyDisplay.empty()) keyDisplay = "Click to enter key...";
    else keyDisplay = std::string(keyDisplay.length(), '*');
    if (keyDisplay.length() > 20) keyDisplay = "..." + keyDisplay.substr(keyDisplay.length() - 17);
    if (g_typingApiKey) keyDisplay += "_";

    drawStepper(1050.f, 760.f, "API Key", keyDisplay, "set_s_keyL", "set_s_keyR");
}

void UIManager::drawTutorialsMenu(sf::RenderWindow& window) {
    drawBackButton(window, "btn_back", 100.f, 100.f);
    drawPremiumText(window, "TUTORIALS & GUIDES", 960.f, 120.f, 40, sf::Color::White, sf::Color(50, 50, 50), sf::Color::Black);

    std::vector<std::string> topics = { "Getting Started", "Drawing", "Animation", "Layers", "Selection", "Pixel Mode", "Keybinds", "Exporting", "AI Features" };
    std::vector<std::string> descs = {
        "Learn the basics of Wisdom Park.",
        "Master the brush, pencil, and eraser.",
        "Understand the timeline and frames.",
        "Organize your artwork effectively.",
        "Copy, paste, move and isolate art.",
        "Create pixel-perfect retro sprites.",
        "Speed up workflow with shortcuts.",
        "Render to PNG and Sprite Sheets.",
        "Use local and cloud AI generators."
    };
    std::vector<std::string> fullText = {
        "Wisdom Park is designed for professional 2D animation and pixel art.\n\nStart by creating a new project from the main menu.\nUse the left toolbar for drawing tools, and the bottom timeline\nto manage your animation frames.",
        "Select the Brush or Pencil tool (B or P).\nThe brush offers smooth anti-aliased strokes for normal art.\nThe pencil locks to absolute grid pixels for retro art.\nHold Right-Click or Middle-Click to pan the canvas.",
        "The Timeline at the bottom holds your frames.\nClick '+' to duplicate or add a new frame.\nPress 'Play' to preview your animation.\nEnable Onion Skinning to see previous/next frames as references.",
        "Layers are essential for organizing complex scenes.\nUse the Layer Panel on the right to add, delete, or merge.\nLock layers to prevent accidental edits.\nToggle visibility or adjust opacity for blending effects.",
        "Use the Select Tool (M) to draw a lasso around your art.\nOnce selected, click and drag to move the pixels.\nYou can flip selections horizontally or vertically.\nPress 'Delete' to clear the selected area.",
        "Pixel Mode disables all anti-aliasing and forces a hard grid.\nEnable 'Pixel Perfect' mode in the tool properties to automatically\nclean up jagged lines while drawing fast curves.\nUse the tile view to draw seamless repeating textures.",
        "Every major action has a keyboard shortcut.\nPress the 'Keybinds' button on the main menu or hit ESC\nin the workspace to rebind them to your preference.\nStandard defaults: B (Brush), E (Eraser), Ctrl+Z (Undo).",
        "When your animation is complete, click Export.\nYou can save the current frame as a PNG, or export the\ntire timeline as a sequential Sprite Sheet for use\nin game engines like Unity, Godot, or Unreal Engine.",
        "AI Features require an active API Key in Settings.\nUse the selection tool, type a prompt in the bottom left,\nand press Enter. The AI will generate or modify the selected area\nbased on your theme settings (Structure, Clutter, Custom)."
    };

    if (activeTutorialIndex == -1) {
        int c = 0, r = 0;
        for (size_t i = 0; i < topics.size(); ++i) {
            float hov = getHover("tut_" + std::to_string(i));
            float x = 300.f + static_cast<float>(c) * 450.f;
            float y = 250.f + static_cast<float>(r) * 220.f;
            sf::FloatRect bounds(x, y, 400.f, 180.f);
            drawGlassPanel(window, bounds, hov);

            sf::Text t(topics[i], font, 24);
            t.setFillColor(sf::Color(255, 200, 100));
            t.setPosition(bounds.left + 20.f, bounds.top + 20.f);
            window.draw(t);

            sf::Text d(descs[i], font, 16);
            d.setFillColor(sf::Color(180, 180, 180));
            d.setPosition(bounds.left + 20.f, bounds.top + 70.f);
            window.draw(d);

            sf::Text hint("Click to read >", font, 14);
            hint.setFillColor(sf::Color(100, 200, 255, static_cast<sf::Uint8>(hov * 255.f)));
            hint.setPosition(bounds.left + 20.f, bounds.top + 140.f);
            window.draw(hint);

            c++;
            if (c >= 3) { c = 0; r++; }
        }
    }
    else {
        sf::FloatRect mainBounds(460.f, 250.f, 1000.f, 600.f);
        drawGlassPanel(window, mainBounds, 0.0f);

        sf::Text tTitle(topics[activeTutorialIndex], font, 36);
        tTitle.setFillColor(sf::Color(255, 200, 100));
        tTitle.setPosition(500.f, 290.f);
        window.draw(tTitle);

        sf::RectangleShape line(sf::Vector2f(920.f, 2.f));
        line.setPosition(500.f, 350.f);
        line.setFillColor(sf::Color(255, 255, 255, 40));
        window.draw(line);

        sf::Text fText(fullText[activeTutorialIndex], font, 22);
        fText.setFillColor(sf::Color(220, 220, 220));
        fText.setLineSpacing(1.5f);
        fText.setPosition(500.f, 380.f);
        window.draw(fText);

        float hovBack = getHover("tut_back_btn");
        sf::FloatRect bBounds(860.f, 750.f, 200.f, 60.f);
        drawGlassPanel(window, bBounds, hovBack);

        sf::Text bText("Return to Topics", font, 18);
        bText.setFillColor(sf::Color::White);
        bText.setOrigin(bText.getLocalBounds().width / 2.f, bText.getLocalBounds().height / 2.f);
        bText.setPosition(bBounds.left + bBounds.width / 2.f, bBounds.top + bBounds.height / 2.f);
        window.draw(bText);
    }
}

void UIManager::drawCreditsMenu(sf::RenderWindow& window) {
    drawBackButton(window, "btn_back", 100.f, 100.f);
    drawPremiumText(window, "CREDITS", 960.f, 120.f, 40, sf::Color::White, sf::Color(50, 50, 50), sf::Color::Black);

    sf::FloatRect mainBounds(660.f, 250.f, 600.f, 600.f);
    drawGlassPanel(window, mainBounds, 0.0f);

    float titleHov = getHover("cred_title");
    sf::Text studioText("WISDOM PARK STUDIO", font, 26);
    studioText.setFillColor(easterEggClicks >= 5 ? sf::Color(255, 100, 100) : sf::Color(255, 200, 100 + static_cast<int>(titleHov * 155.f)));
    sf::FloatRect sb = studioText.getLocalBounds();
    studioText.setOrigin(sb.left + sb.width / 2.f, sb.top + sb.height / 2.f);
    studioText.setPosition(960.f, 300.f);
    window.draw(studioText);

    std::string creds = "Lead Developer & Architect:\nAhmad Arnaoute (AtodDev)\n\n"
        "Education:\nUniversitatea Politehnica Bucure?ti\n"
        "Facultatea de Automatic? ?i Calculatoare (324CD)\n\n"
        "Other Projects & Contributions:\n"
        "- Oppia Foundation\n"
        "- safe-comment-stripper\n"
        "- iMeditatii\n"
        "- AtodDev's Poop Tracker\n\n"
        "Libraries: SFML 2.6.x, nlohmann::json\n"
        "License: Commercial";

    sf::Text t(creds, font, 20);
    t.setFillColor(sf::Color(220, 220, 220));
    t.setLineSpacing(1.5f);

    sf::FloatRect tb = t.getLocalBounds();
    t.setOrigin(tb.left + tb.width / 2.f, tb.top + tb.height / 2.f);
    t.setPosition(960.f, 570.f);
    window.draw(t);

    if (easterEggClicks >= 5) {
        sf::Text egg("Engine Core Unlocked.", font, 14);
        egg.setFillColor(sf::Color(100, 255, 100));
        egg.setOrigin(egg.getLocalBounds().width / 2.f, egg.getLocalBounds().height / 2.f);
        egg.setPosition(960.f, 820.f);
        window.draw(egg);
    }
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
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            if (currentMenuState == MenuState::Settings && g_typingApiKey) {
                if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter) {
                    g_typingApiKey = false;
                    AIManager::getInstance().saveSettingsLocally();
                    showMessage("Key Applied Successfully!", sf::Color::Green);
                    return;
                }
            }
            if (currentMenuState == MenuState::Main) {
                std::vector<std::string> buttons = { "Projects", "Settings", "Tutorials", "Keybinds", "Credits", "Exit" };
                float by = 420.f;
                for (const auto& btn : buttons) {
                    sf::FloatRect bBounds(150.f, by, 400.f, 60.f);
                    if (bBounds.contains(mousePos)) {
                        if (btn == "Projects") currentMenuState = MenuState::Projects;
                        else if (btn == "Settings") currentMenuState = MenuState::Settings;
                        else if (btn == "Tutorials") { currentMenuState = MenuState::Tutorials; activeTutorialIndex = -1; }
                        else if (btn == "Credits") { currentMenuState = MenuState::Credits; easterEggClicks = 0; }
                        else if (btn == "Keybinds") {
                            keybindPanel.toggle();
                            return;
                        }
                        else if (btn == "Exit") window.close();
                        return;
                    }
                    by += 80.f;
                }
            }
            else if (currentMenuState == MenuState::Projects) {
                sf::FloatRect backBounds(100.f, 100.f, 120.f, 50.f);
                if (backBounds.contains(mousePos)) {
                    currentMenuState = MenuState::Main;
                    return;
                }

                ProjectMetadata meta;
                std::string action = projectBrowser.handleClick(mousePos, meta);
                if (action == "new_project") newProjectModal.open();
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
                    else showMessage("Failed to load project files.", sf::Color::Red);
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
                        else showMessage("Failed to load native project.", sf::Color::Red);
                    }
                }
            }
            else if (currentMenuState == MenuState::Settings) {
                if (sf::FloatRect(1050.f + 200.f, 760.f - 10.f, 150.f, 40.f).contains(mousePos)) {
                    g_typingApiKey = true;
                }
                else {
                    if (g_typingApiKey) {
                        g_typingApiKey = false;
                        AIManager::getInstance().saveSettingsLocally();
                        showMessage("AI Configurations Applied and Saved", sf::Color::Green);
                    }
                }
                sf::FloatRect backBounds(100.f, 100.f, 120.f, 50.f);
                if (backBounds.contains(mousePos)) {
                    currentMenuState = MenuState::Main;
                    return;
                }

                g_resW = settings.resWidth;
                g_resH = settings.resHeight;

                auto checkToggle = [&](float x, float y) { return sf::FloatRect(x, y, 300.f, 30.f).contains(mousePos); };
                auto checkStepperL = [&](float x, float y) { return sf::FloatRect(x + 190.f, y - 10.f, 40.f, 40.f).contains(mousePos); };
                auto checkStepperR = [&](float x, float y) { return sf::FloatRect(x + 260.f, y - 10.f, 40.f, 40.f).contains(mousePos); };

                bool displayChanged = false;

                if (g_resDropdownOpen) {
                    sf::FloatRect dropBounds(380.f + 180.f, 510.f + 25.f, 160.f, 3 * 30.f);
                    if (dropBounds.contains(mousePos)) {
                        int index = static_cast<int>(mousePos.y - (510.f + 25.f)) / 30;
                        if (index == 0) { settings.resWidth = 1280; settings.resHeight = 720; }
                        else if (index == 1) { settings.resWidth = 1600; settings.resHeight = 900; }
                        else if (index == 2) { settings.resWidth = 1920; settings.resHeight = 1080; }

                        g_resW = settings.resWidth;
                        g_resH = settings.resHeight;
                        displayChanged = true;
                    }
                    g_resDropdownOpen = false;
                }
                else if (sf::FloatRect(380.f + 180.f, 510.f - 5.f, 160.f, 30.f).contains(mousePos)) {
                    g_resDropdownOpen = true;
                }
                else {
                    if (checkToggle(380.f, 310.f)) {
                        uiFullscreen = !uiFullscreen;
                        if (uiFullscreen) uiBorderless = false;
                        settings.fullscreen = uiFullscreen;
                        settings.borderless = uiBorderless;
                        displayChanged = true;
                    }

                    if (checkToggle(380.f, 360.f)) {
                        uiBorderless = !uiBorderless;
                        if (uiBorderless) uiFullscreen = false;
                        settings.fullscreen = uiFullscreen;
                        settings.borderless = uiBorderless;
                        displayChanged = true;
                    }

                    if (checkToggle(380.f, 410.f)) {
                        uiVsync = !uiVsync;
                        settings.vsync = uiVsync;
                        window.setVerticalSyncEnabled(uiVsync);
                    }

                    if (checkStepperL(380.f, 460.f)) { uiFpsLimit = (uiFpsLimit == 60) ? 240 : ((uiFpsLimit == 144) ? 60 : 144); settings.fpsLimit = uiFpsLimit; window.setFramerateLimit(uiFpsLimit); }
                    if (checkStepperR(380.f, 460.f)) { uiFpsLimit = (uiFpsLimit == 60) ? 144 : ((uiFpsLimit == 144) ? 240 : 60); settings.fpsLimit = uiFpsLimit; window.setFramerateLimit(uiFpsLimit); }

                    if (checkToggle(1050.f, 310.f)) { uiAutoBackup = !uiAutoBackup; settings.autoBackup = uiAutoBackup; }
                    if (checkToggle(380.f, 710.f)) { uiHwAccel = !uiHwAccel; settings.hwAccel = uiHwAccel; }

                    if (checkStepperL(380.f, 760.f)) { uiAnimFps = (uiAnimFps == 12) ? 60 : ((uiAnimFps == 24) ? 12 : 24); settings.animFps = uiAnimFps; }
                    if (checkStepperR(380.f, 760.f)) { uiAnimFps = (uiAnimFps == 12) ? 24 : ((uiAnimFps == 24) ? 60 : 12); settings.animFps = uiAnimFps; }

                    if (checkStepperL(380.f, 810.f)) { uiHistorySize = (uiHistorySize == 15) ? 50 : ((uiHistorySize == 30) ? 15 : 30); settings.historySize = uiHistorySize; }
                    if (checkStepperR(380.f, 810.f)) { uiHistorySize = (uiHistorySize == 15) ? 30 : ((uiHistorySize == 30) ? 50 : 15); settings.historySize = uiHistorySize; }

                    if (checkStepperL(1050.f, 710.f)) AIManager::getInstance().cycleProvider(-1);
                    if (checkStepperR(1050.f, 710.f)) AIManager::getInstance().cycleProvider(1);
                }

                SettingsManager::saveSettings(settings);

                if (displayChanged) {
                    if (uiFullscreen) {
                        window.create(sf::VideoMode::getDesktopMode(), "Wisdom Park", sf::Style::Fullscreen);
                    }
                    else if (uiBorderless) {
                        window.create(sf::VideoMode::getDesktopMode(), "Wisdom Park", sf::Style::None);
                        window.setPosition(sf::Vector2i(0, 0));
                    }
                    else {
                        window.create(sf::VideoMode(settings.resWidth, settings.resHeight), "Wisdom Park", sf::Style::Default);
                        sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
                        window.setPosition(sf::Vector2i(
                            std::max(0, static_cast<int>((desktop.width - settings.resWidth) / 2)),
                            std::max(0, static_cast<int>((desktop.height - settings.resHeight) / 2))
                        ));
                    }

                    window.setFramerateLimit(uiFpsLimit);
                    window.setVerticalSyncEnabled(uiVsync);
                    window.setView(WisdomUI::WorkspaceLayout::GetLetterboxView(window.getSize()));
                }
            }
            else if (currentMenuState == MenuState::Tutorials) {
                sf::FloatRect backBounds(100.f, 100.f, 120.f, 50.f);
                if (backBounds.contains(mousePos)) {
                    if (activeTutorialIndex != -1) activeTutorialIndex = -1;
                    else currentMenuState = MenuState::Main;
                    return;
                }

                if (activeTutorialIndex == -1) {
                    int c = 0, r = 0;
                    for (int i = 0; i < 9; ++i) {
                        float x = 300.f + static_cast<float>(c) * 450.f;
                        float y = 250.f + static_cast<float>(r) * 220.f;
                        sf::FloatRect bounds(x, y, 400.f, 180.f);
                        if (bounds.contains(mousePos)) {
                            activeTutorialIndex = i;
                            return;
                        }
                        c++;
                        if (c >= 3) { c = 0; r++; }
                    }
                }
                else {
                    sf::FloatRect bBounds(860.f, 750.f, 200.f, 60.f);
                    if (bBounds.contains(mousePos)) {
                        activeTutorialIndex = -1;
                        return;
                    }
                }
            }
            else if (currentMenuState == MenuState::Credits) {
                sf::FloatRect backBounds(100.f, 100.f, 120.f, 50.f);
                if (backBounds.contains(mousePos)) {
                    currentMenuState = MenuState::Main;
                    return;
                }

                sf::Text dummyText("WISDOM PARK STUDIO", font, 26);
                sf::FloatRect sb = dummyText.getLocalBounds();
                sf::FloatRect titleBounds(960.f - sb.width / 2.f, 300.f - sb.height / 2.f, sb.width, sb.height);
                if (titleBounds.contains(mousePos)) {
                    easterEggClicks++;
                }
            }
            else {
                sf::FloatRect backBounds(100.f, 100.f, 120.f, 50.f);
                if (backBounds.contains(mousePos)) {
                    currentMenuState = MenuState::Main;
                    return;
                }
            }
        }

        if (currentMenuState == MenuState::Settings && g_typingApiKey) {
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::V && event.key.control) {
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
        if (m_topBar.HandleEvent(event, window)) {
            if (currentMenuState == MenuState::Main && !showUnsavedWarning) {
                currentState = AppState::Welcome;
            }
            return;
        }

        if (currentMenuState == MenuState::Main && !showUnsavedWarning) {
            currentState = AppState::Welcome;
            return;
        }

        if (m_toolOptionsBar.HandleEvent(event, window,
            [&](float sz) {
                if (canvas.getPixelMode()) canvas.setPixelBrushSize(static_cast<int>(sz));
                else canvas.setBrushSize(sz);
            },
            [&]() { canvas.togglePixelPerfect(); }
        )) return;
        if (m_toolDock.HandleEvent(event, window)) return;
        if (m_rightDockTabs.HandleEvent(event, window)) return;
        if (m_statusBar.HandleEvent(event, window)) return;

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
            if (mousePos.x > 1920.f - 44.f - 300.f && mousePos.x < 1920.f - 44.f) return;
        }
        else if (m_activeRightTab == RightTabMode::Palette) {
            if (colorPalettePanel.handleEvent(event, mousePos, canvas)) return;
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                std::string cpAction = colorPalettePanel.processClick(mousePos, canvas);
                if (cpAction == "color_close") { m_activeRightTab = RightTabMode::None; return; }
            }
            if (mousePos.x > 1920.f - 44.f - 300.f && mousePos.x < 1920.f - 44.f) return;
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
            if (mousePos.x > 1920.f - 44.f - 300.f && mousePos.x < 1920.f - 44.f) return;
        }
        else if (m_activeRightTab == RightTabMode::Assets) {
            if (assetBrowser) {
                assetBrowser->handleEvent(event, window, canvas, timeline.getCurrentFrame());
                if (mousePos.x > 1920.f - 44.f - 300.f && mousePos.x < 1920.f - 44.f) return;
            }
        }
        else if (m_activeRightTab == RightTabMode::Audio) {
            if (audioPanel.getIsVisible()) {
                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    std::string action = audioPanel.handleClick(mousePos, timeline.getCurrentFrame());
                    if (action == "imported") {
                        showMessage("Audio Directory Scanned Successfully", sf::Color::Green);
                        return;
                    }
                    if (action == "closed") {
                        m_activeRightTab = RightTabMode::None;
                        if (audioPanel.getIsVisible()) audioPanel.toggle();
                        return;
                    }
                    if (audioPanel.handleEvent(event, mousePos)) return;
                }
            }
            if (mousePos.x > 1920.f - 44.f - 300.f && mousePos.x < 1920.f - 44.f) return;
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
                    if (colorPalettePanel.getIsEyedropperActive()) {
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
                                showMessage("Color Picked", sf::Color::Green);
                            }
                        }
                        return;
                    }
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
                        bool isFrameEmpty = true;
                        const Frame* curFrame = canvas.getFrameReadOnly(timeline.getCurrentFrame());
                        if (curFrame) {
                            for (const auto& layer : curFrame->layers) {
                                sf::Image img = layer.texture->getTexture().copyToImage();
                                const sf::Uint8* pixels = img.getPixelsPtr();
                                size_t totalPixels = static_cast<size_t>(img.getSize().x) * static_cast<size_t>(img.getSize().y) * 4;
                                for (size_t i = 3; i < totalPixels; i += 4) {
                                    if (pixels[i] > 0) {
                                        isFrameEmpty = false;
                                        break;
                                    }
                                }
                                if (!isFrameEmpty) break;
                            }
                        }

                        if (isFrameEmpty) {
                            if (showingText && uiText.getString() == sf::String("Current frame is empty. Press Right again to create another.") && textClock.getElapsedTime().asSeconds() < 2.0f) {
                                canvas.addFrame(timeline.getCurrentFrame());
                                timeline.addFrameAfter(timeline.getCurrentFrame());
                                timeline.nextFrame();
                                showingText = false;
                            }
                            else showMessage("Current frame is empty. Press Right again to create another.", sf::Color::Yellow);
                        }
                        else {
                            canvas.addFrame(timeline.getCurrentFrame());
                            timeline.addFrameAfter(timeline.getCurrentFrame());
                            timeline.nextFrame();
                        }
                    }
                }

                if (keybindManager.isActionTriggered("time_prev", event)) {
                    if (timeline.getCurrentFrame() > 0) {
                        timeline.prevFrame();
                    }
                    else {
                        bool isFrameEmpty = true;
                        const Frame* curFrame = canvas.getFrameReadOnly(timeline.getCurrentFrame());
                        if (curFrame) {
                            for (const auto& layer : curFrame->layers) {
                                sf::Image img = layer.texture->getTexture().copyToImage();
                                const sf::Uint8* pixels = img.getPixelsPtr();
                                size_t totalPixels = static_cast<size_t>(img.getSize().x) * static_cast<size_t>(img.getSize().y) * 4;
                                for (size_t i = 3; i < totalPixels; i += 4) {
                                    if (pixels[i] > 0) {
                                        isFrameEmpty = false;
                                        break;
                                    }
                                }
                                if (!isFrameEmpty) break;
                            }
                        }

                        if (isFrameEmpty) {
                            if (showingText && uiText.getString() == sf::String("Current frame is empty. Press Left again to create another.") && textClock.getElapsedTime().asSeconds() < 2.0f) {
                                canvas.addFrame(-1);
                                timeline.addFrameAfter(-1);
                                timeline.setFrame(0);
                                showingText = false;
                            }
                            else showMessage("Current frame is empty. Press Left again to create another.", sf::Color::Yellow);
                        }
                        else {
                            canvas.addFrame(-1);
                            timeline.addFrameAfter(-1);
                            timeline.setFrame(0);
                        }
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
                        showMessage("Object Z-Coordinate +1 (Moved Up)", sf::Color::Cyan);
                    }
                }
                if (event.key.code == sf::Keyboard::PageDown) {
                    int curL = canvas.getActiveLayer();
                    if (curL > 0) {
                        canvas.moveLayer(timeline.getCurrentFrame(), curL, curL - 1);
                        showMessage("Object Z-Coordinate -1 (Moved Down)", sf::Color::Cyan);
                    }
                }
                if (event.key.code == sf::Keyboard::E && event.key.control) {
                    canvas.commitSelection(timeline.getCurrentFrame());
                    canvas.mergeDown(timeline.getCurrentFrame());
                    showMessage("Merged Object Down", sf::Color::Green);
                }
                if (event.key.code == sf::Keyboard::B && event.key.control) {
                    if (m_activeRightTab == RightTabMode::Assets) {
                        m_activeRightTab = RightTabMode::None;
                        if (assetBrowser && assetBrowser->getIsVisible()) assetBrowser->toggle();
                    }
                    else {
                        m_activeRightTab = RightTabMode::Assets;
                        if (assetBrowser && !assetBrowser->getIsVisible()) assetBrowser->toggle();
                    }
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

void UIManager::update(sf::RenderWindow & window, AppState currentState, AppSettings & settings, float dt, Canvas & canvas, Timeline & timeline) {
#if defined(_WIN32)
    static bool s_dragDropHooked = false;
    if (!s_dragDropHooked) {
        HWND hwnd = window.getSystemHandle();
        DragAcceptFiles(hwnd, TRUE);
        g_originalWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)DropHookProc);
        s_dragDropHooked = true;
    }

    if (!g_droppedFiles.empty()) {
        assetManager.importAssets(g_droppedFiles);
        g_droppedFiles.clear();

        m_activeRightTab = RightTabMode::Assets;
        if (assetBrowser && !assetBrowser->getIsVisible()) assetBrowser->toggle();
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
        m_rightDockTabs.SetTabState("assets", m_activeRightTab == RightTabMode::Assets);
        m_rightDockTabs.SetTabState("audio", m_activeRightTab == RightTabMode::Audio);
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

        if (m_activeRightTab == RightTabMode::Assets && assetBrowser) {
            if (!assetBrowser->getIsVisible()) assetBrowser->toggle();
            assetBrowser->setBounds(sf::FloatRect(1920.f - 44.f - 280.f, 68.0f, 280.f, 1080.0f - 68.0f - 24.0f));
            assetBrowser->update(dt);
        }
        else if (assetBrowser && assetBrowser->getIsVisible()) {
            assetBrowser->toggle();
        }

        if (m_activeRightTab == RightTabMode::Audio) {
            if (!audioPanel.getIsVisible()) audioPanel.toggle();
            audioPanel.update(dt);
        }
        else if (audioPanel.getIsVisible()) {
            audioPanel.toggle();
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
        drawStartMenu(window);
        if (currentMenuState == MenuState::Projects) {
            projectBrowser.draw(window);
            drawBackButton(window, "btn_back", 100.f, 100.f);
        }
        if (newProjectModal.getIsOpen()) newProjectModal.draw(window);
        if (keybindPanel.isVisible()) keybindPanel.draw(window);
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
        else if (m_activeRightTab == RightTabMode::Assets && assetBrowser) {
            sf::FloatRect abBounds(1920.f - 44.f - 300.f, 68.0f, 300.f, 1080.0f - 68.0f - 24.0f);
            WisdomUI::Theme::DrawSunsetPanel(window, abBounds, 1.0f);
            assetBrowser->draw(window);
        }
        else if (m_activeRightTab == RightTabMode::Audio) {
            sf::FloatRect audBounds(1920.f - 44.f - 300.f, 68.0f, 300.f, 1080.0f - 68.0f - 24.0f);
            WisdomUI::Theme::DrawSunsetPanel(window, audBounds, 1.0f);
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
    }
}

bool loadStudioFont(sf::Font& font) {
    if (font.loadFromFile("Resources/font.ttf")) return true;
    if (font.loadFromFile("../Resources/font.ttf")) return true;
    if (font.loadFromFile("../../Resources/font.ttf")) return true;
    if (font.loadFromFile("C:/Windows/Fonts/arial.ttf")) return true;
    return false;
}