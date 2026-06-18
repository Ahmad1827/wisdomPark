#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include "../core/AppState.h"
#include "../core/SettingsManager.h"
#include "../core/Canvas.h"
#include "../core/Timeline.h"
#include "../ai/AIHelper.h"
#include "Screens/WelcomeScreen.h"
#include "Screens/AISettingsModal.h"
#include "LeftToolbar.h"
#include "RightProperties.h"
#include "BottomTimeline.h"

class UIManager {
private:
    sf::Texture bgTexture;
    sf::Sprite bgSprite;

    WelcomeScreen welcomeScreen;
    AISettingsModal settingsModal;
    LeftToolbar leftToolbar;
    RightProperties rightProperties;
    BottomTimeline bottomTimeline;

    sf::Font font;
    std::string currentPrompt;
    bool isTypingPrompt;
    sf::RectangleShape promptBox;
    sf::Text promptDisplay;

    sf::Text uiText;
    bool showingText;
    float textAlpha;
    sf::Clock textClock;

    bool isLightingMode;
    int promptQuantity;
    bool focusMode;

    void showMessage(const std::string& msg, sf::Color color);

public:
    UIManager();
    void init();
    void handleEvent(const sf::Event& event, sf::RenderWindow& window, AppState& currentState, AppSettings& settings, Canvas& canvas, Timeline& timeline, AIHelper& aiHelper);
    void update(sf::RenderWindow& window, AppState currentState, AppSettings& settings, float dt, Canvas& canvas);
    void draw(sf::RenderWindow& window, AppState currentState, Canvas& canvas, AIHelper& aiHelper, Timeline& timeline);
};