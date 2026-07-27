#pragma once
#include "../core/ITool.h"
#include "../SpriteEngine/UI/include/SpriteSheetStudioPanel.h"

class SpriteSheetStudioTool : public ITool {
private:
    StudioUI::SpriteSheetStudioPanel m_panel;
    sf::FloatRect m_bounds;

public:
    SpriteSheetStudioTool() = default;
    ~SpriteSheetStudioTool() override = default;

    void Initialize() override;
    void HandleEvent(const sf::Event& event, const sf::RenderWindow& window) override;
    void Update(float deltaTime, const sf::RenderWindow& window) override;
    void Render(sf::RenderWindow& window) override;
    void SetBounds(const sf::FloatRect& bounds) override;
};