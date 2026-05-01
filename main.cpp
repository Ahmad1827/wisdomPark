#include <SFML/Graphics.hpp>
#include <cmath>
#include <vector>
#include <memory>
#include <string>
#include <iostream>
#include <filesystem>
#include <fstream>
#include "AIHelper.h"

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
    sf::Text errorText;
    sf::Clock errorClock;
    bool showingError;
    float errorAlpha; // NEW: Controls the fade

    void addNewFrame() {
        auto tex = std::make_unique<sf::RenderTexture>();
        tex->create(1920, 1080);
        tex->clear(sf::Color::Transparent);
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
        errorText.setFont(mainFont);
        errorText.setCharacterSize(60);
        errorText.setFillColor(sf::Color::Red);
        errorText.setOutlineColor(sf::Color::White);
        errorText.setOutlineThickness(4.f);
        showingError = false;
        errorAlpha = 255.0f;
    }

    void processEvents() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();

            if (currentState == AppState::Menu) {
                if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);
                    if (startButton.getGlobalBounds().contains(mousePos)) {
                        currentState = AppState::Painting;
                    }
                }
            }
            else if (currentState == AppState::Painting) {
                if (event.type == sf::Event::KeyPressed) {
                    if (event.key.code == sf::Keyboard::Num1 || event.key.code == sf::Keyboard::B) brushColor = sf::Color::Black;
                    if (event.key.code == sf::Keyboard::T) saveToDataset();
                    if (event.key.code == sf::Keyboard::U) sliceSpriteSheet("C:\\Path\\To\\Your\\spritesheet.png", 16, 16, "C:\\Path\\To\\Your\\SlicedItemsFolder");
                    if (event.key.code == sf::Keyboard::Backspace) {
                        removeLastFromDataset();
                        aiMascot.trainOnDataset("dataset.json");
                    }
                    if (event.key.code == sf::Keyboard::C) {
                        undoHistory.push_back(frames[currentFrame]->getTexture().copyToImage());
                        redoHistory.clear();
                        frames[currentFrame]->clear(sf::Color::Transparent);
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
                    if (event.key.code == sf::Keyboard::R) resetAnimation();
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
                    if (event.type == sf::Event::MouseButtonPressed) {
                        sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);

                        if (event.mouseButton.button == sf::Mouse::Middle && drawArea.contains(mousePos)) {
                            sf::Image currentImg = frames[currentFrame]->getTexture().copyToImage();
                            brushColor = currentImg.getPixel(mousePos.x, mousePos.y);
                            if (brushColor == sf::Color::Transparent) brushColor = sf::Color::White;
                        }
                        else if (event.mouseButton.button == sf::Mouse::Left) {
                            if (paletteVignette.contains(mousePos)) brushColor = sf::Color(255, 105, 180);
                            else if (paletteFerris.contains(mousePos)) brushColor = sf::Color(0, 191, 255);
                            else if (paletteEraser.contains(mousePos)) brushColor = sf::Color::Transparent;
                            else if (paletteBrush.contains(mousePos)) brushColor = sf::Color::Black;

                            // NEW: Block clicking the mascot if an error is currently fading
                            else if (aiMascot.getBounds().contains(mousePos) && !showingError) {
                                aiMascot.toggle();
                                if (aiMascot.isActive()) {
                                    undoHistory.push_back(frames[currentFrame]->getTexture().copyToImage());
                                    redoHistory.clear();

                                    std::string errorMsg = aiMascot.startGeneratingComplexArt(drawArea);
                                    if (!errorMsg.empty()) {
                                        errorText.setString(errorMsg);
                                        sf::FloatRect textRect = errorText.getLocalBounds();
                                        errorText.setOrigin(textRect.left + textRect.width / 2.0f, textRect.top + textRect.height / 2.0f);
                                        errorText.setPosition(1920.0f / 2.0f, 150.0f);
                                        showingError = true;
                                        errorAlpha = 255.0f; // Reset Alpha
                                        errorClock.restart();

                                        undoHistory.pop_back();
                                        aiMascot.toggle();
                                    }
                                }
                            }
                            else if (drawArea.contains(mousePos)) {
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

                    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
                        isDrawing = false;
                    }

                    if (event.type == sf::Event::MouseMoved) {
                        currentMousePos = sf::Vector2f(event.mouseMove.x, event.mouseMove.y);
                        window.setMouseCursorVisible(!drawArea.contains(currentMousePos));

                        if (isDrawing) {
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

            // NEW: Handle Error Fading Logic
            if (showingError) {
                float timePassed = errorClock.getElapsedTime().asSeconds();
                if (timePassed > 1.5f) { // Fast 1.5 second max duration
                    showingError = false;
                }
                else {
                    // Start fading out after 0.5 seconds
                    if (timePassed > 0.5f) {
                        errorAlpha -= 255.0f * (1.0f / 60.0f); // Fast fade
                        if (errorAlpha < 0) errorAlpha = 0;
                    }

                    sf::Color fillColor = errorText.getFillColor();
                    sf::Color outlineColor = errorText.getOutlineColor();
                    fillColor.a = static_cast<sf::Uint8>(errorAlpha);
                    outlineColor.a = static_cast<sf::Uint8>(errorAlpha);

                    errorText.setFillColor(fillColor);
                    errorText.setOutlineColor(outlineColor);
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

                if (drawArea.contains(currentMousePos)) {
                    brushPreview.setPosition(currentMousePos);
                }
                else {
                    brushPreview.setPosition(50, 940);
                }
                window.draw(brushPreview);
            }

            // NEW: Draw fading error
            if (showingError) {
                window.draw(errorText);
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
        : window(sf::VideoMode(1920, 1080), "Wisdom Park"),
        currentFrame(0), isDrawing(false), brushSize(5.0f),
        brushColor(sf::Color::Black), isPlaying(false),
        timePerFrame(1.0f / 12.0f),
        currentState(AppState::Menu)
    {
        window.setFramerateLimit(60);
        setupUI();
        addNewFrame();
        aiMascot.trainOnDataset("dataset.json");
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
