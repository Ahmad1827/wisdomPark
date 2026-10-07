#include "ExportModal.h"
#include "../../core/NativeDialogs.h"
#include "../UITheme.h"
#include "MenuWidgets.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

using WisdomUI::Theme;
using WisdomUI::Animation;
using namespace MenuWidgets;

namespace {

    const int kSheetColumns = 10;

    // Room kept free under the preview image for the frame and zoom readout.
    const float kPreviewPadding = 20.0f;
    const float kPreviewFooter = 40.0f;

    void drawCaption(sf::RenderWindow& window, const sf::Font& font, const std::string& text, float x, float y, float width) {
        Theme::DrawCrispText(window, font, text, 14, x, y, Theme::SunsetAmber, kTextShadow, false, true);

        float textW = Theme::MeasureText(font, text, 14);
        sf::RectangleShape rule(sf::Vector2f(std::max(0.0f, width - textW - 12.0f), 1.0f));
        rule.setPosition(x + textW + 12.0f, y);
        rule.setFillColor(Theme::SunsetPlum);
        window.draw(rule);
    }

    // Row with a label on the left and a sliding switch on the right; the whole row is the click target.
    void drawToggleRow(sf::RenderWindow& window, const sf::Font& font, const sf::FloatRect& b, const std::string& label, bool on, bool hovered) {
        float t = Theme::AnimateHover(b, hovered);
        float centerY = b.top + b.height * 0.5f;

        sf::RectangleShape bg(sf::Vector2f(b.width, b.height));
        bg.setPosition(b.left, b.top);
        bg.setFillColor(Theme::Mix(sf::Color(22, 14, 36, 240), sf::Color(48, 30, 70, 248), t));
        bg.setOutlineThickness(1.0f);
        bg.setOutlineColor(Theme::Mix(Theme::Border, Theme::SunsetPeach, t));
        window.draw(bg);

        Theme::DrawCrispText(window, font, label, 17, b.left + 18.0f, centerY, Theme::Mix(Theme::TextPrimary, Theme::SunsetGold, t), kTextShadow, false, true);

        // The knob slides between the two ends; AnimateHover doubles as the tween.
        sf::FloatRect sw(b.left + b.width - 80.0f, b.top + (b.height - 28.0f) * 0.5f, 64.0f, 28.0f);
        float slide = Theme::AnimateHover(sw, on);

        sf::ConvexShape track = Theme::ChamferedRect(sw.left, sw.top, sw.width, sw.height, 4.0f);
        track.setFillColor(Theme::Mix(Theme::PanelInset, Theme::SunsetAmber, slide));
        track.setOutlineThickness(1.0f);
        track.setOutlineColor(Theme::Mix(Theme::SunsetPlum, Theme::SunsetGold, slide));
        window.draw(track);

        const float knob = sw.height - 8.0f;
        sf::RectangleShape knobShape(sf::Vector2f(knob, knob));
        knobShape.setPosition(std::floor(sw.left + 4.0f + slide * (sw.width - knob - 8.0f)), sw.top + 4.0f);
        knobShape.setFillColor(Theme::Mix(Theme::TextMuted, Theme::SunsetDeepDark, slide));
        window.draw(knobShape);

        const char* state = on ? "ON" : "OFF";
        float stateW = Theme::MeasureText(font, state, 15);
        Theme::DrawCrispText(window, font, state, 15, sw.left - stateW - 14.0f, centerY, on ? Theme::SunsetGold : Theme::TextMuted, sf::Color::Transparent, false, true);
    }

    // Two-line export button. The primary one gets the filled amber treatment.
    void drawExportButton(sf::RenderWindow& window, const sf::Font& font, const sf::FloatRect& bounds, const std::string& title, const std::string& note, bool primary, bool hovered) {
        float t = Theme::AnimateHover(bounds, hovered);
        bool pressed = hovered && sf::Mouse::isButtonPressed(sf::Mouse::Left);
        sf::FloatRect b = shifted(bounds, 0.0f, pressed ? 1.0f : 0.0f);

        float glowAlpha = primary ? (26.0f + 12.0f * std::sin(Theme::s_time * 3.0f) + 46.0f * t) : (48.0f * t);
        if (glowAlpha > 1.0f) {
            sf::ConvexShape glow = Theme::ChamferedRect(b.left - 4.0f, b.top - 4.0f, b.width + 8.0f, b.height + 8.0f, 7.0f);
            glow.setFillColor(Theme::WithAlpha(primary ? Theme::SunsetGold : Theme::SunsetPeach, glowAlpha));
            window.draw(glow);
        }

        sf::ConvexShape body = Theme::ChamferedRect(b.left, b.top, b.width, b.height, 4.0f);
        body.setFillColor(primary ? Theme::Mix(Theme::SunsetAmber, Theme::SunsetGold, t) : Theme::Mix(Theme::ButtonIdle, Theme::ButtonHover, t));
        body.setOutlineThickness(1.0f);
        body.setOutlineColor(primary ? Theme::SunsetGlow : Theme::Mix(Theme::Border, Theme::SunsetPeach, t));
        window.draw(body);

        sf::RectangleShape lower(sf::Vector2f(b.width, b.height * 0.5f - 4.0f));
        lower.setPosition(b.left, b.top + b.height * 0.5f);
        lower.setFillColor(primary ? sf::Color(120, 50, 10, 34) : sf::Color(0, 0, 0, 38));
        window.draw(lower);

        float cx = b.left + b.width * 0.5f;
        if (primary) {
            Theme::DrawCrispText(window, font, title, 20, cx, b.top + 26.0f, Theme::SunsetDeepDark, sf::Color::Transparent, true, true);
            Theme::DrawCrispText(window, font, note, 14, cx, b.top + 51.0f, sf::Color(92, 48, 18), sf::Color::Transparent, true, true);
        }
        else {
            Theme::DrawCrispText(window, font, title, 20, cx, b.top + 26.0f, Theme::Mix(Theme::TextPrimary, Theme::SunsetGold, t), kTextShadow, true, true);
            Theme::DrawCrispText(window, font, note, 14, cx, b.top + 51.0f, Theme::TextSecondary, sf::Color::Transparent, true, true);
        }
    }

}

ExportModal::ExportModal()
    : exportSize(0, 0), previewScale(1.0f), frameMegabytes(0.0f),
    isOpen(false), transparentBg(true), autoCrop(false), linkedCanvas(nullptr), activeFrame(0), openTime(0.0f) {}

void ExportModal::init() {
    font.loadFromFile("assets/Jersey10-Regular.ttf");

    modalBounds = sf::FloatRect(1920.f / 2.f - 520.f, 1080.f / 2.f - 340.f, 1040.f, 680.f);

    closeBtnBounds = sf::FloatRect(modalBounds.left + modalBounds.width - 150.f, modalBounds.top + 32.f, 110.f, 42.f);
    previewAreaBounds = sf::FloatRect(modalBounds.left + 40.f, modalBounds.top + 132.f, 560.f, 500.f);

    transCheckboxBounds = sf::FloatRect(modalBounds.left + 624.f, modalBounds.top + 156.f, 376.f, 50.f);
    cropCheckboxBounds = sf::FloatRect(modalBounds.left + 624.f, modalBounds.top + 214.f, 376.f, 50.f);

    exportPngBtnBounds = sf::FloatRect(modalBounds.left + 624.f, modalBounds.top + 474.f, 376.f, 72.f);
    exportSheetBtnBounds = sf::FloatRect(modalBounds.left + 624.f, modalBounds.top + 560.f, 376.f, 72.f);
}

void ExportModal::open(Canvas& canvas, int frameIndex) {
    linkedCanvas = &canvas;
    activeFrame = frameIndex;
    isOpen = true;
    openTime = Theme::s_time;
    updatePreview();
}

void ExportModal::close() {
    isOpen = false;
    linkedCanvas = nullptr;
}

bool ExportModal::getIsOpen() const {
    return isOpen;
}

void ExportModal::updatePreview() {
    if (!linkedCanvas) return;

    sf::Image flatImg = ExportManager::flattenFrame(*linkedCanvas, activeFrame);
    sf::IntRect crop = autoCrop ? ExportManager::calculateAutoCrop(flatImg) : sf::IntRect(0, 0, flatImg.getSize().x, flatImg.getSize().y);
    sf::Image finalImg = ExportManager::applyCropAndBackground(flatImg, crop, transparentBg);

    bool pixelArt = linkedCanvas->getPixelMode();
    previewTex.loadFromImage(finalImg);
    previewTex.setSmooth(!pixelArt);
    previewSprite.setTexture(previewTex, true);

    exportSize = previewTex.getSize();
    float texW = static_cast<float>(std::max(1u, exportSize.x));
    float texH = static_cast<float>(std::max(1u, exportSize.y));

    float stageW = previewAreaBounds.width - kPreviewPadding * 2.f;
    float stageH = previewAreaBounds.height - kPreviewPadding - kPreviewFooter;
    float fit = std::min(stageW / texW, stageH / texH);

    // Pixel art is enlarged by whole steps so it stays sharp; everything else is never blown up.
    if (pixelArt && fit >= 1.0f) previewScale = std::floor(fit);
    else previewScale = std::min(fit, 1.0f);

    previewSprite.setScale(previewScale, previewScale);

    float px = previewAreaBounds.left + (previewAreaBounds.width - texW * previewScale) / 2.f;
    float py = previewAreaBounds.top + kPreviewPadding + (stageH - texH * previewScale) / 2.f;
    previewSprite.setPosition(std::floor(px), std::floor(py));

    size_t estimatedBytes = static_cast<size_t>(exportSize.x) * exportSize.y * 4;
    frameMegabytes = static_cast<float>(estimatedBytes) / (1024.f * 1024.f);
}

void ExportModal::exportFrames() {
    if (!linkedCanvas) return;

    if (linkedCanvas->getFrameCount() > 1) {
        std::string folder = NativeDialogs::selectFolderDialog();
        if (!folder.empty()) {
            ExportManager::exportPNGSequence(*linkedCanvas, folder, transparentBg, autoCrop);
            close();
        }
    }
    else {
        std::string file = NativeDialogs::saveFileDialog("PNG Files\0*.png\0", "png", "export.png");
        if (!file.empty()) {
            ExportManager::exportSingleImage(*linkedCanvas, activeFrame, file, transparentBg, autoCrop);
            close();
        }
    }
}

void ExportModal::exportSheet() {
    if (!linkedCanvas) return;

    std::string file = NativeDialogs::saveFileDialog("PNG Files\0*.png\0", "png", "spritesheet.png");
    if (!file.empty()) {
        ExportManager::exportSpriteSheet(*linkedCanvas, file, kSheetColumns, transparentBg, autoCrop);
        close();
    }
}

void ExportModal::updateHover(sf::Vector2f mousePos) {}

void ExportModal::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    if (!isOpen) return;

    sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
    sf::Vector2f mousePos = window.mapPixelToCoords(pixelPos);

    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Escape) close();
        else if (event.key.code == sf::Keyboard::Enter) exportFrames();
        return;
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (closeBtnBounds.contains(mousePos)) {
            close();
        }
        else if (transCheckboxBounds.contains(mousePos)) {
            transparentBg = !transparentBg;
            updatePreview();
        }
        else if (cropCheckboxBounds.contains(mousePos)) {
            autoCrop = !autoCrop;
            updatePreview();
        }
        else if (exportPngBtnBounds.contains(mousePos)) {
            exportFrames();
        }
        else if (exportSheetBtnBounds.contains(mousePos)) {
            exportSheet();
        }
    }
}

void ExportModal::draw(sf::RenderWindow& window) {
    if (!isOpen) return;

    float appear = Animation::EaseOutCubic((Theme::s_time - openTime) / 0.22f);

    sf::VertexArray scrim(sf::Quads, 4);
    sf::Color scrimTop = faded(sf::Color(12, 6, 22, 236), appear);
    sf::Color scrimBottom = faded(sf::Color(12, 6, 22, 210), appear);
    scrim[0] = sf::Vertex(sf::Vector2f(0.0f, 0.0f), scrimTop);
    scrim[1] = sf::Vertex(sf::Vector2f(1920.0f, 0.0f), scrimTop);
    scrim[2] = sf::Vertex(sf::Vector2f(1920.0f, 1080.0f), scrimBottom);
    scrim[3] = sf::Vertex(sf::Vector2f(0.0f, 1080.0f), scrimBottom);
    window.draw(scrim);

    // The whole panel rises into place; shifting the view keeps every bound below unchanged.
    sf::View savedView = window.getView();
    sf::View risingView = savedView;
    risingView.move(0.0f, -std::floor(22.0f * (1.0f - appear)));
    window.setView(risingView);

    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    const float left = modalBounds.left + 40.f;
    const float rightCol = transCheckboxBounds.left;
    const float rightColW = transCheckboxBounds.width;
    size_t frameCount = linkedCanvas ? linkedCanvas->getFrameCount() : 1;
    bool pixelArt = linkedCanvas && linkedCanvas->getPixelMode();

    Theme::DrawSunsetPanel(window, modalBounds, appear);

    // ---- Header ----
    Theme::DrawCrispText(window, font, "EXPORT STUDIO", 34, left, modalBounds.top + 50.f, Theme::SunsetGold, Theme::SunsetCoralDark, false, true);
    Theme::DrawCrispText(window, font, "CHOOSE HOW TO SAVE YOUR ARTWORK", 16, left + 2.f, modalBounds.top + 84.f, Theme::SunsetPeach, kTextShadow, false, true);

    sf::RectangleShape rule(sf::Vector2f(modalBounds.width - 80.f, 1.0f));
    rule.setPosition(left, modalBounds.top + 108.f);
    rule.setFillColor(Theme::SunsetPlum);
    window.draw(rule);

    sf::RectangleShape accent(sf::Vector2f(220.0f * Animation::EaseOutCubic((Theme::s_time - openTime - 0.08f) / 0.4f), 2.0f));
    accent.setPosition(left, modalBounds.top + 107.f);
    accent.setFillColor(Theme::SunsetAmber);
    window.draw(accent);

    Theme::DrawSunsetButton(window, closeBtnBounds, "CANCEL", font, 15, false, closeBtnBounds.contains(mPos), false, 1.0f);

    // ---- Preview ----
    const sf::FloatRect& frame = previewAreaBounds;
    sf::RectangleShape previewFrame(sf::Vector2f(frame.width, frame.height));
    previewFrame.setPosition(frame.left, frame.top);
    previewFrame.setFillColor(Theme::WithAlpha(Theme::PanelInset, 240.0f));
    previewFrame.setOutlineThickness(1.0f);
    previewFrame.setOutlineColor(Theme::SunsetPlum);
    window.draw(previewFrame);

    sf::FloatRect art = previewSprite.getGlobalBounds();
    if (art.width > 0.0f && art.height > 0.0f) {
        sf::RectangleShape artShadow(sf::Vector2f(art.width, art.height));
        artShadow.setPosition(art.left + 4.f, art.top + 6.f);
        artShadow.setFillColor(sf::Color(4, 2, 8, 170));
        window.draw(artShadow);

        // The checkerboard marks the pixels that will be see-through in the exported file.
        if (transparentBg) drawCheckerboard(window, art, 14.0f, 1.0f);
        window.draw(previewSprite);

        sf::RectangleShape artEdge(sf::Vector2f(art.width, art.height));
        artEdge.setPosition(art.left, art.top);
        artEdge.setFillColor(sf::Color::Transparent);
        artEdge.setOutlineThickness(1.5f);
        artEdge.setOutlineColor(Theme::SunsetGold);
        window.draw(artEdge);
    }
    else {
        Theme::DrawCrispText(window, font, "NOTHING TO PREVIEW", 16, frame.left + frame.width * 0.5f, frame.top + frame.height * 0.5f, Theme::TextMuted, sf::Color::Transparent, true, true);
    }

    float footerY = frame.top + frame.height - kPreviewFooter * 0.5f;
    Theme::DrawCrispText(window, font, "FRAME " + std::to_string(activeFrame + 1) + " OF " + std::to_string(frameCount), 14, frame.left + 18.f, footerY, Theme::TextSecondary, sf::Color::Transparent, false, true);

    std::string zoom = std::to_string(static_cast<int>(std::round(previewScale * 100.0f))) + "%";
    Theme::DrawCrispText(window, font, zoom, 14, frame.left + frame.width - 18.f - Theme::MeasureText(font, zoom, 14), footerY, Theme::TextMuted, sf::Color::Transparent, false, true);

    // ---- Options ----
    drawCaption(window, font, "OPTIONS", rightCol, modalBounds.top + 138.f, rightColW);
    drawToggleRow(window, font, transCheckboxBounds, "Transparent Background", transparentBg, transCheckboxBounds.contains(mPos));
    drawToggleRow(window, font, cropCheckboxBounds, "Trim Empty Edges", autoCrop, cropCheckboxBounds.contains(mPos));

    // ---- Details ----
    sf::FloatRect infoCard(rightCol, modalBounds.top + 280.f, rightColW, 178.f);
    sf::RectangleShape infoBg(sf::Vector2f(infoCard.width, infoCard.height));
    infoBg.setPosition(infoCard.left, infoCard.top);
    infoBg.setFillColor(Theme::WithAlpha(Theme::PanelInset, 240.0f));
    infoBg.setOutlineThickness(1.0f);
    infoBg.setOutlineColor(Theme::SunsetPlum);
    window.draw(infoBg);

    drawCardTitle(window, font, "EXPORT DETAILS", infoCard, 1.0f);

    char sizeBuf[24];
    if (frameMegabytes < 0.1f) std::snprintf(sizeBuf, sizeof(sizeBuf), "~%.1f KB", frameMegabytes * 1024.f);
    else std::snprintf(sizeBuf, sizeof(sizeBuf), "~%.2f MB", frameMegabytes);

    int infoRow = 0;
    auto drawInfo = [&](const std::string& label, const std::string& value, sf::Color valueColor) {
        float y = infoCard.top + 66.f + static_cast<float>(infoRow++) * 30.f;
        Theme::DrawCrispText(window, font, label, 15, infoCard.left + 22.f, y, Theme::TextMuted, sf::Color::Transparent, false, true);
        Theme::DrawCrispText(window, font, value, 16, infoCard.left + infoCard.width - 22.f - Theme::MeasureText(font, value, 16), y, valueColor, sf::Color::Transparent, false, true);
        };

    drawInfo("Dimensions", std::to_string(exportSize.x) + " x " + std::to_string(exportSize.y) + " px", Theme::SunsetGold);
    drawInfo("Frame Size", sizeBuf, Theme::TextPrimary);
    drawInfo("Timeline", std::to_string(frameCount) + (frameCount == 1 ? " Frame" : " Frames"), Theme::TextPrimary);
    drawInfo("Color Mode", pixelArt ? "Pixel Art" : "RGBA", Theme::TextPrimary);

    // ---- Export ----
    bool sequence = frameCount > 1;
    std::string pngNote = sequence ? (std::to_string(frameCount) + " numbered PNG files in a folder") : "This frame as a single image";
    drawExportButton(window, font, exportPngBtnBounds, sequence ? "EXPORT SEQUENCE" : "EXPORT PNG", pngNote, true, exportPngBtnBounds.contains(mPos));

    std::string sheetNote = sequence ? ("All frames in one image, " + std::to_string(kSheetColumns) + " per row") : "All frames in one image";
    drawExportButton(window, font, exportSheetBtnBounds, "EXPORT SPRITE SHEET", sheetNote, false, exportSheetBtnBounds.contains(mPos));

    window.setView(savedView);
}
