#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "UIManager.h"
#include "../core/NativeDialogs.h"
#include "../core/ExportManager.h"
#include "../ai/AIManager.h"
#include "../ui/AIPanel.h"
#include "../ui/AIReviewModal.h"
#include "../ui/LoadingScene.h"
#include "../ui/ChatPanel.h"
#include "json.hpp"
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
static LoadingScene g_loadingScene;
static bool g_loadingWasActive = false;
static float g_loadingStatusTimer = 0.f;

static ChatPanel g_chatPanel;
// True while a chat message is with Claude. Unlike the other assistant requests the app stays usable
// meanwhile, so the loading scene is not shown for it.
static bool g_chatPending = false;
// The assistant folder the pending message was sent from, and the one whose conversation the panel shows
static std::string g_chatSentDir;
static std::string g_chatShownDir;
static std::string g_chatStatus;
static float g_chatStatusTimer = 0.f;
// Claude's drawing works one pixel at a time in Python and takes longer the more there is to draw, so it
// draws at most this size pixel for pixel; anything larger it draws at the size the chat's detail setting
// says, and the app scales that up
static const int kChatMaxCanvas = 256;

// An area dragged out on the canvas for the chat, in canvas pixels. Unlike a selection it can cover empty
// canvas, and it stays for the messages after until the artist clears it.
static bool g_chatMarking = false;
static bool g_chatMarkDragging = false;
static bool g_chatMarkActive = false;
static sf::Vector2f g_chatMarkStart;
static sf::Vector2f g_chatMarkEnd;

static sf::IntRect chatMarkRect(const Canvas& canvas) {
    sf::Vector2u size = canvas.getCanvasSize();
    int left = std::clamp(static_cast<int>(std::floor(std::min(g_chatMarkStart.x, g_chatMarkEnd.x))), 0, static_cast<int>(size.x));
    int top = std::clamp(static_cast<int>(std::floor(std::min(g_chatMarkStart.y, g_chatMarkEnd.y))), 0, static_cast<int>(size.y));
    int right = std::clamp(static_cast<int>(std::ceil(std::max(g_chatMarkStart.x, g_chatMarkEnd.x))), 0, static_cast<int>(size.x));
    int bottom = std::clamp(static_cast<int>(std::ceil(std::max(g_chatMarkStart.y, g_chatMarkEnd.y))), 0, static_cast<int>(size.y));
    return sf::IntRect(left, top, right - left, bottom - top);
}

// The part of the canvas a chat message is about: the marked area, else the selection, else the whole canvas.
// Claude draws it shrunk by `scale`, so each pixel it draws becomes a scale x scale block.
struct ChatArea {
    sf::IntRect rect;
    int scale = 1;
    bool selected = false;
    sf::Vector2u workSize() const { return { static_cast<unsigned>((rect.width + scale - 1) / scale), static_cast<unsigned>((rect.height + scale - 1) / scale) }; }
};
static ChatArea g_chatSentArea;

static ChatArea chatAreaFor(const Canvas& canvas) {
    ChatArea area;
    sf::Vector2u size = canvas.getCanvasSize();
    area.rect = sf::IntRect(0, 0, static_cast<int>(size.x), static_cast<int>(size.y));
    sf::IntRect mark = chatMarkRect(canvas);
    if (g_chatMarkActive && mark.width >= 4 && mark.height >= 4) {
        area.rect = mark;
        area.selected = true;
    }
    else if (canvas.getSelection().isActive()) {
        sf::FloatRect box = canvas.getSelection().getBoundingBox();
        int left = static_cast<int>(std::floor(box.left));
        int top = static_cast<int>(std::floor(box.top));
        sf::IntRect sel(left, top, static_cast<int>(std::ceil(box.left + box.width)) - left, static_cast<int>(std::ceil(box.top + box.height)) - top);
        sf::IntRect inside;
        // A selection only a few pixels across is more likely a stray click than an area to draw in
        if (sel.intersects(area.rect, inside) && inside.width >= 4 && inside.height >= 4) {
            area.rect = inside;
            area.selected = true;
        }
    }
    int longest = std::max(area.rect.width, area.rect.height);
    int side = g_chatPanel.detailSide();
    if (longest > kChatMaxCanvas && side > 0) area.scale = (longest + side - 1) / side;
    return area;
}

// The area cut out of a full frame at Claude's size: each block's middle pixel stands for the block, so a
// drawing scaled up by growIntoArea comes back out exactly
static sf::Image shrinkArea(const sf::Image& full, const ChatArea& area) {
    sf::Vector2u work = area.workSize();
    sf::Image out;
    out.create(work.x, work.y, sf::Color::Transparent);
    for (unsigned y = 0; y < work.y; ++y) {
        for (unsigned x = 0; x < work.x; ++x) {
            int sx = area.rect.left + std::min(static_cast<int>(x) * area.scale + area.scale / 2, area.rect.width - 1);
            int sy = area.rect.top + std::min(static_cast<int>(y) * area.scale + area.scale / 2, area.rect.height - 1);
            out.setPixel(x, y, full.getPixel(sx, sy));
        }
    }
    return out;
}

// Claude's drawing scaled up with hard edges over the area of a full frame; the rest of the frame stays
static void growIntoArea(sf::Image& full, const sf::Image& drawn, const ChatArea& area) {
    sf::Vector2u work = drawn.getSize();
    for (int y = 0; y < area.rect.height; ++y) {
        for (int x = 0; x < area.rect.width; ++x) {
            unsigned sx = std::min(static_cast<unsigned>(x / area.scale), work.x - 1);
            unsigned sy = std::min(static_cast<unsigned>(y / area.scale), work.y - 1);
            full.setPixel(area.rect.left + x, area.rect.top + y, drawn.getPixel(sx, sy));
        }
    }
}

// Says in the chat header what Claude will draw, when it is not simply the whole canvas
static std::string chatAreaLabel(const ChatArea& area) {
    sf::Vector2u work = area.workSize();
    std::string label = area.selected ? "area " + std::to_string(area.rect.width) + "x" + std::to_string(area.rect.height) : "";
    if (area.scale > 1) label += std::string(label.empty() ? "" : ", ") + "drawn " + std::to_string(work.x) + "x" + std::to_string(work.y) + " x" + std::to_string(area.scale);
    return label;
}

// Each project keeps its conversation, Claude Code session and Claude's drawing code in <project>/assistant
static std::string chatDirFor(const std::string& projectPath) {
    return projectPath.empty() ? "projects/_unsaved_assistant" : projectPath + "/assistant";
}

// The layer Claude draws on, found by name since the artist can reorder layers; -1 when there is none yet
static int findClaudeLayer(const Canvas& canvas) {
    const Frame* f = canvas.getFrameReadOnly(0);
    if (!f) return -1;
    for (int i = 0; i < static_cast<int>(f->layers.size()); ++i) {
        if (f->layers[i].name == "Claude") return i;
    }
    return -1;
}

// When the artist had a selection, only that area is sent to the assistant; these remember
// where it came from so the answer can be put back into the full frame for review.
static bool g_assistHasRegion = false;
static sf::IntRect g_assistRegion;
static sf::Image g_assistFullImage;

// What the assistant was asked earlier in this project and what became of each result, so a
// follow-up ("bigger", "try again") makes sense to it.
struct AssistTurn { std::string request; std::string outcome; };
static std::vector<AssistTurn> g_assistTurns;
static std::string g_assistTurnsProject;
static std::string g_assistPendingRequest;
static sf::IntRect g_assistPendingArea;
static sf::Image g_assistLastResult;
// Each option as the bridge returned it (the selected area only), in the order the review shows them
static std::vector<sf::Image> g_assistOptionsRaw;
static sf::IntRect g_assistLastArea;
static bool g_assistLastDiscarded = false;
// The request box was opened to ask for the next animation frame / the request in flight is one
static bool g_assistFramePrompt = false;
static bool g_assistFrameMode = false;
// How many next frames the request in flight asked for
static int g_assistFrameTotal = 1;
// The request as it was typed, for Try Again in the review window
static std::string g_assistLastHint;
// Set by Try Again and by a follow-up typed in the review window: the next request works on the same area as the last
static bool g_assistRedo = false;
static const size_t kAssistMaxTurns = 6;
static const int kAssistMaxLayers = 24;
// On a large canvas the bridge works on a part of it and cuts that part out of the layer and frame renders
// itself. Above this size they are left out, since saving them all would stall the app.
static const int kAssistContextMaxPixels = 1024 * 1024;

// Where this project's requests are remembered between runs; empty while the project has no folder yet
static std::string g_assistMemoryFile;

static void forgetAssistTurns() {
    g_assistTurns.clear();
    g_assistLastDiscarded = false;
}

static void saveAssistTurns() {
    if (g_assistMemoryFile.empty()) return;
    std::error_code ec;
    if (!std::filesystem::is_directory(std::filesystem::path(g_assistMemoryFile).parent_path(), ec)) return;
    if (g_assistTurns.empty()) {
        std::filesystem::remove(g_assistMemoryFile, ec);
        return;
    }
    // Two lines per request: what was asked, then what became of the result
    std::ofstream out(g_assistMemoryFile, std::ios::trunc);
    for (const auto& turn : g_assistTurns) {
        std::string request = turn.request;
        std::replace(request.begin(), request.end(), '\n', ' ');
        out << request << "\n" << turn.outcome << "\n";
    }
}

// Call before reading or adding to the memory: switches it over when another project has been opened
static void syncAssistProject(const std::string& projectPath, const std::string& projectName) {
    const std::string key = projectPath + "|" + projectName;
    if (g_assistTurnsProject == key) return;
    forgetAssistTurns();
    g_assistTurnsProject = key;
    g_assistMemoryFile = projectPath.empty() ? "" : projectPath + "/assistant_memory.txt";

    std::ifstream in(g_assistMemoryFile);
    std::string request, outcome;
    while (!g_assistMemoryFile.empty() && std::getline(in, request) && std::getline(in, outcome)) {
        if (!request.empty() && request.back() == '\r') request.pop_back();
        if (!outcome.empty() && outcome.back() == '\r') outcome.pop_back();
        g_assistTurns.push_back({ request, outcome });
    }
    while (g_assistTurns.size() > kAssistMaxTurns) g_assistTurns.erase(g_assistTurns.begin());
}

// The request box: text as typed (any language), and where the caret sits in it
static sf::String g_promptText;
static size_t g_promptCaret = 0;
static const size_t kPromptMaxLength = 400;

static void refreshPrompt(sf::Text& display, std::string& utf8) {
    g_promptCaret = std::min(g_promptCaret, g_promptText.getSize());
    sf::String shown = g_promptText;
    shown.insert(g_promptCaret, "|");
    // When the text outgrows the box, the start scrolls out of view so the caret stays visible
    size_t skip = 0;
    display.setString(sf::String("> ") + shown);
    while (display.getLocalBounds().width > 570.f && skip < g_promptCaret) {
        ++skip;
        display.setString(sf::String("> ...") + shown.substring(skip));
    }
    auto bytes = g_promptText.toUtf8();
    utf8.assign(bytes.begin(), bytes.end());
}

// An area dragged out on the canvas while the request box is open, in canvas pixels. Unlike a
// selection it can cover empty canvas, which is how "draw a tree here" is said.
static bool g_assistMarkDragging = false;
static bool g_assistMarkActive = false;
static sf::Vector2f g_assistMarkStart;
static sf::Vector2f g_assistMarkEnd;

static sf::Vector2f toCanvasPixel(const Canvas& canvas, sf::Vector2f logicalPos) {
    sf::FloatRect area = canvas.getDrawArea();
    sf::Vector2u size = canvas.getCanvasSize();
    return sf::Vector2f((logicalPos.x - area.left) * static_cast<float>(size.x) / area.width, (logicalPos.y - area.top) * static_cast<float>(size.y) / area.height);
}

static sf::IntRect assistMarkRect(const Canvas& canvas) {
    sf::Vector2u size = canvas.getCanvasSize();
    int left = std::clamp(static_cast<int>(std::floor(std::min(g_assistMarkStart.x, g_assistMarkEnd.x))), 0, static_cast<int>(size.x));
    int top = std::clamp(static_cast<int>(std::floor(std::min(g_assistMarkStart.y, g_assistMarkEnd.y))), 0, static_cast<int>(size.y));
    int right = std::clamp(static_cast<int>(std::ceil(std::max(g_assistMarkStart.x, g_assistMarkEnd.x))), 0, static_cast<int>(size.x));
    int bottom = std::clamp(static_cast<int>(std::ceil(std::max(g_assistMarkStart.y, g_assistMarkEnd.y))), 0, static_cast<int>(size.y));
    return sf::IntRect(left, top, right - left, bottom - top);
}

static void setAssistOutcome(const std::string& outcome, bool discarded) {
    if (!g_assistTurns.empty()) g_assistTurns.back().outcome = outcome;
    saveAssistTurns();
    // The option that was on screen is the one a follow-up will be about
    int shown = g_aiReviewModal.getOptionIndex();
    if (shown >= 0 && shown < static_cast<int>(g_assistOptionsRaw.size())) g_assistLastResult = g_assistOptionsRaw[shown];
    g_assistLastDiscarded = discarded;
}

static std::string jsonQuote(const std::string& text) {
    std::string out = "\"";
    for (unsigned char c : text) {
        if (c == '"' || c == '\\') { out += '\\'; out += static_cast<char>(c); }
        else if (c < 0x20) out += ' ';
        else out += static_cast<char>(c);
    }
    return out + "\"";
}

// Writes temp_ai_context.json for scripts/claude_art.py: the frame's layers one by one (cut to the
// area being sent) and the earlier requests. The flat canvas alone cannot show either.
static void writeAssistContext(Canvas& canvas, int frameIndex, const sf::IntRect& area, const std::vector<sf::Color>* allowedColors, int optionCount, bool nextFrame, int newFrames, float fps) {
    std::ostringstream json;
    json << "{\"options\":" << optionCount << ",\"output\":\"" << (g_aiPanel.getAssistChangesOnly() ? "changes" : "whole") << "\","
        << "\"review\":" << (g_aiPanel.getAssistSelfCheck() ? "true" : "false") << ",";

    // In an animation the frames around this one show what the subject looks like in motion
    int frameCount = static_cast<int>(canvas.getFrameCount());
    json << "\"mode\":\"" << (nextFrame ? "next_frame" : "edit") << "\",\"new_frames\":" << newFrames << ",\"frame\":" << (frameIndex + 1)
        << ",\"frame_count\":" << frameCount << ",\"fps\":" << fps << ",\"frames\":[";
    if (frameCount > 1 && area.width * area.height <= kAssistContextMaxPixels) {
        bool firstFrame = true;
        for (int i = std::max(0, frameIndex - 3); i <= std::min(frameCount - 1, frameIndex + 1); ++i) {
            if (i == frameIndex) continue;
            sf::Image whole = canvas.flattenFrameToImage(i);
            sf::Image cut;
            cut.create(static_cast<unsigned int>(area.width), static_cast<unsigned int>(area.height), sf::Color::Transparent);
            cut.copy(whole, 0, 0, area, false);
            std::string file = "temp_ai_frame_" + std::to_string(i + 1) + ".png";
            if (!cut.saveToFile(file)) continue;
            json << (firstFrame ? "" : ",") << "{\"number\":" << (i + 1) << ",\"file\":" << jsonQuote(file) << "}";
            firstFrame = false;
        }
    }
    json << "],";
    if (allowedColors) {
        json << "\"allowed_colors\":[";
        for (size_t i = 0; i < allowedColors->size(); ++i) {
            char hex[8];
            snprintf(hex, sizeof(hex), "#%02x%02x%02x", (*allowedColors)[i].r, (*allowedColors)[i].g, (*allowedColors)[i].b);
            json << (i > 0 ? "," : "") << "\"" << hex << "\"";
        }
        json << "],";
    }
    json << "\"layers\":[";

    const Frame* frame = canvas.getFrameReadOnly(frameIndex);
    int layerCount = frame ? static_cast<int>(frame->layers.size()) : 0;
    if (layerCount >= 2 && layerCount <= kAssistMaxLayers && area.width * area.height <= kAssistContextMaxPixels) {
        sf::RenderTexture layerTex;
        bool first = true;
        for (int i = 0; i < layerCount; ++i) {
            if (!canvas.renderLayerToTexture(frameIndex, i, layerTex)) continue;
            sf::Image whole = layerTex.getTexture().copyToImage();
            sf::Image cut;
            cut.create(static_cast<unsigned int>(area.width), static_cast<unsigned int>(area.height), sf::Color::Transparent);
            cut.copy(whole, 0, 0, area, false);
            std::string file = "temp_ai_layer_" + std::to_string(i) + ".png";
            if (!cut.saveToFile(file)) continue;

            // Read again after the render: the pointer is only good until the frame list changes
            const Layer& layer = canvas.getFrameReadOnly(frameIndex)->layers[i];
            if (!first) json << ",";
            first = false;
            json << "{\"name\":" << jsonQuote(layer.name)
                << ",\"visible\":" << (layer.visible ? "true" : "false")
                << ",\"locked\":" << (layer.locked ? "true" : "false")
                << ",\"opacity\":" << layer.opacity
                << ",\"active\":" << (i == canvas.getActiveLayer() ? "true" : "false")
                << ",\"file\":" << jsonQuote(file) << "}";
        }
    }

    json << "],\"history\":[";
    for (size_t i = 0; i < g_assistTurns.size(); ++i) {
        if (i > 0) json << ",";
        json << "{\"request\":" << jsonQuote(g_assistTurns[i].request) << ",\"outcome\":" << jsonQuote(g_assistTurns[i].outcome) << "}";
    }
    json << "]";

    // A discarded result is worth showing again only when the same area is being worked on
    std::error_code ec;
    std::filesystem::remove("temp_ai_previous.png", ec);
    bool sameArea = g_assistLastArea == area
        && static_cast<int>(g_assistLastResult.getSize().x) == area.width
        && static_cast<int>(g_assistLastResult.getSize().y) == area.height;
    if (g_assistLastDiscarded && !g_assistTurns.empty() && sameArea && g_assistLastResult.saveToFile("temp_ai_previous.png")) {
        json << ",\"previous\":\"temp_ai_previous.png\"";
    }
    json << "}";

    std::ofstream out("temp_ai_context.json", std::ios::binary);
    out << json.str();
}

static bool g_selectingOutlineColor = false;
static sf::Color g_outlineColor = sf::Color::Black;
static bool g_selectingGridColor = false;

UIManager::UIManager() : isTypingPrompt(false), showingText(false), textAlpha(255.0f), isLightingMode(false), focusMode(false), projManager(nullptr), activeProjectName("Untitled_Project"), activeProjectPath(""), showUnsavedWarning(false), currentMenuState(MenuState::Main), startupTime(0.0f), activeTutorialIndex(-1), uiFullscreen(true), uiBorderless(false), uiVsync(true), uiAutoBackup(true), uiHwAccel(true), uiFpsLimit(60), uiAnimFps(12), uiHistorySize(100), easterEggClicks(0), m_debugUseSpriteStudio(false) {}

void UIManager::init(ProjectManager* pm, Canvas* baseCanvas) {
    projManager = pm;
    keybindManager.init();

    bgTexture.loadFromFile("assets/landofwisdompark2.jfif");
    bgSprite.setTexture(bgTexture);
    m_park.init();

    font.loadFromFile("assets/Jersey10-Regular.ttf");

    projectBrowser.init(pm);
    keybindPanel.init(&keybindManager);
    exportModal.init();
    newProjectModal.init();

    layerPanel.init();
    colorPalettePanel.init();
    rightProperties.init();
    audioPanel.init();

    m_toolOptionsBar.SetOutlineColor(g_outlineColor);

    AIManager::getInstance().init();
    g_aiPanel.init();
    g_chatPanel.init();
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
        },
        [this]() {
            m_activeRightTab = (m_activeRightTab == RightTabMode::Chat) ? RightTabMode::None : RightTabMode::Chat;
            if (m_activeRightTab == RightTabMode::Chat) g_chatPanel.focusInput();
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
    m_toolDock.AddTool("ai_gen", "Studio Assistant", [this, baseCanvas]() {
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

void UIManager::startAIAutocomplete(Canvas& canvas, Timeline& timeline, const std::string& hint, bool nextFrame) {
    AIManager& ai = AIManager::getInstance();
    const bool redo = g_assistRedo;
    g_assistRedo = false;
    const std::string provider = ai.getActiveProvider();
    if (!ai.testConnection(provider)) {
        showMessage(provider == "Claude Code"
            ? "Claude Code was not found on this computer. Install it, or switch the Assistant Engine to Claude in Settings"
            : "Enter a Claude access key in Settings first, or switch the Assistant Engine to Claude Code", sf::Color::Red);
        return;
    }

    int curFrame = timeline.getCurrentFrame();
    // A new frame is always the whole canvas, so a selection does not narrow it
    bool hadSelection = canvas.getSelection().isActive() && !nextFrame;
    sf::FloatRect selBox = canvas.getSelection().getBoundingBox();
    canvas.commitSelection(curFrame);
    // An area marked from the request box wins over a selection, and is used exactly as drawn
    bool marked = g_assistMarkActive && !nextFrame;
    sf::IntRect markRect = assistMarkRect(canvas);
    if (redo && !nextFrame) {
        marked = true;
        markRect = g_assistLastArea;
    }
    g_assistLastHint = hint;
    g_assistMarkActive = false;
    g_assistMarkDragging = false;

    AIRequest req;
    req.prompt = hint;
    req.operation = AIOperation::Edit;
    req.baseImage = canvas.flattenFrameToImage(curFrame);

    // Colour limit chosen in the assistant panel. The bridge adds what is on the canvas and the current colour itself.
    std::vector<sf::Color> allowedColors;
    int colorMode = g_aiPanel.getAssistColorMode();
    if (colorMode == 1) {
        const sf::Image& whole = req.baseImage;
        for (unsigned int y = 0; y < whole.getSize().y && allowedColors.size() < 256; ++y) {
            for (unsigned int x = 0; x < whole.getSize().x && allowedColors.size() < 256; ++x) {
                sf::Color c = whole.getPixel(x, y);
                if (c.a < 128) continue;
                c.a = 255;
                if (std::find(allowedColors.begin(), allowedColors.end(), c) == allowedColors.end()) allowedColors.push_back(c);
            }
        }
    }
    else if (colorMode == 2) allowedColors = g_aiPanel.getSelectedPaletteColors();

    g_assistHasRegion = false;
    if (marked || hadSelection) {
        int imgW = static_cast<int>(req.baseImage.getSize().x);
        int imgH = static_cast<int>(req.baseImage.getSize().y);
        // The selection box hugs the selected pixels, so widen it a little: the assistant needs to see
        // what surrounds them and have some room to draw
        int pad = std::max(2, static_cast<int>(std::max(selBox.width, selBox.height)) / 8);
        int left = std::clamp(static_cast<int>(std::floor(selBox.left)) - pad, 0, imgW);
        int top = std::clamp(static_cast<int>(std::floor(selBox.top)) - pad, 0, imgH);
        int right = std::clamp(static_cast<int>(std::ceil(selBox.left + selBox.width)) + pad, 0, imgW);
        int bottom = std::clamp(static_cast<int>(std::ceil(selBox.top + selBox.height)) + pad, 0, imgH);
        if (marked) {
            left = std::clamp(markRect.left, 0, imgW);
            top = std::clamp(markRect.top, 0, imgH);
            right = std::clamp(markRect.left + markRect.width, 0, imgW);
            bottom = std::clamp(markRect.top + markRect.height, 0, imgH);
        }
        bool wholeCanvas = (left == 0 && top == 0 && right == imgW && bottom == imgH);
        if ((marked || (selBox.width >= 1.f && selBox.height >= 1.f)) && right > left && bottom > top && !wholeCanvas) {
            g_assistHasRegion = true;
            g_assistRegion = sf::IntRect(left, top, right - left, bottom - top);
            g_assistFullImage = req.baseImage;

            sf::Image area;
            area.create(static_cast<unsigned int>(right - left), static_cast<unsigned int>(bottom - top), sf::Color::Transparent);
            area.copy(g_assistFullImage, 0, 0, g_assistRegion, false);
            req.baseImage = area;
        }
    }
    req.isPixelMode = canvas.getPixelMode();
    req.primaryColor = canvas.getPrimaryColor();
    req.width = static_cast<int>(req.baseImage.getSize().x);
    req.height = static_cast<int>(req.baseImage.getSize().y);

    syncAssistProject(activeProjectPath, activeProjectName);
    g_assistFrameTotal = nextFrame ? g_aiPanel.getAssistFrameCount() : 1;
    if (!nextFrame) g_assistPendingRequest = hint;
    else if (g_assistFrameTotal == 1) g_assistPendingRequest = "[draw the next animation frame] " + hint;
    else g_assistPendingRequest = "[draw the next " + std::to_string(g_assistFrameTotal) + " animation frames] " + hint;
    g_assistFrameMode = nextFrame;
    g_assistPendingArea = g_assistHasRegion ? g_assistRegion : sf::IntRect(0, 0, req.width, req.height);
    // Several frames in a row are one take: options would multiply the wait for each of them
    int optionCount = g_assistFrameTotal > 1 ? 1 : g_aiPanel.getAssistOptionCount();
    writeAssistContext(canvas, curFrame, g_assistPendingArea, colorMode != 0 ? &allowedColors : nullptr, optionCount, nextFrame, g_assistFrameTotal, timeline.getFps());

    AIResult res = ai.executeRequest(req);
    if (!res.success) {
        g_assistHasRegion = false;
        showMessage("Assistant: " + res.errorMessage, sf::Color::Red);
    }
    else if (g_assistHasRegion) showMessage(marked ? "Working on the marked area only" : "Working on the selected area only", sf::Color(0, 191, 255));
}

void UIManager::sendChatMessage(Canvas& canvas, Timeline& timeline, const std::string& message) {
    AIManager& ai = AIManager::getInstance();
    if (ai.isProcessingAsync()) {
        g_chatPanel.addMessage(ChatPanel::Role::Error, "The assistant is still busy with another request.");
        return;
    }
    if (ai.getActiveProvider() != "Claude Code") {
        g_chatPanel.addMessage(ChatPanel::Role::Error, "The chat needs the Claude Code engine. Switch the Assistant Engine to Claude Code in Settings.");
        return;
    }
    if (!ai.testConnection("Claude Code")) {
        g_chatPanel.addMessage(ChatPanel::Role::Error, "Claude Code was not found on this computer. Install it, then try again.");
        return;
    }
    sf::Vector2u size = canvas.getCanvasSize();
    // Read before the selection is committed below, which ends it
    ChatArea area = chatAreaFor(canvas);
    sf::Vector2u work = area.workSize();

    // The timeline goes to Claude as it is now: each frame as it looks, and Claude's own layer, so the bridge
    // can tell which of Claude's frames the artist painted over since the last reply. Both are cut to the
    // area and shrunk the way Claude will draw it.
    int curFrame = timeline.getCurrentFrame();
    canvas.commitSelection(curFrame);
    std::string dir = chatDirFor(activeProjectPath);
    std::error_code ec;
    std::filesystem::remove_all(dir + "/canvas", ec);
    std::filesystem::create_directories(dir + "/canvas", ec);
    int frameCount = static_cast<int>(canvas.getFrameCount());
    int claudeLayer = findClaudeLayer(canvas);
    for (int i = 0; i < frameCount; ++i) {
        char name[32];
        snprintf(name, sizeof(name), "/canvas/frame_%03d.png", i);
        shrinkArea(canvas.flattenFrameToImage(i), area).saveToFile(dir + name);
        sf::RenderTexture layerTex;
        if (claudeLayer >= 0 && canvas.renderLayerToTexture(i, claudeLayer, layerTex)) {
            snprintf(name, sizeof(name), "/canvas/claude_%03d.png", i);
            shrinkArea(layerTex.getTexture().copyToImage(), area).saveToFile(dir + name);
        }
    }

    nlohmann::json job = {
        {"dir", dir}, {"width", work.x}, {"height", work.y}, {"fps", timeline.getFps()},
        {"current_frame", curFrame}, {"frame_count", frameCount}, {"message", message}, {"model", g_chatPanel.modelId()}
    };
    std::string where;
    if (area.selected) {
        where = "The artist marked a " + std::to_string(area.rect.width) + "x" + std::to_string(area.rect.height) + " area at ("
            + std::to_string(area.rect.left) + ", " + std::to_string(area.rect.top) + ") of their " + std::to_string(size.x) + "x"
            + std::to_string(size.y) + " canvas. You draw only that area; the rest of the canvas stays as it is.";
    }
    if (area.scale > 1) {
        std::string s = std::to_string(area.scale);
        std::string w = std::to_string(work.x) + "x" + std::to_string(work.y);
        if (where.empty()) where = "The canvas is " + std::to_string(size.x) + "x" + std::to_string(size.y) + ".";
        where += " You work at 1/" + s + " size: the canvas images are shrunk to " + w + ", and each pixel you draw becomes a "
            + s + "x" + s + " block, so draw it as " + w + " pixel art. That size is only how you work: to the artist, call it "
            "their canvas (or their area) and don't mention " + w + " or the scaling.";
    }
    if (!where.empty()) job["area_note"] = where;
    AIRequest req;
    req.operation = AIOperation::Chat;
    req.prompt = job.dump();
    g_chatPanel.addMessage(ChatPanel::Role::Artist, message);
    AIResult res = ai.executeRequest(req);
    if (!res.success) {
        g_chatPanel.addMessage(ChatPanel::Role::Error, res.errorMessage);
        return;
    }
    g_chatPending = true;
    g_chatSentDir = dir;
    g_chatSentArea = area;
    g_chatStatus = "Sending to Claude";
    g_chatStatusTimer = 0.f;
}

void UIManager::applyChatReply(Canvas& canvas, Timeline& timeline, const AIResult& result) {
    g_chatPending = false;
    g_chatStatus.clear();
    g_chatPanel.finishRun(result.success);
    const std::string dir = g_chatSentDir;
    if (!result.success) {
        g_chatPanel.addMessage(ChatPanel::Role::Error, result.errorMessage == "Task canceled by user." ? "Stopped." : result.errorMessage);
        return;
    }

    std::string reply = "Done.";
    std::vector<int> changed;
    std::string usageNote;
    try {
        std::ifstream in(dir + "/reply.json", std::ios::binary);
        nlohmann::json data = nlohmann::json::parse(in);
        reply = data.value("reply", reply);
        for (const auto& v : data["changed"]) changed.push_back(v.get<int>());
        const nlohmann::json& limits = data.contains("limits") && data["limits"].is_object() ? data["limits"] : nlohmann::json::object();
        if (limits.contains("five_hour")) {
            const auto& five = limits["five_hour"];
            const auto& week = limits.contains("seven_day") ? limits["seven_day"] : nlohmann::json::object();
            usageNote = g_chatPanel.reportUsage(five.value("utilization", -1.f), five.value("resetsAt", 0LL),
                week.value("utilization", -1.f), week.value("resetsAt", 0LL), data.value("cost", 0.0));
        }
    }
    catch (...) {
        g_chatPanel.addMessage(ChatPanel::Role::Error, "Claude finished, but its reply could not be read.");
        return;
    }
    // Another project was opened while Claude worked: its frames belong to the project it was asked in
    if (dir != chatDirFor(activeProjectPath)) {
        showMessage("Claude replied in another project; open it again to see the conversation", sf::Color::Yellow);
        return;
    }
    g_chatPanel.addMessage(ChatPanel::Role::Claude, reply);
    if (!usageNote.empty()) g_chatPanel.addMessage(ChatPanel::Role::Note, usageNote);
    if (changed.empty()) return;
    std::sort(changed.begin(), changed.end());

    // All of it is one undo step: Ctrl+Z takes back the whole reply
    size_t undoDepth = canvas.getUndoDepth();
    int curFrame = timeline.getCurrentFrame();
    canvas.commitSelection(curFrame);
    int claudeLayer = findClaudeLayer(canvas);
    if (claudeLayer < 0) {
        canvas.addLayer(0, "Claude");
        claudeLayer = findClaudeLayer(canvas);
    }
    const Frame* first = canvas.getFrameReadOnly(0);
    if (claudeLayer < 0 || !first || first->layers[claudeLayer].locked) {
        g_chatPanel.addMessage(ChatPanel::Role::Error, "The Claude layer is locked, so the frames could not be placed. Unlock it and ask again.");
        return;
    }

    int placed = 0;
    int added = 0;
    int objects = 0;
    for (int index : changed) {
        while (static_cast<int>(canvas.getFrameCount()) <= index) {
            int at = static_cast<int>(canvas.getFrameCount());
            canvas.addFrameAt(at);
            timeline.addFrameAt(at);
            ++added;
        }
        timeline.syncWithCanvas(static_cast<int>(canvas.getFrameCount()));
        char name[32];
        snprintf(name, sizeof(name), "/frames/%03d.png", index);
        sf::Image drawn;
        if (!drawn.loadFromFile(dir + name)) continue;
        // Claude drew the area it was sent; the rest of the Claude layer on this frame is kept
        sf::Image img;
        sf::RenderTexture layerTex;
        if (canvas.renderLayerToTexture(index, claudeLayer, layerTex)) img = layerTex.getTexture().copyToImage();
        else img.create(canvas.getCanvasSize().x, canvas.getCanvasSize().y, sf::Color::Transparent);
        growIntoArea(img, drawn, g_chatSentArea);
        int made = canvas.setLayerAsObjects(index, claudeLayer, img);
        if (made >= 0) { ++placed; objects += made; }
    }
    // The Claude layer is left active, so the select tool picks up what Claude drew right away
    canvas.setActiveLayer(claudeLayer, changed.front());
    timeline.syncWithCanvas(static_cast<int>(canvas.getFrameCount()));
    canvas.collapseUndoSteps(undoDepth);
    timeline.setFrame(changed.front());

    std::string what = placed == 1 ? "frame " + std::to_string(changed.front() + 1) : std::to_string(placed) + " frames";
    if (added > 0) what += " (" + std::to_string(added) + " new)";
    showMessage("Claude drew " + what + " as " + std::to_string(objects) + (objects == 1 ? " object" : " objects") + " on the Claude layer (now active, so the select tool picks them). Ctrl+Z undoes it", sf::Color::Green);
}

void UIManager::handleChatAction(const std::string& action, Canvas& canvas, Timeline& timeline) {
    if (action == "send") sendChatMessage(canvas, timeline, g_chatPanel.takeMessage());
    else if (action == "stop") {
        AIManager::getInstance().abortTask();
        cancelClaudeArt();
    }
    else if (action == "close") m_activeRightTab = RightTabMode::None;
    else if (action == "mark_area") {
        // The button clears an area that is marked (or being marked), and otherwise starts marking one
        if (g_chatMarking || g_chatMarkDragging || g_chatMarkActive) {
            g_chatMarking = g_chatMarkDragging = g_chatMarkActive = false;
        }
        else {
            g_chatMarking = true;
            showMessage("Drag on the canvas to mark where Claude draws. Esc cancels", sf::Color(217, 119, 87));
        }
    }
    else if (action == "new_chat") {
        // Claude keeps its drawing code and what it last drew, so it can still pick up the sprite; only
        // the conversation starts over
        std::string dir = chatDirFor(activeProjectPath);
        std::error_code ec;
        std::filesystem::remove(dir + "/session.txt", ec);
        std::filesystem::remove(dir + "/chat.jsonl", ec);
        g_chatPanel.clearMessages();
        g_chatPanel.addMessage(ChatPanel::Role::Note, "New conversation. Claude forgets the chat but keeps the sprite it was drawing.");
    }
}

// Send / Cancel sit to the right of the request box (promptBox is 600x50 at 660,780)
static const sf::FloatRect kPromptSendBounds(1268.f, 780.f, 84.f, 50.f);
static const sf::FloatRect kPromptCancelBounds(1360.f, 780.f, 84.f, 50.f);

void UIManager::openAIPrompt(bool contour, bool nextFrame) {
    isTypingPrompt = true;
    g_assistFramePrompt = nextFrame;
    g_assistMarkActive = false;
    g_assistMarkDragging = false;
    syncAssistProject(activeProjectPath, activeProjectName);
    // The contour entry only pre-fills the request; Claude still reads it as free text
    g_promptText = contour ? "make contour of " : "";
    g_promptCaret = g_promptText.getSize();
    refreshPrompt(promptDisplay, currentPrompt);
    if (nextFrame && g_aiPanel.getAssistFrameCount() > 1) showMessage("Say what happens over the next " + std::to_string(g_aiPanel.getAssistFrameCount()) + " frames, or just press Enter to continue the motion", sf::Color(0, 191, 255));
    else if (nextFrame) showMessage("Say what happens in the next frame, or just press Enter to continue the motion", sf::Color(0, 191, 255));
    else if (contour) showMessage("Name what to outline (or leave it to outline your drawing), then press Enter", sf::Color(0, 191, 255));
    else showMessage("Describe what to draw or change, then press Enter", sf::Color(0, 191, 255));
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

// Darkens the side of the artwork the menu sits on.
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
        m_arcade.draw(window, font);
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
    // The park in the background opens with the first click or key press; after that this does nothing
    if (event.type == sf::Event::MouseButtonPressed || event.type == sf::Event::KeyPressed) {
        m_park.open();
    }

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

    if (AIManager::getInstance().isProcessingAsync() && !g_chatPending) {
        auto killAiProcess = [&]() {
            AIManager::getInstance().abortTask();
            // Ends only the assistant's own processes; other Python programs on the machine keep running
            cancelClaudeArt();
            std::ofstream f("temp_ai_output.png");
            f.close();
            };

        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
            killAiProcess();
            return;
        }

        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            sf::Vector2i mPos(event.mouseButton.x, event.mouseButton.y);
            sf::Vector2f viewPos = window.mapPixelToCoords(mPos, WisdomUI::WorkspaceLayout::GetLetterboxView(window.getSize()));

            if (g_loadingScene.handleClick(viewPos)) killAiProcess();
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

    if (isTypingPrompt && event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (kPromptSendBounds.contains(mousePos)) {
            isTypingPrompt = false;
            startAIAutocomplete(canvas, timeline, currentPrompt, g_assistFramePrompt);
            return;
        }
        if (kPromptCancelBounds.contains(mousePos)) {
            isTypingPrompt = false;
            return;
        }
        if (promptBox.getGlobalBounds().contains(mousePos)) return;
        if (!g_assistFramePrompt && !g_aiPanel.containsPoint(mousePos) && canvas.getDrawArea().contains(logicalMousePos)) {
            g_assistMarkDragging = true;
            g_assistMarkActive = false;
            g_assistMarkStart = g_assistMarkEnd = toCanvasPixel(canvas, logicalMousePos);
            return;
        }
    }
    if (isTypingPrompt && g_assistMarkDragging) {
        if (event.type == sf::Event::MouseMoved) {
            g_assistMarkEnd = toCanvasPixel(canvas, logicalMousePos);
            return;
        }
        if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
            g_assistMarkEnd = toCanvasPixel(canvas, logicalMousePos);
            g_assistMarkDragging = false;
            // A plain click, or a sliver, clears the mark instead of setting one
            sf::IntRect rect = assistMarkRect(canvas);
            g_assistMarkActive = rect.width >= 2 && rect.height >= 2;
            return;
        }
    }

    if (g_aiReviewModal.getIsOpen()) {
        std::string res = g_aiReviewModal.handleEvent(event, mousePos);
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
            g_aiReviewModal.close();
            res = "reject";
        }

        const sf::Image original = g_aiReviewModal.getOriginalImage();
        const sf::Image result = g_aiReviewModal.getResultImage();
        int curFrame = timeline.getCurrentFrame();
        // Accepting can take several canvas operations; they are folded into one undo step at the end
        size_t undoDepth = canvas.getUndoDepth();

        if (res == "accept_new") {
            // Only the pixels the AI changed go on the new layer, so the existing layers stay untouched underneath
            sf::Image added = result;
            sf::Vector2u origSize = original.getSize();
            for (unsigned int y = 0; y < added.getSize().y && y < origSize.y; ++y) {
                for (unsigned int x = 0; x < added.getSize().x && x < origSize.x; ++x) {
                    if (original.getPixel(x, y) == result.getPixel(x, y)) added.setPixel(x, y, sf::Color::Transparent);
                }
            }
            if (canvas.applyImageToLayer(curFrame, added, true, "Assistant Sketch")) {
                showMessage("Suggestion added as new layer", sf::Color::Green);
                setAssistOutcome("the artist kept it, as a new layer", false);
            }
            else showMessage("Could not add the suggestion layer", sf::Color::Red);
        }
        else if (res == "accept_replace") {
            // The suggestion is the whole frame flattened. Only the pixels it changed are written, onto
            // what the active layer already holds, so the other layers do not get baked into this one.
            sf::Image merged = result;
            sf::RenderTexture layerTex;
            if (original.getSize() == result.getSize() && canvas.renderLayerToTexture(curFrame, canvas.getActiveLayer(), layerTex)) {
                sf::Image layerImage = layerTex.getTexture().copyToImage();
                if (layerImage.getSize() == result.getSize()) {
                    for (unsigned int y = 0; y < result.getSize().y; ++y) {
                        for (unsigned int x = 0; x < result.getSize().x; ++x) {
                            if (original.getPixel(x, y) != result.getPixel(x, y)) layerImage.setPixel(x, y, result.getPixel(x, y));
                        }
                    }
                    merged = layerImage;
                }
            }
            if (canvas.applyImageToLayer(curFrame, merged, false)) {
                // The change can land underneath a layer that covers it; say so rather than look like nothing happened
                sf::Image now = canvas.flattenFrameToImage(curFrame);
                bool covered = false;
                if (now.getSize() == result.getSize()) {
                    for (unsigned int y = 0; y < result.getSize().y && !covered; ++y) {
                        for (unsigned int x = 0; x < result.getSize().x; ++x) {
                            if (original.getPixel(x, y) != result.getPixel(x, y) && now.getPixel(x, y) != result.getPixel(x, y)) { covered = true; break; }
                        }
                    }
                }
                if (covered) showMessage("Added to the active layer, but layers above it cover part of it", sf::Color::Yellow);
                else showMessage("Layer updated with suggestion", sf::Color::Green);
                setAssistOutcome("the artist kept it, on the active layer", false);
            }
            else showMessage("Active layer is locked", sf::Color::Red);
        }
        else if (res == "accept_project") {
            if (canvas.getIsDirty()) {
                g_aiReviewModal.open(original, result);
                showMessage("Save the current project first", sf::Color::Red);
            }
            else {
                activeProjectName = "AI_Art_" + std::to_string(static_cast<long long>(std::time(nullptr)));
                activeProjectPath = "projects/" + activeProjectName + ".wpk";
                canvas.clearCanvasImages();
                canvas.clearObjectSelection();
                pm.createNewProject(activeProjectName, static_cast<int>(result.getSize().x), static_cast<int>(result.getSize().y), 12, canvas.getPixelMode(), canvas);
                canvas.applyImageToLayer(timeline.getCurrentFrame(), result, false);
                canvas.clearHistory();
                showMessage("Created Project: " + activeProjectName, sf::Color::Green);
            }
        }
        else if (res == "accept_frame") {
            canvas.commitSelection(curFrame);
            canvas.clearObjectSelection();
            // Several frames asked for at once are all kept: each is added after the one before it, as the
            // changes from that one
            std::vector<sf::Image> newFrames = g_aiReviewModal.isFrameSequence() ? g_aiReviewModal.getOptions() : std::vector<sf::Image>{ result };
            sf::Image prev = original;
            int at = curFrame;
            bool failed = false;
            for (const sf::Image& img : newFrames) {
                canvas.duplicateFrame(at);
                timeline.duplicateFrame(at);
                timeline.syncWithCanvas(static_cast<int>(canvas.getFrameCount()));
                int newFrame = at + 1;
                timeline.setFrame(newFrame);

                // The new frame starts as a copy, layers and all. What changed goes onto the active layer,
                // which is the whole story whenever the moving parts live on that layer.
                bool placed = false;
                sf::Image untouched;
                sf::RenderTexture layerTex;
                if (prev.getSize() == img.getSize() && canvas.renderLayerToTexture(newFrame, canvas.getActiveLayer(), layerTex)) {
                    untouched = layerTex.getTexture().copyToImage();
                    if (untouched.getSize() == img.getSize()) {
                        sf::Image moved = untouched;
                        for (unsigned int y = 0; y < img.getSize().y; ++y) {
                            for (unsigned int x = 0; x < img.getSize().x; ++x) {
                                if (prev.getPixel(x, y) != img.getPixel(x, y)) moved.setPixel(x, y, img.getPixel(x, y));
                            }
                        }
                        if (canvas.applyImageToLayer(newFrame, moved, false)) {
                            sf::Image now = canvas.flattenFrameToImage(newFrame);
                            placed = now.getSize() == img.getSize()
                                && std::equal(now.getPixelsPtr(), now.getPixelsPtr() + static_cast<size_t>(now.getSize().x) * now.getSize().y * 4, img.getPixelsPtr());
                            if (!placed) canvas.applyImageToLayer(newFrame, untouched, false);
                        }
                    }
                }

                if (placed) showMessage("Added as frame " + std::to_string(newFrame + 1), sf::Color::Green);
                else {
                    // Other layers hold parts that moved too, and layer visibility is shared by every frame, so the
                    // copy cannot be patched up. The frame is rebuilt empty and drawn whole onto one visible layer.
                    canvas.deleteFrame(newFrame);
                    timeline.deleteFrame(newFrame);
                    canvas.addFrameAt(newFrame);
                    timeline.addFrameAt(newFrame);
                    timeline.syncWithCanvas(static_cast<int>(canvas.getFrameCount()));
                    timeline.setFrame(newFrame);
                    if (const Frame* blank = canvas.getFrameReadOnly(newFrame)) {
                        int target = canvas.getActiveLayer();
                        int count = static_cast<int>(blank->layers.size());
                        if (target < 0 || target >= count || !blank->layers[target].visible || blank->layers[target].locked) {
                            for (int i = count - 1; i >= 0; --i) {
                                if (blank->layers[i].visible && !blank->layers[i].locked) { target = i; break; }
                            }
                        }
                        canvas.setActiveLayer(target, newFrame);
                    }
                    if (canvas.applyImageToLayer(newFrame, img, false)) showMessage("Added as frame " + std::to_string(newFrame + 1) + ", drawn onto one layer", sf::Color::Green);
                    else { showMessage("Could not draw the new frame: no unlocked layer", sf::Color::Red); failed = true; }
                }
                prev = img;
                at = at + 1;
            }
            if (newFrames.size() > 1 && !failed) showMessage("Added " + std::to_string(newFrames.size()) + " frames after frame " + std::to_string(curFrame + 1), sf::Color::Green);
            setAssistOutcome(newFrames.size() > 1 ? "the artist kept them, as the next " + std::to_string(newFrames.size()) + " frames" : "the artist kept it, as the next frame", false);
        }
        else if (res == "reject") {
            showMessage("Suggestion discarded", sf::Color::Cyan);
            setAssistOutcome("the artist discarded it", true);
        }
        else if (res == "retry" || res == "refine") {
            // The result goes back to the assistant as discarded, so it knows what not to repeat and what
            // a follow-up such as "bigger" refers to
            bool refine = res == "refine";
            setAssistOutcome(refine ? "the artist discarded it and asked for a change" : "the artist discarded it and asked for another try", true);
            g_assistRedo = true;
            startAIAutocomplete(canvas, timeline, refine ? g_aiReviewModal.getFollowUp() : g_assistLastHint, g_assistFrameMode);
        }
        if (res == "accept_new" || res == "accept_replace" || res == "accept_frame") canvas.collapseUndoSteps(undoDepth);
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
                        m_arcade.reset();
                    }
                    return;
                }

                if (m_startMenuFullscreenBtnBounds.contains(mousePos)) {
                    toggleFullscreen(window, settings);
                    return;
                }

                if (m_useMinigameWelcome) {
                    std::string station = m_arcade.stationAt(mousePos);
                    if (!station.empty()) {
                        triggerArcadeStation(station, window);
                        return;
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
                            clickedKeyField = TextField(rowRect).contains(mousePos) && AIManager::getInstance().providerRequiresApiKey(AIManager::getInstance().getActiveProvider());
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
                        showMessage("Assistant settings applied and saved", sf::Color::Green);
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

        // Marking the chat's area takes the canvas clicks (and Esc) until the area is dragged out
        if (g_chatMarking || g_chatMarkDragging) {
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
                g_chatMarking = g_chatMarkDragging = false;
                return;
            }
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left
                && canvas.getDrawArea().contains(logicalMousePos) && !g_chatPanel.containsPoint(mousePos)) {
                g_chatMarkDragging = true;
                g_chatMarkActive = false;
                g_chatMarkStart = g_chatMarkEnd = toCanvasPixel(canvas, logicalMousePos);
                return;
            }
            if (g_chatMarkDragging && event.type == sf::Event::MouseMoved) {
                g_chatMarkEnd = toCanvasPixel(canvas, logicalMousePos);
                return;
            }
            if (g_chatMarkDragging && event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
                g_chatMarkEnd = toCanvasPixel(canvas, logicalMousePos);
                g_chatMarkDragging = g_chatMarking = false;
                sf::IntRect rect = chatMarkRect(canvas);
                // A plain click, or a sliver, marks nothing
                g_chatMarkActive = rect.width >= 4 && rect.height >= 4;
                return;
            }
        }

        // The chat goes first so Esc can interrupt Claude or leave its box before it closes the dock
        if (m_activeRightTab == RightTabMode::Chat && g_chatPanel.handleEvent(event, mousePos)) {
            handleChatAction(g_chatPanel.takeAction(), canvas, timeline);
            return;
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

        // While the request box is open, typing must reach it even with the mouse still over the panel
        bool promptKeyEvent = isTypingPrompt && (event.type == sf::Event::TextEntered || event.type == sf::Event::KeyPressed || event.type == sf::Event::KeyReleased);
        if (g_aiPanel.getIsVisible() && !promptKeyEvent) {
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
                    else if (action == "action:open_chat") {
                        m_activeRightTab = RightTabMode::Chat;
                        g_chatPanel.focusInput();
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
                if (g_promptCaret > 0) g_promptText.erase(--g_promptCaret);
            }
            else if (event.text.unicode >= 32 && event.text.unicode != 127 && g_promptText.getSize() < kPromptMaxLength) {
                g_promptText.insert(g_promptCaret++, sf::String(static_cast<sf::Uint32>(event.text.unicode)));
            }
            refreshPrompt(promptDisplay, currentPrompt);
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
                bool contourPrompt = keybindManager.isActionTriggered("ai_contour", event);
                if (event.key.code == sf::Keyboard::Numpad6 || contourPrompt || keybindManager.isActionTriggered("ai_prompt", event)) {
                    if (isTypingPrompt) isTypingPrompt = false;
                    else openAIPrompt(contourPrompt);
                    return;
                }

                if (isTypingPrompt) {
                    if (event.key.code == sf::Keyboard::Enter) {
                        isTypingPrompt = false;
                        startAIAutocomplete(canvas, timeline, currentPrompt, g_assistFramePrompt);
                    }
                    else if (event.key.code == sf::Keyboard::Escape) {
                        isTypingPrompt = false;
                    }
                    else if (event.key.code == sf::Keyboard::Tab && !g_assistTurns.empty()) {
                        forgetAssistTurns();
                        saveAssistTurns();
                        showMessage("Starting fresh: earlier requests forgotten", sf::Color::Cyan);
                    }
                    else {
                        if (event.key.code == sf::Keyboard::Left && g_promptCaret > 0) --g_promptCaret;
                        else if (event.key.code == sf::Keyboard::Right) ++g_promptCaret;
                        else if (event.key.code == sf::Keyboard::Home) g_promptCaret = 0;
                        else if (event.key.code == sf::Keyboard::End) g_promptCaret = g_promptText.getSize();
                        else if (event.key.code == sf::Keyboard::Delete && g_promptCaret < g_promptText.getSize()) g_promptText.erase(g_promptCaret);
                        else if (event.key.code == sf::Keyboard::V && event.key.control) {
                            // Pasted text goes in on one line, as far as there is room
                            sf::String pasted = sf::Clipboard::getString();
                            for (size_t i = 0; i < pasted.getSize() && g_promptText.getSize() < kPromptMaxLength; ++i) {
                                sf::Uint32 ch = pasted[i];
                                if (ch == '\r') continue;
                                if (ch < 32) ch = ' ';
                                g_promptText.insert(g_promptCaret++, sf::String(ch));
                            }
                        }
                        refreshPrompt(promptDisplay, currentPrompt);
                    }
                    return;
                }

                if (keybindManager.isActionTriggered("ai_complete", event)) {
                    startAIAutocomplete(canvas, timeline, "");
                    return;
                }
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
    m_park.update(dt);

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

    bool assistantBusy = AIManager::getInstance().isProcessingAsync() && !g_chatPending;
    if (assistantBusy) {
        if (!g_loadingWasActive) {
            g_loadingScene.reset();
            g_loadingStatusTimer = 0.f;
        }
        g_loadingStatusTimer -= dt;
        if (g_loadingStatusTimer <= 0.f) {
            g_loadingStatusTimer = 0.5f;
            std::ifstream statusFile("temp_ai_status.txt");
            std::string status;
            if (statusFile && std::getline(statusFile, status) && !status.empty()) g_loadingScene.setTitle(status);
        }
        g_loadingScene.update(dt, mousePos);
    }
    g_loadingWasActive = assistantBusy;

    if (currentState != AppState::Welcome) m_wasOnMainMenu = false;

    if (currentState == AppState::Welcome) {
        if (!keybindPanel.isVisible()) {
            projectBrowser.updateHover(mousePos);
            updateStartMenu(dt, mousePos);
            if (m_useMinigameWelcome) {
                bool paused = currentMenuState != MenuState::Main || newProjectModal.getIsOpen() || exportModal.getIsOpen()
                    || m_showKeybinds || !window.hasFocus();
                std::string station = m_arcade.update(dt, mousePos, paused);
                if (!station.empty()) triggerArcadeStation(station, window);
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
        m_rightDockTabs.SetTabState("chat", m_activeRightTab == RightTabMode::Chat);
        m_rightDockTabs.Update(dt, mousePos);

        // The panel shows the conversation of the project that is open, reloading it when that changes
        std::string chatDir = chatDirFor(activeProjectPath);
        if (chatDir != g_chatShownDir && !g_chatPending) {
            g_chatPanel.loadHistory(chatDir + "/chat.jsonl");
            g_chatShownDir = chatDir;
            g_chatMarking = g_chatMarkDragging = g_chatMarkActive = false;
        }
        sf::IntRect chatMark = chatMarkRect(canvas);
        g_chatPanel.setMarking(g_chatMarking || g_chatMarkDragging,
            g_chatMarkActive ? "area " + std::to_string(chatMark.width) + "x" + std::to_string(chatMark.height) : "");
        // What the time estimate needs: whether Claude has a sprite going here already, and the size it will draw at
        static float chatContextTimer = 0.f;
        chatContextTimer -= dt;
        if (chatContextTimer <= 0.f && m_activeRightTab == RightTabMode::Chat) {
            chatContextTimer = 1.f;
            std::error_code ec;
            ChatArea area = chatAreaFor(canvas);
            sf::Vector2u work = area.workSize();
            std::string label = activeProjectPath.empty() ? std::string("unsaved project") : std::filesystem::path(activeProjectPath).filename().string() + "/assistant";
            bool scaled = std::max(area.rect.width, area.rect.height) > kChatMaxCanvas;
            g_chatPanel.setContext(label, std::filesystem::exists(chatDir + "/sprite.py", ec), work.x, work.y, chatAreaLabel(area), scaled);
        }
        if (g_chatPending) {
            g_chatStatusTimer -= dt;
            if (g_chatStatusTimer <= 0.f) {
                g_chatStatusTimer = 0.5f;
                std::ifstream statusFile("temp_ai_status.txt");
                std::string status;
                if (statusFile && std::getline(statusFile, status) && !status.empty()) g_chatStatus = status;
            }
        }
        g_chatPanel.update(dt, focusMode, m_activeRightTab == RightTabMode::Chat, g_chatPending, g_chatStatus);
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

        if (g_chatPending && AIManager::getInstance().hasAsyncFinished()) {
            sf::Image unused;
            applyChatReply(canvas, timeline, AIManager::getInstance().getAsyncResult(unused));
        }
        if (AIManager::getInstance().hasAsyncFinished()) {
            sf::Image originalImage;
            AIResult asyncRes = AIManager::getInstance().getAsyncResult(originalImage);
            if (asyncRes.success) {
                bool regionFits = g_assistHasRegion
                    && static_cast<int>(asyncRes.resultImage.getSize().x) == g_assistRegion.width
                    && static_cast<int>(asyncRes.resultImage.getSize().y) == g_assistRegion.height;

                // Further options, or the frames after the first when several were asked for, sit next to the first one
                g_assistOptionsRaw.assign(1, asyncRes.resultImage);
                for (int n = 2; n <= 4; ++n) {
                    sf::Image option;
                    std::string file = "temp_ai_output_" + std::to_string(n) + ".png";
                    if (std::filesystem::exists(file) && option.loadFromFile(file) && option.getSize() == asyncRes.resultImage.getSize()) g_assistOptionsRaw.push_back(option);
                    // Frames follow one another, so a missing one ends the run
                    else if (g_assistFrameTotal > 1) break;
                }

                std::vector<sf::Image> shown = g_assistOptionsRaw;
                if (regionFits) {
                    // Put the reworked area back into the full frame so review and accept see the whole canvas
                    for (auto& option : shown) {
                        sf::Image whole = g_assistFullImage;
                        whole.copy(option, static_cast<unsigned int>(g_assistRegion.left), static_cast<unsigned int>(g_assistRegion.top), sf::IntRect(0, 0, 0, 0), false);
                        option = whole;
                    }
                }
                g_aiReviewModal.open(regionFits ? g_assistFullImage : originalImage, shown);
                g_aiReviewModal.setNextFrameMode(g_assistFrameMode);
                g_aiReviewModal.setFrameSequence(g_assistFrameMode && g_assistFrameTotal > 1);
                if (g_aiReviewModal.isFrameSequence()) {
                    bool all = static_cast<int>(shown.size()) == g_assistFrameTotal;
                    showMessage(std::to_string(shown.size()) + (all ? "" : " of " + std::to_string(g_assistFrameTotal)) + " new frames ready: step through them with the arrows", all ? sf::Color::Green : sf::Color::Yellow);
                }
                else if (g_assistFrameTotal > 1) showMessage("Only the first of " + std::to_string(g_assistFrameTotal) + " frames could be drawn", sf::Color::Yellow);
                else if (shown.size() > 1) showMessage(std::to_string(shown.size()) + " options ready: pick one with the arrows", sf::Color::Green);
                else showMessage("Suggestion ready", sf::Color::Green);

                // On a canvas too large to edit in one go the bridge says which part it worked on
                std::ifstream noteFile("temp_ai_note.txt");
                std::string note;
                if (noteFile && std::getline(noteFile, note) && !note.empty()) showMessage(note, sf::Color::Yellow);

                g_assistTurns.push_back({ g_assistPendingRequest, "shown to the artist" });
                if (g_assistTurns.size() > kAssistMaxTurns) g_assistTurns.erase(g_assistTurns.begin());
                saveAssistTurns();
                g_assistLastResult = asyncRes.resultImage;
                g_assistLastArea = g_assistPendingArea;
                g_assistLastDiscarded = false;
            }
            else {
                showMessage("Assistant error: " + asyncRes.errorMessage, sf::Color::Red);
            }
            g_assistHasRegion = false;
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

    if (m_park.isReady()) {
        m_park.draw(window);
    }
    else {
        bgSprite.setPosition(0.f, 0.f);
        bgSprite.setScale(1920.f / bgTexture.getSize().x, 1080.f / bgTexture.getSize().y);
        bgSprite.setColor(sf::Color::White);
        window.draw(bgSprite);
    }

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
        // The chat's area, over the canvas, while the chat is open or Claude is drawing in it
        if ((g_chatMarkDragging || g_chatMarkActive) && (m_activeRightTab == RightTabMode::Chat || g_chatPending)) {
            sf::IntRect mark = chatMarkRect(canvas);
            if (mark.width > 0 && mark.height > 0) {
                sf::FloatRect area = canvas.getDrawArea();
                sf::Vector2u cs = canvas.getCanvasSize();
                sf::Vector2f unit(area.width / static_cast<float>(cs.x), area.height / static_cast<float>(cs.y));
                sf::RectangleShape box(sf::Vector2f(mark.width * unit.x, mark.height * unit.y));
                box.setPosition(area.left + mark.left * unit.x, area.top + mark.top * unit.y);
                box.setFillColor(sf::Color(217, 119, 87, 30));
                box.setOutlineColor(sf::Color(217, 119, 87));
                box.setOutlineThickness(-2.f / std::max(0.01f, canvas.getTransform().getMatrix()[0]));
                window.draw(box, canvas.getTransform());
            }
        }
        // Drawn every frame so it can slide out after its tab is closed
        g_chatPanel.draw(window);

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
            sf::Vector2f promptMouse = window.mapPixelToCoords(sf::Mouse::getPosition(window));
            WisdomUI::Theme::DrawSunsetButton(window, kPromptSendBounds, "Send", font, 14, false, kPromptSendBounds.contains(promptMouse), true, 1.0f);
            WisdomUI::Theme::DrawSunsetButton(window, kPromptCancelBounds, "Cancel", font, 14, false, kPromptCancelBounds.contains(promptMouse), false, 1.0f);
            float hintY = 838.f;
            if (!g_assistFramePrompt) {
                sf::IntRect mark = assistMarkRect(canvas);
                bool showMark = (g_assistMarkDragging || g_assistMarkActive) && mark.width > 0 && mark.height > 0;
                if (showMark) {
                    sf::FloatRect area = canvas.getDrawArea();
                    sf::Vector2u cs = canvas.getCanvasSize();
                    sf::Vector2f unit(area.width / static_cast<float>(cs.x), area.height / static_cast<float>(cs.y));
                    sf::RectangleShape box(sf::Vector2f(mark.width * unit.x, mark.height * unit.y));
                    box.setPosition(area.left + mark.left * unit.x, area.top + mark.top * unit.y);
                    box.setFillColor(sf::Color(255, 196, 64, 36));
                    box.setOutlineColor(WisdomUI::Theme::SunsetGold);
                    box.setOutlineThickness(-2.f / std::max(0.01f, canvas.getTransform().getMatrix()[0]));
                    window.draw(box, canvas.getTransform());
                }
                std::string markHint = showMark
                    ? "Working on the marked area (" + std::to_string(mark.width) + "x" + std::to_string(mark.height) + "). Click the canvas to clear it."
                    : "Drag on the canvas to mark the area to work on, or send to use the whole canvas.";
                WisdomUI::Theme::DrawCrispText(window, font, markHint, 15, 664.f, hintY, WisdomUI::Theme::SunsetGold, sf::Color(0, 0, 0, 200), false, true);
                hintY += 22.f;
            }
            if (g_assistFramePrompt) {
                int frameCount = g_aiPanel.getAssistFrameCount();
                std::string frameHint = frameCount > 1
                    ? "Next " + std::to_string(frameCount) + " frames: say what happens over them, or leave it empty to continue the motion."
                    : "Next frame: say what happens, or leave it empty to continue the motion.";
                WisdomUI::Theme::DrawCrispText(window, font, frameHint, 15, 664.f, hintY, WisdomUI::Theme::SunsetGold, sf::Color(0, 0, 0, 200), false, true);
                hintY += 22.f;
            }
            if (!g_assistTurns.empty() && g_assistTurnsProject == activeProjectPath + "|" + activeProjectName) {
                std::string memo = "Remembers your last " + std::string(g_assistTurns.size() == 1 ? "request" : std::to_string(g_assistTurns.size()) + " requests")
                    + ", so you can follow up (\"bigger\", \"try again\"). Tab starts fresh.";
                WisdomUI::Theme::DrawCrispText(window, font, memo, 15, 664.f, hintY, WisdomUI::Theme::SunsetGold, sf::Color(0, 0, 0, 200), false, true);
                hintY += 22.f;
            }
            // The panel may be closed while typing, so say which of its limits are in force
            std::string limits;
            if (g_aiPanel.getAssistColorMode() == 1) limits = "Colours: drawing only";
            else if (g_aiPanel.getAssistColorMode() == 2) limits = "Colours: " + g_aiPanel.getSelectedPaletteName();
            // Several frames in a row are always a single take
            bool singleTake = g_assistFramePrompt && g_aiPanel.getAssistFrameCount() > 1;
            if (g_aiPanel.getAssistOptionCount() > 1 && !singleTake) limits += std::string(limits.empty() ? "" : "   |   ") + std::to_string(g_aiPanel.getAssistOptionCount()) + " options";
            if (!g_aiPanel.getAssistChangesOnly()) limits += std::string(limits.empty() ? "" : "   |   ") + "whole canvas output";
            if (!limits.empty()) WisdomUI::Theme::DrawCrispText(window, font, limits, 15, 664.f, hintY, WisdomUI::Theme::SunsetGold, sf::Color(0, 0, 0, 200), false, true);
        }

        keybindPanel.draw(window);
        if (exportModal.getIsOpen()) exportModal.draw(window);
        if (newProjectModal.getIsOpen()) newProjectModal.draw(window);

        if (AIManager::getInstance().isProcessingAsync() && !g_chatPending) {
            g_loadingScene.draw(window, font, window.mapPixelToCoords(sf::Mouse::getPosition(window)));
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

void UIManager::triggerArcadeStation(const std::string& id, sf::RenderWindow& window) {
    m_arcade.flashStation(id);

    if (id == "exit") window.close();
    else if (id == "new_project") newProjectModal.open();
    else if (id == "projects") currentMenuState = MenuState::Projects;
    else if (id == "settings") currentMenuState = MenuState::Settings;
    else if (id == "tutorials") { currentMenuState = MenuState::Tutorials; activeTutorialIndex = -1; }
    else if (id == "keybinds") m_showKeybinds = true;
    else if (id == "credits") { currentMenuState = MenuState::Credits; easterEggClicks = 0; }
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
        m_gitImgClient.pushAsync(stripImg, repo, filename, commitMsg, [this](bool success, bool serverUp) {
            m_isPushingGitImg = false;
            if (success) {
                showMessage("Successfully pushed animation strip to GitImg!", sf::Color::Green);
            }
            else if (!serverUp) {
                showMessage("GitImg server is down", sf::Color::Red);
            }
            else {
                showMessage("Failed to push animation strip to GitImg", sf::Color::Red);
            }
            });
        };

    if (!m_gitImgClient.isAuthenticated()) {
        std::thread([this, user, pass, doPush]() {
            bool serverUp = false;
            if (m_gitImgClient.login(user, pass, &serverUp)) {
                GitImgClient::saveCredentials(user, pass);
                doPush();
            }
            else if (!serverUp) {
                // Not a rejected login: keep the saved credentials
                m_isPushingGitImg = false;
                showMessage("GitImg server is down", sf::Color::Red);
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
        m_gitImgClient.pushAsync(img, repo, filename, commitMsg, [this](bool success, bool serverUp) {
            m_isPushingGitImg = false;
            if (success) {
                showMessage("Successfully pushed to GitImg!", sf::Color::Green);
            }
            else if (!serverUp) {
                showMessage("GitImg server is down", sf::Color::Red);
            }
            else {
                showMessage("Failed to push to GitImg", sf::Color::Red);
            }
            });
        };

    if (!m_gitImgClient.isAuthenticated()) {
        std::thread([this, user, pass, doPush]() {
            bool serverUp = false;
            if (m_gitImgClient.login(user, pass, &serverUp)) {
                GitImgClient::saveCredentials(user, pass);
                doPush();
            }
            else if (!serverUp) {
                // Not a rejected login: keep the saved credentials
                m_isPushingGitImg = false;
                showMessage("GitImg server is down", sf::Color::Red);
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

