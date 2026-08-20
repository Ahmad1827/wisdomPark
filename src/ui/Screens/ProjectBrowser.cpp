#include "ProjectBrowser.h"
#include "../UITheme.h"

ProjectBrowser::ProjectBrowser() : pm(nullptr), showDeleteConfirm(false) {}

void ProjectBrowser::init(ProjectManager* projectManager) {
    pm = projectManager;
    font.loadFromFile("assets/font.otf");

    deleteModalBounds = sf::FloatRect(1920.f / 2.f - 250.f, 1080.f / 2.f - 130.f, 500.f, 260.f);
    confirmBtnBounds = sf::FloatRect(deleteModalBounds.left + 40.f, deleteModalBounds.top + 180.f, 190.f, 44.f);
    cancelBtnBounds = sf::FloatRect(deleteModalBounds.left + 270.f, deleteModalBounds.top + 180.f, 190.f, 44.f);

    openFileBtnBounds = sf::FloatRect(1320.f, 190.f, 160.f, 38.f);
    newProjectBtnBounds = sf::FloatRect(1500.f, 190.f, 180.f, 38.f);

    refreshList();
}

void ProjectBrowser::refreshList() {
    if (pm) projects = pm->getRecentProjects();
}

void ProjectBrowser::updateHover(sf::Vector2f mousePos) {}

std::string ProjectBrowser::handleClick(sf::Vector2f mousePos, ProjectMetadata& outMeta) {
    if (showDeleteConfirm) {
        if (cancelBtnBounds.contains(mousePos)) {
            showDeleteConfirm = false;
        }
        else if (confirmBtnBounds.contains(mousePos)) {
            if (pm) {
                pm->deleteProject(projectToDelete);
                refreshList();
            }
            showDeleteConfirm = false;
        }
        return "";
    }

    if (newProjectBtnBounds.contains(mousePos)) return "new_project";
    if (openFileBtnBounds.contains(mousePos)) return "open_native";

    for (size_t i = 0; i < deleteBtnsList.size(); ++i) {
        if (deleteBtnsList[i].contains(mousePos) && i < projects.size()) {
            projectToDelete = projects[i].name;
            showDeleteConfirm = true;
            return "";
        }
    }

    for (size_t i = 0; i < cardBoundsList.size(); ++i) {
        if (cardBoundsList[i].contains(mousePos) && i < projects.size()) {
            outMeta = projects[i];
            return "load_project";
        }
    }

    return "";
}

void ProjectBrowser::draw(sf::RenderWindow& window) {
    WisdomUI::Theme::DrawCrispText(window, font, "RECENT ARCHIVES", 22, 900.f, 194.f, WisdomUI::Theme::SunsetAmber, sf::Color(14, 6, 20));

    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    WisdomUI::Theme::DrawSunsetButton(window, openFileBtnBounds, "Open Disk File", font, 11, false, openFileBtnBounds.contains(mPos), false, 1.0f);
    WisdomUI::Theme::DrawSunsetButton(window, newProjectBtnBounds, "+ New Project", font, 11, false, newProjectBtnBounds.contains(mPos), true, 1.0f);

    sf::RectangleShape line(sf::Vector2f(780.f, 1.5f));
    line.setPosition(900.f, 240.f);
    line.setFillColor(WisdomUI::Theme::SunsetPlum);
    window.draw(line);

    cardBoundsList.clear();
    deleteBtnsList.clear();

    float rx = 900.f;
    float ry = 265.f;

    for (const auto& meta : projects) {
        sf::FloatRect cardRect(rx, ry, 780.f, 96.f);
        cardBoundsList.push_back(cardRect);

        bool isHov = cardRect.contains(mPos);

        sf::RectangleShape card(sf::Vector2f(cardRect.width, cardRect.height));
        card.setPosition(cardRect.left, cardRect.top);
        card.setFillColor(isHov ? WisdomUI::Theme::SunsetSkyMid : WisdomUI::Theme::SunsetSkyTop);
        card.setOutlineThickness(1.5f);
        card.setOutlineColor(isHov ? WisdomUI::Theme::SunsetAmber : WisdomUI::Theme::SunsetCoralDark);
        window.draw(card);

        sf::FloatRect thumbRect(rx + 10.f, ry + 8.f, 130.f, 80.f);
        sf::RectangleShape thumbFrame(sf::Vector2f(thumbRect.width, thumbRect.height));
        thumbFrame.setPosition(thumbRect.left, thumbRect.top);
        thumbFrame.setFillColor(WisdomUI::Theme::SunsetDeepDark);
        thumbFrame.setOutlineThickness(1.f);
        thumbFrame.setOutlineColor(WisdomUI::Theme::SunsetPlum);

        if (meta.thumbnail.getSize().x > 0) {
            thumbFrame.setTexture(&meta.thumbnail);
        }
        window.draw(thumbFrame);

        WisdomUI::Theme::DrawCrispText(window, font, meta.name, 18, rx + 160.f, ry + 16.f, isHov ? WisdomUI::Theme::SunsetGold : WisdomUI::Theme::TextPrimary);

        std::string typeStr = meta.isPixelMode ? "Pixel Mode" : "Standard Engine";
        std::string details = typeStr + "  |  " + std::to_string(meta.width) + "x" + std::to_string(meta.height) + "  |  " + meta.lastModified;
        WisdomUI::Theme::DrawCrispText(window, font, details, 12, rx + 160.f, ry + 56.f, WisdomUI::Theme::TextSecondary);

        sf::FloatRect delRect(rx + 724.f, ry + 28.f, 40.f, 40.f);
        deleteBtnsList.push_back(delRect);
        WisdomUI::Theme::DrawSunsetButton(window, delRect, "X", font, 13, false, delRect.contains(mPos), true, 1.0f);

        ry += 112.f;
    }

    if (showDeleteConfirm) {
        sf::RectangleShape overlay(sf::Vector2f(1920.f, 1080.f));
        overlay.setFillColor(sf::Color(10, 4, 16, 210));
        window.draw(overlay);

        WisdomUI::Theme::DrawSunsetPanel(window, deleteModalBounds, 1.0f);

        WisdomUI::Theme::DrawCrispText(window, font, "DELETE CONFIRMATION", 16, deleteModalBounds.left + deleteModalBounds.width / 2.f, deleteModalBounds.top + 35.f, WisdomUI::Theme::SunsetCoral, sf::Color(14, 6, 20), true, true);
        WisdomUI::Theme::DrawCrispText(window, font, "Remove '" + projectToDelete + "' permanently?", 13, deleteModalBounds.left + deleteModalBounds.width / 2.f, deleteModalBounds.top + 80.f, WisdomUI::Theme::TextPrimary, sf::Color(14, 6, 20), true, true);
        WisdomUI::Theme::DrawCrispText(window, font, "This action cannot be undone.", 11, deleteModalBounds.left + deleteModalBounds.width / 2.f, deleteModalBounds.top + 115.f, WisdomUI::Theme::SunsetPeach, sf::Color(14, 6, 20), true, true);

        WisdomUI::Theme::DrawSunsetButton(window, confirmBtnBounds, "Delete Forever", font, 12, false, confirmBtnBounds.contains(mPos), true, 1.0f);
        WisdomUI::Theme::DrawSunsetButton(window, cancelBtnBounds, "Cancel", font, 12, false, cancelBtnBounds.contains(mPos), false, 1.0f);
    }
}