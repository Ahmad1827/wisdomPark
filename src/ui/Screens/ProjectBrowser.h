#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include "../../core/ProjectManager.h"

struct ProjectCard {
    sf::RectangleShape background;
    sf::Sprite thumbnail;
    sf::Text nameText;
    sf::Text detailText;
    ProjectMetadata meta;
    bool isHovered = false;
};

class ProjectBrowser {
private:
    sf::RectangleShape overlay;
    sf::RectangleShape modalBg;
    sf::Font font;
    sf::Text title;

    sf::RectangleShape newProjectBtn;
    sf::Text newProjectText;

    std::vector<ProjectCard> cards;
    ProjectManager* projManager;

public:
    ProjectBrowser();
    void init(ProjectManager* pm);
    void refresh();

    void updateHover(sf::Vector2f mousePos);
    std::string handleClick(sf::Vector2f mousePos, ProjectMetadata& outMeta);

    void draw(sf::RenderWindow& window);
};