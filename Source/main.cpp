#include <SFML/Graphics.hpp>
#include "SpriteSheetStudioPanel.h"

int main() {
    sf::RenderWindow window(sf::VideoMode(1280, 720), "Wisdom Park Asset Tools — Sprite Sheet Studio");
    window.setFramerateLimit(60);

    StudioUI::SpriteSheetStudioPanel editorPanel;
    editorPanel.Initialize();

    sf::Clock clock;

    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();
        sf::Event event;

        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }
            if (event.type == sf::Event::Resized) {
                sf::FloatRect visibleArea(0, 0, event.size.width, event.size.height);
                window.setView(sf::View(visibleArea));
                editorPanel.SetBounds(visibleArea);
            }

            editorPanel.HandleEvent(event, window);
        }

        editorPanel.Update(deltaTime, window);

        window.clear();
        editorPanel.Render(window);
        window.display();
    }

    return 0;
}