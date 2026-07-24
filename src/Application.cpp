#include "Application.h"

Application::Application()
    : window(sf::VideoMode(1920, 1080), "Wisdom Park", sf::Style::Default),
    currentState(AppState::Welcome),
    currentFullscreenState(false) {
    window.setFramerateLimit(60);
    window.setKeyRepeatEnabled(false);

    sf::View view(sf::FloatRect(0.f, 0.f, 1920.f, 1080.f));
    window.setView(view);

    appSettings = SettingsManager::loadSettings();

    canvas.init();
    uiManager.init(&projectManager, &canvas);

    aiHelper.trainOnDataset("dataset.json");
    aiHelper.loadThesaurus("thesaurus.txt");
}

void Application::applyVideoMode(bool fullscreen) {
    if (fullscreen == currentFullscreenState) {
        return;
    }

    currentFullscreenState = fullscreen;

    if (fullscreen) {
        window.create(sf::VideoMode::getFullscreenModes()[0], "Wisdom Park Studio", sf::Style::Fullscreen);
    }
    else {
        window.create(sf::VideoMode(1600, 900), "Wisdom Park Studio", sf::Style::Default);
    }

    sf::View view(sf::FloatRect(0.f, 0.f, 1920.f, 1080.f));
    window.setView(view);

    window.setFramerateLimit(uiManager.getFpsLimit());
    window.setKeyRepeatEnabled(false);
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

        if (event.type == sf::Event::Resized) {
            sf::FloatRect visibleArea(0.f, 0.f, 1920.f, 1080.f);
            window.setView(sf::View(visibleArea));
        }

        uiManager.handleEvent(event, window, currentState, appSettings, canvas, timeline, aiHelper, projectManager);
    }
}

void Application::update(float dt) {
    applyVideoMode(uiManager.isFullscreen());

    if (currentState == AppState::Painting) {
        timeline.update(static_cast<float>(canvas.getFrameCount()));
        aiHelper.update(*canvas.getActiveRenderTexture(timeline.getCurrentFrame()));
    }
    uiManager.update(window, currentState, appSettings, dt, canvas, timeline);
}

void Application::render() {
    window.clear(sf::Color(15, 15, 18));

    sf::View defaultView(sf::FloatRect(0.f, 0.f, 1920.f, 1080.f));
    window.setView(defaultView);

    uiManager.draw(window, currentState, canvas, aiHelper, timeline);
    window.display();
}