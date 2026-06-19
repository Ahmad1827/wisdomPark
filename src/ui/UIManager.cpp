#include "UIManager.h"
#include <iostream>

UIManager::UIManager() : isTypingPrompt(false), showingText(false), textAlpha(255.0f), isLightingMode(false), promptQuantity(1), focusMode(false), projManager(nullptr), activeProjectName("Untitled_Project") {}

void UIManager::init(ProjectManager* pm) {
    projManager = pm;
    bgTexture.loadFromFile("assets/landofwisdompark.png");
    bgSprite.setTexture(bgTexture);

    font.loadFromFile("assets/font.otf");

    projectBrowser.init(pm);
    settingsModal.init();
    leftToolbar.init();
    rightProperties.init();
    bottomTimeline.init();
    colorPalettePanel.init();

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

void UIManager::handleEvent(const sf::Event& event, sf::RenderWindow& window, AppState& currentState, AppSettings& settings, Canvas& canvas, Timeline& timeline, AIHelper& aiHelper, ProjectManager& pm) {
    sf::Vector2f mousePos(static_cast<float>(sf::Mouse::getPosition(window).x), static_cast<float>(sf::Mouse::getPosition(window).y));
    sf::Vector2f logicalMousePos = canvas.getInverseTransform().transformPoint(mousePos);

    if (settingsModal.getIsOpen()) {
        if (event.type == sf::Event::TextEntered) settingsModal.handleTextEntered(event.text.unicode);
        else if (event.type == sf::Event::KeyPressed) settingsModal.handleKeyPress(event.key.code, settings);
        else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) settingsModal.handleClick(mousePos, settings);
        return;
    }

    if (currentState == AppState::Welcome) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            ProjectMetadata meta;
            std::string action = projectBrowser.handleClick(mousePos, meta);

            if (action == "new_project") {
                activeProjectName = "New_Project_" + std::to_string(std::time(nullptr));
                pm.createNewProject(activeProjectName, 1920, 1080, 12, canvas);
                currentState = AppState::Painting;
                showMessage("Created New Project", sf::Color::Green);
            }
            else if (action == "load_project") {
                activeProjectName = meta.name;
                int loadedFps = 12;
                if (pm.loadProject(meta.name, canvas, loadedFps)) {
                    timeline.setFrame(0);
                    currentState = AppState::Painting;
                    showMessage("Loaded Project: " + meta.name, sf::Color::Green);
                }
                else {
                    showMessage("Failed to load project files.", sf::Color::Red);
                }
            }
        }
    }
    else if (currentState == AppState::Painting) {
        if (event.type == sf::Event::TextEntered && isTypingPrompt) {
            if (event.text.unicode == '\b' && !currentPrompt.empty()) currentPrompt.pop_back();
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
                    if (!settings.isConfigured()) showMessage("ERROR: Configure AI Provider in Settings (Press ESC)", sf::Color::Red);
                    else {
                        bool isFill; std::string parsedTheme;
                        aiHelper.parseCommand(currentPrompt, promptQuantity, isFill, parsedTheme);
                        aiHelper.setTheme(parsedTheme);
                        showMessage("AI Configured. Click Canvas to Generate: " + parsedTheme, sf::Color::Green);
                    }
                }
            }

            if (isTypingPrompt) return;

            if (event.key.code == sf::Keyboard::Escape) settingsModal.open(settings);
            if (event.key.code == sf::Keyboard::Tab) {
                focusMode = !focusMode;
                showMessage(focusMode ? "Focus Mode: ON (Press TAB to exit)" : "Focus Mode: OFF", sf::Color::White);
            }

            if (event.key.code == sf::Keyboard::Right) timeline.nextFrame();
            if (event.key.code == sf::Keyboard::Left) timeline.prevFrame();
            if (event.key.code == sf::Keyboard::Space) timeline.togglePlayback();
            if (event.key.code == sf::Keyboard::Home) timeline.setFrame(0);
            if (event.key.code == sf::Keyboard::End) timeline.setFrame(canvas.getFrameCount() - 1);

            if (event.key.code == sf::Keyboard::Delete) {
                if (canvas.getActiveTool() == ToolType::Select) canvas.deleteSelection(timeline.getCurrentFrame());
                else if (canvas.getFrameCount() > 1) {
                    canvas.deleteFrame(timeline.getCurrentFrame());
                    if (timeline.getCurrentFrame() >= static_cast<int>(canvas.getFrameCount())) timeline.setFrame(canvas.getFrameCount() - 1);
                }
            }

            if (ctrlPressed && event.key.code == sf::Keyboard::C) canvas.copySelection();
            if (ctrlPressed && event.key.code == sf::Keyboard::V) canvas.pasteSelection(timeline.getCurrentFrame());
            if (ctrlPressed && event.key.code == sf::Keyboard::D) {
                canvas.commitSelection(timeline.getCurrentFrame());
                canvas.setActiveTool(ToolType::Brush);
            }

            if (ctrlPressed && event.key.code == sf::Keyboard::N) { canvas.addFrame(timeline.getCurrentFrame()); timeline.nextFrame(); }
            if (ctrlPressed && event.key.code == sf::Keyboard::S) {
                if (pm.saveProject(activeProjectName, canvas, 12)) showMessage("Project Saved Successfully!", sf::Color::Green);
                else showMessage("Error Saving Project!", sf::Color::Red);
            }

            if (ctrlPressed && event.key.code == sf::Keyboard::Z) canvas.undo();
            if (ctrlPressed && event.key.code == sf::Keyboard::Y) canvas.redo();
        }

        if (!timeline.isPlaying() && !settingsModal.getIsOpen()) {
            if (event.type == sf::Event::MouseButtonPressed) {

                sf::Color pCol, sCol;
                if (colorPalettePanel.handleClick(mousePos, pCol, sCol)) {
                    if (event.mouseButton.button == sf::Mouse::Right) canvas.setSecondaryColor(sCol);
                    else canvas.setPrimaryColor(pCol);
                    return;
                }

                if (event.mouseButton.button == sf::Mouse::Left) {
                    std::string leftAction = leftToolbar.handleClick(mousePos, settings.isConfigured());
                    if (!leftAction.empty()) {
                        if (leftAction == "ai_disabled") showMessage("Configure an AI provider in Settings.", sf::Color::Red);
                        else if (leftAction == "brush") canvas.setActiveTool(ToolType::Brush);
                        else if (leftAction == "pencil") canvas.setActiveTool(ToolType::Pencil);
                        else if (leftAction == "eraser") canvas.setActiveTool(ToolType::Eraser);
                        else if (leftAction == "fill") canvas.setActiveTool(ToolType::Fill);
                        else if (leftAction == "select") canvas.setActiveTool(ToolType::Select);
                        else if (leftAction == "ai_gen") {
                            isTypingPrompt = true; currentPrompt = ""; promptDisplay.setString("> _");
                            showMessage("AI Terminal: Type prompt and press Enter", sf::Color(0, 191, 255));
                        }
                        return;
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
                        if (rightAction == "section_toggle" || rightAction == "pin_toggle") return;
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
                                if (timeline.getCurrentFrame() >= static_cast<int>(canvas.getFrameCount())) timeline.setFrame(canvas.getFrameCount() - 1);
                            }
                        }
                        return;
                    }

                    int clickedFrame = bottomTimeline.handleFrameClick(mousePos, canvas.getFrameCount());
                    if (clickedFrame != -1) { timeline.setFrame(clickedFrame); return; }

                    if (aiHelper.getBounds().contains(logicalMousePos)) {
                        if (!settings.isConfigured()) { showMessage("Configure AI Provider (Press ESC) first!", sf::Color::Red); }
                        else {
                            aiHelper.toggle();
                            if (aiHelper.isActive()) {
                                canvas.saveUndoState();
                                sf::Image currentImg = canvas.getActiveRenderTexture(timeline.getCurrentFrame())->getTexture().copyToImage();
                                std::string errorMsg = aiHelper.startGeneratingComplexArt(canvas.getDrawArea(), currentImg, settings.activeProvider, settings.apiKeys[settings.activeProvider], false);
                                if (!errorMsg.empty()) {
                                    showMessage(errorMsg, sf::Color::Red);
                                    canvas.undo();
                                    aiHelper.toggle();
                                }
                                else {
                                    showMessage("AI Generation Started...", sf::Color::Yellow);
                                }
                            }
                        }
                        return;
                    }
                }

                if (canvas.getActiveTool() != ToolType::Select) {
                    canvas.commitSelection(timeline.getCurrentFrame());
                }

                canvas.handleMousePressed(logicalMousePos, event.mouseButton.button == sf::Mouse::Right, timeline.getCurrentFrame());
            }

            if (event.type == sf::Event::MouseButtonReleased) {
                canvas.handleMouseReleased(logicalMousePos, timeline.getCurrentFrame());
            }

            if (event.type == sf::Event::MouseMoved) {
                canvas.handleMouseMoved(logicalMousePos, timeline.getCurrentFrame());
            }

            if (event.type == sf::Event::MouseWheelScrolled && event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
                canvas.setBrushSize(canvas.getBrushSize() + event.mouseWheelScroll.delta);
            }
        }
    }
}

void UIManager::update(sf::RenderWindow& window, AppState currentState, AppSettings& settings, float dt, Canvas& canvas) {
    sf::Vector2f mousePos(static_cast<float>(sf::Mouse::getPosition(window).x), static_cast<float>(sf::Mouse::getPosition(window).y));

    if (settingsModal.getIsOpen()) settingsModal.updateHover(mousePos);

    if (currentState == AppState::Welcome) {
        projectBrowser.updateHover(mousePos);
    }
    else if (currentState == AppState::Painting) {

        leftToolbar.update(dt, focusMode);
        rightProperties.update(dt, focusMode);
        bottomTimeline.update(dt, focusMode);
        colorPalettePanel.update(dt, focusMode);

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
        colorPalettePanel.updateHover(mousePos);

        if (showingText && textClock.getElapsedTime().asSeconds() > 2.0f) showingText = false;
        else if (showingText && textClock.getElapsedTime().asSeconds() > 1.5f) {
            textAlpha = std::max(0.0f, textAlpha - 255.0f * (1.0f / 60.0f));
            sf::Color fc = uiText.getFillColor(); fc.a = static_cast<sf::Uint8>(textAlpha);
            sf::Color oc = uiText.getOutlineColor(); oc.a = static_cast<sf::Uint8>(textAlpha);
            uiText.setFillColor(fc); uiText.setOutlineColor(oc);
        }
    }
}

void UIManager::draw(sf::RenderWindow& window, AppState currentState, Canvas& canvas, AIHelper& aiHelper, Timeline& timeline) {
    window.draw(bgSprite);

    if (currentState == AppState::Welcome) {
        projectBrowser.draw(window);
    }
    else if (currentState == AppState::Painting) {

        sf::RenderStates canvasStates;
        canvasStates.transform = canvas.getTransform();

        canvas.draw(window, timeline.getCurrentFrame(), timeline.isPlaying(), canvasStates);

        if (isLightingMode) {
            sf::Vector2f mousePos(static_cast<float>(sf::Mouse::getPosition(window).x), static_cast<float>(sf::Mouse::getPosition(window).y));
            sf::Vector2f logicalSunPos = canvas.getInverseTransform().transformPoint(mousePos);
            std::vector<sf::FloatRect> bounds; std::vector<std::string> cats;
            for (const auto& item : aiHelper.getHistory()) { bounds.push_back(item.bounds); cats.push_back(item.category); }
            canvas.drawShadows(window, logicalSunPos, bounds, cats, canvasStates);
        }

        sf::RenderStates defaultStates;
        aiHelper.draw(window);

        leftToolbar.draw(window, SettingsManager::loadSettings().isConfigured());
        rightProperties.draw(window);
        bottomTimeline.draw(window, timeline, canvas);
        colorPalettePanel.draw(window);

        if (showingText) window.draw(uiText);
        if (isTypingPrompt) { window.draw(promptBox); window.draw(promptDisplay); }
    }

    if (settingsModal.getIsOpen()) settingsModal.draw(window);
}