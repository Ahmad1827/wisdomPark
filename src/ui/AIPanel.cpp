#include "AIPanel.h"
#include "../UI/UITheme.h"
#include <algorithm>
#include <cmath>

AIPanel::AIPanel()
    : position(64.f, 78.f),
    size(320.f, 510.f),
    isVisible(false),
    isDraggingPanel(false),
    currentTab(AssistantTab::Tools),
    isPixelMode(true),
    lightingGuideActive(false),
    paletteCheckActive(false),
    ghostOverlayActive(false),
    currentLessonIdx(0),
    currentStepIdx(0) {}

void AIPanel::init() {
    font.loadFromFile("assets/font.otf");
    position = sf::Vector2f(64.f, 78.f);

    lessons.clear();

    TutorialLesson potion;
    potion.name = "Health Potion";
    potion.steps = {
        { "1. Bottle Outline", "Draw the rounded flask and neck silhouette using a dark charcoal outline.", sf::Color(40, 30, 45) },
        { "2. Base Liquid", "Fill the lower reservoir with a vibrant ruby red tone.", sf::Color(215, 40, 75) },
        { "3. Glass Highlights", "Place sharp 1px white highlights on the upper left curve of the glass.", sf::Color(255, 255, 255) },
        { "4. Cork & Shadows", "Add a warm wooden cork on top and shade the bottom right with deep burgundy.", sf::Color(110, 20, 45) }
    };
    lessons.push_back(potion);

    TutorialLesson sword;
    sword.name = "Iron Shortsword";
    sword.steps = {
        { "1. Diagonal Blade", "Trace a 45-degree diagonal line for the core blade spine.", sf::Color(70, 80, 95) },
        { "2. Edge Fill", "Draw lighter steel along the upper edge and darker gunmetal along the bottom.", sf::Color(170, 185, 200) },
        { "3. Guard & Grip", "Add a perpendicular crossguard in bronze and a 3px leather handle.", sf::Color(180, 110, 45) },
        { "4. Glint & Contrast", "Place a single pure white glint pixel near the tip of the blade.", sf::Color(255, 255, 255) }
    };
    lessons.push_back(sword);

    TutorialLesson tree;
    tree.name = "Pine Tree";
    tree.steps = {
        { "1. Trunk Spine", "Place a straight 3px wide vertical column in dark timber brown.", sf::Color(65, 40, 30) },
        { "2. Canopy Tiers", "Block out three stacked triangular silhouettes in dark forest green.", sf::Color(25, 75, 40) },
        { "3. Toplit Needles", "Stipple pale olive green pixels onto the top edges facing the sky.", sf::Color(85, 160, 65) },
        { "4. Underbrush Shadow", "Add deep blue-green under each foliage shelf for depth.", sf::Color(15, 40, 35) }
    };
    lessons.push_back(tree);
}

void AIPanel::toggle() {
    isVisible = !isVisible;
}

bool AIPanel::getIsVisible() const {
    return isVisible;
}

void AIPanel::setPixelMode(bool pixelMode) {
    isPixelMode = pixelMode;
}

bool AIPanel::getPixelMode() const {
    return isPixelMode;
}

bool AIPanel::isLightingGuideActive() const {
    return lightingGuideActive;
}

bool AIPanel::isPaletteCheckActive() const {
    return paletteCheckActive;
}

bool AIPanel::isGhostOverlayActive() const {
    return ghostOverlayActive;
}

const TutorialStep* AIPanel::getCurrentTutorialStep() const {
    if (currentLessonIdx >= 0 && currentLessonIdx < static_cast<int>(lessons.size())) {
        const auto& steps = lessons[currentLessonIdx].steps;
        if (currentStepIdx >= 0 && currentStepIdx < static_cast<int>(steps.size())) {
            return &steps[currentStepIdx];
        }
    }
    return nullptr;
}

void AIPanel::update(float dt) {
    if (!isVisible) return;

    float bx = position.x;
    float by = position.y;

    headerGripBounds = sf::FloatRect(bx + 8.f, by + 6.f, size.x - 16.f, 26.f);

    float tabW = (size.x - 32.f) / 2.f;
    toolsTabBounds = sf::FloatRect(bx + 12.f, by + 38.f, tabW, 26.f);
    tutorialsTabBounds = sf::FloatRect(bx + 20.f + tabW, by + 38.f, tabW, 26.f);

    if (currentTab == AssistantTab::Tools) {
        modeToggleBounds = sf::FloatRect(bx + 12.f, by + 72.f, size.x - 24.f, 24.f);

        float btnW = size.x - 24.f;
        float y = by + 120.f;

        btn1Bounds = sf::FloatRect(bx + 12.f, y, btnW, 30.f);
        y += 36.f;
        btn2Bounds = sf::FloatRect(bx + 12.f, y, btnW, 30.f);
        y += 50.f;
        btn3Bounds = sf::FloatRect(bx + 12.f, y, btnW, 30.f);
        y += 36.f;
        btn4Bounds = sf::FloatRect(bx + 12.f, y, btnW, 30.f);
        y += 36.f;
        btn5Bounds = sf::FloatRect(bx + 12.f, y, btnW, 30.f);
    }
    else {
        float y = by + 75.f;
        tutPrevLessonBounds = sf::FloatRect(bx + 12.f, y, 32.f, 26.f);
        tutNextLessonBounds = sf::FloatRect(bx + size.x - 44.f, y, 32.f, 26.f);

        y = by + 380.f;
        tutPrevStepBounds = sf::FloatRect(bx + 12.f, y, 80.f, 28.f);
        tutNextStepBounds = sf::FloatRect(bx + 98.f, y, 80.f, 28.f);
        tutToggleGhostBounds = sf::FloatRect(bx + 184.f, y, size.x - 196.f, 28.f);
    }

    backBtnBounds = sf::FloatRect(bx + 12.f, by + size.y - 36.f, size.x - 24.f, 26.f);
}

void AIPanel::draw(sf::RenderWindow& window) {
    if (!isVisible) return;

    sf::FloatRect panelBounds(position.x, position.y, size.x, size.y);
    WisdomUI::Theme::DrawSunsetPanel(window, panelBounds, 1.0f);

    sf::RectangleShape gripBg(sf::Vector2f(headerGripBounds.width, headerGripBounds.height));
    gripBg.setPosition(headerGripBounds.left, headerGripBounds.top);
    gripBg.setFillColor(WisdomUI::Theme::SunsetDeepDark);
    gripBg.setOutlineThickness(1.f);
    gripBg.setOutlineColor(WisdomUI::Theme::SunsetPlum);
    window.draw(gripBg);

    WisdomUI::Theme::DrawCrispText(window, font, ":: ART ASSISTANT ::", 12, headerGripBounds.left + headerGripBounds.width / 2.0f, headerGripBounds.top + headerGripBounds.height / 2.0f, WisdomUI::Theme::SunsetAmber, sf::Color(14, 6, 20), true, true);

    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    bool toolsActive = (currentTab == AssistantTab::Tools);
    WisdomUI::Theme::DrawSunsetButton(window, toolsTabBounds, "Smart Tools", font, 11, toolsActive, toolsTabBounds.contains(mPos), toolsActive, 1.0f);
    WisdomUI::Theme::DrawSunsetButton(window, tutorialsTabBounds, "Tutorials", font, 11, !toolsActive, tutorialsTabBounds.contains(mPos), !toolsActive, 1.0f);

    if (currentTab == AssistantTab::Tools) {
        std::string modeLabel = isPixelMode ? "Target Mode: [ PIXEL ART ]" : "Target Mode: [ HIGH-RES / NORMAL ]";
        WisdomUI::Theme::DrawSunsetButton(window, modeToggleBounds, modeLabel, font, 10, isPixelMode, modeToggleBounds.contains(mPos), isPixelMode, 1.0f);

        std::string s1Title = isPixelMode ? "SILHOUETTE & INKING" : "LINEWORK & SHAPES";
        WisdomUI::Theme::DrawCrispText(window, font, s1Title, 10, position.x + 14.f, position.y + 104.f, WisdomUI::Theme::TextSecondary);

        std::string b1 = isPixelMode ? "Auto-Contour (1px)" : "Outer Stroke Contour";
        std::string b2 = isPixelMode ? "Clean Corner Jaggies" : "Edge Anti-Alias / Soften";
        WisdomUI::Theme::DrawSunsetButton(window, btn1Bounds, b1, font, 11, false, btn1Bounds.contains(mPos), false, 1.0f);
        WisdomUI::Theme::DrawSunsetButton(window, btn2Bounds, b2, font, 11, false, btn2Bounds.contains(mPos), false, 1.0f);

        std::string s2Title = isPixelMode ? "SHADING & PALETTE" : "LIGHTING & COLOR";
        WisdomUI::Theme::DrawCrispText(window, font, s2Title, 10, position.x + 14.f, position.y + 224.f, WisdomUI::Theme::TextSecondary);

        std::string b3 = isPixelMode ? (lightingGuideActive ? "Lighting Guide [ON]" : "Lighting Guide [OFF]") : (lightingGuideActive ? "Vignette Lighting [ON]" : "Vignette Lighting [OFF]");
        std::string b4 = isPixelMode ? (paletteCheckActive ? "Palette Audit [ON]" : "Palette Audit [OFF]") : "Auto Contrast Boost";
        std::string b5 = isPixelMode ? "Generate 4-Step Cel Ramp" : "Generate Harmonious Swatches";

        WisdomUI::Theme::DrawSunsetButton(window, btn3Bounds, b3, font, 11, lightingGuideActive, btn3Bounds.contains(mPos), lightingGuideActive, 1.0f);
        WisdomUI::Theme::DrawSunsetButton(window, btn4Bounds, b4, font, 11, isPixelMode && paletteCheckActive, btn4Bounds.contains(mPos), isPixelMode && paletteCheckActive, 1.0f);
        WisdomUI::Theme::DrawSunsetButton(window, btn5Bounds, b5, font, 11, false, btn5Bounds.contains(mPos), false, 1.0f);

        sf::FloatRect tipBox(position.x + 12.f, position.y + 385.f, size.x - 24.f, 48.f);
        sf::RectangleShape box(sf::Vector2f(tipBox.width, tipBox.height));
        box.setPosition(tipBox.left, tipBox.top);
        box.setFillColor(WisdomUI::Theme::SunsetDeepDark);
        box.setOutlineThickness(1.0f);
        box.setOutlineColor(WisdomUI::Theme::SunsetPlum);
        window.draw(box);

        std::string tip1 = isPixelMode ? "Pixel Mode: Enforces strict grid and palette." : "Normal Mode: Allows smooth blends & strokes.";
        WisdomUI::Theme::DrawCrispText(window, font, tip1, 10, tipBox.left + 8.f, tipBox.top + 6.f, WisdomUI::Theme::SunsetGold);
        WisdomUI::Theme::DrawCrispText(window, font, "Actions apply directly to the active canvas layer.", 9, tipBox.left + 8.f, tipBox.top + 26.f, WisdomUI::Theme::TextSecondary);
    }
    else {
        WisdomUI::Theme::DrawSunsetButton(window, tutPrevLessonBounds, "<", font, 11, false, tutPrevLessonBounds.contains(mPos), false, 1.0f);
        WisdomUI::Theme::DrawSunsetButton(window, tutNextLessonBounds, ">", font, 11, false, tutNextLessonBounds.contains(mPos), false, 1.0f);

        std::string lessonTitle = lessons.empty() ? "None" : lessons[currentLessonIdx].name;
        WisdomUI::Theme::DrawCrispText(window, font, lessonTitle, 12, position.x + size.x / 2.f, position.y + 88.f, WisdomUI::Theme::SunsetGold, sf::Color::Black, true, true);

        const TutorialStep* step = getCurrentTutorialStep();
        if (step) {
            std::string stepProg = "Step " + std::to_string(currentStepIdx + 1) + " of " + std::to_string(lessons[currentLessonIdx].steps.size());
            WisdomUI::Theme::DrawCrispText(window, font, stepProg, 10, position.x + 14.f, position.y + 115.f, WisdomUI::Theme::TextSecondary);
            WisdomUI::Theme::DrawCrispText(window, font, step->title, 12, position.x + 14.f, position.y + 132.f, WisdomUI::Theme::SunsetAmber);

            sf::FloatRect cardBox(position.x + 12.f, position.y + 155.f, size.x - 24.f, 150.f);
            sf::RectangleShape card(sf::Vector2f(cardBox.width, cardBox.height));
            card.setPosition(cardBox.left, cardBox.top);
            card.setFillColor(WisdomUI::Theme::SunsetDeepDark);
            card.setOutlineThickness(1.0f);
            card.setOutlineColor(WisdomUI::Theme::SunsetPlum);
            window.draw(card);

            WisdomUI::Theme::DrawCrispText(window, font, "INSTRUCTIONS:", 10, cardBox.left + 10.f, cardBox.top + 10.f, WisdomUI::Theme::SunsetGold);

            std::string text = step->instruction;
            float lineY = cardBox.top + 30.f;
            size_t maxCharsPerLine = 34;
            for (size_t i = 0; i < text.size(); i += maxCharsPerLine) {
                std::string line = text.substr(i, maxCharsPerLine);
                WisdomUI::Theme::DrawCrispText(window, font, line, 10, cardBox.left + 10.f, lineY, WisdomUI::Theme::TextPrimary);
                lineY += 16.f;
            }

            WisdomUI::Theme::DrawCrispText(window, font, "Suggested Tone:", 10, cardBox.left + 10.f, cardBox.top + 115.f, WisdomUI::Theme::TextSecondary);
            sf::RectangleShape swatch(sf::Vector2f(22.f, 22.f));
            swatch.setPosition(cardBox.left + 115.f, cardBox.top + 112.f);
            swatch.setFillColor(step->hintColor);
            swatch.setOutlineThickness(1.f);
            swatch.setOutlineColor(sf::Color::White);
            window.draw(swatch);
        }

        WisdomUI::Theme::DrawSunsetButton(window, tutPrevStepBounds, "< Prev", font, 11, false, tutPrevStepBounds.contains(mPos), false, 1.0f);
        WisdomUI::Theme::DrawSunsetButton(window, tutNextStepBounds, "Next >", font, 11, false, tutNextStepBounds.contains(mPos), false, 1.0f);
        WisdomUI::Theme::DrawSunsetButton(window, tutToggleGhostBounds, ghostOverlayActive ? "Guide [ON]" : "Guide [OFF]", font, 10, ghostOverlayActive, tutToggleGhostBounds.contains(mPos), ghostOverlayActive, 1.0f);
    }

    WisdomUI::Theme::DrawSunsetButton(window, backBtnBounds, "< CLOSE ASSISTANT", font, 11, false, backBtnBounds.contains(mPos), false, 1.0f);
}

bool AIPanel::handleEvent(const sf::Event& event, sf::Vector2f mousePos) {
    if (!isVisible) return false;

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (headerGripBounds.contains(mousePos)) {
            isDraggingPanel = true;
            dragOffset = mousePos - position;
            return true;
        }

        if (!sf::FloatRect(position.x, position.y, size.x, size.y).contains(mousePos)) {
            return false;
        }

        if (toolsTabBounds.contains(mousePos)) {
            currentTab = AssistantTab::Tools;
            return true;
        }

        if (tutorialsTabBounds.contains(mousePos)) {
            currentTab = AssistantTab::Tutorials;
            return true;
        }

        return true;
    }
    else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        if (isDraggingPanel) {
            isDraggingPanel = false;
            return true;
        }
    }
    else if (event.type == sf::Event::MouseMoved && isDraggingPanel) {
        position = mousePos - dragOffset;
        position.x = std::clamp(position.x, 56.f, 1920.f - size.x);
        position.y = std::clamp(position.y, 40.f, 1080.f - size.y);
        return true;
    }

    return sf::FloatRect(position.x, position.y, size.x, size.y).contains(mousePos);
}

std::string AIPanel::handleClick(sf::Vector2f mousePos) {
    if (!isVisible) return "";

    if (backBtnBounds.contains(mousePos)) {
        toggle();
        return "close";
    }

    if (currentTab == AssistantTab::Tools) {
        if (modeToggleBounds.contains(mousePos)) {
            isPixelMode = !isPixelMode;
            return "tool:toggle_mode";
        }
        if (btn1Bounds.contains(mousePos)) return "tool:contour";
        if (btn2Bounds.contains(mousePos)) return "tool:jaggies";
        if (btn3Bounds.contains(mousePos)) {
            lightingGuideActive = !lightingGuideActive;
            return "tool:lighting";
        }
        if (btn4Bounds.contains(mousePos)) {
            if (isPixelMode) {
                paletteCheckActive = !paletteCheckActive;
                return "tool:palette";
            }
            else {
                return "tool:contrast";
            }
        }
        if (btn5Bounds.contains(mousePos)) return "tool:ramp";
    }
    else {
        if (tutPrevLessonBounds.contains(mousePos)) {
            if (!lessons.empty()) {
                currentLessonIdx = (currentLessonIdx - 1 + static_cast<int>(lessons.size())) % static_cast<int>(lessons.size());
                currentStepIdx = 0;
            }
            return "tutorial:lesson_changed";
        }
        if (tutNextLessonBounds.contains(mousePos)) {
            if (!lessons.empty()) {
                currentLessonIdx = (currentLessonIdx + 1) % static_cast<int>(lessons.size());
                currentStepIdx = 0;
            }
            return "tutorial:lesson_changed";
        }
        if (tutPrevStepBounds.contains(mousePos)) {
            if (currentStepIdx > 0) currentStepIdx--;
            return "tutorial:step_changed";
        }
        if (tutNextStepBounds.contains(mousePos)) {
            if (currentLessonIdx >= 0 && currentLessonIdx < static_cast<int>(lessons.size())) {
                if (currentStepIdx < static_cast<int>(lessons[currentLessonIdx].steps.size()) - 1) {
                    currentStepIdx++;
                }
            }
            return "tutorial:step_changed";
        }
        if (tutToggleGhostBounds.contains(mousePos)) {
            ghostOverlayActive = !ghostOverlayActive;
            return "tutorial:toggle_ghost";
        }
    }
    return "";
}



std::vector<sf::Color> AIPanel::generateRampOrHarmony(sf::Color baseColor, bool pixelMode) {
    std::vector<sf::Color> result;
    if (pixelMode) {
        sf::Color highlight(
            static_cast<uint8_t>(std::min(255, baseColor.r + 45)),
            static_cast<uint8_t>(std::min(255, baseColor.g + 45)),
            static_cast<uint8_t>(std::min(255, baseColor.b + 20)),
            255
        );
        sf::Color shadow(
            static_cast<uint8_t>(std::max(0, baseColor.r - 40)),
            static_cast<uint8_t>(std::max(0, baseColor.g - 45)),
            static_cast<uint8_t>(std::max(0, baseColor.b - 20)),
            255
        );
        sf::Color deepShadow(
            static_cast<uint8_t>(std::max(0, shadow.r - 40)),
            static_cast<uint8_t>(std::max(0, shadow.g - 40)),
            static_cast<uint8_t>(std::max(0, shadow.b - 15)),
            255
        );
        result = { highlight, baseColor, shadow, deepShadow };
    }
    else {
        sf::Color comp(
            static_cast<uint8_t>(255 - baseColor.r),
            static_cast<uint8_t>(255 - baseColor.g),
            static_cast<uint8_t>(255 - baseColor.b),
            255
        );
        sf::Color warm(
            static_cast<uint8_t>(std::min(255, baseColor.r + 30)),
            static_cast<uint8_t>(baseColor.g),
            static_cast<uint8_t>(std::max(0, baseColor.b - 20)),
            255
        );
        sf::Color cool(
            static_cast<uint8_t>(std::max(0, baseColor.r - 20)),
            static_cast<uint8_t>(baseColor.g),
            static_cast<uint8_t>(std::min(255, baseColor.b + 30)),
            255
        );
        result = { baseColor, comp, warm, cool };
    }
    return result;
}