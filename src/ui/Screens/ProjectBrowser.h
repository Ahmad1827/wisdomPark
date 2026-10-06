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

    sf::FloatRect backBtnBounds;
    sf::FloatRect newProjectBtnBounds;
    sf::FloatRect openFileBtnBounds;
    sf::FloatRect confirmBtnBounds;
    sf::FloatRect cancelBtnBounds;
    sf::FloatRect deleteModalBounds;

    float scrollOffset;
    float maxScroll;

public:
    ProjectBrowser();
    void init(ProjectManager* projectManager);
    void refreshList();
    size_t getProjectCount() const { return projects.size(); }
    void updateHover(sf::Vector2f mousePos);
    std::string handleClick(sf::Vector2f mousePos, ProjectMetadata& outMeta);
    void handleScroll(float delta);
    // Draws the buttons and project grid; `time` is seconds since the screen opened.
    void draw(sf::RenderWindow& window, float time);
};