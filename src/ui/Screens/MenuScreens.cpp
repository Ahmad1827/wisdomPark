// Start-menu sub-screens: settings, tutorials and credits.
// Geometry comes from MenuLayouts.h, which the click handling in UIManager.cpp shares.
#include "../UIManager.h"
#include "../UITheme.h"
#include "MenuLayouts.h"
#include "MenuWidgets.h"
#include "../../ai/AIManager.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

extern int g_resW;
extern int g_resH;
extern bool g_resDropdownOpen;
extern bool g_typingApiKey;

using WisdomUI::Theme;
using WisdomUI::Animation;

using namespace MenuWidgets;

void UIManager::drawSubScreenHeader(sf::RenderWindow& window, const std::string& title, const std::string& subtitle, float time) {
    sf::VertexArray scrim(sf::Quads, 4);
    scrim[0] = sf::Vertex(sf::Vector2f(0.0f, 0.0f), sf::Color(12, 6, 22, 232));
    scrim[1] = sf::Vertex(sf::Vector2f(1920.0f, 0.0f), sf::Color(12, 6, 22, 232));
    scrim[2] = sf::Vertex(sf::Vector2f(1920.0f, 1080.0f), sf::Color(12, 6, 22, 196));
    scrim[3] = sf::Vertex(sf::Vector2f(0.0f, 1080.0f), sf::Color(12, 6, 22, 196));
    window.draw(scrim);

    drawFireflies(window);

    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    sf::FloatRect back = MenuLayout::BackButton();
    Theme::DrawSunsetButton(window, back, "<  BACK", font, 16, false, back.contains(mousePos), false, 1.0f);

    float a = Animation::EaseOutCubic(time / 0.4f);
    float x = MenuLayout::kContentLeft - 24.0f * (1.0f - a);

    Theme::DrawCrispText(window, font, title, 60, x + 4.0f, 164.0f, faded(sf::Color(10, 4, 18, 200), a), sf::Color::Transparent, false, true);
    Theme::DrawCrispText(window, font, title, 60, x, 160.0f, faded(Theme::SunsetGold, a), faded(Theme::SunsetCoralDark, a), false, true);
    Theme::DrawCrispText(window, font, subtitle, 18, MenuLayout::kContentLeft + 2.0f, 206.0f, faded(Theme::SunsetPeach, a), faded(kTextShadow, a), false, true);

    sf::RectangleShape rule(sf::Vector2f(MenuLayout::kContentWidth, 1.0f));
    rule.setPosition(MenuLayout::kContentLeft, 238.0f);
    rule.setFillColor(Theme::SunsetPlum);
    window.draw(rule);

    sf::RectangleShape accent(sf::Vector2f(320.0f * Animation::EaseOutCubic((time - 0.1f) / 0.4f), 2.0f));
    accent.setPosition(MenuLayout::kContentLeft, 237.0f);
    accent.setFillColor(Theme::SunsetAmber);
    window.draw(accent);
}

void UIManager::drawSettingsMenu(sf::RenderWindow& window) {
    using namespace MenuLayout;

    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    drawSubScreenHeader(window, "SETTINGS", "DISPLAY, STORAGE, PERFORMANCE AND AI", m_screenTime);

    auto sectionAlpha = [&](int section) {
        return Animation::EaseOutCubic((m_screenTime - 0.08f - 0.07f * static_cast<float>(section)) / 0.4f);
        };

    for (int s = 0; s < kSettingSectionCount; ++s) {
        float a = sectionAlpha(s);
        if (a <= 0.0f) continue;
        sf::FloatRect card = SettingsSection(s);
        drawCard(window, card, 0.0f, a);
        drawCardTitle(window, font, kSettingSections[s], card, a);
    }

    AIManager& ai = AIManager::getInstance();
    sf::FloatRect resolutionButton;

    for (const SettingRow& row : kSettingRows) {
        float a = sectionAlpha(row.section);
        if (a <= 0.0f) continue;

        sf::FloatRect r = SettingsRowBounds(row);
        float centerY = r.top + r.height * 0.5f;
        bool interactive = (row.kind != SettingKind::Info);
        bool hovered = interactive && !g_resDropdownOpen && r.contains(mousePos);
        float t = Theme::AnimateHover(r, hovered);

        if (t > 0.01f) {
            sf::RectangleShape wash(sf::Vector2f(r.width, r.height));
            wash.setPosition(r.left, r.top);
            wash.setFillColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(16.0f * t * a)));
            window.draw(wash);
        }

        sf::Color labelCol = interactive ? Theme::Mix(Theme::TextPrimary, Theme::SunsetGold, t) : Theme::TextSecondary;
        Theme::DrawCrispText(window, font, row.label, 19, r.left + 14.0f, centerY, faded(labelCol, a), faded(kTextShadow, a), false, true);

        std::string value;
        bool toggleOn = false;
        switch (row.id) {
        case SettingId::Fullscreen: toggleOn = uiFullscreen; break;
        case SettingId::VSync: toggleOn = uiVsync; break;
        case SettingId::AutoBackup: toggleOn = uiAutoBackup; break;
        case SettingId::HwAccel: toggleOn = uiHwAccel; break;
        case SettingId::FpsLimit: value = std::to_string(uiFpsLimit) + " FPS"; break;
        case SettingId::Resolution: value = std::to_string(g_resW) + " x " + std::to_string(g_resH); break;
        case SettingId::Theme: value = "Sunset"; break;
        case SettingId::Autosave: value = "5 Mins"; break;
        case SettingId::VaultDir: value = "/Projects"; break;
        case SettingId::ExportFormat: value = "PNG / Sheet"; break;
        case SettingId::PreviewRate: value = std::to_string(uiAnimFps) + " FPS"; break;
        case SettingId::UndoHistory: value = std::to_string(uiHistorySize) + " Steps"; break;
        case SettingId::GridContrast: value = "High"; break;
        case SettingId::AiProvider: value = ai.getActiveProvider(); break;
        case SettingId::ApiKey: break;
        }

        switch (row.kind) {
        case SettingKind::Toggle: {
            // The knob slides between the two ends; AnimateHover doubles as the tween.
            sf::FloatRect sw = ToggleSwitch(r);
            float on = Theme::AnimateHover(sw, toggleOn);

            sf::ConvexShape track = Theme::ChamferedRect(sw.left, sw.top, sw.width, sw.height, 4.0f);
            track.setFillColor(faded(Theme::Mix(Theme::PanelInset, Theme::SunsetAmber, on), a));
            track.setOutlineThickness(1.0f);
            track.setOutlineColor(faded(Theme::Mix(Theme::SunsetPlum, Theme::SunsetGold, on), a));
            window.draw(track);

            const float knob = sw.height - 8.0f;
            sf::RectangleShape knobShape(sf::Vector2f(knob, knob));
            knobShape.setPosition(std::floor(sw.left + 4.0f + on * (sw.width - knob - 8.0f)), sw.top + 4.0f);
            knobShape.setFillColor(faded(Theme::Mix(Theme::TextMuted, Theme::SunsetDeepDark, on), a));
            window.draw(knobShape);

            const char* state = toggleOn ? "ON" : "OFF";
            float stateW = Theme::MeasureText(font, state, 15);
            Theme::DrawCrispText(window, font, state, 15, sw.left - stateW - 14.0f, centerY, faded(toggleOn ? Theme::SunsetGold : Theme::TextMuted, a), sf::Color::Transparent, false, true);
            break;
        }
        case SettingKind::Stepper: {
            sf::FloatRect left = StepperLeft(r);
            sf::FloatRect right = StepperRight(r);
            sf::FloatRect box = StepperValue(r);

            sf::RectangleShape valueBox(sf::Vector2f(box.width, box.height));
            valueBox.setPosition(box.left, box.top);
            valueBox.setFillColor(faded(Theme::PanelInset, a));
            valueBox.setOutlineThickness(1.0f);
            valueBox.setOutlineColor(faded(Theme::SunsetPlum, a));
            window.draw(valueBox);
            Theme::DrawCrispText(window, font, value, 17, box.left + box.width * 0.5f, centerY, faded(Theme::SunsetGold, a), sf::Color::Transparent, true, true);

            if (a > 0.6f) {
                Theme::DrawSunsetButton(window, left, "<", font, 18, false, left.contains(mousePos) && !g_resDropdownOpen, false, 1.0f);
                Theme::DrawSunsetButton(window, right, ">", font, 18, false, right.contains(mousePos) && !g_resDropdownOpen, false, 1.0f);
            }
            break;
        }
        case SettingKind::Dropdown: {
            resolutionButton = DropdownButton(r);
            if (a > 0.6f) {
                Theme::DrawSunsetButton(window, resolutionButton, value + (g_resDropdownOpen ? "   ^" : "   v"), font, 17, false, resolutionButton.contains(mousePos), g_resDropdownOpen, 1.0f);
            }
            break;
        }
        case SettingKind::Info: {
            float valueW = Theme::MeasureText(font, value, 17);
            Theme::DrawCrispText(window, font, value, 17, r.left + r.width - valueW - 18.0f, centerY, faded(Theme::TextMuted, a), sf::Color::Transparent, false, true);
            break;
        }
        case SettingKind::TextField: {
            std::string keyDisplay = ai.getApiKey(ai.getActiveProvider());
            if (keyDisplay.empty()) keyDisplay = g_typingApiKey ? "" : "Click to enter key...";
            else keyDisplay = std::string(std::min(static_cast<size_t>(16), keyDisplay.length()), '*');
            if (g_typingApiKey) keyDisplay += "_";

            sf::FloatRect field = TextField(r);
            sf::RectangleShape box(sf::Vector2f(field.width, field.height));
            box.setPosition(field.left, field.top);
            box.setFillColor(faded(Theme::PanelInset, a));
            box.setOutlineThickness(g_typingApiKey ? 1.5f : 1.0f);
            box.setOutlineColor(faded(g_typingApiKey ? Theme::SunsetGold : Theme::Mix(Theme::SunsetPlum, Theme::SunsetPeach, t), a));
            window.draw(box);

            Theme::DrawCrispText(window, font, keyDisplay, 16, field.left + 14.0f, centerY, faded(g_typingApiKey ? Theme::SunsetGold : Theme::SunsetPeach, a), sf::Color::Transparent, false, true);
            break;
        }
        }
    }

    // Drawn last so the open list sits above the rows beneath it.
    if (g_resDropdownOpen) {
        sf::FloatRect first = DropdownOption(resolutionButton, 0);
        sf::FloatRect menu(first.left, first.top, first.width, first.height * static_cast<float>(kResolutionOptionCount));
        Theme::DrawSunsetPanel(window, menu, 1.0f);

        for (int i = 0; i < kResolutionOptionCount; ++i) {
            sf::FloatRect opt = DropdownOption(resolutionButton, i);
            bool current = (kResolutionOptions[i][0] == g_resW && kResolutionOptions[i][1] == g_resH);
            float t = Theme::AnimateHover(opt, opt.contains(mousePos));

            if (t > 0.01f) {
                sf::RectangleShape hover(sf::Vector2f(opt.width - 8.0f, opt.height - 6.0f));
                hover.setPosition(opt.left + 4.0f, opt.top + 3.0f);
                hover.setFillColor(Theme::WithAlpha(Theme::SunsetPlum, 220.0f * t));
                window.draw(hover);
            }

            std::string label = std::to_string(kResolutionOptions[i][0]) + " x " + std::to_string(kResolutionOptions[i][1]);
            sf::Color col = current ? Theme::SunsetAmber : Theme::Mix(Theme::TextPrimary, Theme::SunsetGold, t);
            Theme::DrawCrispText(window, font, label, 17, opt.left + 16.0f, opt.top + opt.height * 0.5f, col, kTextShadow, false, true);
        }
    }
}

void UIManager::drawTutorialsMenu(sf::RenderWindow& window) {
    using namespace MenuLayout;

    static const std::pair<const char*, const char*> topics[kTutorialCount] = {
        {"Getting Started", "Canvas setup, drawing tools and basic workspace navigation."},
        {"Drawing & Inking", "Smooth brush, retro pixel pencil, eraser and symmetry guides."},
        {"Timeline & Frames", "Frame duplication, playback speed, timing and onion skinning."},
        {"Layer Management", "Blend modes, opacity sliders, visibility and layer merging."},
        {"Selection & Transform", "Isolate, drag, flip, duplicate and transform selections."},
        {"Pixel Perfect Mode", "Strip jagged pixel doublets for authentic crisp retro lines."},
        {"Keyboard Matrix", "Speed up animation workflows with custom keys and shortcuts."},
        {"Exporting Studio", "Render sequential PNG sequences and packed sprite sheets."},
        {"AI Co-Pilot Suite", "Generate palette suggestions, shading ramps and variations."}
    };

    static const char* fullText[kTutorialCount] = {
        "Wisdom Park is built for high-speed 2D animation and pixel art.\n\n"
        "- Create a new project from the launchpad or press Ctrl+N.\n"
        "- Use the left toolbar for drawing tools, or hit 1-9 on your number row.\n"
        "- Press Space or Tab to toggle the bottom timeline and preview sequences.",

        "Select the Brush or Pencil tool (1 or 2, or B / P).\n\n"
        "- Brush: Smooth anti-aliased strokes with dynamic radius scaling.\n"
        "- Pencil: Strict grid-locked pixels ideal for retro sprite craft.\n"
        "- Right-Click / Middle-Click: Pan around the canvas smoothly.",

        "The Timeline bar manages animation frames.\n\n"
        "- Press Shift+N or click '+' to duplicate/add a new frame.\n"
        "- Press Space to toggle real-time playback.\n"
        "- Press O to toggle Onion Skinning for reference silhouettes.",

        "Layers isolate independent art components.\n\n"
        "- Open the Layer Panel on the right dock to add or reorder layers.\n"
        "- Toggle visibility or lock layers to protect your strokes.\n"
        "- Adjust layer opacity sliders for shading and lighting.",

        "Use the Selection Tool (5 or M) to isolate artwork.\n\n"
        "- Once selected, drag pixels anywhere across the canvas.\n"
        "- Press H or V to flip selections horizontally or vertically.\n"
        "- Press Delete to wipe the selected pixels instantly.",

        "Pixel Mode disables smoothing and enforces strict tile alignments.\n\n"
        "- Enable 'Pixel Perfect' in the Tool Options bar to automatically clean\n"
        "  up redundant double-corner pixels on fast strokes.\n"
        "- Use Tile Mode to test seamlessly repeating environment textures.",

        "Every major studio action has a dedicated shortcut.\n\n"
        "- Use the number keys (1 to 0, -, =) for instant tool swapping.\n"
        "- Press K or open Keybinds from the launchpad to customize them.\n"
        "- Undo with Ctrl+Z, redo with Ctrl+Y, save with Ctrl+S.",

        "When your timeline animation is complete, open Export Studio (Ctrl+E).\n\n"
        "- Output transparent individual PNG sequences for video editors.\n"
        "- Pack the entire timeline into compact sprite sheets ready for game engines.",

        "AI Co-Pilot features require an active API key in Settings.\n\n"
        "- Open the Color Assistant to generate 4-color shading ramps.\n"
        "- Import palettes from Lospec links or paste hex codes directly.\n"
        "- All active palettes can be pinned and saved to your studio presets."
    };

    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    drawSubScreenHeader(window, "TUTORIALS", "NINE SHORT GUIDES TO THE STUDIO", m_screenTime);

    if (activeTutorialIndex < 0 || activeTutorialIndex >= kTutorialCount) {
        for (int i = 0; i < kTutorialCount; ++i) {
            float a = Animation::EaseOutCubic((m_tutorialTime - 0.05f - 0.04f * static_cast<float>(i)) / 0.4f);
            if (a <= 0.0f) continue;

            sf::FloatRect bounds = TutorialCard(i);
            float t = Theme::AnimateHover(bounds, bounds.contains(mousePos));
            sf::FloatRect card = shifted(bounds, 0.0f, 18.0f * (1.0f - a) - 4.0f * t);
            drawCard(window, card, t, a);

            std::string number = twoDigits(i + 1);
            float numberW = Theme::MeasureText(font, number, 46);
            Theme::DrawCrispText(window, font, number, 46, card.left + card.width - numberW - 26.0f, card.top + 52.0f, faded(Theme::Mix(Theme::SunsetPlum, Theme::SunsetAmber, t), a), sf::Color::Transparent, false, true);

            sf::RectangleShape tick(sf::Vector2f(36.0f + 20.0f * t, 3.0f));
            tick.setPosition(card.left + 26.0f, card.top + 28.0f);
            tick.setFillColor(faded(Theme::SunsetAmber, a));
            window.draw(tick);

            Theme::DrawCrispText(window, font, topics[i].first, 25, card.left + 26.0f, card.top + 68.0f, faded(Theme::Mix(Theme::TextPrimary, Theme::SunsetGold, t), a), faded(kTextShadow, a), false, true);
            Theme::DrawCrispText(window, font, topics[i].second, 16, card.left + 26.0f, card.top + 110.0f, faded(Theme::TextSecondary, a), sf::Color::Transparent, false, true);

            Theme::DrawCrispText(window, font, "READ GUIDE  >", 15, card.left + 26.0f + 8.0f * t, card.top + card.height - 34.0f, faded(Theme::Mix(Theme::SunsetPeach, Theme::SunsetGold, t), a), sf::Color::Transparent, false, true);
        }
        return;
    }

    // ---- Open guide: chapter list on the left, text on the right ----
    for (int i = 0; i < kTutorialCount; ++i) {
        sf::FloatRect item = TutorialNavItem(i);
        bool active = (i == activeTutorialIndex);
        float t = Theme::AnimateHover(item, item.contains(mousePos));
        float centerY = item.top + item.height * 0.5f;

        if (active || t > 0.01f) {
            sf::VertexArray wash(sf::Quads, 4);
            sf::Color washStart = Theme::WithAlpha(Theme::SunsetAmber, active ? 70.0f : 46.0f * t);
            sf::Color washEnd = Theme::WithAlpha(Theme::SunsetAmber, 0.0f);
            wash[0] = sf::Vertex(sf::Vector2f(item.left, item.top), washStart);
            wash[1] = sf::Vertex(sf::Vector2f(item.left + item.width, item.top), washEnd);
            wash[2] = sf::Vertex(sf::Vector2f(item.left + item.width, item.top + item.height), washEnd);
            wash[3] = sf::Vertex(sf::Vector2f(item.left, item.top + item.height), washStart);
            window.draw(wash);
        }
        if (active) {
            sf::RectangleShape bar(sf::Vector2f(4.0f, item.height - 12.0f));
            bar.setPosition(item.left, item.top + 6.0f);
            bar.setFillColor(Theme::SunsetAmber);
            window.draw(bar);
        }

        Theme::DrawCrispText(window, font, twoDigits(i + 1), 15, item.left + 18.0f, centerY, active ? Theme::SunsetAmber : Theme::TextMuted, sf::Color::Transparent, false, true);
        sf::Color nameCol = active ? Theme::SunsetGold : Theme::Mix(Theme::TextPrimary, Theme::SunsetGold, t);
        Theme::DrawCrispText(window, font, topics[i].first, 19, item.left + 58.0f + 6.0f * t, centerY, nameCol, kTextShadow, false, true);
    }

    float ca = Animation::EaseOutCubic(m_tutorialTime / 0.35f);
    sf::FloatRect content = shifted(TutorialContent(), 20.0f * (1.0f - ca), 0.0f);
    drawCard(window, content, 0.0f, ca);

    const float textX = content.left + 44.0f;
    Theme::DrawCrispText(window, font, "CHAPTER " + twoDigits(activeTutorialIndex + 1), 15, textX, content.top + 46.0f, faded(Theme::SunsetAmber, ca), sf::Color::Transparent, false, true);
    Theme::DrawCrispText(window, font, topics[activeTutorialIndex].first, 40, textX, content.top + 92.0f, faded(Theme::SunsetGold, ca), faded(kTextShadow, ca), false, true);
    Theme::DrawCrispText(window, font, topics[activeTutorialIndex].second, 17, textX, content.top + 136.0f, faded(Theme::TextSecondary, ca), sf::Color::Transparent, false, true);

    sf::RectangleShape rule(sf::Vector2f(content.width - 88.0f, 1.0f));
    rule.setPosition(textX, content.top + 168.0f);
    rule.setFillColor(Theme::WithAlpha(Theme::SunsetPlum, 255.0f * ca));
    window.draw(rule);

    // Body: plain lines are paragraphs, "- " lines become bullets, indented lines continue a bullet.
    std::istringstream body(fullText[activeTutorialIndex]);
    std::string line;
    float y = content.top + 214.0f;
    while (std::getline(body, line)) {
        if (line.empty()) {
            y += 20.0f;
            continue;
        }
        bool bullet = (line.rfind("- ", 0) == 0);
        bool continuation = (line.rfind("  ", 0) == 0);
        if (bullet) {
            drawDiamond(window, textX + 8.0f, y, 5.0f, faded(Theme::SunsetAmber, ca));
            line = line.substr(2);
        }
        else if (continuation) {
            line = line.substr(2);
        }
        float indent = (bullet || continuation) ? 32.0f : 0.0f;
        Theme::DrawCrispText(window, font, line, 21, textX + indent, y, faded(bullet || continuation ? Theme::TextPrimary : Theme::TextSecondary, ca), faded(kTextShadow, ca), false, true);
        y += 44.0f;
    }

    sf::FloatRect returnBtn = TutorialReturnButton();
    Theme::DrawSunsetButton(window, returnBtn, "ALL GUIDES", font, 16, false, returnBtn.contains(mousePos), false, 1.0f);
}

void UIManager::drawCreditsMenu(sf::RenderWindow& window) {
    using namespace MenuLayout;

    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    drawSubScreenHeader(window, "CREDITS", "WHO BUILT WISDOM PARK, AND WITH WHAT", m_screenTime);

    auto appear = [&](float delay) { return Animation::EaseOutCubic((m_screenTime - delay) / 0.4f); };

    // ---- Author ----
    float authorA = appear(0.08f);
    if (authorA > 0.0f) {
        sf::FloatRect card = shifted(CreditsAuthor(), -20.0f * (1.0f - authorA), 0.0f);
        drawCard(window, card, 0.0f, authorA);

        const float x = card.left + 40.0f;
        float pulse = 0.5f + 0.5f * std::sin(m_screenTime * 2.0f);
        drawDiamond(window, x + 26.0f, card.top + 82.0f, 30.0f + 3.0f * pulse, Theme::WithAlpha(Theme::SunsetAmber, 60.0f * authorA));
        drawDiamond(window, x + 26.0f, card.top + 82.0f, 24.0f, faded(Theme::SunsetAmber, authorA));
        drawDiamond(window, x + 26.0f, card.top + 82.0f, 13.0f, faded(Theme::SunsetDeepDark, authorA));

        Theme::DrawCrispText(window, font, "LEAD DEVELOPER & ARCHITECT", 15, x + 76.0f, card.top + 62.0f, faded(Theme::SunsetAmber, authorA), sf::Color::Transparent, false, true);
        Theme::DrawCrispText(window, font, "AHMAD ARNAOUTE", 40, x + 76.0f, card.top + 100.0f, faded(Theme::SunsetGold, authorA), faded(kTextShadow, authorA), false, true);

        const char* handle = "ATODDEV";
        float handleW = Theme::MeasureText(font, handle, 15) + 22.0f;
        sf::RectangleShape tag(sf::Vector2f(handleW, 26.0f));
        tag.setPosition(x, card.top + 150.0f);
        tag.setFillColor(sf::Color::Transparent);
        tag.setOutlineThickness(1.0f);
        tag.setOutlineColor(faded(Theme::SunsetViolet, authorA));
        window.draw(tag);
        Theme::DrawCrispText(window, font, handle, 15, x + handleW * 0.5f, card.top + 163.0f, faded(Theme::TextSecondary, authorA), sf::Color::Transparent, true, true);

        sf::RectangleShape rule(sf::Vector2f(card.width - 80.0f, 1.0f));
        rule.setPosition(x, card.top + 204.0f);
        rule.setFillColor(Theme::WithAlpha(Theme::SunsetPlum, 255.0f * authorA));
        window.draw(rule);

        const std::pair<const char*, const char*> facts[2] = {
            { "ROLE", "Engine, Core Systems & UI Architecture" },
            { "SPECIALIZATION", "High-Performance Pixel Pipeline" }
        };
        float y = card.top + 246.0f;
        for (const auto& fact : facts) {
            Theme::DrawCrispText(window, font, fact.first, 14, x, y, faded(Theme::TextMuted, authorA), sf::Color::Transparent, false, true);
            Theme::DrawCrispText(window, font, fact.second, 20, x, y + 32.0f, faded(Theme::TextPrimary, authorA), faded(kTextShadow, authorA), false, true);
            y += 92.0f;
        }
    }

    // ---- Lists ----
    struct Block {
        const char* title;
        bool bullets;
        std::vector<const char*> lines;
    };
    const Block blocks[3] = {
        { "ACADEMIC AFFILIATION", false, {
            "POLITEHNICA University of Bucharest",
            "Faculty of Automatic Control and Computers",
            "Group: 324CD",
            "Bucharest, Romania" } },
        { "TECHNOLOGY STACK", true, {
            "SFML 2.6.x (Graphics, Window, Systems)",
            "nlohmann::json (Data Architecture)",
            "C++17 Standard Compliant Systems",
            "Commercial & Open Studio License" } },
        { "PROJECTS & CONTRIBUTIONS", true, {
            "Oppia Foundation (Testing Architecture)",
            "safe-comment-stripper (CLI Utility)",
            "iMeditatii Education Platform",
            "Progress & Progress Aggregator",
            "Wisdom Park Retro Studio Suite" } }
    };

    for (int i = 0; i < 3; ++i) {
        float a = appear(0.16f + 0.08f * static_cast<float>(i));
        if (a <= 0.0f) continue;

        sf::FloatRect card = shifted(CreditsBlock(i), 20.0f * (1.0f - a), 0.0f);
        drawCard(window, card, 0.0f, a);
        drawCardTitle(window, font, blocks[i].title, card, a);

        float y = card.top + 80.0f;
        for (const char* line : blocks[i].lines) {
            float textX = card.left + 24.0f;
            if (blocks[i].bullets) {
                drawDiamond(window, textX + 6.0f, y, 4.0f, faded(Theme::SunsetAmber, a));
                textX += 26.0f;
            }
            Theme::DrawCrispText(window, font, line, 18, textX, y, faded(Theme::TextPrimary, a), faded(kTextShadow, a), false, true);
            y += 38.0f;
        }
    }

    // ---- System core (click to calibrate) ----
    float eggA = appear(0.4f);
    if (eggA > 0.0f) {
        sf::FloatRect egg = CreditsEgg();
        bool unlocked = (easterEggClicks >= 5);
        float t = Theme::AnimateHover(egg, egg.contains(mousePos));
        drawCard(window, egg, std::max(t, unlocked ? 0.6f : 0.0f), eggA);

        std::string coreStatus = unlocked ? "ENGINE CORE MATRIX UNLOCKED (DEBUG MODE ACTIVE)" : "WISDOM PARK STUDIO SYSTEM CORE (CLICK TO CALIBRATE)";
        float centerX = egg.left + egg.width * 0.5f;
        Theme::DrawCrispText(window, font, coreStatus, 20, centerX, egg.top + 40.0f, faded(unlocked ? Theme::SunsetGold : Theme::Mix(Theme::SunsetAmber, Theme::SunsetGold, t), eggA), faded(kTextShadow, eggA), true, true);
        Theme::DrawCrispText(window, font, "Build: 2026.8  |  Direct Hardware Render Pipeline Online", 15, centerX, egg.top + 74.0f, faded(Theme::TextSecondary, eggA), sf::Color::Transparent, true, true);
    }
}
