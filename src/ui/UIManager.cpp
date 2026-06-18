#include "UIManager.h"
#include <iostream>

UIManager::UIManager() : isTypingPrompt(false), showingText(false), textAlpha(255.0f), isLightingMode(false), promptQuantity(1), focusMode(false) {}

void UIManager::init() {
    bgTexture.loadFromFile("assets/landofwisdompark.png");
    bgSprite.setTexture(bgTexture);

    font.loadFromFile("assets/font.otf");

    welcomeScreen.init();
    settingsModal.init();
    leftToolbar.init();
    rightProperties.init();
    bottomTimeline.init();

    uiText.setFont(font);
    uiText.setCharacterSize(30);
    uiText.setOutlineColor(sf::Color(0, 0, 0, 150));
    uiText.setOutlineThickness(2.f);

    promptBox.setSize(sf::Vector2f(600.f, 50.f));
    promptBox.setPosition(1920.f / 2.f - 300.f, 1080.f - 300.f);
    promptBox.setFillColor(sf::Color(15, 15, 18, 220));
    promptBox.setOutlineThickness(1.f);
    promptBox.setOutlineColor(sf::Color(255, 255, 255, 30));

    promptDisplay.setFont(font);
    promptDisplay.setCharacterSize(20);
    promptDisplay.setFillColor(sf::Color::White);
    promptDisplay.setPosition(1920.f / 2.f - 290.f, 1080.f - 288.f);
}

void UIManager::showMessage(const std::string& msg, sf::Color color) {
    uiText.setString(msg);
    uiText.setFillColor(color);
    sf::FloatRect textRect = uiText.getLocalBounds();
    uiText.setOrigin(textRect.left + textRect.width / 2.0f, textRect.top + textRect.height / 2.0f);
    uiText.setPosition(1920.0f / 2.0f, 80.0f);
    showingText = true;
    textAlpha = 255.0f;
    textClock.restart();
}

void UIManager::handleEvent(const sf::Event& event, sf::RenderWindow& window, AppState& currentState, AppSettings& settings, Canvas& canvas, Timeline& timeline, AIHelper& aiHelper) {
    sf::Vector2f mousePos(static_cast<float>(sf::Mouse::getPosition(window).x), static_cast<float>(sf::Mouse::getPosition(window).y));

    sf::Vector2f logicalMousePos = canvas.getInverseTransform().transformPoint(mousePos);

    if (settingsModal.getIsOpen()) {
        if (event.type == sf::Event::TextEntered) {
            settingsModal.handleTextEntered(event.text.unicode);
        }
        else if (event.type == sf::Event::KeyPressed) {
            settingsModal.handleKeyPress(event.key.code, settings);
        }
        else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            settingsModal.handleClick(mousePos, settings);
        }
        return;
    }

    if (currentState == AppState::Welcome) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            std::string action = welcomeScreen.handleClick(mousePos);
            if (action == "new_project") {
                currentState = AppState::Painting;
            }
            else if (action == "config_ai") {
                settingsModal.open(settings);
            }
            else if (action == "exit") {
                window.close();
            }
        }
    }
    else if (currentState == AppState::Painting) {
        if (event.type == sf::Event::TextEntered && isTypingPrompt) {
            if (event.text.unicode == '\b' && !currentPrompt.empty()) {
                currentPrompt.pop_back();
            }
            else if (event.text.unicode < 128 && event.text.unicode != '\r' && event.text.unicode != '\n' && event.text.unicode != '\b') {
                currentPrompt += static_cast<char>(event.text.unicode);
            }
            promptDisplay.setString("> " + currentPrompt + "_");
        }

        if (event.type == sf::Event::KeyPressed) {
            bool ctrlPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::RControl);

            if (event.key.code == sf::Keyboard::Enter) {
                isTypingPrompt = !isTypingPrompt;
                if (isTypingPrompt) {
                    currentPrompt = "";
                    promptDisplay.setString("> _");
                    showMessage("AI Terminal: Type prompt and press Enter", sf::Color(0, 191, 255));
                }
                else {
                    if (!settings.isConfigured()) {
                        showMessage("ERROR: Configure AI Provider in Settings (Press ESC)", sf::Color::Red);
                    }
                    else {
                        bool isFill;
                        std::string parsedTheme;
                        aiHelper.parseCommand(currentPrompt, promptQuantity, isFill, parsedTheme);
                        aiHelper.setTheme(parsedTheme);
                        showMessage("AI Configured. Click Canvas to Generate: " + parsedTheme, sf::Color::Green);
                    }
                }
            }

            if (isTypingPrompt) return;

            if (event.key.code == sf::Keyboard::Escape) {
                settingsModal.open(settings);
            }
            if (event.key.code == sf::Keyboard::Tab) {
                focusMode = !focusMode;
                if (focusMode) showMessage("Focus Mode: ON (Press TAB to exit)", sf::Color::White);
                else showMessage("Focus Mode: OFF", sf::Color::White);
            }

            if (event.key.code == sf::Keyboard::Right) timeline.nextFrame();
            if (event.key.code == sf::Keyboard::Left) timeline.prevFrame();
            if (event.key.code == sf::Keyboard::Space) timeline.togglePlayback();
            if (event.key.code == sf::Keyboard::Home) timeline.setFrame(0);
            if (event.key.code == sf::Keyboard::End) timeline.setFrame(canvas.getFrameCount() - 1);

            if (event.key.code == sf::Keyboard::Delete) {
                if (canvas.getFrameCount() > 1) {
                    canvas.deleteFrame(timeline.getCurrentFrame());
                    if (timeline.getCurrentFrame() >= canvas.getFrameCount()) {
                        timeline.setFrame(canvas.getFrameCount() - 1);
                    }
                }
            }

            if (ctrlPressed && event.key.code == sf::Keyboard::D) {
                canvas.duplicateFrame(timeline.getCurrentFrame());
                timeline.nextFrame();
            }
            if (ctrlPressed && event.key.code == sf::Keyboard::N) {
                canvas.addFrame(timeline.getCurrentFrame());
                timeline.nextFrame();
            }

            if (event.key.code == sf::Keyboard::C) canvas.saveUndoState(timeline.getCurrentFrame());
            if (event.key.code == sf::Keyboard::Z) canvas.undo(timeline.getCurrentFrame());
            if (event.key.code == sf::Keyboard::Y) canvas.redo(timeline.getCurrentFrame());
        }

        if (!timeline.isPlaying() && !settingsModal.getIsOpen()) {
            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {

                std::string leftAction = leftToolbar.handleClick(mousePos, settings.isConfigured());
                if (!leftAction.empty()) {
                    if (leftAction == "ai_disabled") {
                        showMessage("Configure an AI provider (Press ESC).", sf::Color::Red);
                    }
                    else if (leftAction == "pencil") {
                        canvas.setBrushColor(sf::Color::Black);
                    }
                    else if (leftAction == "eraser") {
                        canvas.setBrushColor(sf::Color::Transparent);
                    }
                    else if (leftAction == "ai_gen") {
                        isTypingPrompt = true;
                        currentPrompt = "";
                        promptDisplay.setString("> _");
                        showMessage("AI Terminal: Type prompt and press Enter", sf::Color(0, 191, 255));
                    }
                    return; // Prevent drawing if panel clicked
                }

                std::string rightAction = rightProperties.handleClick(mousePos);
                if (!rightAction.empty()) {
                    if (rightAction == "theme_all") aiHelper.setTheme("all");
                    else if (rightAction == "theme_struct") aiHelper.setTheme("structure");
                    else if (rightAction == "theme_clutter") aiHelper.setTheme("clutter");
                    else if (rightAction == "theme_custom") aiHelper.setTheme("custom");
                    else if (rightAction == "theme_wfc") aiHelper.setTheme("wfc");
                    else if (rightAction == "toggle_light") isLightingMode = !isLightingMode;
                    else if (rightAction == "toggle_terrain") aiHelper.toggleTerrain();
                    else if (rightAction == "onion_toggle") canvas.setOnionSkin(!canvas.isOnionSkinEnabled(), canvas.getOnionSkinOpacity());
                    else if (rightAction == "onion_op_up") canvas.setOnionSkin(canvas.isOnionSkinEnabled(), canvas.getOnionSkinOpacity() + 25.f);
                    else if (rightAction == "onion_op_down") canvas.setOnionSkin(canvas.isOnionSkinEnabled(), canvas.getOnionSkinOpacity() - 25.f);

                    rightProperties.syncState(aiHelper.getTheme(), isLightingMode, aiHelper.isTerrainEnabled(), canvas.isOnionSkinEnabled(), canvas.getOnionSkinOpacity());
                    return;
                }

                std::string bottomAction = bottomTimeline.handleClick(mousePos);
                if (!bottomAction.empty()) {
                    if (bottomAction == "play") timeline.togglePlayback();
                    else if (bottomAction == "add") { canvas.addFrame(timeline.getCurrentFrame()); timeline.nextFrame(); }
                    else if (bottomAction == "dup") { canvas.duplicateFrame(timeline.getCurrentFrame()); timeline.nextFrame(); }
                    else if (bottomAction == "del") {
                        if (canvas.getFrameCount() > 1) {
                            canvas.deleteFrame(timeline.getCurrentFrame());
                            if (timeline.getCurrentFrame() >= canvas.getFrameCount()) timeline.setFrame(canvas.getFrameCount() - 1);
                        }
                    }
                    return;
                }

                int clickedFrame = bottomTimeline.handleFrameClick(mousePos, canvas.getFrameCount());
                if (clickedFrame != -1) {
                    timeline.setFrame(clickedFrame);
                    return;
                }

                if (aiHelper.getBounds().contains(logicalMousePos)) {
                    if (!settings.isConfigured()) {
                        showMessage("Configure AI Provider (Press ESC) first!", sf::Color::Red);
                    }
                    else {
                        aiHelper.toggle();
                        if (aiHelper.isActive()) {
                            canvas.saveUndoState(timeline.getCurrentFrame());
                            sf::Image currentImg = canvas.getActiveRenderTexture(timeline.getCurrentFrame())->getTexture().copyToImage();

                            int spawnedCount = 0;
                            for (int i = 0; i < promptQuantity; ++i) {
                                sf::Image iterImg = canvas.getActiveRenderTexture(timeline.getCurrentFrame())->getTexture().copyToImage();
                                std::string errorMsg = aiHelper.startGeneratingComplexArt(canvas.getDrawArea(), iterImg, settings.activeProvider, settings.apiKeys[settings.activeProvider], false);

                                if (!errorMsg.empty()) {
                                    if (spawnedCount == 0) {
                                        showMessage(errorMsg, sf::Color::Red);
                                        canvas.undo(timeline.getCurrentFrame());
                                        aiHelper.toggle();
                                    }
                                    else if (promptQuantity == 999) {
                                        showMessage("Canvas Filled!", sf::Color::Green);
                                    }
                                    else {
                                        showMessage("Spawned " + std::to_string(spawnedCount) + " (Canvas Full!)", sf::Color::Yellow);
                                    }
                                    break;
                                }
                                spawnedCount++;
                                if (promptQuantity > 1) {
                                    aiHelper.forceFinish(*canvas.getActiveRenderTexture(timeline.getCurrentFrame()));
                                }
                            }
                            if (spawnedCount == promptQuantity && promptQuantity > 1) {
                                showMessage("Spawned all " + std::to_string(spawnedCount) + " items!", sf::Color::Green);
                            }
                        }
                    }
                    return;
                }

                canvas.handleMousePressed(logicalMousePos, false, timeline.getCurrentFrame());
            }

            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
                canvas.handleMouseReleased();
            }

            if (event.type == sf::Event::MouseMoved) {
                canvas.handleMouseMoved(logicalMousePos, timeline.getCurrentFrame());
            }

            if (event.type == sf::Event::MouseWheelScrolled) {
                if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
                    canvas.setBrushSize(canvas.getBrushSize() + event.mouseWheelScroll.delta);
                }
            }
        }
    }
}

void UIManager::update(sf::RenderWindow& window, AppState currentState, AppSettings& settings, float dt, Canvas& canvas) {
    sf::Vector2f mousePos(static_cast<float>(sf::Mouse::getPosition(window).x), static_cast<float>(sf::Mouse::getPosition(window).y));

    if (settingsModal.getIsOpen()) {
        settingsModal.updateHover(mousePos);
    }

    if (currentState == AppState::Welcome) {
        welcomeScreen.updateHover(mousePos);
        welcomeScreen.updateStatus(settings.isConfigured(), settings.activeProvider);
    }
    else if (currentState == AppState::Painting) {

        leftToolbar.update(dt, focusMode);
        rightProperties.update(dt, focusMode);
        bottomTimeline.update(dt, focusMode);

        sf::FloatRect availableSpace(
            std::max(0.f, leftToolbar.getPanelRightEdge()),
            0.f,
            std::min(1920.f, rightProperties.getPanelLeftEdge()) - std::max(0.f, leftToolbar.getPanelRightEdge()),
            std::min(1080.f, bottomTimeline.getPanelTopEdge())
        );
        canvas.updateTransform(dt, availableSpace);

        leftToolbar.updateHover(mousePos);
        rightProperties.updateHover(mousePos);
        bottomTimeline.updateHover(mousePos);

        if (showingText && textClock.getElapsedTime().asSeconds() > 2.0f) {
            showingText = false;
        }
        else if (showingText && textClock.getElapsedTime().asSeconds() > 1.5f) {
            textAlpha -= 255.0f * (1.0f / 60.0f);
            if (textAlpha < 0) textAlpha = 0;

            sf::Color fillColor = uiText.getFillColor();
            sf::Color outlineColor = uiText.getOutlineColor();
            fillColor.a = static_cast<sf::Uint8>(textAlpha);
            outlineColor.a = static_cast<sf::Uint8>(textAlpha);
            uiText.setFillColor(fillColor);
            uiText.setOutlineColor(outlineColor);
        }
    }
}

void UIManager::draw(sf::RenderWindow& window, AppState currentState, Canvas& canvas, AIHelper& aiHelper, Timeline& timeline) {
    window.draw(bgSprite);

    if (currentState == AppState::Welcome) {
        welcomeScreen.draw(window);
    }
    else if (currentState == AppState::Painting) {

        sf::RenderStates canvasStates;
        canvasStates.transform = canvas.getTransform();

        canvas.draw(window, timeline.getCurrentFrame(), timeline.isPlaying(), canvasStates);

        if (isLightingMode) {
            sf::Vector2f mousePos(static_cast<float>(sf::Mouse::getPosition(window).x), static_cast<float>(sf::Mouse::getPosition(window).y));
            sf::Vector2f logicalSunPos = canvas.getInverseTransform().transformPoint(mousePos);

            std::vector<sf::FloatRect> bounds;
            std::vector<std::string> cats;
            for (const auto& item : aiHelper.getHistory()) {
                bounds.push_back(item.bounds);
                cats.push_back(item.category);
            }
            canvas.drawShadows(window, logicalSunPos, bounds, cats, canvasStates);
        }

        sf::RenderStates defaultStates;
        aiHelper.draw(window);

        leftToolbar.draw(window, SettingsManager::loadSettings().isConfigured());
        rightProperties.draw(window);
        bottomTimeline.draw(window, timeline, canvas);

        if (showingText) {
            window.draw(uiText);
        }

        if (isTypingPrompt) {
            window.draw(promptBox);
            window.draw(promptDisplay);
        }
    }

    if (settingsModal.getIsOpen()) {
        settingsModal.draw(window);
    }
}