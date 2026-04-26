#include <SFML/Graphics.hpp>
#include <cmath>
#include <vector>
#include <memory>
#include "AIHelper.h"

class WisdomPark {
private:
    sf::RenderWindow window;
    sf::Texture bgTexture, canvasTexture, deskTexture;
    sf::Sprite bgSprite, canvasSprite, deskSprite;
    std::vector<std::unique_ptr<sf::RenderTexture>> frames;
    int currentFrame;

    bool isDrawing;
    sf::Vector2f lastPos;
    float brushSize;
    sf::Color brushColor;

    bool isPlaying;
    sf::Clock playClock;
    float timePerFrame;

    sf::FloatRect drawArea;
    sf::FloatRect paletteVignette;
    sf::FloatRect paletteFerris;
    sf::FloatRect paletteEraser;
    sf::FloatRect paletteBrush;

    AIHelper aiMascot;

    void addNewFrame() {
        auto tex = std::make_unique<sf::RenderTexture>();
        tex->create(1920, 1080);
        tex->clear(sf::Color::Transparent);
        frames.push_back(std::move(tex));
    }

    void resetAnimation() {
        frames.clear();
        currentFrame = 0;
        addNewFrame();
    }

    void setupUI() {
        bgTexture.loadFromFile("assets/landofwisdompark.png");
        bgSprite.setTexture(bgTexture);

        deskTexture.loadFromFile("assets/workbench.png");
        deskSprite.setTexture(deskTexture);
        float deskScaleX = 1920.0f / deskSprite.getLocalBounds().width;
        float deskScaleY = 300.0f / deskSprite.getLocalBounds().height;
        deskSprite.setScale(deskScaleX, deskScaleY);
        float deskY = 1080.0f - 300.0f;
        deskSprite.setPosition(0, deskY);

        canvasTexture.loadFromFile("assets/canvas.png");
        canvasSprite.setTexture(canvasTexture);
        float canvasScale = 700.0f / canvasSprite.getLocalBounds().height;
        canvasSprite.setScale(canvasScale, canvasScale);
        float canvasWidth = canvasSprite.getLocalBounds().width * canvasScale;
        float canvasHeight = canvasSprite.getLocalBounds().height * canvasScale;
        float canvasX = (1920.0f - canvasWidth) / 2.0f;
        float canvasY = 20.0f;
        canvasSprite.setPosition(canvasX, canvasY);

        float frameOffsetX = canvasWidth * 0.08f;
        float frameOffsetYTop = canvasHeight * 0.16f;
        float frameOffsetYBot = canvasHeight * 0.16f;
        drawArea = sf::FloatRect(
            canvasX + frameOffsetX,
            canvasY + frameOffsetYTop,
            canvasWidth - (frameOffsetX * 2.0f),
            canvasHeight - frameOffsetYTop - frameOffsetYBot
        );

        paletteEraser = sf::FloatRect(100, deskY + 40, 250, 200);
        paletteBrush = sf::FloatRect(1500, deskY + 20, 300, 250);
        paletteVignette = sf::FloatRect(650, deskY + 50, 90, 90);
        paletteFerris = sf::FloatRect(800, deskY + 50, 90, 90);
    }

    void processEvents() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();

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

                if (event.key.code == sf::Keyboard::Space && !isPlaying) {
                    isPlaying = true;
                    currentFrame = 0;
                    playClock.restart();
                }
            }

            if (event.type == sf::Event::KeyReleased) {
                if (event.key.code == sf::Keyboard::Space) {
                    isPlaying = false;
                    currentFrame = frames.size() - 1;
                }
            }

            if (!isPlaying) {
                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);

                    if (paletteVignette.contains(mousePos)) brushColor = sf::Color(255, 105, 180);
                    else if (paletteFerris.contains(mousePos)) brushColor = sf::Color(0, 191, 255);
                    else if (paletteEraser.contains(mousePos)) brushColor = sf::Color::Transparent;
                    else if (paletteBrush.contains(mousePos)) brushColor = sf::Color::Black;
                    else if (aiMascot.getBounds().contains(mousePos)) {
                        aiMascot.toggle();
                        if (aiMascot.isActive()) aiMascot.analyzeFrame(*frames[currentFrame]);
                    }
                    else if (drawArea.contains(mousePos)) {
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

                    if (!drawArea.contains(currentPos)) {
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

        window.draw(bgSprite);
        window.draw(canvasSprite);

        if (!isPlaying && currentFrame > 0) {
            sf::Sprite onionSkin(frames[currentFrame - 1]->getTexture());
            onionSkin.setColor(sf::Color(255, 255, 255, 60));
            window.draw(onionSkin);
        }

        sf::Sprite currentSprite(frames[currentFrame]->getTexture());
        window.draw(currentSprite);

        window.draw(deskSprite);
        aiMascot.draw(window);

        if (!isPlaying) {
            sf::CircleShape brushPreview(brushSize / 2.f);
            brushPreview.setFillColor(brushColor == sf::Color::Transparent ? sf::Color::White : brushColor);
            brushPreview.setOutlineThickness(1);
            brushPreview.setOutlineColor(sf::Color::Black);
            brushPreview.setPosition(50, 940);
            window.draw(brushPreview);
        }

        window.display();
    }

public:
    WisdomPark()
        : window(sf::VideoMode(1920, 1080), "Wisdom Park"),
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