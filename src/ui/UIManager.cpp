#include "UIManager.h"
#include "../core/NativeDialogs.h"
#include <iostream>
#include <algorithm>
#include <ctime>
#include <filesystem>

UIManager::UIManager() : isTypingPrompt(false), showingText(false), textAlpha(255.0f), isLightingMode(false), promptQuantity(1), focusMode(false), projManager(nullptr), activeProjectName("Untitled_Project"), activeProjectPath(""), isPanning(false), isDraggingSizeSlider(false) {}

void UIManager::init(ProjectManager* pm, Canvas* baseCanvas) {
    projManager = pm;
    keybindManager.init();

    bgTexture.loadFromFile("assets/landofwisdompark.png");
    bgSprite.setTexture(bgTexture);

    font.loadFromFile("assets/font.otf");

    projectBrowser.init(pm);
    settingsModal.init();
    keybindPanel.init(&keybindManager);
    exportModal.init();
    newProjectModal.init();

    leftToolbar.init();
    layerPanel.init();
    colorPalettePanel.init();
    rightProperties.init();
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

    toolBg.setSize(sf::Vector2f(44.f, 180.f));
    toolBg.setFillColor(sf::Color(25, 25, 30, 240));
    toolBg.setOutlineThickness(1.f);
    toolBg.setOutlineColor(sf::Color(100, 100, 110));

    sizeSliderBg.setSize(sf::Vector2f(10.f, 100.f));
    sizeSliderBg.setFillColor(sf::Color(15, 15, 20));
    sizeSliderBg.setOutlineThickness(1.f);
    sizeSliderBg.setOutlineColor(sf::Color(60, 60, 70));

    sizeSliderHandle.setSize(sf::Vector2f(20.f, 10.f));
    sizeSliderHandle.setFillColor(sf::Color(0, 191, 255));

    sizeLabelText.setFont(font);
    sizeLabelText.setString("SIZE");
    sizeLabelText.setCharacterSize(10);
    sizeLabelText.setFillColor(sf::Color(180, 180, 180));

    sizeValueText.setFont(font);
    sizeValueText.setCharacterSize(12);
    sizeValueText.setFillColor(sf::Color::White);

    pixelPerfBtn.setSize(sf::Vector2f(30.f, 20.f));
    pixelPerfBtn.setFillColor(sf::Color(40, 40, 50));
    pixelPerfText.setFont(font);
    pixelPerfText.setString("PERF");
    pixelPerfText.setCharacterSize(10);
    pixelPerfText.setFillColor(sf::Color::White);
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

    if (exportModal.getIsOpen()) {
        exportModal.handleEvent(event, window);
        return;
    }

    if (newProjectModal.getIsOpen()) {
        std::string res = newProjectModal.handleEvent(event, window);
        if (res == "create_normal") {
            activeProjectName = "New_Project_" + std::to_string(static_cast<long long>(std::time(nullptr)));
            activeProjectPath = "";
            canvas.setPixelMode(false);
            pm.createNewProject(activeProjectName, 1920, 1080, 12, false, canvas);
            currentState = AppState::Painting;
            showMessage("Created Normal Project", sf::Color::Green);
        }
        else if (res == "create_pixel") {
            activeProjectName = "Pixel_Art_" + std::to_string(static_cast<long long>(std::time(nullptr)));
            activeProjectPath = "";
            canvas.setPixelMode(true);
            pm.createNewProject(activeProjectName, 64, 64, 12, true, canvas);
            currentState = AppState::Painting;
            showMessage("Created Pixel Art Project", sf::Color::Green);
        }
        return;
    }

    if (currentState == AppState::Welcome) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            ProjectMetadata meta;
            std::string action = projectBrowser.handleClick(mousePos, meta);

            if (action == "new_project") {
                newProjectModal.open();
            }
            else if (action == "load_project") {
                activeProjectName = meta.name;
                activeProjectPath = meta.path;
                int loadedFps = 12;
                bool isPix = false;
                if (pm.loadProject(meta.path, canvas, loadedFps, isPix)) {
                    timeline.setFrame(0);
                    canvas.setPixelMode(isPix);
                    currentState = AppState::Painting;
                    showMessage("Loaded Project: " + meta.name, sf::Color::Green);
                }
                else {
                    showMessage("Failed to load project files.", sf::Color::Red);
                }
            }
            else if (action == "open_native") {
                std::string file = NativeDialogs::openFileDialog("Wisdom Park Projects\0*.wpk\0All Files\0*.*\0");
                if (!file.empty()) {
                    activeProjectPath = file;
                    activeProjectName = std::filesystem::path(file).stem().string();
                    int loadedFps = 12;
                    bool isPix = false;
                    if (pm.loadProject(activeProjectPath, canvas, loadedFps, isPix)) {
                        timeline.setFrame(0);
                        canvas.setPixelMode(isPix);
                        currentState = AppState::Painting;
                        showMessage("Loaded Native Project", sf::Color::Green);
                    }
                    else {
                        showMessage("Failed to load native project.", sf::Color::Red);
                    }
                }
            }
        }
        if (keybindManager.isActionTriggered("proj_new", event)) {
            newProjectModal.open();
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

            if (keybindManager.isActionTriggered("ui_settings", event)) keybindPanel.toggle();

            if (keybindManager.isActionTriggered("export_png", event)) exportModal.open(canvas, timeline.getCurrentFrame());

            if (keybindManager.isActionTriggered("proj_save", event)) {
                if (activeProjectPath.empty()) {
                    std::string file = NativeDialogs::saveFileDialog("Wisdom Park Projects\0*.wpk\0", "wpk", activeProjectName);
                    if (!file.empty()) {
                        activeProjectPath = file;
                        if (pm.saveProjectAs(activeProjectPath, activeProjectName, canvas, static_cast<int>(timeline.getFps()), canvas.getPixelMode())) {
                            showMessage("Project Saved Successfully!", sf::Color::Green);
                        }
                        else showMessage("Error Saving Project!", sf::Color::Red);
                    }
                }
                else {
                    if (pm.saveProjectAs(activeProjectPath, activeProjectName, canvas, static_cast<int>(timeline.getFps()), canvas.getPixelMode())) {
                        showMessage("Project Saved Successfully!", sf::Color::Green);
                    }
                    else showMessage("Error Saving Project!", sf::Color::Red);
                }
            }

            if (keybindManager.isActionTriggered("proj_save_as", event)) {
                std::string file = NativeDialogs::saveFileDialog("Wisdom Park Projects\0*.wpk\0", "wpk", activeProjectName);
                if (!file.empty()) {
                    activeProjectPath = file;
                    if (pm.saveProjectAs(activeProjectPath, activeProjectName, canvas, static_cast<int>(timeline.getFps()), canvas.getPixelMode())) {
                        showMessage("Project Saved As Successfully!", sf::Color::Green);
                    }
                    else showMessage("Error Saving Project!", sf::Color::Red);
                }
            }

            if (keybindManager.isActionTriggered("proj_open", event)) {
                std::string file = NativeDialogs::openFileDialog("Wisdom Park Projects\0*.wpk\0All Files\0*.*\0");
                if (!file.empty()) {
                    activeProjectPath = file;
                    activeProjectName = std::filesystem::path(file).stem().string();
                    int loadedFps = 12;
                    bool isPix = false;
                    if (pm.loadProject(activeProjectPath, canvas, loadedFps, isPix)) {
                        timeline.setFrame(0);
                        canvas.setPixelMode(isPix);
                        showMessage("Loaded Native Project", sf::Color::Green);
                    }
                    else {
                        showMessage("Failed to load native project.", sf::Color::Red);
                    }
                }
            }

            if (keybindManager.isActionTriggered("proj_new", event)) {
                newProjectModal.open();
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

            if (canvas.getPixelMode()) {
                if (keybindManager.isActionTriggered("tool_brush", event)) {
                    canvas.cyclePixelBrushSize();
                    showMessage("Brush Size: " + std::to_string(canvas.getPixelBrushSize()) + "px", sf::Color::Green);
                }
                if (keybindManager.isActionTriggered("view_grid", event)) { canvas.togglePixelGrid(); }
                if (keybindManager.isActionTriggered("tool_move", event)) { canvas.toggleTileMode(); }
                if (keybindManager.isActionTriggered("layer_vis", event)) { canvas.resetView(); showMessage("View Reset", sf::Color::Green); }
                if (keybindManager.isActionTriggered("tool_eraser", event)) { canvas.commitSelection(timeline.getCurrentFrame()); canvas.setActiveTool(ToolType::Eraser); leftToolbar.setActiveTool("eraser"); }
                if (keybindManager.isActionTriggered("tool_pencil", event)) { canvas.commitSelection(timeline.getCurrentFrame()); canvas.setActiveTool(ToolType::Pencil); leftToolbar.setActiveTool("pencil"); }
            }
            else {
                if (keybindManager.isActionTriggered("tool_brush", event)) { canvas.commitSelection(timeline.getCurrentFrame()); canvas.setActiveTool(ToolType::Brush); leftToolbar.setActiveTool("brush"); }
                if (keybindManager.isActionTriggered("tool_pencil", event)) { canvas.commitSelection(timeline.getCurrentFrame()); canvas.setActiveTool(ToolType::Pencil); leftToolbar.setActiveTool("pencil"); }
                if (keybindManager.isActionTriggered("tool_eraser", event)) { canvas.commitSelection(timeline.getCurrentFrame()); canvas.setActiveTool(ToolType::Eraser); leftToolbar.setActiveTool("eraser"); }
                if (keybindManager.isActionTriggered("tool_fill", event)) { canvas.commitSelection(timeline.getCurrentFrame()); canvas.setActiveTool(ToolType::Fill); leftToolbar.setActiveTool("fill"); }
                if (keybindManager.isActionTriggered("tool_select", event)) { canvas.setActiveTool(ToolType::Select); leftToolbar.setActiveTool("select"); }
            }

            if (keybindManager.isActionTriggered("edit_undo", event)) canvas.undo();
            if (keybindManager.isActionTriggered("edit_redo", event)) canvas.redo();

            if (keybindManager.isActionTriggered("tool_eyedropper", event)) {
                if (canvas.getDrawArea().contains(logicalMousePos)) {
                    sf::Image flat = ExportManager::flattenFrame(canvas, timeline.getCurrentFrame());
                    sf::Vector2f texScale(static_cast<float>(canvas.getCanvasSize().x) / canvas.getDrawArea().width, static_cast<float>(canvas.getCanvasSize().y) / canvas.getDrawArea().height);
                    int px = static_cast<int>((logicalMousePos.x - canvas.getDrawArea().left) * texScale.x);
                    int py = static_cast<int>((logicalMousePos.y - canvas.getDrawArea().top) * texScale.y);
                    if (px >= 0 && px < static_cast<int>(flat.getSize().x) && py >= 0 && py < static_cast<int>(flat.getSize().y)) {
                        sf::Color picked = flat.getPixel(px, py);
                        canvas.setPrimaryColor(picked);
                        colorPalettePanel.setColors(picked, canvas.getSecondaryColor());
                        colorPalettePanel.getColorManager().addRecentColor(picked);
                        showMessage("Color Picked", sf::Color::Green);
                    }
                }
            }
        }

        if (!timeline.isPlaying() && !settingsModal.getIsOpen() && !keybindPanel.isVisible() && !exportModal.getIsOpen() && !newProjectModal.getIsOpen()) {

            if (layerPanel.handleEvent(event, mousePos, canvas, timeline.getCurrentFrame())) return;
            if (colorPalettePanel.handleEvent(event, mousePos, canvas)) return;

            if (event.type == sf::Event::MouseMoved) {
                if (layerPanel.getHandleBounds().contains(mousePos)) {
                    if (rightProperties.isPanelPinned()) rightProperties.forceClose();
                    if (colorPalettePanel.isPanelPinned()) colorPalettePanel.forceClose();
                }
                else if (rightProperties.getHandleBounds().contains(mousePos)) {
                    if (layerPanel.isPanelPinned()) layerPanel.forceClose();
                    if (colorPalettePanel.isPanelPinned()) colorPalettePanel.forceClose();
                }
                else if (colorPalettePanel.getHandleBounds().contains(mousePos)) {
                    if (layerPanel.isPanelPinned()) layerPanel.forceClose();
                    if (rightProperties.isPanelPinned()) rightProperties.forceClose();
                }
            }

            if (event.type == sf::Event::MouseWheelScrolled && event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {
                canvas.zoom(event.mouseWheelScroll.delta);
            }

            if (event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Middle || event.mouseButton.button == sf::Mouse::Right) {
                    isPanning = true;
                    lastPanMousePos = mousePos;
                    return;
                }

                if (event.mouseButton.button == sf::Mouse::Left || event.mouseButton.button == sf::Mouse::Right) {
                    if (colorPalettePanel.handleClick(mousePos, canvas)) {
                        return;
                    }
                }

                if (event.mouseButton.button == sf::Mouse::Left) {

                    if (sizeSliderBg.getGlobalBounds().contains(mousePos)) {
                        isDraggingSizeSlider = true;
                        return;
                    }

                    if (canvas.getPixelMode() && pixelPerfBtn.getGlobalBounds().contains(mousePos)) {
                        canvas.togglePixelPerfect();
                        return;
                    }

                    std::string lpAction = layerPanel.processClick(mousePos, canvas, timeline.getCurrentFrame());
                    if (!lpAction.empty()) {
                        if (lpAction == "layer_push") {
                            int cur = timeline.getCurrentFrame();
                            if (cur == static_cast<int>(canvas.getFrameCount()) - 1) {
                                canvas.addFrame(cur);
                                timeline.addFrameAfter(cur);
                            }
                            canvas.pushLayerToNextFrame(cur, canvas.getActiveLayer());
                            timeline.nextFrame();
                        }
                        else if (lpAction == "layer_pin" && layerPanel.isPanelPinned()) {
                            rightProperties.forceClose();
                            colorPalettePanel.forceClose();
                        }
                        return;
                    }

                    std::string rpAction = rightProperties.handleClick(mousePos);
                    if (!rpAction.empty()) {
                        if (rpAction == "pin_toggle" || rpAction == "handle_click" || rpAction == "section_toggle") return;

                        if (rpAction == "fps_up") timeline.setFps(timeline.getFps() + 1.0f);
                        else if (rpAction == "fps_down") timeline.setFps(std::max(1.0f, timeline.getFps() - 1.0f));
                        else if (rpAction == "theme_all") aiHelper.setTheme("all");
                        else if (rpAction == "theme_struct") aiHelper.setTheme("structure");
                        else if (rpAction == "theme_clutter") aiHelper.setTheme("clutter");
                        else if (rpAction == "theme_custom") aiHelper.setTheme("custom");
                        else if (rpAction == "theme_wfc") aiHelper.setTheme("wfc");
                        else if (rpAction == "toggle_light") isLightingMode = !isLightingMode;
                        else if (rpAction == "toggle_terrain") aiHelper.toggleTerrain();
                        else if (rpAction == "onion_toggle") canvas.setOnionSkin(!canvas.isOnionSkinEnabled(), canvas.getOnionSkinPrevOpacity(), canvas.getOnionSkinNextOpacity());
                        else if (rpAction == "onion_op_up") canvas.setOnionSkin(canvas.isOnionSkinEnabled(), canvas.getOnionSkinPrevOpacity() + 25.f, canvas.getOnionSkinNextOpacity() + 25.f);
                        else if (rpAction == "onion_op_down") canvas.setOnionSkin(canvas.isOnionSkinEnabled(), canvas.getOnionSkinPrevOpacity() - 25.f, canvas.getOnionSkinNextOpacity() - 25.f);
                        return;
                    }

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
                        else if (bottomAction == "export") {
                            exportModal.open(canvas, timeline.getCurrentFrame());
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

                if (!isPanning) {
                    canvas.handleMousePressed(logicalMousePos, event.mouseButton.button == sf::Mouse::Right, timeline.getCurrentFrame());
                }
            }

            if (event.type == sf::Event::MouseButtonReleased) {
                if (isDraggingSizeSlider && event.mouseButton.button == sf::Mouse::Left) {
                    isDraggingSizeSlider = false;
                    return;
                }

                if (event.mouseButton.button == sf::Mouse::Right && isPanning) {
                    isPanning = false;
                    return;
                }
                else if (event.mouseButton.button == sf::Mouse::Middle) {
                    isPanning = false;
                    return;
                }

                if (!isPanning) {
                    canvas.handleMouseReleased(logicalMousePos, timeline.getCurrentFrame());

                    if (canvas.getDrawArea().contains(logicalMousePos)) {
                        sf::Image img = canvas.getActiveRenderTexture(timeline.getCurrentFrame())->getTexture().copyToImage();
                        timeline.getFrameData(timeline.getCurrentFrame()).thumbnail = img;
                    }
                }
            }

            if (event.type == sf::Event::MouseMoved) {
                if (isDraggingSizeSlider) {
                    float localY = mousePos.y - sizeSliderBg.getPosition().y;
                    float percent = 1.0f - std::clamp(localY / sizeSliderBg.getSize().y, 0.0f, 1.0f);

                    if (canvas.getPixelMode()) {
                        int newSize = 1 + static_cast<int>(percent * 31.0f);
                        canvas.setPixelBrushSize(newSize);
                    }
                    else {
                        float newSize = 1.0f + (percent * 99.0f);
                        canvas.setBrushSize(newSize);
                    }
                }

                if (isPanning) {
                    canvas.pan(mousePos - lastPanMousePos);
                    lastPanMousePos = mousePos;
                }

                if (!isPanning && !isDraggingSizeSlider) {
                    canvas.handleMouseMoved(logicalMousePos, mousePos, timeline.getCurrentFrame());
                }
            }
        }
    }
}

void UIManager::update(sf::RenderWindow& window, AppState currentState, AppSettings& settings, float dt, Canvas& canvas) {
    sf::Vector2f mousePos(static_cast<float>(sf::Mouse::getPosition(window).x), static_cast<float>(sf::Mouse::getPosition(window).y));

    if (settingsModal.getIsOpen()) settingsModal.updateHover(mousePos);
    if (keybindPanel.isVisible()) keybindPanel.updateHover(mousePos);
    if (exportModal.getIsOpen()) exportModal.updateHover(mousePos);
    if (newProjectModal.getIsOpen()) newProjectModal.updateHover(mousePos);

    if (currentState == AppState::Welcome) {
        projectBrowser.updateHover(mousePos);
    }
    else if (currentState == AppState::Painting) {

        leftToolbar.update(dt, focusMode);

        bool isLayerOpen = layerPanel.isHovered() || layerPanel.isPanelPinned() || layerPanel.getCurrentX() < 1919.f;
        bool isColorOpen = colorPalettePanel.isHovered() || colorPalettePanel.isPanelPinned() || colorPalettePanel.getCurrentX() < 1919.f;
        bool isPropOpen = rightProperties.isHovered() || rightProperties.isPanelPinned() || rightProperties.getCurrentX() < 1919.f;

        layerPanel.updateHover(mousePos, !isColorOpen && !isPropOpen);
        colorPalettePanel.updateHover(mousePos, !isLayerOpen && !isPropOpen);
        rightProperties.updateHover(mousePos, !isLayerOpen && !isColorOpen);

        rightProperties.update(dt, focusMode);
        layerPanel.update(dt, focusMode);
        colorPalettePanel.update(dt, focusMode, canvas);
        bottomTimeline.update(dt, focusMode);

        float availLeft = std::max(0.0f, static_cast<float>(leftToolbar.getPanelRightEdge()));
        float edgeL = static_cast<float>(layerPanel.getCurrentX());
        float edgeC = static_cast<float>(colorPalettePanel.getCurrentX());
        float edgeP = static_cast<float>(rightProperties.getCurrentX());
        float availRight = std::min(1920.0f, std::min(edgeL, std::min(edgeC, edgeP)));

        float availTop = 0.0f;
        float availBottom = std::min(1080.0f, static_cast<float>(bottomTimeline.getPanelTopEdge()));
        float availWidth = std::max(0.0f, availRight - availLeft);
        float availHeight = std::max(0.0f, availBottom - availTop);

        sf::FloatRect availableSpace(availLeft, availTop, availWidth, availHeight);
        canvas.updateTransform(dt, availableSpace);

        leftToolbar.updateHover(mousePos);
        bottomTimeline.updateHover(mousePos);

        float tbX = leftToolbar.getPanelRightEdge();
        toolBg.setPosition(tbX + 15.f, 100.f);
        sizeLabelText.setPosition(tbX + 22.f, 110.f);
        sizeSliderBg.setPosition(tbX + 32.f, 130.f);

        float handlePercent = 0.f;
        if (canvas.getPixelMode()) {
            handlePercent = (canvas.getPixelBrushSize() - 1.0f) / 31.0f;
            sizeValueText.setString(std::to_string(canvas.getPixelBrushSize()));
        }
        else {
            handlePercent = (canvas.getBrushSize() - 1.0f) / 99.0f;
            sizeValueText.setString(std::to_string(static_cast<int>(canvas.getBrushSize())));
        }

        float handleY = sizeSliderBg.getPosition().y + sizeSliderBg.getSize().y - (handlePercent * sizeSliderBg.getSize().y);
        sizeSliderHandle.setPosition(sizeSliderBg.getPosition().x - 5.f, handleY - 5.f);

        sizeValueText.setPosition(tbX + 22.f + (canvas.getPixelMode() && canvas.getPixelBrushSize() < 10 ? 5.f : 0.f), sizeSliderBg.getPosition().y + sizeSliderBg.getSize().y + 10.f);

        pixelPerfBtn.setPosition(tbX + 17.f, 245.f);
        pixelPerfText.setPosition(tbX + 19.f, 248.f);

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
        if (newProjectModal.getIsOpen()) newProjectModal.draw(window);
    }
    else if (currentState == AppState::Painting) {

        rightProperties.syncState(aiHelper.getTheme(), isLightingMode, aiHelper.isTerrainEnabled(), canvas.isOnionSkinEnabled(), canvas.getOnionSkinPrevOpacity(), timeline.getFps());

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

        layerPanel.draw(window, canvas, timeline.getCurrentFrame());
        colorPalettePanel.draw(window);
        rightProperties.draw(window);

        window.draw(toolBg);
        window.draw(sizeLabelText);
        window.draw(sizeSliderBg);
        window.draw(sizeSliderHandle);
        window.draw(sizeValueText);

        if (canvas.getPixelMode()) {
            if (canvas.isPixelPerfectEnabled()) {
                pixelPerfBtn.setOutlineThickness(1.f);
                pixelPerfBtn.setOutlineColor(sf::Color(0, 191, 255));
                pixelPerfBtn.setFillColor(sf::Color(60, 60, 80));
            }
            else {
                pixelPerfBtn.setOutlineThickness(0.f);
                pixelPerfBtn.setFillColor(sf::Color(40, 40, 50));
            }
            window.draw(pixelPerfBtn);
            window.draw(pixelPerfText);
        }

        bottomTimeline.syncOnionState(canvas.isOnionSkinEnabled(), canvas.getOnionSkinPrevCount(), canvas.getOnionSkinNextCount());
        bottomTimeline.draw(window, timeline, canvas);

        if (showingText) window.draw(uiText);
        if (isTypingPrompt) { window.draw(promptBox); window.draw(promptDisplay); }

        keybindPanel.draw(window);
        if (exportModal.getIsOpen()) exportModal.draw(window);
    }

    if (settingsModal.getIsOpen()) settingsModal.draw(window);
}