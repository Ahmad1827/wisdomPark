#include <SFML/Graphics.hpp>
#include <iostream>
#include <cmath>
#include <string>

int main() {
    sf::RenderWindow window(sf::VideoMode(800, 600), "Software Desen - S: Salveaza, E: Guma, C: Sterge");
    window.setFramerateLimit(60);

    sf::RenderTexture canvas;
    canvas.create(800, 600);
    canvas.clear(sf::Color::White);

    sf::Sprite canvasSprite(canvas.getTexture());

    bool isDrawing = false;
    sf::Vector2f lastPos;
    float brushSize = 5.0f;
    sf::Color brushColor = sf::Color::Black;
    int saveCount = 0;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Num1 || event.key.code == sf::Keyboard::B) brushColor = sf::Color::Black;
                if (event.key.code == sf::Keyboard::Num2) brushColor = sf::Color::Red;
                if (event.key.code == sf::Keyboard::Num3) brushColor = sf::Color::Green;
                if (event.key.code == sf::Keyboard::Num4) brushColor = sf::Color::Blue;
                if (event.key.code == sf::Keyboard::Num5) brushColor = sf::Color::Yellow;
                if (event.key.code == sf::Keyboard::E) brushColor = sf::Color::White;
                if (event.key.code == sf::Keyboard::C) canvas.clear(sf::Color::White);

                if (event.key.code == sf::Keyboard::S) {
                    saveCount++;
                    std::string filename = "desen_" + std::to_string(saveCount) + ".png";
                    sf::Image screenshot = canvas.getTexture().copyToImage();
                    if (screenshot.saveToFile(filename)) {
                        std::cout << "Salvat cu succes: " << filename << std::endl;
                    }
                    else {
                        std::cout << "Eroare la salvare!" << std::endl;
                    }
                }
            }

            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                isDrawing = true;
                lastPos = sf::Vector2f(event.mouseButton.x, event.mouseButton.y);

                sf::CircleShape dot(brushSize / 2.f);
                dot.setOrigin(brushSize / 2.f, brushSize / 2.f);
                dot.setPosition(lastPos);
                dot.setFillColor(brushColor);
                canvas.draw(dot);
                canvas.display();
            }

            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
                isDrawing = false;
            }

            if (event.type == sf::Event::MouseMoved && isDrawing) {
                sf::Vector2f currentPos(event.mouseMove.x, event.mouseMove.y);
                sf::Vector2f d = currentPos - lastPos;
                float length = std::sqrt(d.x * d.x + d.y * d.y);

                sf::RectangleShape line(sf::Vector2f(length, brushSize));
                line.setOrigin(0, brushSize / 2.f);
                line.setPosition(lastPos);
                line.setRotation(std::atan2(d.y, d.x) * 180.f / 3.14159265f);
                line.setFillColor(brushColor);
                canvas.draw(line);

                sf::CircleShape circle(brushSize / 2.f);
                circle.setOrigin(brushSize / 2.f, brushSize / 2.f);
                circle.setPosition(currentPos);
                circle.setFillColor(brushColor);
                canvas.draw(circle);

                canvas.display();
                lastPos = currentPos;
            }

            if (event.type == sf::Event::MouseWheelScrolled) {
                if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
                    brushSize += event.mouseWheelScroll.delta;
                    if (brushSize < 1.0f) brushSize = 1.0f;
                    if (brushSize > 100.0f) brushSize = 100.0f;
                }
            }
        }

        window.clear(sf::Color::White);
        window.draw(canvasSprite);

        sf::CircleShape brushPreview(brushSize / 2.f);
        brushPreview.setFillColor(brushColor);
        brushPreview.setOutlineThickness(1);
        brushPreview.setOutlineColor(brushColor == sf::Color::White ? sf::Color::Black : sf::Color(200, 200, 200));
        brushPreview.setPosition(10, 10);
        window.draw(brushPreview);

        window.display();
    }

    return 0;
}