#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

enum class AssistantTab {
    Tools,
    Tutorials
};

struct TutorialStep {
    std::string title;
    std::string instruction;
    sf::Color hintColor;
};

struct TutorialLesson {
    std::string name;
    std::vector<TutorialStep> steps;
};

class AIPanel {
private:
    sf::Font font;
    sf::Vector2f position;
    sf::Vector2f size;

    bool isVisible;
    bool isDraggingPanel;
    sf::Vector2f dragOffset;

    AssistantTab currentTab;
    bool isPixelMode;

    sf::FloatRect headerGripBounds;
    sf::FloatRect toolsTabBounds;
    sf::FloatRect tutorialsTabBounds;
    sf::FloatRect modeToggleBounds;
    sf::FloatRect backBtnBounds;

    sf::FloatRect btn1Bounds;
    sf::FloatRect btn2Bounds;
    sf::FloatRect btn3Bounds;
    sf::FloatRect btn4Bounds;
    sf::FloatRect btn5Bounds;

    sf::FloatRect tutPrevLessonBounds;
    sf::FloatRect tutNextLessonBounds;
    sf::FloatRect tutPrevStepBounds;
    sf::FloatRect tutNextStepBounds;
    sf::FloatRect tutToggleGhostBounds;

    bool lightingGuideActive;
    bool paletteCheckActive;
    bool ghostOverlayActive;

    std::vector<TutorialLesson> lessons;
    int currentLessonIdx;
    int currentStepIdx;

public:
    AIPanel();
    void init();
    void update(float dt);
    void draw(sf::RenderWindow& window);

    bool handleEvent(const sf::Event& event, sf::Vector2f mousePos);
    std::string handleClick(sf::Vector2f mousePos);

    void toggle();
    bool getIsVisible() const;

    void setPixelMode(bool pixelMode);
    bool getPixelMode() const;

    bool isLightingGuideActive() const;
    bool isPaletteCheckActive() const;
    bool isGhostOverlayActive() const;
    const TutorialStep* getCurrentTutorialStep() const;

    static std::vector<sf::Color> generateRampOrHarmony(sf::Color baseColor, bool pixelMode);
};