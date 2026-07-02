#include "Application.h"

Application::Application()
    : window(sf::VideoMode(1920, 1080), "Wisdom Park Studio", sf::Style::Fullscreen),
    currentState(AppState::Welcome) {
    window.setFramerateLimit(60);
    window.setKeyRepeatEnabled(false);

    appSettings = SettingsManager::loadSettings();

    canvas.init();
    uiManager.init(&projectManager, &canvas);

    aiHelper.trainOnDataset("dataset.json");
    aiHelper.loadThesaurus("thesaurus.txt");
}

void Application::run() {
    frameClock.restart();
    while (window.isOpen()) {
        float dt = frameClock.restart().asSeconds();
        processEvents();
        update(dt);
        render();
    }
}

void Application::processEvents() {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window.close();
        }

        uiManager.handleEvent(event, window, currentState, appSettings, canvas, timeline, aiHelper, projectManager);
    }
}

void Application::update(float dt) {
    if (currentState == AppState::Painting) {
        timeline.update(static_cast<float>(canvas.getFrameCount()));
        aiHelper.update(*canvas.getActiveRenderTexture(timeline.getCurrentFrame()));
    }
    uiManager.update(window, currentState, appSettings, dt, canvas);
}

void Application::render() {
    window.clear(sf::Color(15, 15, 18));
    uiManager.draw(window, currentState, canvas, aiHelper, timeline);
    window.display();
}