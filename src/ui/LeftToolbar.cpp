#include "LeftToolbar.h"

LeftToolbar::LeftToolbar() : activeToolId("brush"), width(90.f), currentX(-90.f), targetX(-90.f), state(PanelState::Hidden) {}

void LeftToolbar::init() {
    font.loadFromFile("assets/font.otf");

    background.setSize(sf::Vector2f(width, 1080.f));
    background.setFillColor(sf::Color(15, 15, 18, 220));
    background.setOutlineThickness(1.f);
    background.setOutlineColor(sf::Color(255, 255, 255, 15));

    handleBg.setSize(sf::Vector2f(24.f, 80.f));
    handleBg.setFillColor(sf::Color(30, 30, 35, 200));
    handleBg.setOutlineThickness(1.f);
    handleBg.setOutlineColor(sf::Color(255, 255, 255, 30));

    handleLabel.setFont(font);
    handleLabel.setString(">");
    handleLabel.setCharacterSize(16);
    handleLabel.setFillColor(sf::Color(200, 200, 200));

    pinBtn.setSize(sf::Vector2f(width - 20.f, 24.f));
    pinBtn.setFillColor(sf::Color(255, 255, 255, 10));

    pinLabel.setFont(font);
    pinLabel.setString("Pin");
    pinLabel.setCharacterSize(12);
    pinLabel.setFillColor(sf::Color(180, 180, 180));

    auto makeBtn = [&](std::string id, std::string text, float y, bool aiTool = false) {
        ToolItem btn;
        btn.id = id;
        btn.isAiTool = aiTool;
        btn.rect.setSize(sf::Vector2f(width - 20.f, 40.f));
        btn.label.setFont(font);
        btn.label.setString(text);
        btn.label.setCharacterSize(11);
        sf::FloatRect tRect = btn.label.getLocalBounds();
        btn.label.setOrigin(tRect.left + tRect.width / 2.0f, tRect.top + tRect.height / 2.0f);
        tools.push_back(btn);
        };

    float startY = 60.f;
    float gap = 45.f;

    makeBtn("brush", "Brush", startY);
    makeBtn("pencil", "Pencil", startY + gap * 1);
    makeBtn("eraser", "Erase", startY + gap * 2);
    makeBtn("fill", "Bucket", startY + gap * 3);
    makeBtn("select", "Select", startY + gap * 4);
    makeBtn("magic_wand", "Wand", startY + gap * 5);
    makeBtn("shapes", "Shapes", startY + gap * 6);
    makeBtn("symmetry", "Symmetry", startY + gap * 7);
    makeBtn("ai_gen", "AI Gen", startY + gap * 8, true);
    makeBtn("import_img", "Image", startY + gap * 9);
    makeBtn("audio_panel", "Audio", startY + gap * 10);
    makeBtn("dither_toggle", "Dither", startY + gap * 11);

    auto makeActionBtn = [&](std::string id, std::string text) {
        ToolItem btn;
        btn.id = id;
        btn.rect.setSize(sf::Vector2f(width - 20.f, 30.f));
        btn.label.setFont(font);
        btn.label.setString(text);
        btn.label.setCharacterSize(10);
        sf::FloatRect tRect = btn.label.getLocalBounds();
        btn.label.setOrigin(tRect.left + tRect.width / 2.0f, tRect.top + tRect.height / 2.0f);
        selectionActions.push_back(btn);
        };

    makeActionBtn("flip_h", "Flip H");
    makeActionBtn("flip_v", "Flip V");
    makeActionBtn("dup_sel", "Duplicate");
    makeActionBtn("resize_sel", "Resize");
    makeActionBtn("erase_sel", "Erase");
    makeActionBtn("del_sel", "Deselect");
}

void LeftToolbar::update(float dt, bool focusMode) {
    if (focusMode) targetX = -width;
    else targetX = (state == PanelState::Pinned || state == PanelState::Visible) ? 0.f : -width;

    currentX += (targetX - currentX) * 15.0f * dt;
    background.setPosition(currentX, 0.f);
    handleBg.setPosition(currentX + width, 500.f);
    handleLabel.setPosition(currentX + width + 6.f, 530.f);

    if (state == PanelState::Pinned) {
        handleLabel.setString("x");
        pinLabel.setString("Unpin");
        pinLabel.setFillColor(sf::Color(0, 191, 255));
    }
    else {
        handleLabel.setString(">");
        pinLabel.setString("Pin");
        pinLabel.setFillColor(sf::Color(180, 180, 180));
    }

    pinBtn.setPosition(currentX + 10.f, 20.f);
    pinLabel.setPosition(currentX + 25.f, 24.f);

    float startY = 60.f;
    for (auto& tool : tools) {
        tool.rect.setPosition(currentX + 10.f, startY);
        tool.label.setPosition(currentX + (width / 2.f), startY + 20.f);
        startY += 45.f;
    }

    startY += 10.f;
    for (auto& act : selectionActions) {
        act.rect.setPosition(currentX + 10.f, startY);
        act.label.setPosition(currentX + (width / 2.f), startY + 15.f);
        startY += 35.f;
    }
}

void LeftToolbar::updateHover(sf::Vector2f mousePos) {
    bool inPanel = background.getGlobalBounds().contains(mousePos);
    bool inHandle = handleBg.getGlobalBounds().contains(mousePos);

    if (state == PanelState::Hidden && inHandle) state = PanelState::Visible;
    else if (state == PanelState::Visible && !inPanel && !inHandle) state = PanelState::Hidden;

    for (auto& tool : tools) tool.isHovered = tool.rect.getGlobalBounds().contains(mousePos);
    for (auto& act : selectionActions) act.isHovered = act.rect.getGlobalBounds().contains(mousePos);
}

void LeftToolbar::draw(sf::RenderWindow& window, bool isAIConfigured, bool hasSelection) {
    window.draw(background);

    if (state != PanelState::Pinned) {
        window.draw(handleBg);
        window.draw(handleLabel);
    }

    pinBtn.setFillColor(pinBtn.getGlobalBounds().contains(sf::Vector2f(static_cast<float>(sf::Mouse::getPosition(window).x), static_cast<float>(sf::Mouse::getPosition(window).y))) ? sf::Color(255, 255, 255, 25) : sf::Color(255, 255, 255, 10));
    window.draw(pinBtn);
    window.draw(pinLabel);

    for (auto& tool : tools) {
        bool disabled = tool.isAiTool && !isAIConfigured;
        if (disabled) {
            tool.rect.setFillColor(sf::Color(255, 255, 255, 2));
            tool.label.setFillColor(sf::Color(100, 100, 100));
        }
        else if (tool.id == activeToolId && tool.id != "audio_panel" && tool.id != "import_img" && tool.id != "dither_toggle") {
            tool.rect.setFillColor(sf::Color(0, 122, 204, 180));
            tool.label.setFillColor(sf::Color::White);
        }
        else {
            if (tool.id == "audio_panel") tool.rect.setFillColor(tool.isHovered ? sf::Color(100, 50, 150, 200) : sf::Color(80, 40, 120, 150));
            else tool.rect.setFillColor(tool.isHovered ? sf::Color(255, 255, 255, 20) : sf::Color(255, 255, 255, 5));
            tool.label.setFillColor(sf::Color(220, 220, 225));
        }

        window.draw(tool.rect);
        window.draw(tool.label);
    }

    if (hasSelection) {
        for (auto& act : selectionActions) {
            if (act.id == "erase_sel") act.rect.setFillColor(act.isHovered ? sf::Color(200, 50, 50, 150) : sf::Color(150, 40, 40, 100));
            else if (act.id == "resize_sel") act.rect.setFillColor(act.isHovered ? sf::Color(0, 200, 100, 150) : sf::Color(0, 150, 80, 100));
            else act.rect.setFillColor(act.isHovered ? sf::Color(0, 191, 255, 150) : sf::Color(0, 122, 204, 100));
            act.label.setFillColor(sf::Color::White);
            window.draw(act.rect);
            window.draw(act.label);
        }
    }
}

std::string LeftToolbar::handleClick(sf::Vector2f mousePos, bool isAIConfigured, bool hasSelection) {
    if (pinBtn.getGlobalBounds().contains(mousePos)) {
        state = (state == PanelState::Pinned) ? PanelState::Visible : PanelState::Pinned;
        return "pin_toggle";
    }

    if (state == PanelState::Hidden && handleBg.getGlobalBounds().contains(mousePos)) {
        state = PanelState::Pinned;
        return "handle_click";
    }

    for (const auto& tool : tools) {
        if (tool.rect.getGlobalBounds().contains(mousePos)) {
            if (tool.isAiTool && !isAIConfigured) return "ai_disabled";
            if (tool.id != "import_img" && tool.id != "audio_panel" && tool.id != "dither_toggle") {
                activeToolId = tool.id;
            }
            return tool.id;
        }
    }

    if (hasSelection) {
        for (const auto& act : selectionActions) {
            if (act.rect.getGlobalBounds().contains(mousePos)) {
                return act.id;
            }
        }
    }

    return "";
}

float LeftToolbar::getPanelRightEdge() const { return currentX + width; }
std::string LeftToolbar::getActiveTool() const { return activeToolId; }
void LeftToolbar::setActiveTool(const std::string& id) { activeToolId = id; }