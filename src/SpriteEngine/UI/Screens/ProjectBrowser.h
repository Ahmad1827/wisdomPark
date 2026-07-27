#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include "../../src/core/ProjectManager.h"
class ProjectBrowser {
private:
    ProjectManager* pm;
    sf::Font font;
    std::vector<ProjectMetadata> projects;
    bool showDeleteConfirm;
    std::string projectToDelete;
    sf::RectangleShape confirmBg;
    sf::Text confirmTitle;
    sf::Text confirmWarning;
    sf::RectangleShape confirmBtn;
    sf::Text confirmText;
    sf::RectangleShape cancelBtn;
    sf::Text cancelText;

    // Was missing entirely before - handleClick() had no code path that
    // could ever return "new_project", so there was nothing to click to
    // create a project once you were on this screen.
    sf::RectangleShape newProjectBtn;
    sf::Text newProjectText;
    sf::RectangleShape openFileBtn;
    sf::Text openFileText;

    void refreshList();
public:
    ProjectBrowser();
    void init(ProjectManager* projectManager);
    void updateHover(sf::Vector2f mousePos);
    std::string handleClick(sf::Vector2f mousePos, ProjectMetadata& outMeta);
    void draw(sf::RenderWindow& window);
};