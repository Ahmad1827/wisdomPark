#include <SFML/Graphics.hpp>
#include "ui/UIManager.h"
#include "core/ProjectManager.h"
#include "core/Canvas.h"
#include "core/Timeline.h"
#include "ai/AIHelper.h"

int main() {
    sf::RenderWindow window(sf::VideoMode(1920, 1080), "Wisdom Park", sf::Style::Default);
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