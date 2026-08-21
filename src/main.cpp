#include <SFML/Graphics.hpp>
#include "ui/UIManager.h"
#include "core/ProjectManager.h"
#include "core/Canvas.h"
#include "core/Timeline.h"
#include "ai/AIHelper.h"
#include "core/DragDropHandler.h"

int main() {
    sf::VideoMode desktopMode = sf::VideoMode::getDesktopMode();
    sf::RenderWindow window(desktopMode, "Wisdom Park", sf::Style::Fullscreen);
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