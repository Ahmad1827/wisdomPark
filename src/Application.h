#pragma once
#include <SFML/Graphics.hpp>
#include "core/AppState.h"
#include "core/SettingsManager.h"
#include "core/Canvas.h"
#include "core/Timeline.h"
#include "ui/UIManager.h"
#include "ai/AIHelper.h"

class Application {
private:
    sf::RenderWindow window;
    AppState currentState;
    AppSettings appSettings;
    Canvas canvas;
    Timeline timeline;
    UIManager uiManager;
    AIHelper aiHelper;
    sf::Clock frameClock;

    void processEvents();
    void update(float dt);
    void render();

public:
    Application();
    void run();
};