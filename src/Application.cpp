#include "Application.h"

Application::Application()
    : window(sf::VideoMode(1920, 1080), "Wisdom Park Studio", sf::Style::Fullscreen),
    currentState(AppState::Welcome) {
    window.setFramerateLimit(60);
    window.setKeyRepeatEnabled(false);

    appSettings = SettingsManager::loadSettings();

    canvas.init();
    uiManager.init();

    aiHelper.trainOnDataset("dataset.json");
    aiHelper.loadThesaurus("thesaurus.txt");
}

void Application::run() {
    while (window.isOpen()) {
        processEvents();
        update();
        render();
    }
}

void Application::processEvents() {
    sf::Event event;
    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window.close();
        }
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
            window.close();
        }

        uiManager.handleEvent(event, window, currentState, appSettings, canvas, timeline, aiHelper);
    }
}

void Application::update() {
    if (currentState == AppState::Painting) {
        timeline.update(canvas.getFrameCount());
        aiHelper.update(*canvas.getFrame(timeline.getCurrentFrame()));
    }
    uiManager.update(window, currentState, appSettings);
}

void Application::render() {
    window.clear(sf::Color(25, 25, 25));
    uiManager.draw(window, currentState, canvas, aiHelper, timeline);
    window.display();
}