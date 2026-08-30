#ifndef NOMINMAX
#define NOMINMAX
#endif

#if defined(_WIN32)
#pragma comment(linker, "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
#include <windows.h>
#endif

#include <SFML/Graphics.hpp>
#include <iostream>
#include "ui/UIManager.h"
#include "core/ProjectManager.h"
#include "core/Canvas.h"
#include "core/Timeline.h"
#include "ai/AIHelper.h"
#include "core/DragDropHandler.h"

static sf::Image DownscaleIcon(const sf::Image& src, unsigned int targetSize = 32) {
    sf::Image dest;
    dest.create(targetSize, targetSize);
    unsigned int srcW = src.getSize().x;
    unsigned int srcH = src.getSize().y;

    for (unsigned int y = 0; y < targetSize; ++y) {
        for (unsigned int x = 0; x < targetSize; ++x) {
            unsigned int srcX = (x * srcW) / targetSize;
            unsigned int srcY = (y * srcH) / targetSize;
            dest.setPixel(x, y, src.getPixel(srcX, srcY));
        }
    }
    return dest;
}

void ApplyWindowIcon(sf::RenderWindow& window) {
    sf::Image appIcon;
    bool loaded = appIcon.loadFromFile("wisdomParkicon.png") ||
        appIcon.loadFromFile("wisdomParkicon.jpg") ||
        appIcon.loadFromFile("Resources/wisdomParkicon.png") ||
        appIcon.loadFromFile("Resources/wisdomParkicon.jpg") ||
        appIcon.loadFromFile("assets/wisdomParkicon.png") ||
        appIcon.loadFromFile("assets/wisdomParkicon.jpg");

    if (loaded) {
        sf::Image safeIcon = DownscaleIcon(appIcon, 32);
        window.setIcon(safeIcon.getSize().x, safeIcon.getSize().y, safeIcon.getPixelsPtr());
    }
}

int main() {
#if defined(_WIN32)
    FreeConsole();
#endif

    sf::VideoMode desktopMode = sf::VideoMode::getDesktopMode();
    sf::RenderWindow window(desktopMode, "Wisdom Park", sf::Style::Fullscreen);

    ApplyWindowIcon(window);

    DragDropHandler::Attach(window);
    window.setVerticalSyncEnabled(true);
    window.setFramerateLimit(60);

    ProjectManager pm;
    Canvas canvas;
    Timeline timeline;
    AIHelper aiHelper;
    UIManager uiManager;

    uiManager.init(&pm, &canvas);

    AppState currentState = AppState::Welcome;
    AppSettings settings;
    sf::Clock clock;

    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        sf::Event event;

        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            uiManager.handleEvent(event, window, currentState, settings, canvas, timeline, aiHelper, pm);
        }

        uiManager.update(window, currentState, settings, dt, canvas, timeline);

        window.clear(sf::Color(15, 15, 20));
        uiManager.draw(window, currentState, canvas, aiHelper, timeline);
        window.display();
    }

    return 0;
}