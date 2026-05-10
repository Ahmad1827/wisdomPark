#include <SFML/Graphics.hpp>
#include <cmath>
#include <vector>
#include <memory>
#include <string>
#include <iostream>
#include <filesystem>
#include <fstream>
#include "AIHelper.h"
#include <sstream>
enum class AppState {
    Menu,
    Painting
};

class WisdomPark {
private:
    sf::RenderWindow window;
    sf::Texture bgTexture, canvasTexture, deskTexture;
    sf::Sprite bgSprite, canvasSprite, deskSprite;
    std::vector<std::unique_ptr<sf::RenderTexture>> frames;
    int currentFrame;

    bool isDrawing;
    sf::Vector2f lastPos;
    sf::Vector2f currentMousePos;
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
    std::vector<sf::Image> undoHistory;
    std::vector<sf::Image> redoHistory;

    AppState currentState;
    sf::RectangleShape startButton;
    sf::ConvexShape playIcon;

    sf::Font mainFont;
    sf::Text uiText;
    sf::Clock textClock;
    bool showingText;
    float textAlpha;

    bool isAnimateMode;
    bool isPathMode;
    bool awaitingPathStart;
    bool awaitingPathEnd;
    sf::Vector2f pathStartPos;

    bool awaitingAnimStart;
    bool awaitingAnimEnd;
    sf::Vector2f animStartPos;

    bool isLightingMode;
    bool isTypingPrompt;
    std::string currentPrompt;
    sf::RectangleShape promptBox;
    sf::Text promptDisplay;
    int promptQuantity;

    void showMessage(const std::string& msg, sf::Color color) {
        uiText.setString(msg);
        uiText.setFillColor(color);
        sf::FloatRect textRect = uiText.getLocalBounds();
        uiText.setOrigin(textRect.left + textRect.width / 2.0f, textRect.top + textRect.height / 2.0f);
        uiText.setPosition(1920.0f / 2.0f, 150.0f);
        showingText = true;
        textAlpha = 255.0f;
        textClock.restart();
    }

    void addNewFrame() {
        auto tex = std::make_unique<sf::RenderTexture>();
        tex->create(1920, 1080);
        tex->clear(sf::Color::Transparent);
        tex->display();
        frames.push_back(std::move(tex));
    }

    void resetAnimation() {
        frames.clear();
        currentFrame = 0;
        undoHistory.clear();
        redoHistory.clear();
        aiMascot.clearAllMemory();
        addNewFrame();
    }

    void setupUI() {
        bgTexture.loadFromFile("assets/landofwisdompark.png");
        bgSprite.setTexture(bgTexture);

        deskTexture.loadFromFile("assets/workbench.png", sf::IntRect(114, 702, 1669, 379));
        deskSprite.setTexture(deskTexture);

        float deskX = 114.0f;
        float deskY = 702.0f;
        deskSprite.setPosition(deskX, deskY);
        paletteEraser = sf::FloatRect(100, deskY + 40, 250, 200);
        paletteBrush = sf::FloatRect(1500, deskY + 20, 300, 250);
        paletteVignette = sf::FloatRect(650, deskY + 50, 90, 90);
        paletteFerris = sf::FloatRect(800, deskY + 50, 90, 90);
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

        startButton.setSize(sf::Vector2f(300.f, 120.f));
        startButton.setPosition(1920.f / 2.f - 150.f, 1080.f / 2.f - 60.f);
        startButton.setFillColor(sf::Color(234, 179, 8));
        startButton.setOutlineColor(sf::Color(161, 98, 7));
        startButton.setOutlineThickness(6.f);

        playIcon.setPointCount(3);
        playIcon.setPoint(0, sf::Vector2f(0.f, 0.f));
        playIcon.setPoint(1, sf::Vector2f(0.f, 60.f));
        playIcon.setPoint(2, sf::Vector2f(50.f, 30.f));
        playIcon.setFillColor(sf::Color::White);
        playIcon.setPosition(1920.f / 2.f - 15.f, 1080.f / 2.f - 30.f);

        mainFont.loadFromFile("assets/font.otf");
        uiText.setFont(mainFont);
        uiText.setCharacterSize(60);
        uiText.setOutlineColor(sf::Color::White);
        uiText.setOutlineThickness(4.f);
        showingText = false;
        textAlpha = 255.0f;
        promptBox.setSize(sf::Vector2f(800.f, 60.f));
        promptBox.setPosition(1920.f / 2.f - 400.f, 1080.f - 100.f);
        promptBox.setFillColor(sf::Color(30, 30, 30, 220));
        promptBox.setOutlineColor(sf::Color(0, 191, 255));
        promptBox.setOutlineThickness(3.f);

        promptDisplay.setFont(mainFont);
        promptDisplay.setCharacterSize(40);
        promptDisplay.setFillColor(sf::Color::White);
        promptDisplay.setPosition(1920.f / 2.f - 380.f, 1080.f - 95.f);
    }

    void processEvents() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) window.close();

            if (currentState == AppState::Menu) {
                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2f mousePos(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
                    if (startButton.getGlobalBounds().contains(mousePos)) {
                        currentState = AppState::Painting;
                    }
                }
            }
            else if (currentState == AppState::Painting) {
                if (event.type == sf::Event::TextEntered && isTypingPrompt) {
                    if (event.text.unicode == '\b') {
                        if (!currentPrompt.empty()) currentPrompt.pop_back();
                    }
                    else if (event.text.unicode < 128 && event.text.unicode != '\r' && event.text.unicode != '\n') {
                        currentPrompt += static_cast<char>(event.text.unicode);
                    }
                    promptDisplay.setString("> " + currentPrompt + "_");
                }
                if (event.type == sf::Event::KeyPressed) {
                    if (event.key.code == sf::Keyboard::Enter) {
                        isTypingPrompt = !isTypingPrompt;
                        if (isTypingPrompt) {
                            currentPrompt = "";
                            promptDisplay.setString("> _");
                            showMessage("Terminal: Type prompt and press Enter", sf::Color(0, 191, 255));
                        }
                        else {
                            promptQuantity = 1;
                            bool isFill = false;
                            std::string parsedTheme = "";
                            std::string tempWord;
                            std::stringstream ss(currentPrompt);

                            while (ss >> tempWord) {
                                std::string lowerWord = tempWord;
                                std::transform(lowerWord.begin(), lowerWord.end(), lowerWord.begin(), ::tolower);
                                bool isNum = !tempWord.empty();
                                for (char c : tempWord) if (!isdigit(c)) isNum = false;

                                if (isNum) promptQuantity = std::stoi(tempWord);
                                else if (lowerWord == "fill") isFill = true;
                                else if (lowerWord != "spawn" && lowerWord != "drop" && lowerWord != "with" && lowerWord != "a" && lowerWord != "some") {
                                    if (!parsedTheme.empty()) parsedTheme += " ";
                                    parsedTheme += tempWord;
                                }
                            }

                            if (isFill) promptQuantity = 999;
                            if (parsedTheme.empty()) parsedTheme = currentPrompt;

                            aiMascot.setTheme(parsedTheme);
                            std::string modeMsg = isFill ? "Mode: Fill Canvas with " + parsedTheme : "Prompt set: " + parsedTheme + " (x" + std::to_string(promptQuantity) + ")";
                            showMessage(modeMsg, sf::Color::Green);
                        }
                    }
                    if (isTypingPrompt) continue;
                    if (isTypingPrompt) continue;
                    if (event.key.code == sf::Keyboard::Num1 || event.key.code == sf::Keyboard::B) brushColor = sf::Color::Black;

                    if (event.key.code == sf::Keyboard::Num2 || event.key.code == sf::Keyboard::Numpad2) {
                        isAnimateMode = false; isPathMode = false; awaitingAnimStart = false; awaitingAnimEnd = false; awaitingPathStart = false; awaitingPathEnd = false;
                        aiMascot.setTheme("structure");
                        showMessage("Mode: Structure Only", sf::Color(0, 191, 255));
                    }
                    if (event.key.code == sf::Keyboard::Num3 || event.key.code == sf::Keyboard::Numpad3) {
                        isAnimateMode = false; isPathMode = false; awaitingAnimStart = false; awaitingAnimEnd = false; awaitingPathStart = false; awaitingPathEnd = false;
                        aiMascot.setTheme("clutter");
                        showMessage("Mode: Clutter Only", sf::Color(0, 191, 255));
                    }
                    if (event.key.code == sf::Keyboard::Num4 || event.key.code == sf::Keyboard::Numpad4) {
                        isAnimateMode = false; isPathMode = false; awaitingAnimStart = false; awaitingAnimEnd = false; awaitingPathStart = false; awaitingPathEnd = false;
                        aiMascot.setTheme("all");
                        showMessage("Mode: All Items", sf::Color(0, 191, 255));
                    }
                    if (event.key.code == sf::Keyboard::Num5 || event.key.code == sf::Keyboard::Numpad5) {
                        isAnimateMode = false; isPathMode = false; awaitingAnimStart = false; awaitingAnimEnd = false; awaitingPathStart = false; awaitingPathEnd = false;
                        aiMascot.setTheme("custom");
                        showMessage("Mode: Custom Art Only", sf::Color(0, 191, 255));
                    }
                    if (event.key.code == sf::Keyboard::Num6 || event.key.code == sf::Keyboard::Numpad6) {
                        isAnimateMode = true; isPathMode = false; awaitingAnimStart = false; awaitingAnimEnd = false; awaitingPathStart = false; awaitingPathEnd = false;
                        aiMascot.setTheme("all");
                        showMessage("Mode: Auto-Animate (Click Mascot)", sf::Color(0, 191, 255));
                    }
                    if (event.key.code == sf::Keyboard::Num7 || event.key.code == sf::Keyboard::Numpad7) {
                        isAnimateMode = false; isPathMode = true; awaitingAnimStart = false; awaitingAnimEnd = false; awaitingPathStart = false; awaitingPathEnd = false;
                        showMessage("Mode: Path-Finder (Click Mascot)", sf::Color(0, 191, 255));
                    }
                    if (event.key.code == sf::Keyboard::Num8 || event.key.code == sf::Keyboard::Numpad8) {
                        isLightingMode = !isLightingMode;
                        showMessage(isLightingMode ? "Mode: Dynamic Light ON" : "Mode: Dynamic Light OFF", sf::Color(255, 215, 0));
                    }
                    if (event.key.code == sf::Keyboard::Num9 || event.key.code == sf::Keyboard::Numpad9) {
                        isAnimateMode = false; isPathMode = false; awaitingAnimStart = false; awaitingAnimEnd = false; awaitingPathStart = false; awaitingPathEnd = false;
                        aiMascot.setTheme("wfc");
                        showMessage("Mode: Procedural Generation (WFC)", sf::Color(0, 191, 255));
                    }
                    if (event.key.code == sf::Keyboard::Num0 || event.key.code == sf::Keyboard::Numpad0) {
                        aiMascot.toggleTerrain();
                        showMessage(aiMascot.isTerrainEnabled() ? "Terrain Generation: ON" : "Terrain Generation: OFF", sf::Color(255, 215, 0));
                    }

                    if (event.key.code == sf::Keyboard::T) saveToDataset();
                    if (event.key.code == sf::Keyboard::U) sliceSpriteSheet("assets/spritesheet.png", 16, 16, "assets/sliced");
                    if (event.key.code == sf::Keyboard::Backspace) {
                        removeLastFromDataset();
                        aiMascot.trainOnDataset("dataset.json");
                    }
                    if (event.key.code == sf::Keyboard::C) {
                        undoHistory.push_back(frames[currentFrame]->getTexture().copyToImage());
                        redoHistory.clear();
                        frames[currentFrame]->clear(sf::Color::Transparent);
                        frames[currentFrame]->display();
                        aiMascot.clearCurrentFrame();
                    }
                    if (event.key.code == sf::Keyboard::Right && !isPlaying) {
                        currentFrame++;
                        if (currentFrame >= frames.size()) addNewFrame();
                        aiMascot.setFrame(currentFrame);
                    }
                    if (event.key.code == sf::Keyboard::Left && !isPlaying) {
                        if (currentFrame > 0) currentFrame--;
                        aiMascot.setFrame(currentFrame);
                    }
                    if (event.key.code == sf::Keyboard::R) resetAnimation();
                    if (event.key.code == sf::Keyboard::S) frames[currentFrame]->getTexture().copyToImage().saveToFile("export.png");
                    if (event.key.code == sf::Keyboard::E) {
                        for (size_t i = 0; i < frames.size(); ++i) {
                            frames[i]->getTexture().copyToImage().saveToFile("frame_" + std::to_string(i) + ".png");
                        }
                    }
                    if (event.key.code == sf::Keyboard::I) {
                        massIngestImages("C:\\Path\\To\\Your\\Downloaded\\Images");
                        aiMascot.trainOnDataset("dataset.json");
                    }
                    if (event.key.code == sf::Keyboard::Z && !undoHistory.empty()) {
                        redoHistory.push_back(frames[currentFrame]->getTexture().copyToImage());
                        sf::Texture tex;
                        tex.loadFromImage(undoHistory.back());
                        sf::Sprite spr(tex);
                        frames[currentFrame]->clear(sf::Color::Transparent);
                        frames[currentFrame]->draw(spr);
                        frames[currentFrame]->display();
                        undoHistory.pop_back();
                    }
                    if (event.key.code == sf::Keyboard::Y && !redoHistory.empty()) {
                        undoHistory.push_back(frames[currentFrame]->getTexture().copyToImage());
                        sf::Texture tex;
                        tex.loadFromImage(redoHistory.back());
                        sf::Sprite spr(tex);
                        frames[currentFrame]->clear(sf::Color::Transparent);
                        frames[currentFrame]->draw(spr);
                        frames[currentFrame]->display();
                        redoHistory.pop_back();
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
                    if (event.type == sf::Event::MouseButtonPressed) {
                        sf::Vector2f mousePos(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));

                        if (event.mouseButton.button == sf::Mouse::Middle && drawArea.contains(mousePos)) {
                            sf::Image currentImg = frames[currentFrame]->getTexture().copyToImage();
                            brushColor = currentImg.getPixel(static_cast<unsigned int>(mousePos.x), static_cast<unsigned int>(mousePos.y));
                            if (brushColor == sf::Color::Transparent) brushColor = sf::Color::White;
                        }
                        else if (event.mouseButton.button == sf::Mouse::Left) {
                            if (paletteVignette.contains(mousePos)) brushColor = sf::Color(255, 105, 180);
                            else if (paletteFerris.contains(mousePos)) brushColor = sf::Color(0, 191, 255);
                            else if (paletteEraser.contains(mousePos)) brushColor = sf::Color::Transparent;
                            else if (paletteBrush.contains(mousePos)) brushColor = sf::Color::Black;

                            else if (aiMascot.getBounds().contains(mousePos)) {
                                if (!showingText || uiText.getFillColor() != sf::Color::Red) {
                                    if (isPathMode) {
                                        awaitingPathStart = true;
                                        awaitingPathEnd = false;
                                        showMessage("Click Canvas for Structure A", sf::Color(255, 215, 0));
                                    }
                                    else if (isAnimateMode) {
                                        awaitingAnimStart = true;
                                        awaitingAnimEnd = false;
                                        showMessage("Click Canvas for Start Position", sf::Color(255, 215, 0));
                                    }
                                    else {
                                        aiMascot.toggle();
                                        if (aiMascot.isActive()) {
                                            undoHistory.push_back(frames[currentFrame]->getTexture().copyToImage());
                                            redoHistory.clear();

                                            for (int i = 0; i < promptQuantity; ++i) {
                                                sf::Image currentImg = frames[currentFrame]->getTexture().copyToImage();
                                                std::string errorMsg = aiMascot.startGeneratingComplexArt(drawArea, currentImg, false);

                                                if (!errorMsg.empty()) {
                                                    if (i == 0) {
                                                        showMessage(errorMsg, sf::Color::Red);
                                                        undoHistory.pop_back();
                                                        aiMascot.toggle();
                                                    }
                                                    else if (promptQuantity == 999) {
                                                        showMessage("Canvas Filled!", sf::Color::Green);
                                                    }
                                                    break;
                                                }

                                                if (promptQuantity > 1) {
                                                    aiMascot.forceFinish(*frames[currentFrame]);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            else if (drawArea.contains(mousePos)) {
                                if (isPathMode) {
                                    if (awaitingPathStart) {
                                        pathStartPos = mousePos;
                                        awaitingPathStart = false;
                                        awaitingPathEnd = true;
                                        showMessage("Click Canvas for Structure B", sf::Color(255, 215, 0));
                                    }
                                    else if (awaitingPathEnd) {
                                        aiMascot.generatePath(*frames[currentFrame], pathStartPos, mousePos, drawArea);
                                        awaitingPathEnd = false;
                                        showMessage("Path Connected!", sf::Color::Green);
                                    }
                                }
                                else if (awaitingAnimStart) {
                                    animStartPos = mousePos;
                                    awaitingAnimStart = false;
                                    awaitingAnimEnd = true;
                                    showMessage("Click Canvas for End Position", sf::Color(255, 215, 0));
                                }
                                else if (awaitingAnimEnd) {
                                    sf::Vector2f animEndPos = mousePos;
                                    awaitingAnimEnd = false;

                                    sf::Image currentImg = frames[currentFrame]->getTexture().copyToImage();
                                    std::string errorMsg = aiMascot.startGeneratingComplexArt(drawArea, currentImg, true);

                                    if (!errorMsg.empty()) {
                                        showMessage(errorMsg, sf::Color::Red);
                                    }
                                    else {
                                        aiMascot.cancelSlowDraw();

                                        float aiW = aiMascot.getArtWidth();
                                        float aiH = aiMascot.getArtHeight();

                                        float startX = animStartPos.x - (aiW / 2.0f);
                                        float startY = animStartPos.y - (aiH / 2.0f);
                                        float endX = animEndPos.x - (aiW / 2.0f);
                                        float endY = animEndPos.y - (aiH / 2.0f);

                                        float totalDist = std::sqrt((endX - startX) * (endX - startX) + (endY - startY) * (endY - startY));
                                        int baseFrames = (int)(totalDist / 40.0f); 
                                        int animFrames = baseFrames < 8 ? 8 : (baseFrames > 20 ? 20 : baseFrames); 
                                        int totalSimFrames = animFrames + 15;

                                        while (frames.size() <= currentFrame + totalSimFrames) {
                                            addNewFrame();
                                        }

                                        float dx = (endX - startX) / (animFrames - 1);
                                        float dy = (endY - startY) / (animFrames - 1);
                                        float jumpHeight = (totalDist * 0.35f < 250.0f) ? totalDist * 0.35f : 250.0f;

                                        struct Dust { float x, y, vx, vy; int life, maxLife; };
                                        std::vector<Dust> particles;

                                        for (int i = 0; i < totalSimFrames; ++i) {
                                            if (i < animFrames) {
                                                float progress = i / (float)(animFrames - 1);
                                                float arcY = std::sin(progress * 3.14159265f) * jumpHeight;

                                                float currentX = startX + (dx * i);
                                                float currentY = startY + (dy * i) - arcY;

                                                aiMascot.stampOnCanvas(*frames[currentFrame + i], currentX, currentY);

                                                if (i == 0 || i == animFrames - 1) {
                                                    for (int p = 0; p < 30; ++p) {
                                                        float vx = ((std::rand() % 100) - 50) * 0.15f;
                                                        float vy = ((std::rand() % 100) - 100) * 0.15f;
                                                        int life = 10 + (std::rand() % 10);
                                                        particles.push_back({ currentX + (aiW / 2.0f), currentY + aiH, vx, vy, life, life });
                                                    }
                                                }
                                            }

                                            for (auto& p : particles) {
                                                if (p.life > 0) {
                                                    sf::RectangleShape dustShape(sf::Vector2f(6.0f, 6.0f));
                                                    int alpha = (int)(200.0f * ((float)p.life / p.maxLife));
                                                    dustShape.setFillColor(sf::Color(150, 140, 130, alpha));
                                                    dustShape.setPosition(p.x, p.y);
                                                    frames[currentFrame + i]->draw(dustShape);

                                                    p.x += p.vx;
                                                    p.y += p.vy;
                                                    p.vx *= 0.95f;
                                                    p.vy += 0.4f;
                                                    p.life--;
                                                }
                                            }

                                            frames[currentFrame + i]->display();
                                        }

                                        showMessage("Advanced Physics Animation Generated!", sf::Color::Green);
                                    }
                                }
                                else {
                                    undoHistory.push_back(frames[currentFrame]->getTexture().copyToImage());
                                    redoHistory.clear();
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
                        }
                    }

                    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
                        isDrawing = false;
                    }

                    if (event.type == sf::Event::MouseMoved) {
                        currentMousePos = sf::Vector2f(static_cast<float>(event.mouseMove.x), static_cast<float>(event.mouseMove.y));
                        window.setMouseCursorVisible(!drawArea.contains(currentMousePos));

                        if (isDrawing && !awaitingAnimStart && !awaitingAnimEnd && !awaitingPathStart && !awaitingPathEnd) {
                            if (!drawArea.contains(currentMousePos)) {
                                isDrawing = false;
                                continue;
                            }

                            sf::Vector2f d = currentMousePos - lastPos;
                            float length = std::sqrt(d.x * d.x + d.y * d.y);

                            sf::RectangleShape line(sf::Vector2f(length, brushSize));
                            line.setOrigin(0, brushSize / 2.f);
                            line.setPosition(lastPos);
                            line.setRotation(std::atan2(d.y, d.x) * 180.f / 3.14159265f);
                            line.setFillColor(brushColor);

                            sf::CircleShape circle(brushSize / 2.f);
                            circle.setOrigin(brushSize / 2.f, brushSize / 2.f);
                            circle.setPosition(currentMousePos);
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
                            lastPos = currentMousePos;
                        }
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
    }

    void sliceSpriteSheet(const std::string& filePath, int tileWidth, int tileHeight, const std::string& outputFolder) {
        sf::Image sheet;
        if (!sheet.loadFromFile(filePath)) return;
        std::filesystem::create_directories(outputFolder);
        int cols = sheet.getSize().x / tileWidth;
        int rows = sheet.getSize().y / tileHeight;
        int count = 0;
        for (int y = 0; y < rows; ++y) {
            for (int x = 0; x < cols; ++x) {
                sf::Image tile;
                tile.create(tileWidth, tileHeight, sf::Color::Transparent);
                tile.copy(sheet, 0, 0, sf::IntRect(x * tileWidth, y * tileHeight, tileWidth, tileHeight), true);
                bool isEmpty = true;
                for (unsigned int ty = 0; ty < tile.getSize().y; ++ty) {
                    for (unsigned int tx = 0; tx < tile.getSize().x; ++tx) {
                        if (tile.getPixel(tx, ty).a > 20) {
                            isEmpty = false;
                            break;
                        }
                    }
                    if (!isEmpty) break;
                }
                if (!isEmpty) {
                    tile.saveToFile(outputFolder + "/sliced_" + std::to_string(count) + ".png");
                    count++;
                }
            }
        }
    }

    void massIngestImages(const std::string& folderPath) {
        std::ofstream file("dataset.json", std::ios::app);
        if (!file.is_open()) return;
        int processedCount = 0;
        for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
            if (entry.path().extension() == ".png" || entry.path().extension() == ".jpg") {
                sf::Image img;
                if (img.loadFromFile(entry.path().string())) {
                    unsigned int width = img.getSize().x;
                    unsigned int height = img.getSize().y;
                    for (int y = 0; y < 48; ++y) {
                        std::string row = "";
                        for (int x = 0; x < 48; ++x) {
                            unsigned int sampleX = (x * width) / 48;
                            unsigned int sampleY = (y * height) / 48;
                            sf::Color c = img.getPixel(sampleX, sampleY);
                            if (c.a > 20 && (c.r < 250 || c.g < 250 || c.b < 250)) {
                                row += "X";
                            }
                            else {
                                row += ".";
                            }
                        }
                        file << row << "\n";
                    }
                    file << "\n";
                    processedCount++;
                }
            }
        }
        file.close();
    }

    void update() {
        if (currentState == AppState::Painting) {
            aiMascot.update(*frames[currentFrame]);

            if (isPlaying) {
                if (playClock.getElapsedTime().asSeconds() >= timePerFrame) {
                    currentFrame++;
                    if (currentFrame >= frames.size()) {
                        currentFrame = 0;
                    }
                    playClock.restart();
                }
            }

            if (showingText) {
                float timePassed = textClock.getElapsedTime().asSeconds();
                if (timePassed > 1.5f) {
                    showingText = false;
                }
                else {
                    if (timePassed > 0.5f) {
                        textAlpha -= 255.0f * (1.0f / 60.0f);
                        if (textAlpha < 0) textAlpha = 0;
                    }

                    sf::Color fillColor = uiText.getFillColor();
                    sf::Color outlineColor = uiText.getOutlineColor();
                    fillColor.a = static_cast<sf::Uint8>(textAlpha);
                    outlineColor.a = static_cast<sf::Uint8>(textAlpha);

                    uiText.setFillColor(fillColor);
                    uiText.setOutlineColor(outlineColor);
                }
            }
        }
    }

    void render() {
        window.clear(sf::Color::White);
        window.draw(bgSprite);

        if (currentState == AppState::Menu) {
            window.draw(startButton);
            window.draw(playIcon);
        }
        else if (currentState == AppState::Painting) {
            window.draw(canvasSprite);

            if (!isPlaying && currentFrame > 0) {
                sf::Sprite onionSkin(frames[currentFrame - 1]->getTexture());
                onionSkin.setColor(sf::Color(255, 255, 255, 85));
                window.draw(onionSkin, sf::BlendAlpha);
            }

            if (isLightingMode) {
                sf::Vector2f sunPos = currentMousePos;
                const auto& items = aiMascot.getHistory();

                for (const auto& item : items) {
                    bool isClutter = (item.category == "healing" || item.category == "status-cures" || item.category == "vitamins" || item.category == "clutter");
                    float shadowLen = isClutter ? 30.0f : 150.0f;

                    sf::Vector2f baseCenter(item.bounds.left + item.bounds.width / 2.0f, item.bounds.top + item.bounds.height);
                    sf::Vector2f dir = baseCenter - sunPos;
                    float dist = std::sqrt(dir.x * dir.x + dir.y * dir.y);
                    if (dist > 0) { dir.x /= dist; dir.y /= dist; }

                    sf::ConvexShape shadow;
                    shadow.setPointCount(4);
                    shadow.setPoint(0, sf::Vector2f(item.bounds.left, item.bounds.top + item.bounds.height));
                    shadow.setPoint(1, sf::Vector2f(item.bounds.left + item.bounds.width, item.bounds.top + item.bounds.height));
                    shadow.setPoint(2, sf::Vector2f(item.bounds.left + item.bounds.width + dir.x * shadowLen, item.bounds.top + item.bounds.height + dir.y * shadowLen));
                    shadow.setPoint(3, sf::Vector2f(item.bounds.left + dir.x * shadowLen, item.bounds.top + item.bounds.height + dir.y * shadowLen));

                    shadow.setFillColor(sf::Color(0, 0, 0, 100));
                    window.draw(shadow);
                }

                sf::CircleShape sunShape(15);
                sunShape.setOrigin(15, 15);
                sunShape.setPosition(sunPos);
                sunShape.setFillColor(sf::Color(255, 255, 200, 200));
                sunShape.setOutlineThickness(2);
                sunShape.setOutlineColor(sf::Color::Yellow);
                window.draw(sunShape);
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

                if (drawArea.contains(currentMousePos)) {
                    brushPreview.setPosition(currentMousePos);
                }
                else {
                    brushPreview.setPosition(50, 940);
                }
                window.draw(brushPreview);
            }

            if (showingText) {
                window.draw(uiText);
            }
            if (isTypingPrompt) {
                window.draw(promptBox);
                window.draw(promptDisplay);
            }
        }

        window.display();
    }

    void saveToDataset() {
        sf::Image img = frames[currentFrame]->getTexture().copyToImage();
        std::ifstream inFile("dataset.json");
        std::string content((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
        inFile.close();
        size_t lastBracket = content.find_last_of(']');
        if (lastBracket != std::string::npos) {
            content.erase(lastBracket);
        }
        else {
            content = "[\n";
        }
        std::vector<std::string> newPixels;
        int res = 48;
        for (int y = 0; y < res; ++y) {
            std::string row = "";
            for (int x = 0; x < res; ++x) {
                int px = static_cast<int>(drawArea.left + (x * (drawArea.width / (float)res)));
                int py = static_cast<int>(drawArea.top + (y * (drawArea.height / (float)res)));
                if (px >= 0 && px < 1920 && py >= 0 && py < 1080) {
                    sf::Color c = img.getPixel(px, py);
                    if (c.a > 20) {
                        float lum = (0.299 * c.r + 0.587 * c.g + 0.114 * c.b);
                        if (lum < 40) row += "O";
                        else if (lum < 90) row += "S";
                        else if (lum < 160) row += "X";
                        else if (lum < 220) row += "H";
                        else row += "W";
                    }
                    else {
                        row += ".";
                    }
                }
                else {
                    row += ".";
                }
            }
            newPixels.push_back(row);
        }
        bool isBlank = true;
        for (const auto& r : newPixels) {
            if (r.find_first_not_of('.') != std::string::npos) {
                isBlank = false;
                break;
            }
        }
        if (isBlank) return;
        std::ofstream outFile("dataset.json");
        outFile << content;
        if (content.find("}") != std::string::npos) {
            outFile << ",\n";
        }
        int randomID = rand() % 99999;
        outFile << "  {\n";
        outFile << "    \"name\": \"CustomArt_" << randomID << "\",\n";
        outFile << "    \"category\": \"custom\",\n";
        outFile << "    \"scale\": 1.0,\n";
        outFile << "    \"width\": " << res << ",\n";
        outFile << "    \"height\": " << res << ",\n";
        outFile << "    \"pixels\": [\n";
        for (size_t i = 0; i < newPixels.size(); ++i) {
            outFile << "      \"" << newPixels[i] << "\"";
            if (i < newPixels.size() - 1) outFile << ",";
            outFile << "\n";
        }
        outFile << "    ]\n";
        outFile << "  }\n";
        outFile << "]\n";
        outFile.close();
    }

    void removeLastFromDataset() {
        std::ifstream inFile("dataset.json");
        if (!inFile.is_open()) return;
        std::vector<std::string> allLines;
        std::string line;
        while (std::getline(inFile, line)) {
            allLines.push_back(line);
        }
        inFile.close();
        std::vector<std::vector<std::string>> templates;
        std::vector<std::string> currentBlock;
        for (const auto& l : allLines) {
            if (l.empty() || l.find_first_not_of("\r\n\t ") == std::string::npos) {
                if (currentBlock.size() == 48) {
                    templates.push_back(currentBlock);
                }
                currentBlock.clear();
            }
            else {
                currentBlock.push_back(l);
            }
        }
        if (currentBlock.size() == 48) {
            templates.push_back(currentBlock);
        }
        if (templates.empty()) return;
        templates.pop_back();
        std::ofstream outFile("dataset.json", std::ios::trunc);
        for (const auto& tmpl : templates) {
            for (const auto& r : tmpl) {
                outFile << r << "\n";
            }
            outFile << "\n";
        }
        outFile.close();
    }

public:
    WisdomPark()
        : window(sf::VideoMode(1920, 1080), "Wisdom Park", sf::Style::Fullscreen),
        currentFrame(0), isDrawing(false), brushSize(5.0f),
        brushColor(sf::Color::Black), isPlaying(false),
        timePerFrame(1.0f / 12.0f),
        currentState(AppState::Menu),
        isAnimateMode(false), awaitingAnimStart(false), awaitingAnimEnd(false),
        isPathMode(false), awaitingPathStart(false), awaitingPathEnd(false),
        isLightingMode(false), isTypingPrompt(false), promptQuantity(1)
    {
        window.setFramerateLimit(60);
        window.setKeyRepeatEnabled(false);
        setupUI();
        addNewFrame();
        aiMascot.trainOnDataset("dataset.json");
        aiMascot.loadThesaurus("thesaurus.txt");
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