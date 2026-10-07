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
#include "Screens/MenuLayouts.h"
#include "../UI/WorkspaceLayout.h"
#include "../UI/Panels/TopBar.h"
#include "../UI/Panels/ToolOptionsBar.h"
#include "../UI/Panels/ToolDock.h"
#include "../UI/Panels/StatusBar.h"

int g_resW = 1920;
int g_resH = 1080;
bool g_resDropdownOpen = false;
static bool s_exitToMenuRequested = false;
static bool g_timelineLeftHeld = false;
static bool g_timelineRightHeld = false;
static bool g_warnLeftPending = false;
static bool g_warnRightPending = false;
static sf::Clock g_warnLeftClock;
static sf::Clock g_warnRightClock;
static float g_timelineScrollX = 0.0f;
static bool g_isDraggingTimeline = false;
static float g_timelineDragStartMouseX = 0.0f;
static float g_timelineDragStartScrollX = 0.0f;
static bool g_timelineDragMoved = false;

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

#if defined(_WIN32)
static bool GetClipboardImage(sf::Image& outImage) {
    if (!OpenClipboard(nullptr)) return false;

    HANDLE hData = GetClipboardData(CF_DIB);
    if (!hData) {
        CloseClipboard();
        return false;
    }

    BITMAPINFOHEADER* bmi = static_cast<BITMAPINFOHEADER*>(GlobalLock(hData));
    if (!bmi) {
        CloseClipboard();
        return false;
    }

    int width = bmi->biWidth;
    int height = bmi->biHeight;
    bool topDown = (height < 0);
    height = std::abs(height);
    WORD bpp = bmi->biBitCount;

    if (width <= 0 || height <= 0 || (bpp != 24 && bpp != 32)) {
        GlobalUnlock(hData);
        CloseClipboard();
        return false;
    }

    DWORD colorTableSize = 0;
    if (bmi->biCompression == BI_BITFIELDS) {
        colorTableSize = 3 * sizeof(DWORD);
    }
    else if (bmi->biClrUsed > 0) {
        colorTableSize = bmi->biClrUsed * sizeof(RGBQUAD);
    }

    const unsigned char* srcBits = reinterpret_cast<const unsigned char*>(bmi) + bmi->biSize + colorTableSize;
    outImage.create(width, height);

    int rowStride = ((width * bpp + 31) / 32) * 4;

    bool hasNonZeroAlpha = false;

    for (int y = 0; y < height; ++y) {
        int srcY = topDown ? y : (height - 1 - y);
        const unsigned char* row = srcBits + srcY * rowStride;
        for (int x = 0; x < width; ++x) {
            if (bpp == 32) {
                unsigned char b = row[x * 4 + 0];
                unsigned char g = row[x * 4 + 1];
                unsigned char r = row[x * 4 + 2];
                unsigned char a = row[x * 4 + 3];
                if (a != 0) hasNonZeroAlpha = true;
                outImage.setPixel(x, y, sf::Color(r, g, b, a));
            }
            else if (bpp == 24) {
                unsigned char b = row[x * 3 + 0];
                unsigned char g = row[x * 3 + 1];
                unsigned char r = row[x * 3 + 2];
                outImage.setPixel(x, y, sf::Color(r, g, b, 255));
            }
        }
    }

    if (bpp == 32 && !hasNonZeroAlpha) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                sf::Color c = outImage.getPixel(x, y);
                c.a = 255;
                outImage.setPixel(x, y, c);
            }
        }
    }

    GlobalUnlock(hData);
    CloseClipboard();
    return true;
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
bool g_typingApiKey = false;
static sf::RectangleShape loadingOverlay;
static sf::RectangleShape loadingBox;
static sf::Text loadingText;
static sf::CircleShape loadingSpinner;
static sf::RectangleShape loadingCancelBtn;
static sf::Text loadingCancelText;

static bool g_selectingOutlineColor = false;
static sf::Color g_outlineColor = sf::Color::Black;
static bool g_selectingGridColor = false;

UIManager::UIManager() : isTypingPrompt(false), showingText(false), textAlpha(255.0f), isLightingMode(false), focusMode(false), projManager(nullptr), activeProjectName("Untitled_Project"), activeProjectPath(""), showUnsavedWarning(false), currentMenuState(MenuState::Main), startupTime(0.0f), activeTutorialIndex(-1), uiFullscreen(true), uiBorderless(false), uiVsync(true), uiAutoBackup(true), uiHwAccel(true), uiFpsLimit(60), uiAnimFps(12), uiHistorySize(100), easterEggClicks(0), m_debugUseSpriteStudio(false) {}

void UIManager::init(ProjectManager* pm, Canvas* baseCanvas) {
    projManager = pm;
    keybindManager.init();

    bgTexture.loadFromFile("assets/landofwisdompark2.jfif");
    bgSprite.setTexture(bgTexture);

    font.loadFromFile("assets/Jersey10-Regular.ttf");

    projectBrowser.init(pm);
    keybindPanel.init(&keybindManager);
    exportModal.init();
    newProjectModal.init();

    layerPanel.init();
    colorPalettePanel.init();
    rightProperties.init();
    audioPanel.init();
    initMinigame();

    m_toolOptionsBar.SetOutlineColor(g_outlineColor);

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
    promptBox.setFillColor(sf::Color(WisdomUI::Theme::Panel.r, WisdomUI::Theme::Panel.g, WisdomUI::Theme::Panel.b, 240));
    promptBox.setOutlineThickness(1.5f);
    promptBox.setOutlineColor(WisdomUI::Theme::Border);

    promptDisplay.setFont(font);
    promptDisplay.setCharacterSize(20);
    promptDisplay.setFillColor(WisdomUI::Theme::TextPrimary);
    promptDisplay.setPosition(1920.f / 2.f - 290.f, 1080.f - 288.f);

    assetBrowser = std::make_unique<AssetBrowserPanel>(assetManager, font);
    assetBrowser->setProject("CurrentProject");
    assetBrowser->setBounds(sf::FloatRect(1440.f, WisdomUI::Theme::FloatingPanelY, 390.f, 540.f));

    loadingOverlay.setSize(sf::Vector2f(1920.f, 1080.f));
    loadingOverlay.setFillColor(sf::Color(10, 4, 16, 200));

    loadingBox.setSize(sf::Vector2f(450.f, 220.f));
    loadingBox.setOrigin(225.f, 110.f);
    loadingBox.setPosition(960.f, 540.f);
    loadingBox.setFillColor(WisdomUI::Theme::Panel);
    loadingBox.setOutlineThickness(1.5f);
    loadingBox.setOutlineColor(WisdomUI::Theme::Border);

    loadingText.setFont(font);
    loadingText.setString("Wisdom Park AI is thinking...");
    loadingText.setCharacterSize(16);
    loadingText.setFillColor(WisdomUI::Theme::TextGold);
    loadingText.setOrigin(loadingText.getLocalBounds().width / 2.f, loadingText.getLocalBounds().height / 2.f);
    loadingText.setPosition(960.f, 480.f);

    loadingSpinner.setRadius(25.f);
    loadingSpinner.setPointCount(3);
    loadingSpinner.setFillColor(WisdomUI::Theme::SunsetAmber);
    loadingSpinner.setOrigin(25.f, 25.f);
    loadingSpinner.setPosition(960.f, 540.f);

    loadingCancelBtn.setSize(sf::Vector2f(120.f, 35.f));
    loadingCancelBtn.setOrigin(60.f, 17.5f);
    loadingCancelBtn.setPosition(960.f, 610.f);
    loadingCancelBtn.setFillColor(WisdomUI::Theme::RubyDark);
    loadingCancelBtn.setOutlineThickness(1.f);
    loadingCancelBtn.setOutlineColor(WisdomUI::Theme::RubyHighlight);

    loadingCancelText.setFont(font);
    loadingCancelText.setString("Cancel");
    loadingCancelText.setCharacterSize(14);
    loadingCancelText.setFillColor(WisdomUI::Theme::TextPrimary);
    loadingCancelText.setOrigin(loadingCancelText.getLocalBounds().width / 2.f, loadingCancelText.getLocalBounds().height / 2.f);
    loadingCancelText.setPosition(960.f, 607.f);

    m_gradientPanel.init(&m_gradientConfig);
    m_perspectiveManager.init();
    m_perspectivePanel.init(&m_perspectiveManager);
    m_textManager.init();
    m_textPanel.init(&m_textManager);
    baseCanvas->setPerspectiveManager(&m_perspectiveManager);
    baseCanvas->setTextManager(&m_textManager);

    m_topBar.Initialize(
        font,
        [this, baseCanvas]() { requestNewProject(*baseCanvas); },
        [this, baseCanvas]() {
            std::string file = NativeDialogs::openFileDialog("Wisdom Park Projects\0*.wpk\0All Files\0*.*\0");
            if (!file.empty() && projManager) {
                activeProjectPath = file;
                activeProjectName = std::filesystem::path(file).stem().string();
                int loadedFps = 12;
                bool isPix = false;
                baseCanvas->clearCanvasImages();
                baseCanvas->clearObjectSelection();
                if (projManager->loadProject(activeProjectPath, *baseCanvas, loadedFps, isPix)) {
                    baseCanvas->setPixelMode(isPix);
                    baseCanvas->clearIsDirty();
                    baseCanvas->clearHistory();

                    if (baseCanvas->getFrameCount() > 0) {
                        int targetLayer = (baseCanvas->getFrameReadOnly(0)->layers.size() > 1) ? 1 : 0;
                        baseCanvas->setActiveLayer(targetLayer, 0);
                    }

                    baseCanvas->setActiveTool(ToolType::Brush);
                    m_toolDock.SetActiveTool("brush");
                    m_activeTool.reset(); // Force workspace tool to rebuild fresh view transforms

                    showMessage("Loaded Project: " + activeProjectName, sf::Color::Green);
                }
            }
        },
        [this, baseCanvas]() {
            if (activeProjectPath.empty()) {
                activeProjectPath = "projects/" + activeProjectName + ".wpk";
            }
            if (projManager) {
                if (projManager->saveProjectAs(activeProjectPath, activeProjectName, *baseCanvas, 12, baseCanvas->getPixelMode())) {
                    baseCanvas->clearIsDirty();
                    showMessage("Saved Project: " + activeProjectName, sf::Color::Green);
                }
                else {
                    showMessage("Error Saving Project!", sf::Color::Red);
                }
            }
        },
        [this, baseCanvas]() { exportModal.open(*baseCanvas, 0); },
        [this, baseCanvas]() { baseCanvas->undo(); },
        [this, baseCanvas]() { baseCanvas->redo(); },
        [this]() { m_fullscreenToggleRequested = true; },
        [this]() { s_exitToMenuRequested = true; }
    );
    m_gitImgClient.setBaseUrl(GitImgClient::loadConfigUrl());

    m_topBar.SetPushGitImgCallback([this, baseCanvas](bool opaqueBg) {
        pushToGitImg(*baseCanvas, 0, opaqueBg);
        });
    m_topBar.SetPushSpriteSheetCallback([this, baseCanvas](bool opaqueBg) {
        pushSpriteSheetToGitImg(*baseCanvas, opaqueBg);
        });
    m_topBar.SetToggleTrackerCallback([this](bool active) {
        if (active) {
            if (HandTracker::getInstance().start()) {
                showMessage("Hand Tracker Started", sf::Color::Green);
            }
            else {
                showMessage("Failed to start Hand Tracker", sf::Color::Red);
            }
        }
        else {
            HandTracker::getInstance().stop();
            showMessage("Hand Tracker Stopped", sf::Color::Yellow);
        }
        });
    m_topBar.SetCheckoutCommitCallback([this](const std::string& hash) {
        m_pendingCheckoutHash = hash;
        });
    m_toolOptionsBar.Initialize(font);
    m_toolDock.Initialize(font);
    m_toolDock.SetDeselectCallback([this, baseCanvas]() {
        if (g_aiPanel.getIsVisible()) {
            g_aiPanel.toggle();
            if (baseCanvas) {
                baseCanvas->setActiveTool(ToolType::Brush);
                m_toolDock.SetActiveTool("brush");
            }
            return;
        }
        baseCanvas->setActiveTool(ToolType::None);
        });

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

    m_toolDock.AddTool("brush", "Brush Tool [1 / B]", [this, baseCanvas]() { baseCanvas->setActiveTool(ToolType::Brush); m_toolDock.SetActiveTool("brush"); });
    m_toolDock.AddTool("pencil", "Pencil Tool [2 / P]", [this, baseCanvas]() { baseCanvas->setActiveTool(ToolType::Pencil); m_toolDock.SetActiveTool("pencil"); });
    m_toolDock.AddTool("eraser", "Eraser Tool [3 / E]", [this, baseCanvas]() { baseCanvas->setActiveTool(ToolType::Eraser); m_toolDock.SetActiveTool("eraser"); });
    m_toolDock.AddTool("fill", "Fill Bucket [4 / F]", [this, baseCanvas]() { baseCanvas->setActiveTool(ToolType::Fill); m_toolDock.SetActiveTool("fill"); });
    m_toolDock.AddTool("select", "Selection Box [5 / M]", [this, baseCanvas]() { baseCanvas->setActiveTool(ToolType::Select); m_toolDock.SetActiveTool("select"); });
    m_toolDock.AddTool("magic_wand", "Magic Wand [6 / W]", [this, baseCanvas]() { baseCanvas->setActiveTool(ToolType::MagicWand); m_toolDock.SetActiveTool("magic_wand"); });
    m_toolDock.AddTool("curve", "Curve Tool [7]", [this, baseCanvas]() {
        baseCanvas->setActiveTool(ToolType::Curve);
        m_toolDock.SetActiveTool("curve");
        });

    m_toolDock.AddTool("filled_contour", "Filled Contour [8]", [this, baseCanvas]() {
        baseCanvas->setActiveTool(ToolType::FilledContour);
        m_toolDock.SetActiveTool("filled_contour");
        });
    m_toolDock.AddTool("shapes", "Shapes Tool [9 / U]", [this, baseCanvas]() { baseCanvas->setActiveTool(ToolType::Shapes); m_toolDock.SetActiveTool("shapes"); });
    m_toolDock.AddTool("text", "Text Tool [0 / T]", [this, baseCanvas]() { baseCanvas->setActiveTool(ToolType::Text); m_toolDock.SetActiveTool("text"); });
    m_toolDock.AddTool("gradient", "Gradient Tool [- / G]", [this, baseCanvas]() { baseCanvas->setActiveTool(ToolType::Gradient); m_toolDock.SetActiveTool("gradient"); });
    m_toolDock.AddTool("symmetry", "Symmetry Axis", [this, baseCanvas]() {
        if (baseCanvas->getActiveTool() == ToolType::Symmetry) {
            baseCanvas->clearSymmetry();
            baseCanvas->setActiveTool(ToolType::Brush);
            m_toolDock.SetActiveTool("brush");
            showMessage("Symmetry Removed", sf::Color::Cyan);
        }
        else {
            baseCanvas->setActiveTool(ToolType::Symmetry);
            m_toolDock.SetActiveTool("symmetry");
            if (!baseCanvas->getSymmetryManager().enabled) {
                showMessage("Click & drag to draw symmetry line (Hold Shift to snap)", sf::Color::Yellow);
            }
            else {
                showMessage("Drag handles to adjust line, or click Symmetry again to remove", sf::Color::Yellow);
            }
        }
        });
    m_toolDock.AddTool("perspective", "Perspective Grid", [this, baseCanvas]() { baseCanvas->setActiveTool(ToolType::Perspective); m_toolDock.SetActiveTool("perspective"); });
    m_toolDock.AddTool("ai_gen", "AI Generator", [this, baseCanvas]() {
        g_aiPanel.toggle();
        if (g_aiPanel.getIsVisible()) {
            m_toolDock.SetActiveTool("ai_gen");
        }
        else {
            if (baseCanvas) {
                baseCanvas->setActiveTool(ToolType::Brush);
                m_toolDock.SetActiveTool("brush");
            }
        }
        });

    baseCanvas->setMaxUndoHistory(uiHistorySize);

    std::ifstream sessionFile("projects/last_session.txt");
    if (sessionFile.is_open()) {
        std::string sPath, sName, sMode;
        if (std::getline(sessionFile, sPath) && std::getline(sessionFile, sName) && std::getline(sessionFile, sMode)) {
            if (std::filesystem::exists(sPath) && projManager) {
                activeProjectPath = sPath;
                activeProjectName = sName;
                int fps = 12;
                bool pix = (sMode == "1");
                baseCanvas->clearCanvasImages();
                baseCanvas->clearObjectSelection();
                if (projManager->loadProject(activeProjectPath, *baseCanvas, fps, pix)) {
                    baseCanvas->setPixelMode(pix);
                    baseCanvas->clearIsDirty();
                    baseCanvas->clearHistory();
                    if (baseCanvas->getFrameCount() > 0) {
                        int targetLayer = (baseCanvas->getFrameReadOnly(0)->layers.size() > 1) ? 1 : 0;
                        baseCanvas->setActiveLayer(targetLayer, 0);
                    }
                    baseCanvas->setActiveTool(ToolType::Brush);
                    m_toolDock.SetActiveTool("brush");
                }
            }
        }
        sessionFile.close();
    }

    std::ifstream colorsFile("projects/active_colors.txt");
    if (colorsFile.is_open()) {
        int r1, g1, b1, a1, r2, g2, b2, a2;
        if (colorsFile >> r1 >> g1 >> b1 >> a1 >> r2 >> g2 >> b2 >> a2) {
            sf::Color p(r1, g1, b1, a1);
            sf::Color s(r2, g2, b2, a2);
            baseCanvas->setPrimaryColor(p);
            baseCanvas->setSecondaryColor(s);
            colorPalettePanel.setColors(p, s);
        }
        colorsFile.close();
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

void UIManager::updateStartMenu(float dt, sf::Vector2f mousePos) {
    startupTime += dt;
}

void UIManager::drawStandardMainMenu(sf::RenderWindow& window) {
    sf::Vector2i mousePosI = sf::Mouse::getPosition(window);
    sf::Vector2f mousePos = window.mapPixelToCoords(mousePosI);

    using WisdomUI::Theme;
    using WisdomUI::Animation;

    drawMainMenuBackdrop(window);

    // Each element fades and slides in a moment after the one before it.
    auto appear = [&](float delay) { return Animation::EaseOutCubic((startupTime - delay) / 0.45f); };
    auto faded = [](sf::Color color, float a) {
        color.a = static_cast<sf::Uint8>(static_cast<float>(color.a) * std::clamp(a, 0.0f, 1.0f));
        return color;
        };
    const sf::Color shadow(10, 4, 18);
    const float columnX = getMainMenuItemBounds(0).left;

    // ---- Title ----
    float titleA = appear(0.0f);
    float titleX = columnX - 30.0f * (1.0f - titleA);

    Theme::DrawCrispText(window, font, "WISDOM PARK", 100, titleX + 5.0f, 177.0f, faded(sf::Color(10, 4, 18, 200), titleA), sf::Color::Transparent, false, true);
    Theme::DrawCrispText(window, font, "WISDOM PARK", 100, titleX, 172.0f, faded(Theme::SunsetGold, titleA), faded(Theme::SunsetCoralDark, titleA), false, true);

    float subA = appear(0.12f);
    Theme::DrawCrispText(window, font, "ANIMATION STUDIO  &  PIXEL ART SUITE", 20, columnX + 2.0f, 232.0f, faded(Theme::SunsetPeach, subA), faded(shadow, subA), false, true);

    sf::RectangleShape rule(sf::Vector2f(440.0f * appear(0.2f), 2.0f));
    rule.setPosition(columnX, 270.0f);
    rule.setFillColor(Theme::SunsetAmber);
    window.draw(rule);

    // ---- Menu ----
    struct MenuEntry { const char* label; const char* hint; };
    const MenuEntry entries[7] = {
        { "NEW PROJECT", "CTRL+N" },
        { "OPEN PROJECT", "CTRL+O" },
        { "SETTINGS", "" },
        { "TUTORIALS", "F1" },
        { "KEYBINDS", "K" },
        { "CREDITS", "" },
        { "EXIT", "" }
    };

    for (int i = 0; i < 7; ++i) {
        sf::FloatRect bounds = getMainMenuItemBounds(i);
        float a = appear(0.25f + 0.06f * static_cast<float>(i));
        if (a <= 0.0f) continue;

        bool hovered = bounds.contains(mousePos) && !newProjectModal.getIsOpen() && !m_showKeybinds;
        float t = Theme::AnimateHover(bounds, hovered);
        bool pressed = hovered && sf::Mouse::isButtonPressed(sf::Mouse::Left);
        float x = bounds.left - 36.0f * (1.0f - a);
        float y = bounds.top + (pressed ? 1.0f : 0.0f);
        float centerY = y + bounds.height * 0.5f;

        if (i < 2) {
            // The two main actions are full buttons: New is filled, Open is outlined.
            bool primary = (i == 0);

            if (t > 0.01f) {
                sf::ConvexShape glow = Theme::ChamferedRect(x - 4.0f, y - 4.0f, bounds.width + 8.0f, bounds.height + 8.0f, 8.0f);
                glow.setFillColor(faded(Theme::WithAlpha(primary ? Theme::SunsetGold : Theme::SunsetPeach, 60.0f * t), a));
                window.draw(glow);
            }

            sf::ConvexShape dropShadow = Theme::ChamferedRect(x, y + 4.0f, bounds.width, bounds.height, 6.0f);
            dropShadow.setFillColor(faded(sf::Color(6, 3, 12, 150), a));
            window.draw(dropShadow);

            sf::ConvexShape body = Theme::ChamferedRect(x, y, bounds.width, bounds.height, 6.0f);
            sf::Color fill = primary
                ? Theme::Mix(Theme::SunsetAmber, Theme::SunsetGold, t)
                : Theme::Mix(sf::Color(24, 15, 40, 225), Theme::WithAlpha(Theme::SunsetSkyMid, 240.0f), t);
            body.setFillColor(faded(fill, a));
            body.setOutlineThickness(primary ? 1.0f : 1.5f);
            body.setOutlineColor(faded(primary ? Theme::SunsetGlow : Theme::Mix(Theme::SunsetViolet, Theme::SunsetPeach, t), a));
            window.draw(body);

            sf::RectangleShape shine(sf::Vector2f(bounds.width - 12.0f, 1.0f));
            shine.setPosition(x + 6.0f, y + 1.0f);
            shine.setFillColor(faded(sf::Color(255, 255, 255, primary ? 160 : 50), a));
            window.draw(shine);

            sf::Color textCol = primary ? Theme::SunsetDeepDark : Theme::Mix(Theme::TextPrimary, Theme::SunsetGold, t);
            Theme::DrawCrispText(window, font, entries[i].label, 28, x + 28.0f + 8.0f * t, centerY, faded(textCol, a), primary ? sf::Color::Transparent : faded(shadow, a), false, true);

            sf::Color hintCol = primary ? sf::Color(92, 52, 20) : Theme::TextMuted;
            float hintW = Theme::MeasureText(font, entries[i].hint, 15);
            Theme::DrawCrispText(window, font, entries[i].hint, 15, x + bounds.width - hintW - 24.0f, centerY, faded(hintCol, a), sf::Color::Transparent, false, true);
        }
        else {
            // Secondary entries are plain rows that light up from the left on hover.
            bool isExit = (i == 6);
            sf::Color accent = isExit ? Theme::SunsetCoral : Theme::SunsetAmber;

            if (t > 0.01f) {
                sf::VertexArray wash(sf::Quads, 4);
                sf::Color washStart = Theme::WithAlpha(accent, 70.0f * t * a);
                sf::Color washEnd = Theme::WithAlpha(accent, 0.0f);
                wash[0] = sf::Vertex(sf::Vector2f(x, y), washStart);
                wash[1] = sf::Vertex(sf::Vector2f(x + bounds.width, y), washEnd);
                wash[2] = sf::Vertex(sf::Vector2f(x + bounds.width, y + bounds.height), washEnd);
                wash[3] = sf::Vertex(sf::Vector2f(x, y + bounds.height), washStart);
                window.draw(wash);

                float barH = (bounds.height - 12.0f) * t;
                sf::RectangleShape bar(sf::Vector2f(4.0f, barH));
                bar.setPosition(x, centerY - barH * 0.5f);
                bar.setFillColor(faded(accent, a));
                window.draw(bar);
            }

            sf::Color textCol = Theme::Mix(Theme::TextPrimary, isExit ? Theme::SunsetPeach : Theme::SunsetGold, t);
            Theme::DrawCrispText(window, font, entries[i].label, 24, x + 18.0f + 10.0f * t, centerY, faded(textCol, a), faded(shadow, a), false, true);

            float hintW = Theme::MeasureText(font, entries[i].hint, 14);
            Theme::DrawCrispText(window, font, entries[i].hint, 14, x + bounds.width - hintW - 16.0f, centerY, faded(Theme::TextMuted, a), sf::Color::Transparent, false, true);
        }
    }

    // ---- Recent projects ----
    if (!m_recentProjects.empty()) {
        sf::FloatRect first = getRecentCardBounds(0);
        float headA = appear(0.35f);
        Theme::DrawCrispText(window, font, "JUMP BACK IN", 18, first.left, first.top - 30.0f, faded(Theme::SunsetAmber, headA), faded(shadow, headA), false, true);

        float headW = Theme::MeasureText(font, "JUMP BACK IN", 18);
        sf::RectangleShape headRule(sf::Vector2f((first.width - headW - 16.0f) * headA, 1.0f));
        headRule.setPosition(first.left + headW + 16.0f, first.top - 30.0f);
        headRule.setFillColor(Theme::WithAlpha(Theme::SunsetAmber, 120.0f));
        window.draw(headRule);
    }

    for (size_t i = 0; i < m_recentProjects.size(); ++i) {
        const ProjectMetadata& meta = m_recentProjects[i];
        sf::FloatRect bounds = getRecentCardBounds(static_cast<int>(i));
        float a = appear(0.45f + 0.1f * static_cast<float>(i));
        if (a <= 0.0f) continue;

        bool hovered = bounds.contains(mousePos) && !newProjectModal.getIsOpen() && !m_showKeybinds;
        float t = Theme::AnimateHover(bounds, hovered);
        float x = bounds.left + 40.0f * (1.0f - a);
        float y = bounds.top - 4.0f * t;

        if (t > 0.01f) {
            sf::ConvexShape glow = Theme::ChamferedRect(x - 4.0f, y - 4.0f, bounds.width + 8.0f, bounds.height + 8.0f, 8.0f);
            glow.setFillColor(Theme::WithAlpha(Theme::SunsetPeach, 55.0f * t * a));
            window.draw(glow);
        }

        sf::ConvexShape dropShadow = Theme::ChamferedRect(x, y + 5.0f + 3.0f * t, bounds.width, bounds.height, 6.0f);
        dropShadow.setFillColor(faded(sf::Color(6, 3, 12, 150), a));
        window.draw(dropShadow);

        sf::ConvexShape card = Theme::ChamferedRect(x, y, bounds.width, bounds.height, 6.0f);
        card.setFillColor(faded(Theme::Mix(sf::Color(22, 14, 36, 232), sf::Color(44, 28, 66, 244), t), a));
        card.setOutlineThickness(1.0f);
        card.setOutlineColor(faded(Theme::Mix(Theme::Border, Theme::SunsetPeach, t), a));
        window.draw(card);

        // Thumbnail on a light checkerboard so transparent artwork stays readable.
        const float thumbSize = bounds.height - 28.0f;
        sf::FloatRect thumbBox(x + 14.0f, y + 14.0f, thumbSize, thumbSize);
        sf::RectangleShape thumbBg(sf::Vector2f(thumbSize, thumbSize));
        thumbBg.setPosition(thumbBox.left, thumbBox.top);
        thumbBg.setFillColor(faded(sf::Color(214, 208, 220), a));
        thumbBg.setOutlineThickness(1.0f);
        thumbBg.setOutlineColor(faded(Theme::Mix(Theme::SunsetPlum, Theme::SunsetGold, t), a));
        window.draw(thumbBg);

        const int checks = 6;
        float cell = thumbSize / static_cast<float>(checks);
        for (int cy = 0; cy < checks; ++cy) {
            for (int cx = 0; cx < checks; ++cx) {
                if ((cx + cy) % 2 == 0) continue;
                sf::RectangleShape chk(sf::Vector2f(cell, cell));
                chk.setPosition(thumbBox.left + cx * cell, thumbBox.top + cy * cell);
                chk.setFillColor(faded(sf::Color(184, 176, 194), a));
                window.draw(chk);
            }
        }

        sf::Vector2u texSize = meta.thumbnail.getSize();
        if (texSize.x > 0 && texSize.y > 0) {
            sf::Sprite thumb(meta.thumbnail);
            float scale = std::min(thumbSize / static_cast<float>(texSize.x), thumbSize / static_cast<float>(texSize.y));
            thumb.setScale(scale, scale);
            thumb.setPosition(std::floor(thumbBox.left + (thumbSize - texSize.x * scale) * 0.5f), std::floor(thumbBox.top + (thumbSize - texSize.y * scale) * 0.5f));
            thumb.setColor(faded(sf::Color::White, a));
            window.draw(thumb);
        }

        float textX = thumbBox.left + thumbSize + 20.0f;
        std::string name = meta.name;
        std::replace(name.begin(), name.end(), '_', ' ');
        if (name.length() > 20) name = name.substr(0, 18) + "..";
        Theme::DrawCrispText(window, font, name, 22, textX, y + 34.0f, faded(Theme::Mix(Theme::TextPrimary, Theme::SunsetGold, t), a), faded(shadow, a), false, true);

        std::string details = std::to_string(meta.width) + " x " + std::to_string(meta.height) + "   |   " +
            std::to_string(meta.frameCount) + (meta.frameCount == 1 ? " frame" : " frames");
        Theme::DrawCrispText(window, font, details, 16, textX, y + 68.0f, faded(Theme::TextSecondary, a), sf::Color::Transparent, false, true);

        const char* kind = meta.isPixelMode ? "PIXEL ART" : "ILLUSTRATION";
        float kindW = Theme::MeasureText(font, kind, 13) + 18.0f;
        sf::RectangleShape tag(sf::Vector2f(kindW, 22.0f));
        tag.setPosition(textX, y + bounds.height - 44.0f);
        tag.setFillColor(sf::Color::Transparent);
        tag.setOutlineThickness(1.0f);
        tag.setOutlineColor(faded(meta.isPixelMode ? Theme::SunsetAmber : Theme::SunsetViolet, a));
        window.draw(tag);
        Theme::DrawCrispText(window, font, kind, 13, textX + kindW * 0.5f, y + bounds.height - 33.0f, faded(meta.isPixelMode ? Theme::SunsetAmber : Theme::TextSecondary, a), sf::Color::Transparent, true, true);

        if (t > 0.05f) {
            float openW = Theme::MeasureText(font, "OPEN  >", 15);
            Theme::DrawCrispText(window, font, "OPEN  >", 15, x + bounds.width - openW - 20.0f + 6.0f * t, y + bounds.height - 33.0f, Theme::WithAlpha(Theme::SunsetGold, 255.0f * t * a), sf::Color::Transparent, false, true);
        }
    }

    float footA = appear(0.8f);
    Theme::DrawCrispText(window, font, "v0.1.0-alpha", 14, columnX, 1034.0f, faded(Theme::TextMuted, footA), faded(shadow, footA), false, true);
}

// Darkens the side of the artwork the menu sits on, and adds drifting fireflies.
void UIManager::drawMainMenuBackdrop(sf::RenderWindow& window) {
    const sf::Color dark(12, 6, 22);

    auto gradient = [&](float x0, float x1, float alpha0, float alpha1) {
        sf::VertexArray quad(sf::Quads, 4);
        sf::Color c0 = WisdomUI::Theme::WithAlpha(dark, alpha0);
        sf::Color c1 = WisdomUI::Theme::WithAlpha(dark, alpha1);
        quad[0] = sf::Vertex(sf::Vector2f(x0, 0.0f), c0);
        quad[1] = sf::Vertex(sf::Vector2f(x1, 0.0f), c1);
        quad[2] = sf::Vertex(sf::Vector2f(x1, 1080.0f), c1);
        quad[3] = sf::Vertex(sf::Vector2f(x0, 1080.0f), c0);
        window.draw(quad);
        };

    gradient(0.0f, 420.0f, 235.0f, 215.0f);
    gradient(420.0f, 1000.0f, 215.0f, 0.0f);
    if (!m_recentProjects.empty()) {
        gradient(1150.0f, 1920.0f, 0.0f, 150.0f);
    }

    drawFireflies(window);
}

void UIManager::drawFireflies(sf::RenderWindow& window) {
    for (int i = 0; i < 34; ++i) {
        float seed = static_cast<float>(i);
        float speed = 14.0f + std::fmod(seed * 7.3f, 22.0f);
        float baseX = std::fmod(seed * 197.3f, 1920.0f);
        float travel = std::fmod(startupTime * speed + seed * 131.0f, 1180.0f);
        float px = baseX + std::sin(startupTime * 0.5f + seed * 1.9f) * 26.0f;
        float py = 1100.0f - travel;
        float twinkle = 0.45f + 0.55f * std::sin(startupTime * 2.2f + seed * 2.7f);
        float size = (i % 3 == 0) ? 4.0f : 3.0f;

        sf::RectangleShape halo(sf::Vector2f(size * 3.0f, size * 3.0f));
        halo.setOrigin(size * 1.5f, size * 1.5f);
        halo.setPosition(std::floor(px), std::floor(py));
        halo.setFillColor(sf::Color(255, 196, 110, static_cast<sf::Uint8>(std::max(0.0f, 34.0f * twinkle))));
        window.draw(halo, sf::RenderStates(sf::BlendAdd));

        sf::RectangleShape core(sf::Vector2f(size, size));
        core.setOrigin(size * 0.5f, size * 0.5f);
        core.setPosition(std::floor(px), std::floor(py));
        core.setFillColor(sf::Color(255, 232, 160, static_cast<sf::Uint8>(std::max(0.0f, 210.0f * twinkle))));
        window.draw(core, sf::RenderStates(sf::BlendAdd));
    }
}

sf::FloatRect UIManager::getMainMenuItemBounds(int index) const {
    const float x = 110.0f;
    const float width = 440.0f;
    if (index == 0) return sf::FloatRect(x, 322.0f, width, 70.0f);
    if (index == 1) return sf::FloatRect(x, 404.0f, width, 70.0f);
    return sf::FloatRect(x, 506.0f + static_cast<float>(index - 2) * 60.0f, width, 52.0f);
}

sf::FloatRect UIManager::getRecentCardBounds(int index) const {
    return sf::FloatRect(1300.0f, 322.0f + static_cast<float>(index) * 166.0f, 510.0f, 150.0f);
}

void UIManager::refreshRecentProjects() {
    m_recentProjects.clear();
    if (!projManager) return;

    m_recentProjects = projManager->getRecentProjects();
    if (m_recentProjects.size() > 3) m_recentProjects.resize(3);
    for (auto& meta : m_recentProjects) {
        meta.thumbnail.setSmooth(!meta.isPixelMode);
    }
}

void UIManager::loadProjectFromMenu(const ProjectMetadata& meta, AppState& currentState, Canvas& canvas, Timeline& timeline, ProjectManager& pm) {
    activeProjectName = meta.name;
    activeProjectPath = meta.path;
    int loadedFps = 12;
    bool isPix = false;
    canvas.clearCanvasImages();
    canvas.clearObjectSelection();
    if (pm.loadProject(meta.path, canvas, loadedFps, isPix)) {
        timeline.setFrame(std::max(0, static_cast<int>(canvas.getFrameCount()) - 1));
        canvas.setPixelMode(isPix);
        canvas.clearIsDirty();
        canvas.clearHistory();

        // Ensure a writable artwork layer is selected
        if (canvas.getFrameCount() > 0) {
            int targetLayer = (canvas.getFrameReadOnly(0)->layers.size() > 1) ? 1 : 0;
            canvas.setActiveLayer(targetLayer, 0);
        }

        canvas.setActiveTool(ToolType::Brush);
        m_toolDock.SetActiveTool("brush");
        m_activeTool.reset(); // Force workspace tool to rebuild fresh view transforms

        currentState = AppState::Painting;
        showMessage("Loaded Project: " + meta.name, sf::Color::Green);
    }
    else {
        showMessage("Failed to load project files.", sf::Color::Red);
    }
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
    WisdomUI::Theme::DrawSunsetButton(window, m_welcomeModeToggleBounds, toggleLabel, font, 13, false, isToggleHov, m_useMinigameWelcome, 1.0f);

    bool isFsHov = m_startMenuFullscreenBtnBounds.contains(mousePos);
    std::string fsLabel = uiFullscreen ? "Fullscreen: ON" : "Fullscreen: OFF";
    WisdomUI::Theme::DrawSunsetButton(window, m_startMenuFullscreenBtnBounds, fsLabel, font, 13, false, isFsHov, false, 1.0f);
}

void UIManager::drawBackButton(sf::RenderWindow& window, const std::string& hoverKey, float x, float y) {
    sf::Vector2i mousePosI = sf::Mouse::getPosition(window);
    sf::Vector2f mousePos = window.mapPixelToCoords(mousePosI);

    sf::FloatRect bounds(x, y, 130.f, 38.f);
    bool isHov = bounds.contains(mousePos);
    WisdomUI::Theme::DrawSunsetButton(window, bounds, "< BACK", font, 12, false, isHov, false, 1.0f);
}



bool UIManager::triggerSave(Canvas& canvas, Timeline& timeline) {
    if (activeProjectPath.empty()) {
        activeProjectPath = "projects/" + activeProjectName + ".wpk";
    }
    if (projManager) {
        if (projManager->saveProjectAs(activeProjectPath, activeProjectName, canvas, static_cast<int>(timeline.getFps()), canvas.getPixelMode())) {
            canvas.clearIsDirty();

            std::ofstream sessionFile("projects/last_session.txt");
            if (sessionFile.is_open()) {
                sessionFile << activeProjectPath << "\n" << activeProjectName << "\n" << (canvas.getPixelMode() ? "1" : "0") << "\n";
                sessionFile.close();
            }

            std::ofstream colorsFile("projects/active_colors.txt");
            if (colorsFile.is_open()) {
                sf::Color p = canvas.getPrimaryColor();
                sf::Color s = canvas.getSecondaryColor();
                colorsFile << static_cast<int>(p.r) << " " << static_cast<int>(p.g) << " " << static_cast<int>(p.b) << " " << static_cast<int>(p.a) << "\n";
                colorsFile << static_cast<int>(s.r) << " " << static_cast<int>(s.g) << " " << static_cast<int>(s.b) << " " << static_cast<int>(s.a) << "\n";
                colorsFile.close();
            }

            return true;
        }
    }
    return false;
}

void UIManager::handleEvent(const sf::Event& event, sf::RenderWindow& window, AppState& currentState, AppSettings& settings, Canvas& canvas, Timeline& timeline, AIHelper& aiHelper, ProjectManager& pm) {
    if (event.type == sf::Event::MouseButtonReleased) {
        canvas.resetRecolorLatch();
    }

    if (event.type == sf::Event::Closed) {
        if (currentState == AppState::Painting) {
            triggerSave(canvas, timeline);
        }
    }

    if (handleHandCamWidgetEvents(event)) return;

    if (m_showResizeModal) {
        if (handleResizeModalEvent(event, window, canvas, timeline)) return;
    }

    if (m_showPasteResolutionModal) {
        if (handlePasteResolutionModalEvent(event, window, canvas, timeline)) return;
    }
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
    else if (event.type == sf::Event::MouseMoved) {
        pixelPos = sf::Vector2i(event.mouseMove.x, event.mouseMove.y);
    }
    sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);
    sf::Vector2f logicalMousePos = canvas.getInverseTransform().transformPoint(mousePos);

    if (showUnsavedWarning) {
        handleUnsavedWarningEvent(event, window, currentState, canvas, timeline);
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
            activeProjectPath = "projects/" + activeProjectName + ".wpk";
            canvas.clearCanvasImages();
            canvas.clearObjectSelection();
            canvas.setPixelMode(newProjectModal.getIsPixelMode());
            pm.createNewProject(activeProjectName, newProjectModal.getWidth(), newProjectModal.getHeight(), 12, newProjectModal.getIsPixelMode(), canvas);
            canvas.clearIsDirty();
            canvas.clearHistory();
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
                    for (int i = 0; i < 7; ++i) {
                        if (getMainMenuItemBounds(i).contains(mousePos)) {
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

                    for (size_t i = 0; i < m_recentProjects.size(); ++i) {
                        if (getRecentCardBounds(static_cast<int>(i)).contains(mousePos)) {
                            ProjectMetadata meta = m_recentProjects[i];
                            loadProjectFromMenu(meta, currentState, canvas, timeline, pm);
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
                if (!m_useMinigameWelcome && !newProjectModal.getIsOpen()) {
                    if (event.key.code == sf::Keyboard::N && event.key.control) {
                        newProjectModal.open();
                        return;
                    }
                    if (event.key.code == sf::Keyboard::O && event.key.control) {
                        currentMenuState = MenuState::Projects;
                        return;
                    }
                    if (event.key.code == sf::Keyboard::F1) {
                        currentMenuState = MenuState::Tutorials;
                        activeTutorialIndex = -1;
                        return;
                    }
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
                    loadProjectFromMenu(meta, currentState, canvas, timeline, pm);
                }
                else if (action == "open_native") {
                    std::string file = NativeDialogs::openFileDialog("Wisdom Park Projects\0*.wpk\0All Files\0*.*\0");
                    if (!file.empty()) {
                        activeProjectPath = file;
                        activeProjectName = std::filesystem::path(file).stem().string();
                        int loadedFps = 12;
                        bool isPix = false;
                        canvas.clearCanvasImages();
                        canvas.clearObjectSelection();
                        if (pm.loadProject(activeProjectPath, canvas, loadedFps, isPix)) {
                            timeline.setFrame(std::max(0, static_cast<int>(canvas.getFrameCount()) - 1));
                            canvas.clearIsDirty();
                            canvas.clearHistory();
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
                using namespace MenuLayout;

                if (BackButton().contains(mousePos)) {
                    g_resDropdownOpen = false;
                    currentMenuState = MenuState::Main;
                    return;
                }

                bool displayChanged = false;

                if (g_resDropdownOpen) {
                    for (const SettingRow& row : kSettingRows) {
                        if (row.id != SettingId::Resolution) continue;
                        sf::FloatRect dropBtn = DropdownButton(SettingsRowBounds(row));
                        for (int i = 0; i < kResolutionOptionCount; ++i) {
                            if (DropdownOption(dropBtn, i).contains(mousePos)) {
                                settings.resWidth = kResolutionOptions[i][0];
                                settings.resHeight = kResolutionOptions[i][1];
                                g_resW = settings.resWidth;
                                g_resH = settings.resHeight;
                                displayChanged = true;
                            }
                        }
                    }
                    g_resDropdownOpen = false;
                }
                else {
                    bool clickedKeyField = false;

                    for (const SettingRow& row : kSettingRows) {
                        sf::FloatRect rowRect = SettingsRowBounds(row);
                        if (!rowRect.contains(mousePos)) continue;

                        // Steppers only react on their arrows: -1 for left, +1 for right.
                        int dir = 0;
                        if (StepperLeft(rowRect).contains(mousePos)) dir = -1;
                        else if (StepperRight(rowRect).contains(mousePos)) dir = 1;

                        switch (row.id) {
                        case SettingId::Fullscreen:
                            toggleFullscreen(window, settings);
                            break;
                        case SettingId::VSync:
                            uiVsync = !uiVsync;
                            settings.vsync = uiVsync;
                            window.setVerticalSyncEnabled(uiVsync);
                            break;
                        case SettingId::FpsLimit:
                            if (dir < 0) uiFpsLimit = (uiFpsLimit == 60) ? 240 : ((uiFpsLimit == 144) ? 60 : 144);
                            else if (dir > 0) uiFpsLimit = (uiFpsLimit == 60) ? 144 : ((uiFpsLimit == 144) ? 240 : 60);
                            if (dir != 0) { settings.fpsLimit = uiFpsLimit; window.setFramerateLimit(uiFpsLimit); }
                            break;
                        case SettingId::Resolution:
                            if (DropdownButton(rowRect).contains(mousePos)) g_resDropdownOpen = true;
                            break;
                        case SettingId::AutoBackup:
                            uiAutoBackup = !uiAutoBackup;
                            settings.autoBackup = uiAutoBackup;
                            break;
                        case SettingId::HwAccel:
                            uiHwAccel = !uiHwAccel;
                            settings.hwAccel = uiHwAccel;
                            break;
                        case SettingId::PreviewRate:
                            if (dir < 0) uiAnimFps = (uiAnimFps == 12) ? 60 : ((uiAnimFps == 24) ? 12 : 24);
                            else if (dir > 0) uiAnimFps = (uiAnimFps == 12) ? 24 : ((uiAnimFps == 24) ? 60 : 12);
                            if (dir != 0) settings.animFps = uiAnimFps;
                            break;
                        case SettingId::UndoHistory:
                            if (dir < 0) uiHistorySize = (uiHistorySize == 50) ? 200 : ((uiHistorySize == 100) ? 50 : 100);
                            else if (dir > 0) uiHistorySize = (uiHistorySize == 50) ? 100 : ((uiHistorySize == 100) ? 200 : 50);
                            if (dir != 0) { settings.historySize = uiHistorySize; canvas.setMaxUndoHistory(uiHistorySize); }
                            break;
                        case SettingId::AiProvider:
                            if (dir != 0) AIManager::getInstance().cycleProvider(dir);
                            break;
                        case SettingId::ApiKey:
                            clickedKeyField = TextField(rowRect).contains(mousePos);
                            break;
                        default:
                            break;
                        }
                        break;
                    }

                    if (clickedKeyField) {
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
                if (MenuLayout::BackButton().contains(mousePos)) {
                    if (activeTutorialIndex != -1) activeTutorialIndex = -1;
                    else currentMenuState = MenuState::Main;
                    return;
                }

                if (activeTutorialIndex == -1) {
                    for (int i = 0; i < MenuLayout::kTutorialCount; ++i) {
                        if (MenuLayout::TutorialCard(i).contains(mousePos)) {
                            activeTutorialIndex = i;
                            return;
                        }
                    }
                }
                else {
                    if (MenuLayout::TutorialReturnButton().contains(mousePos)) {
                        activeTutorialIndex = -1;
                        return;
                    }
                    for (int i = 0; i < MenuLayout::kTutorialCount; ++i) {
                        if (MenuLayout::TutorialNavItem(i).contains(mousePos)) {
                            activeTutorialIndex = i;
                            return;
                        }
                    }
                }
            }
            else if (currentMenuState == MenuState::Credits) {
                if (MenuLayout::BackButton().contains(mousePos)) {
                    currentMenuState = MenuState::Main;
                    return;
                }

                if (MenuLayout::CreditsEgg().contains(mousePos)) {
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
            if (s_exitToMenuRequested) {
                s_exitToMenuRequested = false;
                if (canvas.getIsDirty()) {
                    showUnsavedWarning = true;
                }
                else {
                    currentState = AppState::Welcome;
                    currentMenuState = MenuState::Main;
                }
                return;
            }

            if (m_fullscreenToggleRequested) {
                m_fullscreenToggleRequested = false;
                toggleFullscreen(window, settings);
            }
            if (!m_pendingCheckoutHash.empty()) {
                std::string hash = m_pendingCheckoutHash;
                m_pendingCheckoutHash.clear();

                pullCommitToCanvas(hash, canvas, timeline);
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
                if (action == "resize") canvas.enterTransformMode(curFrame);
                else if (action == "flip_h") canvas.flipSelectionHorizontal(curFrame);
                else if (action == "flip_v") canvas.flipSelectionVertical(curFrame);
                else if (action == "duplicate") canvas.duplicateSelection(curFrame);
                else if (action == "merge") {
                    canvas.mergeSelectedObjects(curFrame);
                    showMessage("Merged Objects into One", sf::Color::Green);
                }
                else if (action == "deselect") {
                    canvas.saveUndoState();
                    canvas.commitSelection(curFrame);
                    canvas.clearObjectSelection();
                    showMessage("Deselected", sf::Color::Cyan);
                }
                else if (action == "crop") canvas.cropSelection(curFrame);
                else if (action == "delete") {
                    canvas.saveUndoState();
                    canvas.deleteSelection(curFrame);
                    canvas.commitSelection(curFrame);
                    canvas.clearObjectSelection();
                    showMessage("Deleted Selection", sf::Color::Cyan);
                }
            },
            [&]() {
                canvas.makeOutline(timeline.getCurrentFrame(), g_outlineColor);
                showMessage("Outline Created", sf::Color::Green);
            },
            [&]() {
                g_selectingOutlineColor = true;
                m_activeRightTab = RightTabMode::Palette;
                showMessage("Pick Outline Color from Palette", sf::Color(255, 200, 100));
            },
            [&](float stab) {
                canvas.setStabilizer(stab);
            },
            [&](int newZ) {
                canvas.setSelectionZOrder(newZ, timeline.getCurrentFrame());
                showMessage("Z-Order: " + std::to_string(canvas.getSelectionZOrder(timeline.getCurrentFrame())), sf::Color::Cyan);
            },
            [&](PixelBrushShape shape) {
                canvas.setPixelBrushShape(shape);
                std::string sName = "Square";
                if (shape == PixelBrushShape::Circle) sName = "Circle";
                else if (shape == PixelBrushShape::Slash) sName = "Slash";
                else if (shape == PixelBrushShape::Rectangle) sName = "Rectangle";
                showMessage("Brush Shape: " + sName, sf::Color::Green);
            },
            [&]() {
                canvas.clearSymmetry();
                showMessage("Symmetry Cleared", sf::Color::Cyan);
            }
        )) return;

        if (m_showTimeline) {
            float timelineY = 1080.0f - WisdomUI::Theme::StatusBarHeight - WisdomUI::Theme::TimelineHeight;
            sf::FloatRect timelinePanelBounds(0.0f, timelineY, 1920.0f, WisdomUI::Theme::TimelineHeight + WisdomUI::Theme::StatusBarHeight);

            float cardW = 90.0f;
            float cardSpacing = 12.0f;
            int totalFrames = static_cast<int>(canvas.getFrameCount());
            float trayW = 1920.0f - 24.0f;
            float maxScroll = std::max(0.0f, (24.0f + totalFrames * (cardW + cardSpacing)) - trayW);

            if (g_isDraggingTimeline) {
                if (event.type == sf::Event::MouseMoved) {
                    float deltaX = mousePos.x - g_timelineDragStartMouseX;
                    if (std::abs(deltaX) > 4.0f) {
                        g_timelineDragMoved = true;
                    }
                    g_timelineScrollX = std::clamp(g_timelineDragStartScrollX - deltaX, 0.0f, maxScroll);
                    return;
                }
                if (event.type == sf::Event::MouseButtonReleased) {
                    g_isDraggingTimeline = false;
                    if (!g_timelineDragMoved && event.mouseButton.button == sf::Mouse::Left) {
                        sf::FloatRect trayBounds(12.0f, timelineY + WisdomUI::Theme::TimelineHeaderHeight + 4.0f, 1920.0f - 24.0f, WisdomUI::Theme::TimelineHeight - WisdomUI::Theme::TimelineHeaderHeight - 12.0f);
                        if (trayBounds.contains(mousePos)) {
                            float startX = trayBounds.left + 12.0f - g_timelineScrollX;
                            float cardY = timelineY + WisdomUI::Theme::TimelineHeaderHeight + 14.0f;
                            float cardH = 120.0f;
                            timeline.syncWithCanvas(totalFrames);

                            for (int i = 0; i < totalFrames; ++i) {
                                sf::FloatRect cardBounds(startX, cardY, cardW, cardH);
                                if (cardBounds.contains(mousePos)) {
                                    canvas.commitSelection(timeline.getCurrentFrame());
                                    canvas.clearObjectSelection();
                                    timeline.setFrame(i);
                                    return;
                                }
                                startX += cardW + cardSpacing;
                            }
                        }
                    }
                    return;
                }
            }

            if (timelinePanelBounds.contains(mousePos)) {
                if (event.type == sf::Event::MouseWheelScrolled) {
                    g_timelineScrollX = std::clamp(g_timelineScrollX - event.mouseWheelScroll.delta * 50.0f, 0.0f, maxScroll);
                    return;
                }

                if (event.type == sf::Event::MouseButtonPressed) {
                    if (event.mouseButton.button == sf::Mouse::Middle) {
                        g_isDraggingTimeline = true;
                        g_timelineDragStartMouseX = mousePos.x;
                        g_timelineDragStartScrollX = g_timelineScrollX;
                        g_timelineDragMoved = false;
                        return;
                    }

                    if (event.mouseButton.button == sf::Mouse::Left) {
                        sf::FloatRect playBtn = m_timelineHeader.GetPlayBounds();
                        sf::FloatRect addBtn = m_timelineHeader.GetAddBounds();
                        sf::FloatRect dupBtn = m_timelineHeader.GetDuplicateBounds();
                        sf::FloatRect delBtn = m_timelineHeader.GetDeleteBounds();
                        sf::FloatRect onionBtn = m_timelineHeader.GetOnionBounds();
                        sf::FloatRect closeBtn = m_timelineHeader.GetCloseBounds();

                        if (playBtn.contains(mousePos)) { timeline.togglePlayback(); return; }
                        if (addBtn.contains(mousePos)) {
                            int cur = timeline.getCurrentFrame();
                            canvas.commitSelection(cur);
                            canvas.clearObjectSelection();
                            canvas.addFrame(cur);
                            timeline.addFrameAfter(cur);
                            timeline.syncWithCanvas(static_cast<int>(canvas.getFrameCount()));
                            timeline.setFrame(cur + 1);
                            return;
                        }
                        if (dupBtn.contains(mousePos)) {
                            int cur = timeline.getCurrentFrame();
                            canvas.commitSelection(cur);
                            canvas.clearObjectSelection();
                            canvas.duplicateFrame(cur);
                            timeline.duplicateFrame(cur);
                            timeline.syncWithCanvas(static_cast<int>(canvas.getFrameCount()));
                            timeline.setFrame(cur + 1);
                            return;
                        }
                        if (delBtn.contains(mousePos)) {
                            if (canvas.getFrameCount() > 1) {
                                int cur = timeline.getCurrentFrame();
                                canvas.commitSelection(cur);
                                canvas.clearObjectSelection();
                                canvas.deleteFrame(cur);
                                timeline.deleteFrame(cur);
                                timeline.syncWithCanvas(static_cast<int>(canvas.getFrameCount()));
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

                        sf::FloatRect trayBounds(12.0f, timelineY + WisdomUI::Theme::TimelineHeaderHeight + 4.0f, 1920.0f - 24.0f, WisdomUI::Theme::TimelineHeight - WisdomUI::Theme::TimelineHeaderHeight - 12.0f);
                        if (trayBounds.contains(mousePos)) {
                            g_isDraggingTimeline = true;
                            g_timelineDragStartMouseX = mousePos.x;
                            g_timelineDragStartScrollX = g_timelineScrollX;
                            g_timelineDragMoved = false;
                            return;
                        }
                    }
                    return;
                }

                if (event.type == sf::Event::MouseButtonReleased) {
                    g_isDraggingTimeline = false;
                    return;
                }
            }
        }

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
            bool isKeyEvent = (event.type == sf::Event::KeyPressed || event.type == sf::Event::KeyReleased || event.type == sf::Event::TextEntered);

            if (!isKeyEvent || colorPalettePanel.isTypingInput()) {
                if (colorPalettePanel.handleEvent(event, mousePos, canvas)) {
                    if (colorPalettePanel.wantsClose()) {
                        colorPalettePanel.clearWantsClose();
                        m_activeRightTab = RightTabMode::None;
                        g_selectingOutlineColor = false;
                        g_selectingGridColor = false;
                        return;
                    }
                    if (g_selectingOutlineColor) {
                        g_outlineColor = canvas.getPrimaryColor();
                        m_toolOptionsBar.SetOutlineColor(g_outlineColor);
                    }
                    if (g_selectingGridColor) {
                        canvas.setCustomGridColor(canvas.getPrimaryColor());
                    }
                    return;
                }
            }

            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                std::string cpAction = colorPalettePanel.processClick(mousePos, canvas);
                if (g_selectingOutlineColor) {
                    g_outlineColor = canvas.getPrimaryColor();
                    m_toolOptionsBar.SetOutlineColor(g_outlineColor);
                }
                if (g_selectingGridColor) {
                    canvas.setCustomGridColor(canvas.getPrimaryColor());
                }
                if (cpAction == "color_close") {
                    m_activeRightTab = RightTabMode::None;
                    g_selectingOutlineColor = false;
                    g_selectingGridColor = false;
                    return;
                }
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
            if (assetBrowser->handleEvent(event, window, canvas, timeline.getCurrentFrame())) return;
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
                    if (action == "action:paste_palette") {
                        if (g_aiPanel.importFromClipboard()) {
                            const auto& colors = g_aiPanel.getSelectedPaletteColors();
                            colorPalettePanel.addAdvicePalette(colors);
                            m_activeRightTab = RightTabMode::Palette;
                            showMessage("Imported & Added: " + g_aiPanel.getSelectedPaletteName(), sf::Color::Green);
                        }
                        else {
                            showMessage("Clipboard must contain a Lospec link, slug, or hex codes!", sf::Color::Red);
                        }
                    }
                    else if (action == "action:random_palette") {
                        g_aiPanel.pickRandomPalette();
                        const auto& colors = g_aiPanel.getSelectedPaletteColors();
                        colorPalettePanel.addAdvicePalette(colors);
                        m_activeRightTab = RightTabMode::Palette;
                        showMessage("Added: " + g_aiPanel.getSelectedPaletteName(), sf::Color::Green);
                    }
                    else if (action == "action:send_all_swatches") {
                        const auto& colors = g_aiPanel.getSelectedPaletteColors();
                        for (const auto& col : colors) {
                            colorPalettePanel.getColorManager().addCustomSwatch(col);
                        }
                        m_activeRightTab = RightTabMode::Palette;
                        showMessage("Sent " + std::to_string(colors.size()) + " colors to Swatches!", sf::Color::Green);
                    }
                    else if (action == "action:swatch_picked") {
                        sf::Color picked = g_aiPanel.getLastPickedColor();
                        canvas.setPrimaryColor(picked);
                        colorPalettePanel.setColors(picked, canvas.getSecondaryColor());
                        colorPalettePanel.getColorManager().addRecentColor(picked);
                        showMessage("Color Picked", sf::Color::Cyan);
                    }
                    else if (action == "action:get_advice") {
                        sf::Color cur = canvas.getPrimaryColor();
                        static int s_adviceCounter = 0;
                        auto ramp = g_aiPanel.generateAdvice(cur, s_adviceCounter++);
                        if (!colorPalettePanel.addAdvicePalette(ramp)) {
                            m_activeRightTab = RightTabMode::Palette;
                            showMessage("All 5 Palettes Pinned! Unpin one to replace.", sf::Color::Yellow);
                        }
                        else {
                            m_activeRightTab = RightTabMode::Palette;
                            showMessage("Added Color Advice Palette (Max 5)", sf::Color::Green);
                        }
                    }
                    else if (action == "close") {
                        canvas.setActiveTool(ToolType::Brush);
                        m_toolDock.SetActiveTool("brush");
                        showMessage("Assistant Closed", sf::Color::Cyan);
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

        if (event.type == sf::Event::KeyReleased) {
            if (event.key.code == sf::Keyboard::Left) g_timelineLeftHeld = false;
            if (event.key.code == sf::Keyboard::Right) g_timelineRightHeld = false;
        }

        if (event.type == sf::Event::KeyPressed) {
            if (colorPalettePanel.isTypingInput()) {
                return;
            }
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
                if (!event.key.control && !event.key.alt) {
                    int curFrame = timeline.getCurrentFrame();
                    if (event.key.code == sf::Keyboard::Num1) {
                        canvas.commitSelection(curFrame);
                        canvas.setActiveTool(ToolType::Brush);
                        m_toolDock.SetActiveTool("brush");
                        showMessage("Brush Tool [1]", sf::Color::Cyan);
                    }
                    else if (event.key.code == sf::Keyboard::Num2) {
                        canvas.commitSelection(curFrame);
                        canvas.setActiveTool(ToolType::Pencil);
                        m_toolDock.SetActiveTool("pencil");
                        showMessage("Pencil Tool [2]", sf::Color::Cyan);
                    }
                    else if (event.key.code == sf::Keyboard::Num3) {
                        canvas.commitSelection(curFrame);
                        canvas.setActiveTool(ToolType::Eraser);
                        m_toolDock.SetActiveTool("eraser");
                        showMessage("Eraser Tool [3]", sf::Color::Cyan);
                    }
                    else if (event.key.code == sf::Keyboard::Num4) {
                        canvas.commitSelection(curFrame);
                        canvas.setActiveTool(ToolType::Fill);
                        m_toolDock.SetActiveTool("fill");
                        showMessage("Fill Bucket [4]", sf::Color::Cyan);
                    }
                    else if (event.key.code == sf::Keyboard::Num5) {
                        canvas.setActiveTool(ToolType::Select);
                        m_toolDock.SetActiveTool("select");
                        showMessage("Select Tool [5]", sf::Color::Cyan);
                    }
                    else if (event.key.code == sf::Keyboard::Num6) {
                        canvas.commitSelection(curFrame);
                        canvas.setActiveTool(ToolType::MagicWand);
                        m_toolDock.SetActiveTool("magic_wand");
                        showMessage("Magic Wand [6]", sf::Color::Cyan);
                    }
                    else if (event.key.code == sf::Keyboard::Num7) {
                        canvas.commitSelection(curFrame);
                        canvas.setActiveTool(ToolType::Curve);
                        m_toolDock.SetActiveTool("curve");
                        showMessage("Curve Tool [7]", sf::Color::Cyan);
                    }
                    else if (event.key.code == sf::Keyboard::Num8) {
                        canvas.commitSelection(curFrame);
                        canvas.setActiveTool(ToolType::FilledContour);
                        m_toolDock.SetActiveTool("filled_contour");
                        showMessage("Filled Contour [8]", sf::Color::Cyan);
                    }
                    else if (event.key.code == sf::Keyboard::Num9) {
                        canvas.commitSelection(curFrame);
                        canvas.setActiveTool(ToolType::Shapes);
                        m_toolDock.SetActiveTool("shapes");
                        showMessage("Shapes Tool [9]", sf::Color::Cyan);
                    }
                    else if (event.key.code == sf::Keyboard::Num0) {
                        canvas.commitSelection(curFrame);
                        canvas.setActiveTool(ToolType::Text);
                        m_toolDock.SetActiveTool("text");
                        showMessage("Text Tool [0]", sf::Color::Cyan);
                    }
                    else if (event.key.code == sf::Keyboard::Hyphen) {
                        canvas.commitSelection(curFrame);
                        canvas.setActiveTool(ToolType::Gradient);
                        m_toolDock.SetActiveTool("gradient");
                        showMessage("Gradient Tool [-]", sf::Color::Cyan);
                    }
                    else if (event.key.code == sf::Keyboard::Equal) {
                        canvas.commitSelection(curFrame);
                        canvas.setCustomGridEnabled(!canvas.isCustomGridEnabled());
                        showMessage(canvas.isCustomGridEnabled() ? "Grid: ON" : "Grid: OFF", sf::Color::Cyan);
                    }
                }

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
                        canvas.clearCanvasImages();
                        canvas.clearObjectSelection();
                        if (pm.loadProject(activeProjectPath, canvas, loadedFps, isPix)) {
                            timeline.setFrame(std::max(0, static_cast<int>(canvas.getFrameCount()) - 1));
                            canvas.setPixelMode(isPix);
                            canvas.clearIsDirty();
                            canvas.clearHistory();

                            // Ensure a writable artwork layer is selected

                            // Ensure a writable artwork layer is selected
                            if (canvas.getFrameCount() > 0) {
                                int targetLayer = (canvas.getFrameReadOnly(0)->layers.size() > 1) ? 1 : 0;
                                canvas.setActiveLayer(targetLayer, 0);
                            }

                            canvas.setActiveTool(ToolType::Brush);
                            m_toolDock.SetActiveTool("brush");
                            m_activeTool.reset(); // Force workspace tool to rebuild fresh view transforms

                            currentState = AppState::Painting;
                            showMessage("Loaded Native Project", sf::Color::Green);
                        }
                        else {
                            showMessage("Failed to load native project.", sf::Color::Red);
                        }
                    }
                }

                if (keybindManager.isActionTriggered("proj_new", event)) requestNewProject(canvas);

                if (keybindManager.isActionTriggered("time_play", event)) timeline.togglePlayback();
                if (keybindManager.isActionTriggered("time_start", event)) {
                    canvas.commitSelection(timeline.getCurrentFrame());
                    timeline.setFrame(0);
                }
                if (keybindManager.isActionTriggered("time_end", event)) {
                    canvas.commitSelection(timeline.getCurrentFrame());
                    timeline.setFrame(static_cast<int>(canvas.getFrameCount()) - 1);
                }

                bool isLeft = (event.key.code == sf::Keyboard::Left) || keybindManager.isActionTriggered("time_prev", event);
                bool isRight = (event.key.code == sf::Keyboard::Right) || keybindManager.isActionTriggered("time_next", event);

                if (isLeft) {
                    if (!g_timelineLeftHeld) {
                        g_timelineLeftHeld = true;

                        int total = static_cast<int>(canvas.getFrameCount());
                        timeline.syncWithCanvas(total);
                        int cur = timeline.getCurrentFrame();

                        if (cur > 0) {
                            g_warnLeftPending = false;
                            g_warnRightPending = false;
                            canvas.commitSelection(cur);
                            canvas.clearObjectSelection();
                            timeline.setFrame(cur - 1);
                        }
                        else {
                            if (canvas.isFrameEmpty(0)) {
                                if (g_warnLeftPending && g_warnLeftClock.getElapsedTime().asSeconds() <= 3.5f) {
                                    g_warnLeftPending = false;
                                    canvas.commitSelection(0);
                                    canvas.clearObjectSelection();
                                    canvas.addFrameAt(0);
                                    timeline.addFrameAt(0);
                                    timeline.syncWithCanvas(static_cast<int>(canvas.getFrameCount()));
                                    timeline.setFrame(0);
                                    showMessage("Added Frame 1 at start", sf::Color::Green);
                                }
                                else {
                                    g_warnLeftPending = true;
                                    g_warnLeftClock.restart();
                                    showMessage("Current frame is empty. Press Left Arrow again to add frame.", sf::Color::Yellow);
                                }
                            }
                            else {
                                g_warnLeftPending = false;
                                canvas.commitSelection(0);
                                canvas.clearObjectSelection();
                                canvas.addFrameAt(0);
                                timeline.addFrameAt(0);
                                timeline.syncWithCanvas(static_cast<int>(canvas.getFrameCount()));
                                timeline.setFrame(0);
                                showMessage("Added Frame 1 at start", sf::Color::Green);
                            }
                        }
                    }
                }
                else if (isRight) {
                    if (!g_timelineRightHeld) {
                        g_timelineRightHeld = true;

                        int total = static_cast<int>(canvas.getFrameCount());
                        timeline.syncWithCanvas(total);
                        int cur = timeline.getCurrentFrame();

                        if (cur < total - 1) {
                            g_warnLeftPending = false;
                            g_warnRightPending = false;
                            canvas.commitSelection(cur);
                            canvas.clearObjectSelection();
                            timeline.setFrame(cur + 1);
                        }
                        else {
                            if (canvas.isFrameEmpty(cur)) {
                                if (g_warnRightPending && g_warnRightClock.getElapsedTime().asSeconds() <= 3.5f) {
                                    g_warnRightPending = false;
                                    canvas.commitSelection(cur);
                                    canvas.clearObjectSelection();
                                    canvas.addFrame(cur);
                                    timeline.addFrameAfter(cur);
                                    timeline.syncWithCanvas(static_cast<int>(canvas.getFrameCount()));
                                    timeline.setFrame(cur + 1);
                                    showMessage("Added Frame " + std::to_string(cur + 2), sf::Color::Green);
                                }
                                else {
                                    g_warnRightPending = true;
                                    g_warnRightClock.restart();
                                    showMessage("Current frame is empty. Press Right Arrow again to add frame.", sf::Color::Yellow);
                                }
                            }
                            else {
                                g_warnRightPending = false;
                                canvas.commitSelection(cur);
                                canvas.clearObjectSelection();
                                canvas.addFrame(cur);
                                timeline.addFrameAfter(cur);
                                timeline.syncWithCanvas(static_cast<int>(canvas.getFrameCount()));
                                timeline.setFrame(cur + 1);
                                showMessage("Added Frame " + std::to_string(cur + 2), sf::Color::Green);
                            }
                        }
                    }
                }

                if (keybindManager.isActionTriggered("time_add", event)) {
                    int cur = timeline.getCurrentFrame();
                    canvas.commitSelection(cur);
                    canvas.clearObjectSelection();
                    canvas.addFrame(cur);
                    timeline.addFrameAfter(cur);
                    timeline.syncWithCanvas(static_cast<int>(canvas.getFrameCount()));
                    timeline.setFrame(cur + 1);
                    showMessage("Added Frame " + std::to_string(cur + 2), sf::Color::Green);
                }
                if (keybindManager.isActionTriggered("time_del", event)) {
                    if (canvas.getFrameCount() > 1) {
                        int cur = timeline.getCurrentFrame();
                        canvas.commitSelection(cur);
                        canvas.clearObjectSelection();
                        canvas.deleteFrame(cur);
                        timeline.deleteFrame(cur);
                        timeline.syncWithCanvas(static_cast<int>(canvas.getFrameCount()));
                        if (timeline.getCurrentFrame() >= static_cast<int>(canvas.getFrameCount())) {
                            timeline.setFrame(static_cast<int>(canvas.getFrameCount()) - 1);
                        }
                        showMessage("Deleted Frame " + std::to_string(cur + 1), sf::Color::Cyan);
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
                if (keybindManager.isActionTriggered("edit_del_sel", event) ||
                    (event.type == sf::Event::KeyPressed && (event.key.code == sf::Keyboard::Delete || event.key.code == sf::Keyboard::BackSpace))) {
                    if (canvas.getActiveTool() == ToolType::Select || canvas.getActiveTool() == ToolType::MagicWand) {
                        int cur = timeline.getCurrentFrame();
                        canvas.saveUndoState();
                        canvas.deleteSelection(cur);
                        canvas.commitSelection(cur);
                        canvas.clearObjectSelection();
                        showMessage("Deleted Selection", sf::Color::Cyan);
                    }
                }
                if (keybindManager.isActionTriggered("edit_deselect", event)) {
                    canvas.commitSelection(timeline.getCurrentFrame());
                    canvas.setActiveTool(ToolType::Brush);
                    m_toolDock.SetActiveTool("brush");
                }

                if (event.key.control && event.key.code == sf::Keyboard::R) {
                    m_showResizeModal = true;
                    m_resizeWBuf = std::to_string(canvas.getCanvasSize().x);
                    m_resizeHBuf = std::to_string(canvas.getCanvasSize().y);
                    m_activeResizeField = 0;
                    return;
                }

                if (keybindManager.isActionTriggered("edit_copy", event)) {
                    canvas.copySelection(timeline.getCurrentFrame());
                    showMessage("Selection Copied to Clipboard", sf::Color::Green);
                }

                if (keybindManager.isActionTriggered("edit_paste", event)) {
                    if (canvas.getPixelMode()) {
                        sf::Image clipImg;
                        bool hasImg = false;
#if defined(_WIN32)
                        if (GetClipboardImage(clipImg)) {
                            hasImg = true;
                        }
#endif
                        if (!hasImg && canvas.hasGlobalClipboard() && !canvas.isClipboardVector()) {
                            clipImg = canvas.getGlobalClipboardImage();
                            hasImg = true;
                        }

                        if (hasImg) {
                            m_pendingPasteImage = clipImg;
                            m_showPasteResolutionModal = true;
                            return;
                        }
                    }

#if defined(_WIN32)
                    sf::Image clipImg;
                    if (GetClipboardImage(clipImg)) {
                        canvas.pasteImage(clipImg, timeline.getCurrentFrame());
                        m_toolDock.SetActiveTool("select");
                        showMessage("Pasted from Clipboard (Resize Handles Active)", sf::Color::Green);
                    }
                    else if (canvas.hasGlobalClipboard()) {
                        canvas.pasteGlobalClipboard(timeline.getCurrentFrame());
                        m_toolDock.SetActiveTool("select");
                        showMessage("Pasted Selection from Clipboard", sf::Color::Green);
                    }
                    else {
                        canvas.pasteSelection(timeline.getCurrentFrame());
                        m_toolDock.SetActiveTool("select");
                    }
#else
                    if (canvas.hasGlobalClipboard()) {
                        canvas.pasteGlobalClipboard(timeline.getCurrentFrame());
                        m_toolDock.SetActiveTool("select");
                        showMessage("Pasted Selection from Clipboard", sf::Color::Green);
                    }
                    else {
                        canvas.pasteSelection(timeline.getCurrentFrame());
                        m_toolDock.SetActiveTool("select");
                    }
#endif
                    }
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
        if (!timeline.isPlaying() && !keybindPanel.isVisible() && !exportModal.getIsOpen() && !newProjectModal.getIsOpen() && !g_aiReviewModal.getIsOpen()) {
            if (canvas.getActiveTool() == ToolType::Perspective) {
                m_perspectivePanel.handleEvent(event, mousePos, canvas.getCanvasSize());
            }
            if (canvas.getActiveTool() == ToolType::Text) {
                if (m_textPanel.handleEvent(event, mousePos)) return;
            }
            if (canvas.getActiveTool() == ToolType::Gradient) {
                if (m_gradientPanel.handleEvent(event, mousePos)) return;
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
            }

            if ((event.type == sf::Event::MouseButtonPressed || event.type == sf::Event::MouseButtonReleased) &&
                event.mouseButton.button == sf::Mouse::Left) {
                if (!colorPalettePanel.getIsEyedropperActive() && canvas.getDrawArea().contains(logicalMousePos)) {
                    ToolType t = canvas.getActiveTool();
                    if (t == ToolType::Brush || t == ToolType::Pencil || t == ToolType::Fill ||
                        t == ToolType::Shapes || t == ToolType::Curve || t == ToolType::FilledContour ||
                        t == ToolType::Text) {
                        sf::Color curCol = canvas.getPrimaryColor();
                        const auto& swatches = colorPalettePanel.getColorManager().getCustomSwatches();
                        if (std::find(swatches.begin(), swatches.end(), curCol) == swatches.end()) {
                            colorPalettePanel.getColorManager().addCustomSwatch(curCol);
                        }
                    }
                }
            }

            if (m_activeTool) {
                m_activeTool->HandleEvent(event, window);
            }
        }
    }
}

void UIManager::update(sf::RenderWindow& window, AppState currentState, AppSettings& settings, float dt, Canvas& canvas, Timeline& timeline) {
    WisdomUI::Theme::BeginFrame(dt);

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
                sf::FloatRect assetBrowserRect(1440.f, WisdomUI::Theme::FloatingPanelY, 390.f, 540.f);
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
                    canvas.clearCanvasImages();
                    canvas.clearObjectSelection();
                    if (projManager && projManager->loadProject(filePath, canvas, loadedFps, isPix)) {
                        activeProjectPath = filePath;
                        activeProjectName = std::filesystem::path(filePath).stem().string();
                        timeline.setFrame(std::max(0, static_cast<int>(canvas.getFrameCount()) - 1));
                        canvas.clearIsDirty();
                        canvas.clearHistory();
                        showMessage("Opened Project: " + activeProjectName, sf::Color::Green);
                    }
                }
                else if (isImage) {
                    int curFrame = static_cast<int>(timeline.getCurrentFrame());
                    if (canvas.getPixelMode()) {
                        sf::Image img;
                        if (img.loadFromFile(filePath)) {
                            m_pendingPasteImage = img;
                            m_showPasteResolutionModal = true;
                        }
                    }
                    else {
                        canvas.importImageToActiveLayer(filePath, curFrame);
                        m_toolDock.SetActiveTool("select");
                        showMessage("Placed on Canvas: " + std::filesystem::path(filePath).filename().string(), sf::Color::Green);
                    }
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

    if (keybindPanel.isVisible()) keybindPanel.updateHover(mousePos);
    if (exportModal.getIsOpen()) exportModal.updateHover(mousePos);
    if (newProjectModal.getIsOpen()) newProjectModal.updateHover(mousePos);

    if (AIManager::getInstance().isProcessingAsync()) {
        loadingSpinner.rotate(150.f * dt);
    }

    if (currentState != AppState::Welcome) m_wasOnMainMenu = false;

    if (currentState == AppState::Welcome) {
        if (!keybindPanel.isVisible()) {
            projectBrowser.updateHover(mousePos);
            updateStartMenu(dt, mousePos);
            if (m_useMinigameWelcome) {
                updateMinigame(dt, mousePos, window);
            }

            // Re-read the recent projects each time the main menu comes back into view.
            bool onMainMenu = (currentMenuState == MenuState::Main);
            if (onMainMenu && !m_wasOnMainMenu) refreshRecentProjects();
            m_wasOnMainMenu = onMainMenu;

            // Sub-screens replay their entrance animation whenever they are opened.
            if (currentMenuState != m_lastMenuState) {
                if (currentMenuState == MenuState::Projects) projectBrowser.refreshList();
                m_lastMenuState = currentMenuState;
                m_screenTime = 0.0f;
                m_lastTutorialIndex = activeTutorialIndex;
                m_tutorialTime = 0.0f;
            }
            if (activeTutorialIndex != m_lastTutorialIndex) {
                m_lastTutorialIndex = activeTutorialIndex;
                m_tutorialTime = 0.0f;
            }
            m_screenTime += dt;
            m_tutorialTime += dt;

            if (m_showKeybinds && !m_keybindsWereOpen) m_keybindTime = 0.0f;
            m_keybindsWereOpen = m_showKeybinds;
            m_keybindTime += dt;
        }
    }
    else if (currentState == AppState::Painting) {
        timeline.update(dt);

        {
            std::lock_guard<std::mutex> lock(m_pullMutex);
            if (m_hasPendingPulledImage) {
                m_hasPendingPulledImage = false;
                const sf::Image& img = m_pendingPulledImage;

                int cW = canvas.getCanvasSize().x;
                int cH = canvas.getCanvasSize().y;
                int iW = static_cast<int>(img.getSize().x);
                int iH = static_cast<int>(img.getSize().y);

                bool isAnim = (iW > cW && (iW % cW == 0) && iH == cH);
                if (!isAnim && (iW >= 2 * iH && (iW % iH == 0))) {
                    isAnim = true;
                    cW = iH;
                    cH = iH;
                }

                if (isAnim) {
                    int frameCount = iW / cW;
                    while (static_cast<int>(canvas.getFrameCount()) < frameCount) {
                        canvas.addFrame(-1);
                        timeline.addFrameAfter(timeline.getFrameCount() - 1);
                    }
                    while (static_cast<int>(canvas.getFrameCount()) > frameCount) {
                        int last = static_cast<int>(canvas.getFrameCount()) - 1;
                        canvas.deleteFrame(last);
                        timeline.deleteFrame(last);
                    }

                    for (int f = 0; f < frameCount; ++f) {
                        sf::Image frameImg;
                        frameImg.create(cW, cH);
                        frameImg.copy(img, 0, 0, sf::IntRect(f * cW, 0, cW, cH));
                        canvas.replaceFrameImage(f, frameImg);
                    }
                    timeline.setFrame(0);
                    canvas.clearIsDirty();
                    canvas.clearHistory();
                    showMessage("Pulled Animation (" + std::to_string(frameCount) + " frames) from GitImg!", sf::Color::Green);
                }
                else {
                    int cur = timeline.getCurrentFrame();
                    canvas.replaceFrameImage(cur, img);
                    canvas.clearIsDirty();
                    canvas.clearHistory();
                    showMessage("Pulled Frame into Frame " + std::to_string(cur + 1) + "!", sf::Color::Green);
                }
            }
        }

        bool rightDockOpen = (m_activeRightTab != RightTabMode::None);
        auto regions = m_workspaceLayout.Update(rightDockOpen, m_showTimeline);

        bool symActive = canvas.getSymmetryManager().enabled && canvas.getSymmetryManager().visible &&
            (std::hypot(canvas.getSymmetryManager().endPoint.x - canvas.getSymmetryManager().startPoint.x,
                canvas.getSymmetryManager().endPoint.y - canvas.getSymmetryManager().startPoint.y) > 2.0f);

        m_topBar.SetBounds(regions.topBar);
        m_topBar.SetProjectName(activeProjectName, canvas.getIsDirty());
        m_topBar.Update(dt, mousePos);
        m_topBar.SetSymmetryState(
            symActive,
            [this, &canvas]() {
                canvas.clearSymmetry();
                if (canvas.getActiveTool() == ToolType::Symmetry) {
                    canvas.setActiveTool(ToolType::None);
                    m_toolDock.SetActiveTool("");
                }
                showMessage("Symmetry Deactivated", sf::Color::Cyan);
            }
        );
        m_topBar.SetGridControls(
            true,
            canvas.isCustomGridEnabled(),
            canvas.getCustomGridSize(),
            canvas.getCustomGridColor(),
            [this, &canvas](bool active) {
                canvas.setCustomGridEnabled(active);
                showMessage(active ? "Grid: ON" : "Grid: OFF", sf::Color::Cyan);
            },
            [this, &canvas](int newSize) {
                canvas.setCustomGridSize(newSize);
                showMessage("Grid Size: " + std::to_string(newSize) + "px", sf::Color::Yellow);
            },
            [this]() {
                g_selectingGridColor = true;
                g_selectingOutlineColor = false;
                m_activeRightTab = RightTabMode::Palette;
                showMessage("Pick Grid Color from Palette", sf::Color(100, 200, 255));
            }
        );
        if (m_activeRightTab != RightTabMode::Palette) {
            g_selectingGridColor = false;
        }
        else if (g_selectingGridColor) {
            canvas.setCustomGridColor(canvas.getPrimaryColor());
        }
        

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
        else if (canvas.getActiveTool() == ToolType::Grid) toolName = "Grid";

        float curSize = canvas.getPixelMode() ? static_cast<float>(canvas.getPixelBrushSize()) : canvas.getBrushSize();
        int curZ = canvas.getSelectionZOrder(timeline.getCurrentFrame());
        int maxZ = canvas.getMaxZOrder(timeline.getCurrentFrame());
        bool hasSel = canvas.getSelection().isActive();
        m_toolOptionsBar.SyncState(toolName, curSize, canvas.getPixelMode(), canvas.isPixelPerfectEnabled(), canvas.getStabilizer(), curZ, maxZ, canvas.getPixelBrushShape(), hasSel, false);
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
            sf::FloatRect headerBounds(0.0f, regions.timeline.top, 1920.0f, WisdomUI::Theme::TimelineHeaderHeight);
            m_timelineHeader.SetBounds(headerBounds);
            if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
                g_timelineLeftHeld = false;
            }
            if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
                g_timelineRightHeld = false;
            }

            m_timelineHeader.SyncState(timeline.isPlaying(), timeline.getCurrentFrame(), static_cast<int>(canvas.getFrameCount()), timeline.getFps(), canvas.isOnionSkinEnabled());
            m_timelineHeader.Update(dt, mousePos);
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
    }
}

void UIManager::draw(sf::RenderWindow& window, AppState currentState, Canvas& canvas, AIHelper& aiHelper, Timeline& timeline) {
    window.setView(WisdomUI::WorkspaceLayout::GetLetterboxView(window.getSize()));

    bgSprite.setPosition(0.f, 0.f);
    bgSprite.setScale(1920.f / bgTexture.getSize().x, 1080.f / bgTexture.getSize().y);
    bgSprite.setColor(sf::Color::White);
    window.draw(bgSprite);

    if (currentState == AppState::Welcome) {
        // The keybinds screen covers whatever menu it was opened from.
        if (m_showKeybinds) {
            drawKeybindModal(window);
        }
        else if (currentMenuState == MenuState::Main) drawMainMenu(window);
        else if (currentMenuState == MenuState::Projects) {
            size_t count = projectBrowser.getProjectCount();
            std::string subtitle = (count == 1) ? "1 PROJECT IN YOUR VAULT" : (std::to_string(count) + " PROJECTS IN YOUR VAULT");
            drawSubScreenHeader(window, "PROJECTS", subtitle, m_screenTime);
            projectBrowser.draw(window, m_screenTime);
        }
        else if (currentMenuState == MenuState::Settings) drawSettingsMenu(window);
        else if (currentMenuState == MenuState::Tutorials) drawTutorialsMenu(window);
        else if (currentMenuState == MenuState::Credits) drawCreditsMenu(window);
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

            sf::FloatRect trayBounds(12.0f, timelineY + WisdomUI::Theme::TimelineHeaderHeight + 4.0f, 1920.0f - 24.0f, WisdomUI::Theme::TimelineHeight - WisdomUI::Theme::TimelineHeaderHeight - 12.0f);
            sf::RectangleShape trayBg(sf::Vector2f(trayBounds.width, trayBounds.height));
            trayBg.setPosition(trayBounds.left, trayBounds.top);
            trayBg.setFillColor(WisdomUI::Theme::SunsetDeepDark);
            trayBg.setOutlineThickness(1.0f);
            trayBg.setOutlineColor(WisdomUI::Theme::SunsetPlum);
            window.draw(trayBg);

            m_timelineHeader.Render(window);

            float cardW = 90.0f;
            float cardH = 120.0f;
            float cardSpacing = 12.0f;
            int totalFrames = static_cast<int>(canvas.getFrameCount());
            int curFrame = timeline.getCurrentFrame();

            float maxScroll = std::max(0.0f, (24.0f + totalFrames * (cardW + cardSpacing)) - trayBounds.width);

            static int s_lastTrackedFrame = -1;
            if (s_lastTrackedFrame != curFrame && !g_isDraggingTimeline) {
                s_lastTrackedFrame = curFrame;
                float curCardLeft = 12.0f + curFrame * (cardW + cardSpacing) - g_timelineScrollX;
                float curCardRight = curCardLeft + cardW;
                if (curCardLeft < 12.0f) {
                    g_timelineScrollX = curFrame * (cardW + cardSpacing);
                }
                else if (curCardRight > trayBounds.width - 12.0f) {
                    g_timelineScrollX = (curFrame + 1) * (cardW + cardSpacing) + 24.0f - trayBounds.width;
                }
            }
            g_timelineScrollX = std::clamp(g_timelineScrollX, 0.0f, maxScroll);

            sf::View defaultView = window.getView();
            sf::FloatRect vp = defaultView.getViewport();
            sf::View trayView(sf::FloatRect(trayBounds.left, trayBounds.top, trayBounds.width, trayBounds.height));
            trayView.setViewport(sf::FloatRect(
                vp.left + (trayBounds.left / 1920.0f) * vp.width,
                vp.top + (trayBounds.top / 1080.0f) * vp.height,
                (trayBounds.width / 1920.0f) * vp.width,
                (trayBounds.height / 1080.0f) * vp.height
            ));
            window.setView(trayView);

            float startX = trayBounds.left + 12.0f - g_timelineScrollX;
            float cardY = timelineY + WisdomUI::Theme::TimelineHeaderHeight + 14.0f;

            for (int i = 0; i < totalFrames; ++i) {
                if (startX + cardW >= trayBounds.left && startX <= trayBounds.left + trayBounds.width) {
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

                    canvas.drawFrameThumbnail(window, i, sf::FloatRect(boxX + 2.0f, boxY + 2.0f, boxW - 4.0f, boxH - 4.0f));

                    WisdomUI::Theme::DrawCrispText(window, font, std::to_string(i + 1), 13, startX + cardW / 2.0f, cardY + cardH - 14.0f, isSelected ? WisdomUI::Theme::SunsetAmber : WisdomUI::Theme::TextSecondary, sf::Color::Transparent, true, true);

                    if (isSelected) {
                        sf::RectangleShape selTag(sf::Vector2f(cardW - 12.0f, 2.0f));
                        selTag.setPosition(startX + 6.0f, cardY + 3.0f);
                        selTag.setFillColor(WisdomUI::Theme::SunsetCoral);
                        window.draw(selTag);
                    }
                }

                startX += cardW + cardSpacing;
            }

            window.setView(defaultView);

            if (maxScroll > 0.0f) {
                float trackH = 4.0f;
                float trackY = trayBounds.top + trayBounds.height - 7.0f;
                float trackW = trayBounds.width - 24.0f;
                float trackX = trayBounds.left + 12.0f;

                sf::RectangleShape track(sf::Vector2f(trackW, trackH));
                track.setPosition(trackX, trackY);
                track.setFillColor(sf::Color(15, 8, 20, 180));
                window.draw(track);

                float thumbRatio = trayBounds.width / (24.0f + totalFrames * (cardW + cardSpacing));
                float thumbW = std::clamp(trackW * thumbRatio, 40.0f, trackW);
                float thumbX = trackX + (g_timelineScrollX / maxScroll) * (trackW - thumbW);

                sf::RectangleShape thumb(sf::Vector2f(thumbW, trackH));
                thumb.setPosition(thumbX, trackY);
                thumb.setFillColor(g_isDraggingTimeline ? WisdomUI::Theme::SunsetGold : WisdomUI::Theme::SunsetAmber);
                window.draw(thumb);
            }
        }

        m_toolOptionsBar.Render(window);
        m_toolDock.Render(window);
        m_rightDockTabs.Render(window);
        m_statusBar.Render(window);
        m_topBar.Render(window);

        g_aiPanel.draw(window);
        g_aiReviewModal.draw(window);

        if (showingText) window.draw(uiText);
        if (isTypingPrompt) {
            window.draw(promptBox);
            window.draw(promptDisplay);
        }

        keybindPanel.draw(window);
        if (exportModal.getIsOpen()) exportModal.draw(window);
        if (newProjectModal.getIsOpen()) newProjectModal.draw(window);

        if (AIManager::getInstance().isProcessingAsync()) {
            window.draw(loadingOverlay);
            window.draw(loadingBox);
            window.draw(loadingText);
            window.draw(loadingSpinner);
            window.draw(loadingCancelBtn);
            window.draw(loadingCancelText);
        }

        if (showUnsavedWarning) {
            drawUnsavedWarning(window);
        }

        if (m_showEscapeMenu) {
            drawEscapeMenu(window, canvas, timeline);
        }

        if (m_showResizeModal) {
            drawResizeModal(window, canvas);
        }

        if (m_showPasteResolutionModal) {
            drawPasteResolutionModal(window, canvas);
        }

        m_toolDock.RenderTooltip(window);
    }
    drawHandCamWidget(window);
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

void UIManager::pushSpriteSheetToGitImg(Canvas& canvas, bool opaqueBg) {
    if (m_isPushingGitImg) return;

    std::string user, pass;
    if (!m_gitImgClient.isAuthenticated()) {
        if (!GitImgClient::loadSavedCredentials(user, pass)) {
            if (!GitImgClient::promptCredentials(user, pass)) {
                return;
            }
        }
    }

    std::string commitMsg;
    if (!GitImgClient::promptCommitMessage(commitMsg)) {
        return;
    }

    m_isPushingGitImg = true;
    showMessage("Exporting sprite strip and pushing to GitImg...", sf::Color::Yellow);

    sf::Image stripImg = ExportManager::createHorizontalSpriteStrip(canvas, !opaqueBg);

    std::string repo = activeProjectName;
    std::string filename = "Animation_" + activeProjectName + ".png";

    auto doPush = [this, stripImg, repo, filename, commitMsg]() {
        m_gitImgClient.pushAsync(stripImg, repo, filename, commitMsg, [this](bool success) {
            m_isPushingGitImg = false;
            if (success) {
                showMessage("Successfully pushed animation strip to GitImg!", sf::Color::Green);
            }
            else {
                showMessage("Failed to push animation strip to GitImg", sf::Color::Red);
            }
            });
        };

    if (!m_gitImgClient.isAuthenticated()) {
        std::thread([this, user, pass, doPush]() {
            if (m_gitImgClient.login(user, pass)) {
                GitImgClient::saveCredentials(user, pass);
                doPush();
            }
            else {
                GitImgClient::clearSavedCredentials();
                m_isPushingGitImg = false;
                showMessage("GitImg Authentication Failed", sf::Color::Red);
            }
            }).detach();
    }
    else {
        doPush();
    }
}

void UIManager::pullCommitToCanvas(const std::string& commitHash, Canvas& canvas, Timeline& timeline) {
    showMessage("Pulling commit from GitImg...", sf::Color::Yellow);

    std::thread([this, commitHash]() {
        sf::Image img;
        bool ok = GitImgClient::downloadCommitImage(commitHash, img, false);
        if (!ok) {
            ok = GitImgClient::downloadCommitImage(commitHash, img, true);
        }

        if (ok && img.getSize().x > 0 && img.getSize().y > 0) {
            std::lock_guard<std::mutex> lock(m_pullMutex);
            m_pendingPulledImage = img;
            m_hasPendingPulledImage = true;
        }
        else {
            showMessage("Failed to pull artwork from GitImg", sf::Color::Red);
        }
        }).detach();
}

void UIManager::pushToGitImg(Canvas& canvas, int frameIndex, bool opaqueBg) {
    if (m_isPushingGitImg) return;

    std::string user, pass;
    if (!m_gitImgClient.isAuthenticated()) {
        if (!GitImgClient::loadSavedCredentials(user, pass)) {
            if (!GitImgClient::promptCredentials(user, pass)) {
                return;
            }
        }
    }

    std::string commitMsg;
    if (!GitImgClient::promptCommitMessage(commitMsg)) {
        return;
    }

    m_isPushingGitImg = true;
    showMessage("Exporting RAM PNG and pushing to GitImg...", sf::Color::Yellow);

    sf::Image rawImg = ExportManager::flattenFrame(canvas, frameIndex);
    sf::IntRect fullBounds(0, 0, rawImg.getSize().x, rawImg.getSize().y);
    sf::Image img = ExportManager::applyCropAndBackground(rawImg, fullBounds, !opaqueBg);

    std::string repo = activeProjectName;
    std::string filename = activeProjectName + ".png";

    auto doPush = [this, img, repo, filename, commitMsg]() {
        m_gitImgClient.pushAsync(img, repo, filename, commitMsg, [this](bool success) {
            m_isPushingGitImg = false;
            if (success) {
                showMessage("Successfully pushed to GitImg!", sf::Color::Green);
            }
            else {
                showMessage("Failed to push to GitImg", sf::Color::Red);
            }
            });
        };

    if (!m_gitImgClient.isAuthenticated()) {
        std::thread([this, user, pass, doPush]() {
            if (m_gitImgClient.login(user, pass)) {
                GitImgClient::saveCredentials(user, pass);
                doPush();
            }
            else {
                GitImgClient::clearSavedCredentials();
                m_isPushingGitImg = false;
                showMessage("GitImg Authentication Failed", sf::Color::Red);
            }
            }).detach();
    }
    else {
        doPush();
    }
}

bool UIManager::handleHandCamWidgetEvents(const sf::Event& event) {
    if (!HandTracker::getInstance().isRunning()) return false;

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f mPos(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
        if (m_camToggleBtnBounds.contains(mPos)) {
            m_camMinimized = !m_camMinimized;
            return true;
        }
        if (!m_camMinimized && m_camResizeBounds.contains(mPos)) {
            m_isResizingCam = true;
            m_resizeStartPos = mPos;
            m_resizeStartSize = m_camSize;
            return true;
        }
        if (m_camHeaderBounds.contains(mPos)) {
            m_isDraggingCam = true;
            m_camDragOffset = mPos - m_camPos;
            return true;
        }
        if (m_camWidgetBounds.contains(mPos)) {
            return true;
        }
    }
    else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        if (m_isDraggingCam) {
            m_isDraggingCam = false;
            return true;
        }
        if (m_isResizingCam) {
            m_isResizingCam = false;
            return true;
        }
    }
    else if (event.type == sf::Event::MouseMoved) {
        sf::Vector2f mPos(static_cast<float>(event.mouseMove.x), static_cast<float>(event.mouseMove.y));
        if (m_isDraggingCam) {
            m_camPos = mPos - m_camDragOffset;
            if (m_camPos.x < 0.0f) m_camPos.x = 0.0f;
            if (m_camPos.y < 0.0f) m_camPos.y = 0.0f;
            return true;
        }
        if (m_isResizingCam) {
            float newW = m_resizeStartSize.x + (mPos.x - m_resizeStartPos.x);
            newW = std::max(130.0f, std::min(480.0f, newW));
            m_camSize.x = newW;
            m_camSize.y = newW * 0.75f;
            return true;
        }
    }
    return false;
}

void UIManager::drawHandCamWidget(sf::RenderWindow& window) {
    if (!HandTracker::getInstance().isRunning()) return;

    if (HandTracker::getInstance().updateTexture(m_camTexture)) {
        m_camTexture.setSmooth(true);
    }

    float winW = static_cast<float>(window.getSize().x);
    float winH = static_cast<float>(window.getSize().y);

    if (m_camMinimized) {
        float w = 70.0f;
        float h = 22.0f;

        if (m_camPos.x < 0.0f) {
            m_camPos = sf::Vector2f(winW - w - 20.0f, winH - h - 20.0f);
        }

        m_camWidgetBounds = sf::FloatRect(m_camPos.x, m_camPos.y, w, h);
        m_camToggleBtnBounds = m_camWidgetBounds;
        m_camHeaderBounds = m_camWidgetBounds;
        m_camResizeBounds = sf::FloatRect(0, 0, 0, 0);

        sf::RectangleShape bar(sf::Vector2f(w, h));
        bar.setPosition(m_camPos);
        bar.setFillColor(sf::Color(35, 28, 48, 230));
        bar.setOutlineColor(sf::Color(255, 120, 80));
        bar.setOutlineThickness(1.0f);
        window.draw(bar);

        sf::Text txt("Cam +", font, 11);
        txt.setPosition(m_camPos.x + 12.0f, m_camPos.y + 3.0f);
        txt.setFillColor(sf::Color::White);
        window.draw(txt);
        return;
    }

    float headerH = 22.0f;
    float w = m_camSize.x;
    float camH = m_camSize.y;
    float totalH = headerH + camH;

    if (m_camPos.x < 0.0f) {
        m_camPos = sf::Vector2f(winW - w - 20.0f, winH - totalH - 20.0f);
    }

    m_camWidgetBounds = sf::FloatRect(m_camPos.x, m_camPos.y, w, totalH);
    m_camHeaderBounds = sf::FloatRect(m_camPos.x, m_camPos.y, w - 26.0f, headerH);
    m_camToggleBtnBounds = sf::FloatRect(m_camPos.x + w - 24.0f, m_camPos.y + 2.0f, 20.0f, 18.0f);
    m_camResizeBounds = sf::FloatRect(m_camPos.x + w - 14.0f, m_camPos.y + totalH - 14.0f, 14.0f, 14.0f);

    sf::RectangleShape bg(sf::Vector2f(w, totalH));
    bg.setPosition(m_camPos);
    bg.setFillColor(sf::Color(25, 20, 35, 245));
    bg.setOutlineColor(sf::Color(255, 120, 80));
    bg.setOutlineThickness(1.0f);
    window.draw(bg);

    sf::RectangleShape header(sf::Vector2f(w, headerH));
    header.setPosition(m_camPos);
    header.setFillColor(sf::Color(45, 30, 60, 255));
    window.draw(header);

    sf::Text title("Hand Tracker", font, 11);
    title.setPosition(m_camPos.x + 8.0f, m_camPos.y + 3.0f);
    title.setFillColor(sf::Color(255, 200, 150));
    window.draw(title);

    sf::RectangleShape minBtn(sf::Vector2f(m_camToggleBtnBounds.width, m_camToggleBtnBounds.height));
    minBtn.setPosition(m_camToggleBtnBounds.left, m_camToggleBtnBounds.top);
    minBtn.setFillColor(sf::Color(70, 45, 90));
    window.draw(minBtn);

    sf::Text minTxt("-", font, 12);
    minTxt.setPosition(m_camToggleBtnBounds.left + 6.0f, m_camToggleBtnBounds.top - 1.0f);
    minTxt.setFillColor(sf::Color::White);
    window.draw(minTxt);

    if (m_camTexture.getSize().x > 0) {
        m_camSprite.setTexture(m_camTexture, true);
        m_camSprite.setPosition(m_camPos.x, m_camPos.y + headerH);
        m_camSprite.setScale(w / static_cast<float>(m_camTexture.getSize().x), camH / static_cast<float>(m_camTexture.getSize().y));
        window.draw(m_camSprite);
    }

    sf::VertexArray grip(sf::Lines, 6);
    float rx = m_camPos.x + w;
    float ry = m_camPos.y + totalH;

    grip[0].position = sf::Vector2f(rx - 3.0f, ry - 11.0f);
    grip[1].position = sf::Vector2f(rx - 11.0f, ry - 3.0f);
    grip[2].position = sf::Vector2f(rx - 3.0f, ry - 7.0f);
    grip[3].position = sf::Vector2f(rx - 7.0f, ry - 3.0f);
    grip[4].position = sf::Vector2f(rx - 3.0f, ry - 3.0f);
    grip[5].position = sf::Vector2f(rx - 3.0f, ry - 3.0f);

    for (size_t i = 0; i < 6; ++i) {
        grip[i].color = sf::Color(255, 160, 100);
    }
    window.draw(grip);
}

