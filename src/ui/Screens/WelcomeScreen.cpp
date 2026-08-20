#include "WelcomeScreen.h"
#include "../UITheme.h"
#include <algorithm>
#include <cmath>
#include <random>

WelcomeScreen::WelcomeScreen()
    : currentTipIndex(0), tipTimer(0.0f), globalTime(0.0f),
    aiProviderName("None"), isAiReady(false), targetFps(60), isHwAccel(true) {}

void WelcomeScreen::init() {
    font.loadFromFile("assets/font.otf");

    heroBannerBounds = sf::FloatRect(240.f, 90.f, 1440.f, 150.f);
    telemetryPanelBounds = sf::FloatRect(1020.f, 270.f, 660.f, 440.f);
    tipPanelBounds = sf::FloatRect(1020.f, 730.f, 660.f, 170.f);
    bottomStatusStripBounds = sf::FloatRect(240.f, 920.f, 1440.f, 48.f);

    actionTiles.clear();
    actionTiles.push_back({ "new_project", "Create Canvas", "Initialize dynamic RGBA or pixel grid", "Ctrl+N", sf::FloatRect(240.f, 270.f, 740.f, 98.f), true, 0.0f, 1.0f });
    actionTiles.push_back({ "open_project", "Open Project", "Load native .wpk project archives", "Ctrl+O", sf::FloatRect(240.f, 380.f, 740.f, 98.f), false, 0.0f, 1.0f });
    actionTiles.push_back({ "sprite_studio", "Sprite Sheet Studio", "Inspect, slice & pack multi-frame sheets", "F8", sf::FloatRect(240.f, 490.f, 740.f, 98.f), false, 0.0f, 1.0f });
    actionTiles.push_back({ "config_ai", "Studio Settings & AI", "Configure model providers, keys & drivers", "ESC", sf::FloatRect(240.f, 600.f, 740.f, 98.f), false, 0.0f, 1.0f });
    actionTiles.push_back({ "tutorials", "Knowledge Codex", "Techniques, shortcut lists & guides", "F1", sf::FloatRect(240.f, 710.f, 360.f, 90.f), false, 0.0f, 1.0f });
    actionTiles.push_back({ "exit", "Exit Suite", "Close workspace safely", "Alt+F4", sf::FloatRect(620.f, 710.f, 360.f, 90.f), false, 0.0f, 1.0f });

    proTips = {
        "PRO TIP: Toggle 'Pixel Perfect' mode in Tool Properties to automatically strip jagged double-pixels when drawing fast curves.",
        "PRO TIP: Press F8 at any moment inside the workspace to hot-swap into the embedded Sprite Sheet Studio tool.",
        "PRO TIP: Hold Ctrl while dragging vector shapes to anchor expansion from the exact geometric center point.",
        "PRO TIP: Drag and drop any .png or .wav file from Windows Explorer straight onto the screen to import it into your vault.",
        "PRO TIP: Double click any gradient stop node on the Gradient Tool bar to bind it instantly to the color palette picker.",
        "PRO TIP: Use the Magic Wand tool to extract seamless polygonal lasso outlines from continuous texture islands automatically."
    };

    spawnEmbers();
}

void WelcomeScreen::spawnEmbers() {
    embers.clear();
    std::mt19937 rng(1337);
    std::uniform_real_distribution<float> distX(0.0f, 1920.0f);
    std::uniform_real_distribution<float> distY(0.0f, 1080.0f);
    std::uniform_real_distribution<float> distSpd(20.0f, 60.0f);
    std::uniform_real_distribution<float> distLife(2.0f, 6.0f);
    std::uniform_real_distribution<float> distSz(2.0f, 5.0f);

    for (int i = 0; i < 90; ++i) {
        WelcomePixelEmber e;
        e.x = distX(rng);
        e.y = distY(rng);
        e.vx = (distSpd(rng) * 0.3f) - 10.0f;
        e.vy = -distSpd(rng);
        e.size = std::floor(distSz(rng));
        e.maxLife = distLife(rng);
        e.life = (static_cast<float>(i % 100) / 100.0f) * e.maxLife;

        int variant = i % 3;
        if (variant == 0) e.color = WisdomUI::Theme::SunsetAmber;
        else if (variant == 1) e.color = WisdomUI::Theme::SunsetCoral;
        else e.color = WisdomUI::Theme::SunsetPeach;

        embers.push_back(e);
    }
}

void WelcomeScreen::updateStatus(bool configured, const std::string& provider, int fpsLimit, bool hwAccel) {
    isAiReady = configured;
    aiProviderName = configured ? provider : "Offline";
    targetFps = fpsLimit;
    isHwAccel = hwAccel;
}

void WelcomeScreen::updateHover(sf::Vector2f mousePos) {}

void WelcomeScreen::update(float dt, sf::Vector2f mousePos) {
    globalTime += dt;
    tipTimer += dt;
    if (tipTimer >= 8.0f) {
        tipTimer = 0.0f;
        currentTipIndex = (currentTipIndex + 1) % proTips.size();
    }

    for (auto& e : embers) {
        e.life += dt;
        if (e.life >= e.maxLife) {
            e.life = 0.0f;
            e.x = static_cast<float>(rand() % 1920);
            e.y = 1080.f + static_cast<float>(rand() % 50);
        }
        e.x += (e.vx + std::sin(globalTime * 2.0f + e.y * 0.01f) * 12.0f) * dt;
        e.y += e.vy * dt;
    }

    for (auto& tile : actionTiles) {
        bool hov = tile.bounds.contains(mousePos);
        tile.hoverAlpha += ((hov ? 1.0f : 0.0f) - tile.hoverAlpha) * 16.0f * dt;
        tile.scale += ((hov ? 1.02f : 1.0f) - tile.scale) * 18.0f * dt;
    }
}

void WelcomeScreen::draw(sf::RenderWindow& window) {
    for (const auto& e : embers) {
        float lifeRatio = e.life / e.maxLife;
        float alphaMult = std::sin(lifeRatio * 3.14159f);

        sf::RectangleShape p(sf::Vector2f(e.size, e.size));
        p.setPosition(std::floor(e.x), std::floor(e.y));
        sf::Color c = e.color;
        c.a = static_cast<sf::Uint8>(std::clamp(220.0f * alphaMult, 0.0f, 255.0f));
        p.setFillColor(c);
        window.draw(p);
    }

    WisdomUI::Theme::DrawSunsetPanel(window, heroBannerBounds, 1.0f);

    float crestX = heroBannerBounds.left + 54.0f;
    float crestY = heroBannerBounds.top + 75.0f;
    float pulse = std::sin(globalTime * 3.0f) * 2.0f;

    sf::ConvexShape crest(4);
    crest.setPoint(0, sf::Vector2f(0.0f, -24.0f - pulse));
    crest.setPoint(1, sf::Vector2f(24.0f + pulse, 0.0f));
    crest.setPoint(2, sf::Vector2f(0.0f, 24.0f + pulse));
    crest.setPoint(3, sf::Vector2f(-24.0f - pulse, 0.0f));
    crest.setPosition(std::floor(crestX), std::floor(crestY));
    crest.setFillColor(WisdomUI::Theme::SunsetAmber);
    crest.setOutlineThickness(2.0f);
    crest.setOutlineColor(WisdomUI::Theme::SunsetCoralDark);
    window.draw(crest);

    sf::RectangleShape innerGem(sf::Vector2f(12.0f, 12.0f));
    innerGem.setOrigin(6.0f, 6.0f);
    innerGem.setPosition(std::floor(crestX), std::floor(crestY));
    innerGem.setRotation(45.0f);
    innerGem.setFillColor(WisdomUI::Theme::SunsetGold);
    window.draw(innerGem);

    WisdomUI::Theme::DrawCrispText(window, font, "WISDOM PARK STUDIO", 36, heroBannerBounds.left + 110.0f, heroBannerBounds.top + 34.0f, WisdomUI::Theme::SunsetAmber, sf::Color(14, 6, 20));
    WisdomUI::Theme::DrawCrispText(window, font, "NEXT-GEN 2D ANIMATION & RETRO PIXEL ART SUITE", 13, heroBannerBounds.left + 114.0f, heroBannerBounds.top + 84.0f, WisdomUI::Theme::TextSecondary);

    sf::FloatRect tagBounds(heroBannerBounds.left + heroBannerBounds.width - 210.0f, heroBannerBounds.top + 45.0f, 175.0f, 32.0f);
    sf::RectangleShape tagBg(sf::Vector2f(tagBounds.width, tagBounds.height));
    tagBg.setPosition(tagBounds.left, tagBounds.top);
    tagBg.setFillColor(WisdomUI::Theme::SunsetDeepDark);
    tagBg.setOutlineThickness(1.0f);
    tagBg.setOutlineColor(WisdomUI::Theme::SunsetAmber);
    window.draw(tagBg);

    WisdomUI::Theme::DrawCrispText(window, font, "VERSION 2.6.0-PRO", 11, tagBounds.left + tagBounds.width / 2.0f, tagBounds.top + tagBounds.height / 2.0f, WisdomUI::Theme::SunsetGold, sf::Color::Transparent, true, true);

    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    for (const auto& tile : actionTiles) {
        float x = std::floor(tile.bounds.left);
        float y = std::floor(tile.bounds.top);
        float w = std::floor(tile.bounds.width);
        float h = std::floor(tile.bounds.height);

        sf::FloatRect animRect(x - ((tile.scale - 1.0f) * w * 0.5f), y - ((tile.scale - 1.0f) * h * 0.5f), w * tile.scale, h * tile.scale);
        WisdomUI::Theme::DrawSunsetPanel(window, animRect, 1.0f);

        if (tile.hoverAlpha > 0.05f) {
            sf::RectangleShape hovLine(sf::Vector2f(4.0f, animRect.height - 12.0f));
            hovLine.setPosition(animRect.left + 6.0f, animRect.top + 6.0f);
            sf::Color lc = tile.isAccent ? WisdomUI::Theme::SunsetGold : WisdomUI::Theme::SunsetAmber;
            lc.a = static_cast<sf::Uint8>(255 * tile.hoverAlpha);
            hovLine.setFillColor(lc);
            window.draw(hovLine);
        }

        sf::Color titleColor = (tile.hoverAlpha > 0.5f) ? WisdomUI::Theme::SunsetGold : (tile.isAccent ? WisdomUI::Theme::SunsetAmber : WisdomUI::Theme::TextPrimary);
        WisdomUI::Theme::DrawCrispText(window, font, tile.title, 17, animRect.left + 24.0f, animRect.top + 18.0f, titleColor, sf::Color(14, 6, 20));
        WisdomUI::Theme::DrawCrispText(window, font, tile.subtitle, 11, animRect.left + 24.0f, animRect.top + 48.0f, WisdomUI::Theme::TextSecondary);

        sf::FloatRect keyBadge(animRect.left + animRect.width - 100.0f, animRect.top + (animRect.height - 26.0f) / 2.0f, 80.0f, 26.0f);
        sf::RectangleShape badge(sf::Vector2f(keyBadge.width, keyBadge.height));
        badge.setPosition(keyBadge.left, keyBadge.top);
        badge.setFillColor(WisdomUI::Theme::SunsetDeepDark);
        badge.setOutlineThickness(1.0f);
        badge.setOutlineColor(tile.hoverAlpha > 0.5f ? WisdomUI::Theme::SunsetGold : WisdomUI::Theme::SunsetPlum);
        window.draw(badge);

        WisdomUI::Theme::DrawCrispText(window, font, tile.shortcutHint, 10, keyBadge.left + keyBadge.width / 2.0f, keyBadge.top + keyBadge.height / 2.0f, WisdomUI::Theme::SunsetPeach, sf::Color::Transparent, true, true);
    }

    WisdomUI::Theme::DrawSunsetPanel(window, telemetryPanelBounds, 1.0f);

    sf::FloatRect tHeader(telemetryPanelBounds.left + 12.0f, telemetryPanelBounds.top + 10.0f, telemetryPanelBounds.width - 24.0f, 30.0f);
    sf::RectangleShape thBg(sf::Vector2f(tHeader.width, tHeader.height));
    thBg.setPosition(tHeader.left, tHeader.top);
    thBg.setFillColor(WisdomUI::Theme::SunsetDeepDark);
    thBg.setOutlineThickness(1.0f);
    thBg.setOutlineColor(WisdomUI::Theme::SunsetPlum);
    window.draw(thBg);

    WisdomUI::Theme::DrawCrispText(window, font, ":: STUDIO TELEMETRY & DRIVER STATUS ::", 11, tHeader.left + tHeader.width / 2.0f, tHeader.top + tHeader.height / 2.0f, WisdomUI::Theme::SunsetAmber, sf::Color(14, 6, 20), true, true);

    auto drawDiagnosticRow = [&](float rowY, const std::string& label, const std::string& val, sf::Color valColor, bool statusLed) {
        sf::FloatRect rBounds(telemetryPanelBounds.left + 16.0f, rowY, telemetryPanelBounds.width - 32.0f, 44.0f);
        sf::RectangleShape rBg(sf::Vector2f(rBounds.width, rBounds.height));
        rBg.setPosition(rBounds.left, rBounds.top);
        rBg.setFillColor(WisdomUI::Theme::SunsetSkyTop);
        rBg.setOutlineThickness(1.0f);
        rBg.setOutlineColor(WisdomUI::Theme::SunsetPlum);
        window.draw(rBg);

        sf::CircleShape led(4.0f);
        led.setPosition(rBounds.left + 12.0f, rBounds.top + 18.0f);
        led.setFillColor(statusLed ? sf::Color(80, 240, 120) : sf::Color(240, 80, 80));
        window.draw(led);

        WisdomUI::Theme::DrawCrispText(window, font, label, 12, rBounds.left + 30.0f, rBounds.top + 14.0f, WisdomUI::Theme::TextPrimary);
        WisdomUI::Theme::DrawCrispText(window, font, val, 12, rBounds.left + rBounds.width - 16.0f, rBounds.top + 14.0f, valColor, sf::Color::Transparent, false, false);
        };

    float dy = telemetryPanelBounds.top + 55.0f;
    drawDiagnosticRow(dy, "AI Core Driver Engine", isAiReady ? (aiProviderName + " [ACTIVE]") : "DISCONNECTED", isAiReady ? WisdomUI::Theme::SunsetGold : WisdomUI::Theme::SunsetCoral, isAiReady);
    dy += 54.0f;
    drawDiagnosticRow(dy, "Graphics Pipeline", isHwAccel ? "Direct Acceleration ON" : "Software Fallback", WisdomUI::Theme::SunsetPeach, isHwAccel);
    dy += 54.0f;
    drawDiagnosticRow(dy, "Target Frame Limiter", std::to_string(targetFps) + " FPS Stable", WisdomUI::Theme::SunsetGold, true);
    dy += 54.0f;
    drawDiagnosticRow(dy, "Audio Subsystem (Safe Mode)", "PCM Streaming Active", WisdomUI::Theme::SunsetPeach, true);
    dy += 54.0f;
    drawDiagnosticRow(dy, "Workspace State", "Idle - Ready for Canvas", WisdomUI::Theme::TextSecondary, true);
    dy += 54.0f;
    drawDiagnosticRow(dy, "Input Matrix", "Keyboard + Mouse + Pen Tablet", WisdomUI::Theme::SunsetGold, true);

    WisdomUI::Theme::DrawSunsetPanel(window, tipPanelBounds, 1.0f);

    sf::FloatRect tipGrip(tipPanelBounds.left + 12.0f, tipPanelBounds.top + 10.0f, tipPanelBounds.width - 24.0f, 26.0f);
    sf::RectangleShape tipBg(sf::Vector2f(tipGrip.width, tipGrip.height));
    tipBg.setPosition(tipGrip.left, tipGrip.top);
    tipBg.setFillColor(WisdomUI::Theme::SunsetDeepDark);
    tipBg.setOutlineThickness(1.0f);
    tipBg.setOutlineColor(WisdomUI::Theme::SunsetPlum);
    window.draw(tipBg);

    WisdomUI::Theme::DrawCrispText(window, font, "CREATIVE WORKFLOW ASSISTANT", 11, tipGrip.left + tipGrip.width / 2.0f, tipGrip.top + tipGrip.height / 2.0f, WisdomUI::Theme::SunsetGold, sf::Color(14, 6, 20), true, true);

    if (currentTipIndex >= 0 && currentTipIndex < static_cast<int>(proTips.size())) {
        sf::Text tipTxt(proTips[currentTipIndex], font, 11);
        tipTxt.setPosition(tipPanelBounds.left + 20.0f, tipPanelBounds.top + 50.0f);
        tipTxt.setFillColor(WisdomUI::Theme::TextPrimary);
        tipTxt.setLineSpacing(1.4f);
        window.draw(tipTxt);
    }

    WisdomUI::Theme::DrawSunsetPanel(window, bottomStatusStripBounds, 1.0f);

    WisdomUI::Theme::DrawCrispText(window, font, "QUICK LAUNCH: [B] Brush  |  [P] Pencil  |  [E] Eraser  |  [G] Gradient  |  [U] Shapes  |  [W] Magic Wand  |  [Ctrl+S] Save", 11, bottomStatusStripBounds.left + bottomStatusStripBounds.width / 2.0f, bottomStatusStripBounds.top + bottomStatusStripBounds.height / 2.0f, WisdomUI::Theme::SunsetPeach, sf::Color(14, 6, 20), true, true);
}

std::string WelcomeScreen::handleClick(sf::Vector2f mousePos) {
    for (const auto& tile : actionTiles) {
        if (tile.bounds.contains(mousePos)) {
            return tile.id;
        }
    }
    return "";
}