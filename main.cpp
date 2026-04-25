#include <SFML/Graphics.hpp>
#include <cmath>
#include <vector>
#include <memory>
#include "AIHelper.h"

class WisdomPark {
private:
    sf::RenderWindow window;
    std::vector<std::unique_ptr<sf::RenderTexture>> frames;
    int currentFrame;

    bool isDrawing;
    sf::Vector2f lastPos;
    float brushSize;
    sf::Color brushColor;

    bool isPlaying;
    sf::Clock playClock;
    float timePerFrame;

    sf::RectangleShape parkGround;
    sf::RectangleShape redStall;
    sf::RectangleShape blueStall;
    sf::RectangleShape greenStall;
    sf::RectangleShape eraser;

    AIHelper aiMascot;

    void addNewFrame() {
        auto tex = std::make_unique<sf::RenderTexture>();
        tex->create(800, 600);
        tex->clear(sf::Color::Transparent);
        frames.push_back(std::move(tex));
    }

    void setupUI() {
        parkGround.setSize(sf::Vector2f(800, 100));
        parkGround.setPosition(0, 500);
        parkGround.setFillColor(sf::Color(34, 139, 34));

        redStall.setSize(sf::Vector2f(40, 40));
        redStall.setPosition(20, 530);
        redStall.setFillColor(sf::Color::Red);

        blueStall.setSize(sf::Vector2f(40, 40));
        blueStall.setPosition(70, 530);
        blueStall.setFillColor(sf::Color::Blue);

        greenStall.setSize(sf::Vector2f(40, 40));
        greenStall.setPosition(120, 530);
        greenStall.setFillColor(sf::Color::Green);

        eraser.setSize(sf::Vector2f(40, 40));
        eraser.setPosition(170, 530);
        eraser.setFillColor(sf::Color::White);
        eraser.setOutlineThickness(2);
        eraser.setOutlineColor(sf::Color::Black);
    }

    void processEvents() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Num1 || event.key.code == sf::Keyboard::B) brushColor = sf::Color::Black;
                if (event.key.code == sf::Keyboard::C) frames[currentFrame]->clear(sf::Color::Transparent);

                if (event.key.code == sf::Keyboard::Right && !isPlaying) {
                    currentFrame++;
                    if (currentFrame >= frames.size()) addNewFrame();
                }
                if (event.key.code == sf::Keyboard::Left && !isPlaying) {
                    if (currentFrame > 0) currentFrame--;
                }

                if (event.key.code == sf::Keyboard::Space) {
                    isPlaying = true;
                    currentFrame = 0;
                    playClock.restart();
                }
            }

            if (event.type == sf::Event::KeyReleased) {
                if (event.key.code == sf::Keyboard::Space) {
                    isPlaying = false;
                }
            }

            if (!isPlaying) {
                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);

                    if (redStall.getGlobalBounds().contains(mousePos)) {
                        brushColor = sf::Color::Red;
                    }
                    else if (blueStall.getGlobalBounds().contains(mousePos)) {
                        brushColor = sf::Color::Blue;
                    }
                    else if (greenStall.getGlobalBounds().contains(mousePos)) {
                        brushColor = sf::Color::Green;
                    }
                    else if (eraser.getGlobalBounds().contains(mousePos)) {
                        brushColor = sf::Color::Transparent;
                    }
                    else if (aiMascot.getBounds().contains(mousePos)) {
                        aiMascot.toggle();
                        if (aiMascot.isActive()) {
                            parkGround.setFillColor(sf::Color(255, 223, 0));
                            aiMascot.analyzeFrame(*frames[currentFrame]);
                        }
                        else {
                            parkGround.setFillColor(sf::Color(34, 139, 34));
                        }
                    }
                    else if (mousePos.y < 500) {
                        isDrawing = true;
                        lastPos = mousePos;

                        sf::CircleShape dot(brushSize / 2.f);
                        dot.setOrigin(brushSize / 2.f, brushSize / 2.f);
                        dot.setPosition(lastPos);
                        dot.setFillColor(brushColor);
                        if (brushColor == sf::Color::Transparent)
                            frames[currentFrame]->draw(dot, sf::RenderStates(sf::BlendNone));
                        else
                            frames[currentFrame]->draw(dot);
                        frames[currentFrame]->display();
                    }
                }

                if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
                    isDrawing = false;
                }

                if (event.type == sf::Event::MouseMoved && isDrawing) {
                    sf::Vector2f currentPos(event.mouseMove.x, event.mouseMove.y);

                    if (currentPos.y >= 500) {
                        isDrawing = false;
                        continue;
                    }

                    sf::Vector2f d = currentPos - lastPos;
                    float length = std::sqrt(d.x * d.x + d.y * d.y);

                    sf::RectangleShape line(sf::Vector2f(length, brushSize));
                    line.setOrigin(0, brushSize / 2.f);
                    line.setPosition(lastPos);
                    line.setRotation(std::atan2(d.y, d.x) * 180.f / 3.14159265f);
                    line.setFillColor(brushColor);

                    sf::CircleShape circle(brushSize / 2.f);
                    circle.setOrigin(brushSize / 2.f, brushSize / 2.f);
                    circle.setPosition(currentPos);
                    circle.setFillColor(brushColor);

                    if (brushColor == sf::Color::Transparent) {
                        frames[currentFrame]->draw(line, sf::RenderStates(sf::BlendNone));
                        frames[currentFrame]->draw(circle, sf::RenderStates(sf::BlendNone));
                    }
                    else {
                        frames[currentFrame]->draw(line);
                        frames[currentFrame]->draw(circle);
                    }

                    frames[currentFrame]->display();
                    lastPos = currentPos;
                }
            }

            if (event.type == sf::Event::MouseWheelScrolled) {
                if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
                    brushSize += event.mouseWheelScroll.delta;
                    if (brushSize < 1.0f) brushSize = 1.0f;
                    if (brushSize > 100.0f) brushSize = 100.0f;
                }
            }
        }
    }

    void update() {
        if (isPlaying) {
            if (playClock.getElapsedTime().asSeconds() >= timePerFrame) {
                currentFrame++;
                if (currentFrame >= frames.size()) {
                    currentFrame = 0;
                }
                playClock.restart();
            }
        }
    }

    void render() {
        window.clear(sf::Color::White);

        if (!isPlaying && currentFrame > 0) {
            sf::Sprite onionSkin(frames[currentFrame - 1]->getTexture());
            onionSkin.setColor(sf::Color(255, 255, 255, 60));
            window.draw(onionSkin);
        }

        sf::Sprite currentSprite(frames[currentFrame]->getTexture());
        window.draw(currentSprite);

        window.draw(parkGround);
        window.draw(redStall);
        window.draw(blueStall);
        window.draw(greenStall);
        window.draw(eraser);

        aiMascot.draw(window);

        if (!isPlaying) {
            sf::CircleShape brushPreview(brushSize / 2.f);
            brushPreview.setFillColor(brushColor == sf::Color::Transparent ? sf::Color::White : brushColor);
            brushPreview.setOutlineThickness(1);
            brushPreview.setOutlineColor(sf::Color::Black);
            brushPreview.setPosition(20, 520);
            window.draw(brushPreview);
        }

        window.display();
    }

public:
    WisdomPark()
        : window(sf::VideoMode(800, 600), "Wisdom Park - OOP Version"),
        currentFrame(0), isDrawing(false), brushSize(5.0f),
        brushColor(sf::Color::Black), isPlaying(false),
        timePerFrame(1.0f / 12.0f)
    {
        window.setFramerateLimit(60);
        setupUI();
        addNewFrame();
    }

    void run() {
        while (window.isOpen()) {
            processEvents();
            update();
            render();
        }
    }
};

int main() {
    WisdomPark app;
    app.run();
    return 0;
}