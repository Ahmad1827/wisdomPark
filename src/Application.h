#pragma once
#include <SFML/Graphics.hpp>
#include "core/AppState.h"
#include "core/SettingsManager.h"
#include "core/Canvas.h"
#include "core/Timeline.h"
#include "core/ProjectManager.h"
#include "ai/AIHelper.h"
#include "ui/UIManager.h"

class Application {
public:
    Application();
    void run();

private:
    void processEvents();
    void update(float dt);
    void render();
    void applyVideoMode(bool fullscreen);

    sf::RenderWindow window;
    sf::Clock frameClock;
    AppState currentState;
    AppSettings appSettings;

    Canvas canvas;
    Timeline timeline;
    ProjectManager projectManager;
    AIHelper aiHelper;
    UIManager uiManager;

    bool currentFullscreenState;
};