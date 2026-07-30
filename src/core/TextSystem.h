#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/System/String.hpp>
#include <string>
#include <vector>
#include <map>

struct TextObject {
    std::string id;
    sf::String text;
    std::string fontName;
    int size = 16; // Increased from 8 to a readable default!
    bool bold = false;
    bool italic = false;
    bool underline = false;
    bool strikethrough = false;
    sf::Color color = sf::Color::White;
    float opacity = 100.f;
    float letterSpacing = 1.0f;
    float lineSpacing = 1.0f;
    int alignH = 0;
    int alignV = 0;
    float rotation = 0.0f;
    bool outline = false;
    float outlineThickness = 1.0f;
    sf::Color outlineColor = sf::Color::Black;
    bool shadow = false;
    float shadowOffsetX = 2.0f;
    float shadowOffsetY = 2.0f;
    sf::Color shadowColor = sf::Color(0, 0, 0, 150);
    bool box = false;
    sf::Color boxColor = sf::Color(0, 0, 0, 100);
    float boxPadding = 5.0f;
    sf::Vector2f position;
    sf::Vector2f scale = { 1.f, 1.f };
    bool isEditing = false;
    int layerIndex = 0;

    void render(sf::RenderTarget& target, bool isPixelMode, sf::RenderStates states = sf::RenderStates::Default) const;
    sf::FloatRect getBounds() const;
};

class TextManager {
public:
    void init();
    std::string createText(int frame, int layer, sf::Vector2f pos);
    void deleteText(int frame, const std::string& id);
    TextObject* getText(int frame, const std::string& id);
    TextObject* getEditingText();
    void clearEditingState();
    void render(sf::RenderTarget& target, int frame, int layer, bool isPixelMode, sf::RenderStates states, sf::Vector2u logicalSize);
    std::string hitTest(int frame, int layer, sf::Vector2f pos);
    void rasterizeText(int frame, int layer, const std::string& id, class Canvas& canvas);

    void saveUndoState(int frame);
    void undo(int frame);
    void redo(int frame);

    std::map<int, std::vector<TextObject>> m_frameTexts;
private:
    std::map<int, std::vector<std::vector<TextObject>>> m_undoStack;
    std::map<int, std::vector<std::vector<TextObject>>> m_redoStack;
    sf::RenderTexture m_renderTex;
};