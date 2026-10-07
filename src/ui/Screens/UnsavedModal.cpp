// "Unsaved changes" confirmation. Drawing and click handling share the layout helpers below.
#include "../UIManager.h"
#include "../UITheme.h"
#include "MenuWidgets.h"
#include <algorithm>
#include <string>

using WisdomUI::Theme;
using namespace MenuWidgets;

namespace {

    sf::FloatRect panelBounds() {
        const float w = 580.0f;
        const float h = 300.0f;
        return sf::FloatRect(std::floor((1920.0f - w) * 0.5f), std::floor((1080.0f - h) * 0.5f), w, h);
    }

    // Save, Discard, Cancel from left to right.
    sf::FloatRect buttonBounds(int index) {
        sf::FloatRect p = panelBounds();
        const float gap = 14.0f;
        const float w = std::floor((p.width - 72.0f - gap * 2.0f) / 3.0f);
        return sf::FloatRect(p.left + 36.0f + static_cast<float>(index) * (w + gap), p.top + 190.0f, w, 52.0f);
    }

    ModalClock s_clock;

}

// Opens the New Project modal, asking about unsaved work first when there is any.
void UIManager::requestNewProject(Canvas& canvas) {
    if (canvas.getIsDirty()) {
        m_unsavedThen = UnsavedThen::NewProject;
        showUnsavedWarning = true;
    }
    else {
        newProjectModal.open();
    }
}

void UIManager::drawUnsavedWarning(sf::RenderWindow& window) {
    float elapsed = s_clock.tick();
    float appear = modalAppear(elapsed);
    sf::View savedView = beginModal(window, appear);

    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    sf::FloatRect panel = panelBounds();
    float cx = panel.left + panel.width * 0.5f;

    Theme::DrawSunsetPanel(window, panel, appear);
    drawDialogHeader(window, font, panel, "UNSAVED CHANGES", Theme::SunsetAmber, elapsed);

    std::string name = activeProjectName;
    std::replace(name.begin(), name.end(), '_', ' ');
    if (name.length() > 26) name = name.substr(0, 24) + "..";

    bool forNewProject = (m_unsavedThen == UnsavedThen::NewProject);
    Theme::DrawCrispText(window, font, "'" + name + "' has changes that are not saved.", 18, cx, panel.top + 118.0f, Theme::TextPrimary, kTextShadow, true, true);
    Theme::DrawCrispText(window, font, forNewProject ? "Save them before starting a new project?" : "Save them before leaving?", 16, cx, panel.top + 150.0f, Theme::TextSecondary, kTextShadow, true, true);

    sf::FloatRect save = buttonBounds(0);
    sf::FloatRect discard = buttonBounds(1);
    sf::FloatRect cancel = buttonBounds(2);
    drawPrimaryButton(window, font, save, "SAVE", 18, save.contains(mPos));
    Theme::DrawSunsetButton(window, discard, "DISCARD", font, 18, false, discard.contains(mPos), true, 1.0f);
    Theme::DrawSunsetButton(window, cancel, "CANCEL", font, 18, false, cancel.contains(mPos), false, 1.0f);

    Theme::DrawCrispText(window, font, "ENTER  SAVE   |   ESC  CANCEL", 15, cx, panel.top + 270.0f, Theme::TextMuted, sf::Color::Transparent, true, true);

    window.setView(savedView);
}

void UIManager::handleUnsavedWarningEvent(const sf::Event& event, sf::RenderWindow& window, AppState& currentState, Canvas& canvas, Timeline& timeline) {
    auto dismiss = [&]() {
        showUnsavedWarning = false;
        m_unsavedThen = UnsavedThen::MainMenu;
        };

    // Carries on with whatever the warning interrupted.
    auto proceed = [&]() {
        UnsavedThen then = m_unsavedThen;
        dismiss();
        if (then == UnsavedThen::NewProject) {
            newProjectModal.open();
        }
        else {
            currentState = AppState::Welcome;
            currentMenuState = MenuState::Main;
        }
        };

    auto saveAndProceed = [&]() {
        if (triggerSave(canvas, timeline)) proceed();
        else showMessage("Error Saving Project!", sf::Color::Red);
        };

    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::Escape) dismiss();
        else if (event.key.code == sf::Keyboard::Enter) saveAndProceed();
        return;
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        sf::Vector2f mousePos = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));

        if (buttonBounds(0).contains(mousePos)) saveAndProceed();
        else if (buttonBounds(1).contains(mousePos)) proceed();
        else if (buttonBounds(2).contains(mousePos)) dismiss();
    }
}
