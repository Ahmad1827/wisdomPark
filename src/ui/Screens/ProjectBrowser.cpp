#include "ProjectBrowser.h"

ProjectBrowser::ProjectBrowser() : pm(nullptr), showDeleteConfirm(false) {}

void ProjectBrowser::init(ProjectManager* projectManager) {
    pm = projectManager;
    font.loadFromFile("assets/font.otf");

    confirmBg.setSize(sf::Vector2f(500.f, 250.f));
    confirmBg.setOrigin(250.f, 125.f);
    confirmBg.setPosition(1920.f / 2.f, 1080.f / 2.f);
    confirmBg.setFillColor(sf::Color(30, 30, 35, 255));
    confirmBg.setOutlineThickness(2.f);
    confirmBg.setOutlineColor(sf::Color(150, 50, 50));

    confirmTitle.setFont(font);
    confirmTitle.setCharacterSize(22);
    confirmTitle.setFillColor(sf::Color::White);

    confirmWarning.setFont(font);
    confirmWarning.setString("This action cannot be undone.");
    confirmWarning.setCharacterSize(16);
    confirmWarning.setFillColor(sf::Color(200, 100, 100));

    confirmBtn.setSize(sf::Vector2f(120.f, 40.f));
    confirmBtn.setFillColor(sf::Color(180, 50, 50));
    confirmText.setFont(font);
    confirmText.setString("Delete");
    confirmText.setCharacterSize(18);
    confirmText.setFillColor(sf::Color::White);

    cancelBtn.setSize(sf::Vector2f(120.f, 40.f));
    cancelBtn.setFillColor(sf::Color(80, 80, 90));
    cancelText.setFont(font);
    cancelText.setString("Cancel");
    cancelText.setCharacterSize(18);
    cancelText.setFillColor(sf::Color::White);

    // "+ New Project" - sits to the right of the "Recent Projects" title,
    // same row (y=200, matching the old invisible open-file hitbox height).
    newProjectBtn.setSize(sf::Vector2f(180.f, 40.f));
    newProjectBtn.setPosition(1520.f, 200.f);
    newProjectBtn.setFillColor(sf::Color(50, 150, 220));

    newProjectText.setFont(font);
    newProjectText.setString("+ New Project");
    newProjectText.setCharacterSize(16);
    newProjectText.setFillColor(sf::Color::White);
    sf::FloatRect npb = newProjectText.getLocalBounds();
    newProjectText.setOrigin(npb.left + npb.width / 2.f, npb.top + npb.height / 2.f);
    newProjectText.setPosition(newProjectBtn.getPosition().x + newProjectBtn.getSize().x / 2.f,
        newProjectBtn.getPosition().y + newProjectBtn.getSize().y / 2.f);

    // "Open File" - loads a .wpk from anywhere on disk via the native file
    // dialog, next to the New Project button.
    openFileBtn.setSize(sf::Vector2f(150.f, 40.f));
    openFileBtn.setPosition(1350.f, 200.f);
    openFileBtn.setFillColor(sf::Color(60, 60, 70));

    openFileText.setFont(font);
    openFileText.setString("Open File");
    openFileText.setCharacterSize(16);
    openFileText.setFillColor(sf::Color::White);
    sf::FloatRect ofb = openFileText.getLocalBounds();
    openFileText.setOrigin(ofb.left + ofb.width / 2.f, ofb.top + ofb.height / 2.f);
    openFileText.setPosition(openFileBtn.getPosition().x + openFileBtn.getSize().x / 2.f,
        openFileBtn.getPosition().y + openFileBtn.getSize().y / 2.f);

    refreshList();
}

void ProjectBrowser::refreshList() {
    if (pm) projects = pm->getRecentProjects();
}

void ProjectBrowser::updateHover(sf::Vector2f mousePos) {
    if (showDeleteConfirm) {
        confirmBtn.setFillColor(confirmBtn.getGlobalBounds().contains(mousePos) ? sf::Color(220, 70, 70) : sf::Color(180, 50, 50));
        cancelBtn.setFillColor(cancelBtn.getGlobalBounds().contains(mousePos) ? sf::Color(100, 100, 110) : sf::Color(80, 80, 90));
        return;
    }

    newProjectBtn.setFillColor(newProjectBtn.getGlobalBounds().contains(mousePos) ? sf::Color(70, 170, 240) : sf::Color(50, 150, 220));
    openFileBtn.setFillColor(openFileBtn.getGlobalBounds().contains(mousePos) ? sf::Color(80, 80, 90) : sf::Color(60, 60, 70));
}

std::string ProjectBrowser::handleClick(sf::Vector2f mousePos, ProjectMetadata& outMeta) {
    if (showDeleteConfirm) {
        if (cancelBtn.getGlobalBounds().contains(mousePos)) {
            showDeleteConfirm = false;
        }
        else if (confirmBtn.getGlobalBounds().contains(mousePos)) {
            if (pm) {
                pm->deleteProject(projectToDelete);
                refreshList();
            }
            showDeleteConfirm = false;
        }
        return "";
    }

    if (newProjectBtn.getGlobalBounds().contains(mousePos)) {
        return "new_project";
    }
    if (openFileBtn.getGlobalBounds().contains(mousePos)) {
        return "open_native";
    }

    float rx = 900.f;
    float ry = 280.f;

    for (size_t i = 0; i < projects.size(); ++i) {
        sf::FloatRect cardBounds(rx, ry, 800.f, 100.f);
        sf::FloatRect delBounds(rx + 730.f, ry + 30.f, 40.f, 40.f);

        if (delBounds.contains(mousePos)) {
            projectToDelete = projects[i].name;
            showDeleteConfirm = true;

            confirmTitle.setString("Delete project '" + projectToDelete + "'?");
            sf::FloatRect tb = confirmTitle.getLocalBounds();
            confirmTitle.setOrigin(tb.width / 2.f, tb.height / 2.f);
            confirmTitle.setPosition(1920.f / 2.f, 1080.f / 2.f - 60.f);

            sf::FloatRect wb = confirmWarning.getLocalBounds();
            confirmWarning.setOrigin(wb.width / 2.f, wb.height / 2.f);
            confirmWarning.setPosition(1920.f / 2.f, 1080.f / 2.f - 20.f);

            confirmBtn.setPosition(1920.f / 2.f + 20.f, 1080.f / 2.f + 40.f);
            confirmText.setPosition(confirmBtn.getPosition().x + 30.f, confirmBtn.getPosition().y + 8.f);

            cancelBtn.setPosition(1920.f / 2.f - 140.f, 1080.f / 2.f + 40.f);
            cancelText.setPosition(cancelBtn.getPosition().x + 30.f, cancelBtn.getPosition().y + 8.f);
            return "";
        }

        if (cardBounds.contains(mousePos)) {
            outMeta = projects[i];
            return "load_project";
        }
        ry += 125.f;
    }

    return "";
}

void ProjectBrowser::draw(sf::RenderWindow& window) {
    sf::Text recTitle("Recent Projects", font, 24);
    recTitle.setFillColor(sf::Color::White);
    recTitle.setPosition(900.f, 200.f);
    window.draw(recTitle);

    window.draw(newProjectBtn);
    window.draw(newProjectText);
    window.draw(openFileBtn);
    window.draw(openFileText);

    sf::RectangleShape line(sf::Vector2f(800.f, 2.f));
    line.setPosition(900.f, 240.f);
    line.setFillColor(sf::Color(255, 255, 255, 40));
    window.draw(line);

    float rx = 900.f;
    float ry = 280.f;

    for (const auto& meta : projects) {
        sf::RectangleShape card(sf::Vector2f(800.f, 100.f));
        card.setPosition(rx, ry);
        card.setFillColor(sf::Color(25, 28, 35, 190));
        card.setOutlineThickness(1.5f);
        card.setOutlineColor(sf::Color(100, 100, 120));
        window.draw(card);

        sf::RectangleShape thumb(sf::Vector2f(140.f, 80.f));
        thumb.setPosition(rx + 10.f, ry + 10.f);
        thumb.setFillColor(sf::Color(15, 15, 20));
        if (meta.thumbnail.getSize().x > 0) {
            thumb.setTexture(&meta.thumbnail);
        }
        window.draw(thumb);

        sf::Text pName(meta.name, font, 24);
        pName.setFillColor(sf::Color(255, 200, 100));
        pName.setPosition(rx + 170.f, ry + 15.f);
        window.draw(pName);

        std::string typeStr = meta.isPixelMode ? "Pixel Art" : "Normal";
        std::string details = typeStr + "  |  " + std::to_string(meta.width) + "x" + std::to_string(meta.height) + "  |  Modified: " + meta.lastModified;
        sf::Text pDet(details, font, 14);
        pDet.setFillColor(sf::Color(180, 180, 180));
        pDet.setPosition(rx + 170.f, ry + 55.f);
        window.draw(pDet);

        sf::RectangleShape delBtn(sf::Vector2f(40.f, 40.f));
        delBtn.setPosition(rx + 730.f, ry + 30.f);
        delBtn.setFillColor(sf::Color(150, 40, 40));
        window.draw(delBtn);

        sf::Text xText("X", font, 20);
        xText.setFillColor(sf::Color::White);
        xText.setPosition(delBtn.getPosition().x + 13.f, delBtn.getPosition().y + 8.f);
        window.draw(xText);

        ry += 125.f;
    }

    if (showDeleteConfirm) {
        sf::RectangleShape overlay(sf::Vector2f(1920.f, 1080.f));
        overlay.setFillColor(sf::Color(0, 0, 0, 180));
        window.draw(overlay);

        window.draw(confirmBg);
        window.draw(confirmTitle);
        window.draw(confirmWarning);
        window.draw(confirmBtn);
        window.draw(confirmText);
        window.draw(cancelBtn);
        window.draw(cancelText);
    }
}