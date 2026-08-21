#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include "../../core/ProjectManager.h"

class ProjectBrowser {
private:
    ProjectManager* pm;
    sf::Font font;
    std::vector<ProjectMetadata> projects;
    bool showDeleteConfirm;
    std::string projectToDelete;

    sf::FloatRect containerBounds;
    sf::FloatRect backBtnBounds;
    sf::FloatRect newProjectBtnBounds;
    sf::FloatRect openFileBtnBounds;
    sf::FloatRect confirmBtnBounds;
    sf::FloatRect cancelBtnBounds;
    sf::FloatRect deleteModalBounds;

    std::vector<sf::FloatRect> cardBoundsList;
    std::vector<sf::FloatRect> deleteBtnsList;

    float scrollOffset;
    float maxScroll;

    void refreshList();

public:
    ProjectBrowser();
    void init(ProjectManager* projectManager);
    void updateHover(sf::Vector2f mousePos);
    std::string handleClick(sf::Vector2f mousePos, ProjectMetadata& outMeta);
    void handleScroll(float delta);
    void draw(sf::RenderWindow& window);
};