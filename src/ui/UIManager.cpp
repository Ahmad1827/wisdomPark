#include "UIManager.h"
#include <iostream>
#include <algorithm>
#include <ctime>

UIManager::UIManager() : isTypingPrompt(false), showingText(false), textAlpha(255.0f), isLightingMode(false), promptQuantity(1), focusMode(false), projManager(nullptr), activeProjectName("Untitled_Project") {}

void UIManager::init(ProjectManager* pm) {
    projManager = pm;
    keybindManager.init();

    bgTexture.loadFromFile("assets/landofwisdompark.png");
    bgSprite.setTexture(bgTexture);

    font.loadFromFile("assets/font.otf");

    projectBrowser.init(pm);
    settingsModal.init();
    keybindPanel.init(&keybindManager);

    leftToolbar.init();
    rightPanelManager.init();
    layerPanel.init();
    bottomTimeline.init();

    uiText.setFont(font);
    uiText.setCharacterSize(30);
    uiText.setOutlineColor(sf::Color(0, 0, 0, 150));
    uiText.setOutlineThickness(2.0f);

    promptBox.setSize(sf::Vector2f(600.f, 50.f));
    promptBox.setPosition(1920.f / 2.f - 300.f, 1080.f - 300.f);
    promptBox.setFillColor(sf::Color(15, 15, 18, 220));
    promptBox.setOutlineThickness(1.0f);
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

    if (keybindPanel.isVisible()) {
        keybindPanel.handleEvent(event);
        return;
    }

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
                activeProjectName = "New_Project_" + std::to_string(static_cast<long long>(std::time(nullptr)));
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
        if (keybindManager.isActionTriggered("proj_new", event)) {
            activeProjectName = "New_Project_" + std::to_string(static_cast<long long>(std::time(nullptr)));
            pm.createNewProject(activeProjectName, 1920, 1080, 12, canvas);
            currentState = AppState::Painting;
            showMessage("Created New Project", sf::Color::Green);
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

            if (event.key.code == sf::Keyboard::Escape) {
                settingsModal.open(settings);
            }

            if (keybindManager.isActionTriggered("ui_settings", event)) {
                keybindPanel.toggle();
            }

            if (keybindManager.isActionTriggered("time_next", event)) {
                if (timeline.getCurrentFrame() < timeline.getFrameCount() - 1) {
                    timeline.nextFrame();
                }
                else {
                    bool isFrameEmpty = true;
                    const Frame* curFrame = canvas.getFrameReadOnly(timeline.getCurrentFrame());
                    if (curFrame) {
                        for (const auto& layer : curFrame->layers) {
                            sf::Image img = layer.texture->getTexture().copyToImage();
                            const sf::Uint8* pixels = img.getPixelsPtr();
                            size_t totalPixels = static_cast<size_t>(img.getSize().x) * static_cast<size_t>(img.getSize().y) * 4;
                            for (size_t i = 3; i < totalPixels; i += 4) {
                                if (pixels[i] > 0) {
                                    isFrameEmpty = false;
                                    break;
                                }
                            }
                            if (!isFrameEmpty) break;
                        }
                    }

                    if (isFrameEmpty) {
                        if (showingText && uiText.getString() == sf::String("Current frame is empty. Press Right again to create another.") && textClock.getElapsedTime().asSeconds() < 2.0f) {
                            canvas.addFrame(timeline.getCurrentFrame());
                            timeline.addFrameAfter(timeline.getCurrentFrame());
                            timeline.nextFrame();
                            showingText = false;
                        }
                        else {
                            showMessage("Current frame is empty. Press Right again to create another.", sf::Color::Yellow);
                        }
                    }
                    else {
                        canvas.addFrame(timeline.getCurrentFrame());
                        timeline.addFrameAfter(timeline.getCurrentFrame());
                        timeline.nextFrame();
                    }
                }
            }

            if (keybindManager.isActionTriggered("time_prev", event)) {
                if (timeline.getCurrentFrame() > 0) {
                    timeline.prevFrame();
                }
                else {
                    bool isFrameEmpty = true;
                    const Frame* curFrame = canvas.getFrameReadOnly(timeline.getCurrentFrame());
                    if (curFrame) {
                        for (const auto& layer : curFrame->layers) {
                            sf::Image img = layer.texture->getTexture().copyToImage();
                            const sf::Uint8* pixels = img.getPixelsPtr();
                            size_t totalPixels = static_cast<size_t>(img.getSize().x) * static_cast<size_t>(img.getSize().y) * 4;
                            for (size_t i = 3; i < totalPixels; i += 4) {
                                if (pixels[i] > 0) {
                                    isFrameEmpty = false;
                                    break;
                                }
                            }
                            if (!isFrameEmpty) break;
                        }
                    }

                    if (isFrameEmpty) {
                        if (showingText && uiText.getString() == sf::String("Current frame is empty. Press Left again to create another.") && textClock.getElapsedTime().asSeconds() < 2.0f) {
                            canvas.addFrame(-1);
                            timeline.addFrameAfter(-1);
                            timeline.setFrame(0);
                            showingText = false;
                        }
                        else {
                            showMessage("Current frame is empty. Press Left again to create another.", sf::Color::Yellow);
                        }
                    }
                    else {
                        canvas.addFrame(-1);
                        timeline.addFrameAfter(-1);
                        timeline.setFrame(0);
                    }
                }
            }

            if (keybindManager.isActionTriggered("time_play", event)) timeline.togglePlayback();
            if (keybindManager.isActionTriggered("time_start", event)) timeline.setFrame(0);
            if (keybindManager.isActionTriggered("time_end", event)) timeline.setFrame(static_cast<int>(canvas.getFrameCount()) - 1);

            if (keybindManager.isActionTriggered("time_add", event)) {
                canvas.addFrame(timeline.getCurrentFrame());
                timeline.addFrameAfter(timeline.getCurrentFrame());
                timeline.nextFrame();
            }
            if (keybindManager.isActionTriggered("time_del", event)) {
                if (canvas.getFrameCount() > 1) {
                    int cur = timeline.getCurrentFrame();
                    canvas.deleteFrame(cur);
                    timeline.deleteFrame(cur);
                    if (timeline.getCurrentFrame() >= static_cast<int>(canvas.getFrameCount())) {
                        timeline.setFrame(static_cast<int>(canvas.getFrameCount()) - 1);
                    }
                }
            }

            if (keybindManager.isActionTriggered("layer_new", event)) canvas.addLayer(timeline.getCurrentFrame(), "New Layer");
            if (keybindManager.isActionTriggered("layer_dup", event)) canvas.duplicateLayer(timeline.getCurrentFrame(), canvas.getActiveLayer());
            if (keybindManager.isActionTriggered("layer_del", event)) canvas.deleteLayer(timeline.getCurrentFrame(), canvas.getActiveLayer());
            if (keybindManager.isActionTriggered("layer_merge_down", event)) canvas.mergeDown(timeline.getCurrentFrame());
            if (keybindManager.isActionTriggered("layer_merge_vis", event)) canvas.mergeVisible(timeline.getCurrentFrame());

            if (keybindManager.isActionTriggered("edit_del_sel", event)) {
                if (canvas.getActiveTool() == ToolType::Select) canvas.deleteSelection(timeline.getCurrentFrame());
            }
            if (keybindManager.isActionTriggered("edit_deselect", event)) {
                canvas.commitSelection(timeline.getCurrentFrame());
                canvas.setActiveTool(ToolType::Brush);
            }

            if (keybindManager.isActionTriggered("edit_copy", event)) canvas.copySelection();
            if (keybindManager.isActionTriggered("edit_paste", event)) canvas.pasteSelection(timeline.getCurrentFrame());
            if (keybindManager.isActionTriggered("edit_dup_sel", event)) canvas.duplicateSelection(timeline.getCurrentFrame());

            if (canvas.getActiveTool() == ToolType::Select) {
                if (keybindManager.isActionTriggered("sel_flip_h", event)) canvas.flipSelectionHorizontal(timeline.getCurrentFrame());
                if (keybindManager.isActionTriggered("sel_flip_v", event)) canvas.flipSelectionVertical(timeline.getCurrentFrame());
            }

            if (keybindManager.isActionTriggered("tool_brush", event)) { canvas.commitSelection(timeline.getCurrentFrame()); canvas.setActiveTool(ToolType::Brush); leftToolbar.setActiveTool("brush"); }
            if (keybindManager.isActionTriggered("tool_pencil", event)) { canvas.commitSelection(timeline.getCurrentFrame()); canvas.setActiveTool(ToolType::Pencil); leftToolbar.setActiveTool("pencil"); }
            if (keybindManager.isActionTriggered("tool_eraser", event)) { canvas.commitSelection(timeline.getCurrentFrame()); canvas.setActiveTool(ToolType::Eraser); leftToolbar.setActiveTool("eraser"); }
            if (keybindManager.isActionTriggered("tool_fill", event)) { canvas.commitSelection(timeline.getCurrentFrame()); canvas.setActiveTool(ToolType::Fill); leftToolbar.setActiveTool("fill"); }
            if (keybindManager.isActionTriggered("tool_select", event)) { canvas.setActiveTool(ToolType::Select); leftToolbar.setActiveTool("select"); }

            if (keybindManager.isActionTriggered("proj_save", event)) {
                if (pm.saveProject(activeProjectName, canvas, 12)) showMessage("Project Saved Successfully!", sf::Color::Green);
                else showMessage("Error Saving Project!", sf::Color::Red);
            }

            if (keybindManager.isActionTriggered("edit_undo", event)) canvas.undo();
            if (keybindManager.isActionTriggered("edit_redo", event)) canvas.redo();
        }

        if (!timeline.isPlaying() && !settingsModal.getIsOpen() && !keybindPanel.isVisible()) {

            if (layerPanel.handleEvent(event, mousePos, canvas, timeline.getCurrentFrame())) return;

            if (event.type == sf::Event::MouseButtonPressed) {

                sf::Color pCol, sCol;
                if (rightPanelManager.handlePaletteClick(mousePos, pCol, sCol)) {
                    if (event.mouseButton.button == sf::Mouse::Right) canvas.setSecondaryColor(sCol);
                    else canvas.setPrimaryColor(pCol);
                    return;
                }

                if (event.mouseButton.button == sf::Mouse::Left) {
                    if (layerPanel.handleClick(mousePos, canvas, timeline.getCurrentFrame())) return;

                    int clickedFrame = bottomTimeline.handleFrameClick(mousePos, static_cast<size_t>(canvas.getFrameCount()));
                    if (clickedFrame != -1) {
                        timeline.setFrame(clickedFrame);
                        return;
                    }

                    std::string bottomAction = bottomTimeline.handleClick(mousePos);
                    if (!bottomAction.empty()) {
                        if (bottomAction == "play") timeline.togglePlayback();
                        else if (bottomAction == "add") {
                            canvas.addFrame(timeline.getCurrentFrame());
                            timeline.addFrameAfter(timeline.getCurrentFrame());
                            timeline.nextFrame();
                        }
                        else if (bottomAction == "dup") {
                            canvas.duplicateFrame(timeline.getCurrentFrame());
                            timeline.duplicateFrame(timeline.getCurrentFrame());
                            timeline.nextFrame();
                        }
                        else if (bottomAction == "del") {
                            if (canvas.getFrameCount() > 1) {
                                int cur = timeline.getCurrentFrame();
                                canvas.deleteFrame(cur);
                                timeline.deleteFrame(cur);
                                if (timeline.getCurrentFrame() >= static_cast<int>(canvas.getFrameCount())) {
                                    timeline.setFrame(static_cast<int>(canvas.getFrameCount()) - 1);
                                }
                            }
                        }
                        else if (bottomAction == "onion_toggle") {
                            canvas.setOnionSkin(!canvas.isOnionSkinEnabled(), canvas.getOnionSkinPrevOpacity(), canvas.getOnionSkinNextOpacity());
                        }
                        else if (bottomAction == "onion_prev") {
                            int p = canvas.getOnionSkinPrevCount();
                            p = (p + 1) % 4;
                            canvas.setOnionSkinCounts(p, canvas.getOnionSkinNextCount());
                        }
                        else if (bottomAction == "onion_next") {
                            int n = canvas.getOnionSkinNextCount();
                            n = (n + 1) % 4;
                            canvas.setOnionSkinCounts(canvas.getOnionSkinPrevCount(), n);
                        }
                        return;
                    }

                    bool hasActiveSel = (canvas.getActiveTool() == ToolType::Select);
                    std::string leftAction = leftToolbar.handleClick(mousePos, settings.isConfigured(), hasActiveSel);
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
                        else if (leftAction == "flip_h") canvas.flipSelectionHorizontal(timeline.getCurrentFrame());
                        else if (leftAction == "flip_v") canvas.flipSelectionVertical(timeline.getCurrentFrame());
                        else if (leftAction == "dup_sel") canvas.duplicateSelection(timeline.getCurrentFrame());
                        else if (leftAction == "erase_sel") {
                            canvas.deleteSelection(timeline.getCurrentFrame());
                        }
                        else if (leftAction == "del_sel") {
                            canvas.commitSelection(timeline.getCurrentFrame());
                            canvas.setActiveTool(ToolType::Brush);
                            leftToolbar.setActiveTool("brush");
                        }
                        return;
                    }

                    std::string rightAction = rightPanelManager.handleClick(mousePos, canvas, timeline.getCurrentFrame());
                    if (!rightAction.empty()) {
                        if (rightAction == "fps_up") timeline.setFps(timeline.getFps() + 1.0f);
                        else if (rightAction == "fps_down") timeline.setFps(std::max(1.0f, timeline.getFps() - 1.0f));
                        else if (rightAction == "theme_all") aiHelper.setTheme("all");
                        else if (rightAction == "theme_struct") aiHelper.setTheme("structure");
                        else if (rightAction == "theme_clutter") aiHelper.setTheme("clutter");
                        else if (rightAction == "theme_custom") aiHelper.setTheme("custom");
                        else if (rightAction == "theme_wfc") aiHelper.setTheme("wfc");
                        else if (rightAction == "toggle_light") isLightingMode = !isLightingMode;
                        else if (rightAction == "toggle_terrain") aiHelper.toggleTerrain();
                        else if (rightAction == "onion_toggle") canvas.setOnionSkin(!canvas.isOnionSkinEnabled(), canvas.getOnionSkinPrevOpacity(), canvas.getOnionSkinNextOpacity());
                        else if (rightAction == "onion_op_up") canvas.setOnionSkin(canvas.isOnionSkinEnabled(), canvas.getOnionSkinPrevOpacity() + 25.f, canvas.getOnionSkinNextOpacity() + 25.f);
                        else if (rightAction == "onion_op_down") canvas.setOnionSkin(canvas.isOnionSkinEnabled(), canvas.getOnionSkinPrevOpacity() - 25.f, canvas.getOnionSkinNextOpacity() - 25.f);

                        if (rightAction == "section_toggle" || rightAction == "pin_toggle") return;
                        return;
                    }

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
                if (canvas.getDrawArea().contains(logicalMousePos)) {
                    sf::Image img = canvas.getActiveRenderTexture(timeline.getCurrentFrame())->getTexture().copyToImage();
                    timeline.getFrameData(timeline.getCurrentFrame()).thumbnail = img;
                }
            }

            if (event.type == sf::Event::MouseMoved) {
                canvas.handleMouseMoved(logicalMousePos, mousePos, timeline.getCurrentFrame());
            }

            if (event.type == sf::Event::MouseWheelScrolled && event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
                if (!sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) && !sf::Keyboard::isKeyPressed(sf::Keyboard::RControl)) {
                    canvas.setBrushSize(canvas.getBrushSize() + event.mouseWheelScroll.delta);
                }
            }
        }
    }
}

void UIManager::update(sf::RenderWindow& window, AppState currentState, AppSettings& settings, float dt, Canvas& canvas) {
    sf::Vector2f mousePos(static_cast<float>(sf::Mouse::getPosition(window).x), static_cast<float>(sf::Mouse::getPosition(window).y));

    if (settingsModal.getIsOpen()) settingsModal.updateHover(mousePos);
    if (keybindPanel.isVisible()) keybindPanel.updateHover(mousePos);

    if (currentState == AppState::Welcome) {
        projectBrowser.updateHover(mousePos);
    }
    else if (currentState == AppState::Painting) {

        leftToolbar.update(dt, focusMode);
        rightPanelManager.updateHover(mousePos);
        layerPanel.updateHover(mousePos);

        rightPanelManager.update(dt, focusMode);
        layerPanel.update(dt, focusMode);
        bottomTimeline.update(dt, focusMode);

        float availLeft = std::max(0.0f, static_cast<float>(leftToolbar.getPanelRightEdge()));
        float rightEdge = static_cast<float>(rightPanelManager.getMinLeftEdge());
        float layerEdge = static_cast<float>(layerPanel.getCurrentX());
        float availRight = std::min(1920.0f, std::min(rightEdge, layerEdge));

        float availTop = 0.0f;
        float availBottom = std::min(1080.0f, static_cast<float>(bottomTimeline.getPanelTopEdge()));
        float availWidth = std::max(0.0f, availRight - availLeft);
        float availHeight = std::max(0.0f, availBottom - availTop);

        sf::FloatRect availableSpace(availLeft, availTop, availWidth, availHeight);
        canvas.updateTransform(dt, availableSpace);

        leftToolbar.updateHover(mousePos);
        bottomTimeline.updateHover(mousePos);

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

        rightPanelManager.syncPropertiesState(aiHelper.getTheme(), isLightingMode, aiHelper.isTerrainEnabled(), canvas.isOnionSkinEnabled(), canvas.getOnionSkinPrevOpacity(), timeline.getFps());

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

        leftToolbar.draw(window, SettingsManager::loadSettings().isConfigured(), canvas.getActiveTool() == ToolType::Select);
        rightPanelManager.draw(window, canvas, timeline.getCurrentFrame());
        layerPanel.draw(window, canvas, timeline.getCurrentFrame());

        bottomTimeline.syncOnionState(canvas.isOnionSkinEnabled(), canvas.getOnionSkinPrevCount(), canvas.getOnionSkinNextCount());
        bottomTimeline.draw(window, timeline, canvas);

        if (showingText) window.draw(uiText);
        if (isTypingPrompt) { window.draw(promptBox); window.draw(promptDisplay); }

        keybindPanel.draw(window);
    }

    if (settingsModal.getIsOpen()) settingsModal.draw(window);
}