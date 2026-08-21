#include "ProjectBrowser.h"
#include "../UITheme.h"
#include <algorithm>

ProjectBrowser::ProjectBrowser()
    : pm(nullptr), showDeleteConfirm(false), scrollOffset(0.0f), maxScroll(0.0f) {}

void ProjectBrowser::init(ProjectManager* projectManager) {
    pm = projectManager;
    font.loadFromFile("assets/font.otf");

    containerBounds = sf::FloatRect(280.f, 140.f, 1360.f, 800.f);
    openFileBtnBounds = sf::FloatRect(containerBounds.left + containerBounds.width - 340.f, containerBounds.top + 24.f, 150.f, 38.f);
    newProjectBtnBounds = sf::FloatRect(containerBounds.left + containerBounds.width - 175.f, containerBounds.top + 24.f, 150.f, 38.f);

    deleteModalBounds = sf::FloatRect(1920.f / 2.f - 240.f, 1080.f / 2.f - 120.f, 480.f, 240.f);
    confirmBtnBounds = sf::FloatRect(deleteModalBounds.left + 30.f, deleteModalBounds.top + 160.f, 195.f, 44.f);
    cancelBtnBounds = sf::FloatRect(deleteModalBounds.left + 255.f, deleteModalBounds.top + 160.f, 195.f, 44.f);

    refreshList();
}

void ProjectBrowser::refreshList() {
    if (pm) projects = pm->getRecentProjects();
}

void ProjectBrowser::updateHover(sf::Vector2f mousePos) {}

void ProjectBrowser::handleScroll(float delta) {
    scrollOffset = std::clamp(scrollOffset - delta * 40.0f, 0.0f, maxScroll);
}

std::string ProjectBrowser::handleClick(sf::Vector2f mousePos, ProjectMetadata& outMeta) {
    if (showDeleteConfirm) {
        if (cancelBtnBounds.contains(mousePos)) {
            showDeleteConfirm = false;
            return "";
        }
        if (confirmBtnBounds.contains(mousePos)) {
            if (pm) {
                pm->deleteProject(projectToDelete);
                refreshList();
            }
            showDeleteConfirm = false;
            return "";
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
    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    WisdomUI::Theme::DrawSunsetPanel(window, containerBounds, 1.0f);

    sf::FloatRect headerGrip(containerBounds.left + 20.f, containerBounds.top + 16.f, 320.f, 44.f);
    WisdomUI::Theme::DrawCrispText(window, font, "ARCHIVE VAULT", 24, headerGrip.left, headerGrip.top + 4.f, WisdomUI::Theme::SunsetGold, sf::Color(14, 6, 20));
    WisdomUI::Theme::DrawCrispText(window, font, "RECENT CREATIVE CANVASES", 11, headerGrip.left + 2.f, headerGrip.top + 32.f, WisdomUI::Theme::TextSecondary);

    WisdomUI::Theme::DrawSunsetButton(window, openFileBtnBounds, "Browse Disk", font, 11, false, openFileBtnBounds.contains(mPos), false, 1.0f);
    WisdomUI::Theme::DrawSunsetButton(window, newProjectBtnBounds, "+ New Canvas", font, 11, false, newProjectBtnBounds.contains(mPos), true, 1.0f);

    sf::RectangleShape divider(sf::Vector2f(containerBounds.width - 40.f, 1.5f));
    divider.setPosition(containerBounds.left + 20.f, containerBounds.top + 72.f);
    divider.setFillColor(WisdomUI::Theme::SunsetPlum);
    window.draw(divider);

    cardBoundsList.clear();
    deleteBtnsList.clear();

    float startX = containerBounds.left + 24.f;
    float startY = containerBounds.top + 88.f - scrollOffset;
    float cardWidth = (containerBounds.width - 64.f) / 2.f;
    float cardHeight = 110.f;

    for (size_t i = 0; i < projects.size(); ++i) {
        float col = static_cast<float>(i % 2);
        float row = static_cast<float>(i / 2);
        float cx = startX + col * (cardWidth + 16.f);
        float cy = startY + row * (cardHeight + 14.f);

        sf::FloatRect cardRect(cx, cy, cardWidth, cardHeight);
        cardBoundsList.push_back(cardRect);

        if (cy + cardHeight < containerBounds.top + 74.f || cy > containerBounds.top + containerBounds.height - 20.f) {
            deleteBtnsList.push_back(sf::FloatRect(0, 0, 0, 0));
            continue;
        }

        bool isHov = cardRect.contains(mPos);

        sf::RectangleShape cardBg(sf::Vector2f(cardRect.width, cardRect.height));
        cardBg.setPosition(cardRect.left, cardRect.top);
        cardBg.setFillColor(isHov ? WisdomUI::Theme::SunsetSkyMid : WisdomUI::Theme::SunsetDeepDark);
        cardBg.setOutlineThickness(1.5f);
        cardBg.setOutlineColor(isHov ? WisdomUI::Theme::SunsetGold : WisdomUI::Theme::SunsetPlum);
        window.draw(cardBg);

        sf::FloatRect thumbRect(cx + 10.f, cy + 10.f, 130.f, 90.f);
        sf::RectangleShape thumb(sf::Vector2f(thumbRect.width, thumbRect.height));
        thumb.setPosition(thumbRect.left, thumbRect.top);
        thumb.setFillColor(sf::Color(10, 4, 18));
        thumb.setOutlineThickness(1.f);
        thumb.setOutlineColor(WisdomUI::Theme::SunsetPlum);

        if (projects[i].thumbnail.getSize().x > 0) {
            thumb.setTexture(&projects[i].thumbnail);
        }
        window.draw(thumb);

        sf::Color titleColor = isHov ? WisdomUI::Theme::SunsetGold : WisdomUI::Theme::SunsetAmber;
        WisdomUI::Theme::DrawCrispText(window, font, projects[i].name, 16, cx + 152.f, cy + 14.f, titleColor, sf::Color(14, 6, 20));

        std::string modeBadge = projects[i].isPixelMode ? "PIXEL ART" : "RGBA CANV";
        WisdomUI::Theme::DrawCrispText(window, font, modeBadge, 10, cx + 152.f, cy + 42.f, WisdomUI::Theme::SunsetPeach);

        std::string resStr = std::to_string(projects[i].width) + "x" + std::to_string(projects[i].height) + " px";
        WisdomUI::Theme::DrawCrispText(window, font, resStr, 11, cx + 240.f, cy + 41.f, WisdomUI::Theme::TextPrimary);

        WisdomUI::Theme::DrawCrispText(window, font, "MODIFIED: " + projects[i].lastModified, 10, cx + 152.f, cy + 74.f, WisdomUI::Theme::TextSecondary);

        sf::FloatRect delRect(cx + cardWidth - 42.f, cy + 10.f, 32.f, 32.f);
        deleteBtnsList.push_back(delRect);
        bool delHov = delRect.contains(mPos);

        WisdomUI::Theme::DrawSunsetButton(window, delRect, "X", font, 11, false, delHov, true, 1.0f);
    }

    float totalContentH = (std::ceil(static_cast<float>(projects.size()) / 2.f) * (cardHeight + 14.f));
    maxScroll = std::max(0.0f, totalContentH - (containerBounds.height - 110.f));

    if (showDeleteConfirm) {
        sf::RectangleShape modalOverlay(sf::Vector2f(1920.f, 1080.f));
        modalOverlay.setFillColor(sf::Color(10, 4, 16, 220));
        window.draw(modalOverlay);

        WisdomUI::Theme::DrawSunsetPanel(window, deleteModalBounds, 1.0f);

        WisdomUI::Theme::DrawCrispText(window, font, "DELETE CONFIRMATION", 16, deleteModalBounds.left + deleteModalBounds.width / 2.f, deleteModalBounds.top + 32.f, WisdomUI::Theme::SunsetCoral, sf::Color(14, 6, 20), true, true);
        WisdomUI::Theme::DrawCrispText(window, font, "Delete '" + projectToDelete + "'?", 13, deleteModalBounds.left + deleteModalBounds.width / 2.f, deleteModalBounds.top + 76.f, WisdomUI::Theme::TextPrimary, sf::Color(14, 6, 20), true, true);
        WisdomUI::Theme::DrawCrispText(window, font, "This will permanently remove the file archive.", 11, deleteModalBounds.left + deleteModalBounds.width / 2.f, deleteModalBounds.top + 106.f, WisdomUI::Theme::TextSecondary, sf::Color(14, 6, 20), true, true);

        WisdomUI::Theme::DrawSunsetButton(window, confirmBtnBounds, "Delete Forever", font, 12, false, confirmBtnBounds.contains(mPos), true, 1.0f);
        WisdomUI::Theme::DrawSunsetButton(window, cancelBtnBounds, "Keep Project", font, 12, false, cancelBtnBounds.contains(mPos), false, 1.0f);
    }
}