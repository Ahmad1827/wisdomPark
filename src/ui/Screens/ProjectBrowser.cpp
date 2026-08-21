#include "ProjectBrowser.h"
#include "../UITheme.h"
#include <algorithm>

ProjectBrowser::ProjectBrowser()
    : pm(nullptr), showDeleteConfirm(false), scrollOffset(0.0f), maxScroll(0.0f) {}

void ProjectBrowser::init(ProjectManager* projectManager) {
    pm = projectManager;
    font.loadFromFile("assets/font.otf");

    containerBounds = sf::FloatRect(200.f, 60.f, 1520.f, 960.f);
    backBtnBounds = sf::FloatRect(containerBounds.left + 28.f, containerBounds.top + 22.f, 140.f, 44.f);

    openFileBtnBounds = sf::FloatRect(containerBounds.left + containerBounds.width - 380.f, containerBounds.top + 22.f, 170.f, 44.f);
    newProjectBtnBounds = sf::FloatRect(containerBounds.left + containerBounds.width - 194.f, containerBounds.top + 22.f, 170.f, 44.f);

    deleteModalBounds = sf::FloatRect(1920.f / 2.f - 270.f, 1080.f / 2.f - 140.f, 540.f, 280.f);
    confirmBtnBounds = sf::FloatRect(deleteModalBounds.left + 30.f, deleteModalBounds.top + 190.f, 225.f, 50.f);
    cancelBtnBounds = sf::FloatRect(deleteModalBounds.left + 285.f, deleteModalBounds.top + 190.f, 225.f, 50.f);

    refreshList();
}

void ProjectBrowser::refreshList() {
    if (pm) projects = pm->getRecentProjects();
}

void ProjectBrowser::updateHover(sf::Vector2f mousePos) {}

void ProjectBrowser::handleScroll(float delta) {
    scrollOffset = std::clamp(scrollOffset - delta * 55.0f, 0.0f, maxScroll);
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

    if (backBtnBounds.contains(mousePos)) return "back";
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

    WisdomUI::Theme::DrawSunsetButton(window, backBtnBounds, "< BACK", font, 16, false, backBtnBounds.contains(mPos), false, 1.0f);

    WisdomUI::Theme::DrawCrispText(window, font, "ARCHIVE VAULT", 28, containerBounds.left + 190.f, containerBounds.top + 20.f, WisdomUI::Theme::SunsetGold, sf::Color(14, 6, 20));
    WisdomUI::Theme::DrawCrispText(window, font, "RECENT CREATIVE CANVASES & DISK PROJECTS", 14, containerBounds.left + 192.f, containerBounds.top + 54.f, WisdomUI::Theme::TextSecondary);

    WisdomUI::Theme::DrawSunsetButton(window, openFileBtnBounds, "Browse Disk", font, 15, false, openFileBtnBounds.contains(mPos), false, 1.0f);
    WisdomUI::Theme::DrawSunsetButton(window, newProjectBtnBounds, "+ New Canvas", font, 15, false, newProjectBtnBounds.contains(mPos), true, 1.0f);

    sf::RectangleShape divider(sf::Vector2f(containerBounds.width - 56.f, 2.f));
    divider.setPosition(containerBounds.left + 28.f, containerBounds.top + 86.f);
    divider.setFillColor(WisdomUI::Theme::SunsetPlum);
    window.draw(divider);

    cardBoundsList.clear();
    deleteBtnsList.clear();

    float startX = containerBounds.left + 32.f;
    float startY = containerBounds.top + 104.f - scrollOffset;
    float cardWidth = (containerBounds.width - 84.f) / 2.f;
    float cardHeight = 126.f;

    for (size_t i = 0; i < projects.size(); ++i) {
        float col = static_cast<float>(i % 2);
        float row = static_cast<float>(i / 2);
        float cx = startX + col * (cardWidth + 20.f);
        float cy = startY + row * (cardHeight + 18.f);

        sf::FloatRect cardRect(cx, cy, cardWidth, cardHeight);
        cardBoundsList.push_back(cardRect);

        if (cy + cardHeight < containerBounds.top + 88.f || cy > containerBounds.top + containerBounds.height - 20.f) {
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

        sf::FloatRect thumbRect(cx + 12.f, cy + 12.f, 150.f, 102.f);
        sf::RectangleShape thumbFrame(sf::Vector2f(thumbRect.width, thumbRect.height));
        thumbFrame.setPosition(thumbRect.left, thumbRect.top);
        thumbFrame.setFillColor(sf::Color(18, 8, 28));
        thumbFrame.setOutlineThickness(1.f);
        thumbFrame.setOutlineColor(isHov ? WisdomUI::Theme::SunsetAmber : WisdomUI::Theme::SunsetPlum);
        window.draw(thumbFrame);

        float chkSize = 8.f;
        sf::RectangleShape chk1(sf::Vector2f(chkSize, chkSize)); chk1.setFillColor(sf::Color(28, 14, 40));
        sf::RectangleShape chk2(sf::Vector2f(chkSize, chkSize)); chk2.setFillColor(sf::Color(38, 20, 52));
        for (float ty = thumbRect.top; ty < thumbRect.top + thumbRect.height; ty += chkSize) {
            for (float tx = thumbRect.left; tx < thumbRect.left + thumbRect.width; tx += chkSize) {
                float tw = std::min(chkSize, thumbRect.left + thumbRect.width - tx);
                float th = std::min(chkSize, thumbRect.top + thumbRect.height - ty);
                bool alt = (static_cast<int>((tx - thumbRect.left) / chkSize) + static_cast<int>((ty - thumbRect.top) / chkSize)) % 2 == 0;
                sf::RectangleShape& r = alt ? chk1 : chk2;
                r.setSize(sf::Vector2f(tw, th));
                r.setPosition(tx, ty);
                window.draw(r);
            }
        }

        if (projects[i].thumbnail.getSize().x > 0 && projects[i].thumbnail.getSize().y > 0) {
            sf::Sprite thumbSprite;
            thumbSprite.setTexture(projects[i].thumbnail, true);

            float scaleX = (thumbRect.width - 8.f) / static_cast<float>(projects[i].thumbnail.getSize().x);
            float scaleY = (thumbRect.height - 8.f) / static_cast<float>(projects[i].thumbnail.getSize().y);
            float scale = std::min(scaleX, scaleY);

            thumbSprite.setScale(scale, scale);
            thumbSprite.setColor(sf::Color::White);

            float sprW = static_cast<float>(projects[i].thumbnail.getSize().x) * scale;
            float sprH = static_cast<float>(projects[i].thumbnail.getSize().y) * scale;
            float posX = thumbRect.left + (thumbRect.width - sprW) / 2.f;
            float posY = thumbRect.top + (thumbRect.height - sprH) / 2.f;

            thumbSprite.setPosition(std::floor(posX), std::floor(posY));
            window.draw(thumbSprite);
        }
        else {
            WisdomUI::Theme::DrawCrispText(window, font, "NO PREVIEW", 11, thumbRect.left + thumbRect.width / 2.f, thumbRect.top + thumbRect.height / 2.f, WisdomUI::Theme::TextSecondary, sf::Color::Transparent, true, true);
        }

        sf::Color titleColor = isHov ? WisdomUI::Theme::SunsetGold : WisdomUI::Theme::SunsetAmber;
        WisdomUI::Theme::DrawCrispText(window, font, projects[i].name, 20, cx + 176.f, cy + 16.f, titleColor, sf::Color(14, 6, 20));

        std::string modeBadge = projects[i].isPixelMode ? "PIXEL ART" : "RGBA CANV";
        WisdomUI::Theme::DrawCrispText(window, font, modeBadge, 13, cx + 176.f, cy + 50.f, WisdomUI::Theme::SunsetPeach);

        std::string resStr = std::to_string(projects[i].width) + " x " + std::to_string(projects[i].height) + " px";
        WisdomUI::Theme::DrawCrispText(window, font, resStr, 14, cx + 280.f, cy + 49.f, WisdomUI::Theme::TextPrimary);

        WisdomUI::Theme::DrawCrispText(window, font, "MODIFIED: " + projects[i].lastModified, 13, cx + 176.f, cy + 86.f, WisdomUI::Theme::TextSecondary);

        sf::FloatRect delRect(cx + cardWidth - 52.f, cy + 12.f, 40.f, 40.f);
        deleteBtnsList.push_back(delRect);
        bool delHov = delRect.contains(mPos);

        WisdomUI::Theme::DrawSunsetButton(window, delRect, "X", font, 14, false, delHov, true, 1.0f);
    }

    float totalContentH = (std::ceil(static_cast<float>(projects.size()) / 2.f) * (cardHeight + 18.f));
    maxScroll = std::max(0.0f, totalContentH - (containerBounds.height - 120.f));

    if (showDeleteConfirm) {
        sf::RectangleShape modalOverlay(sf::Vector2f(1920.f, 1080.f));
        modalOverlay.setFillColor(sf::Color(10, 4, 16, 225));
        window.draw(modalOverlay);

        WisdomUI::Theme::DrawSunsetPanel(window, deleteModalBounds, 1.0f);

        WisdomUI::Theme::DrawCrispText(window, font, "DELETE CONFIRMATION", 20, deleteModalBounds.left + deleteModalBounds.width / 2.f, deleteModalBounds.top + 34.f, WisdomUI::Theme::SunsetCoral, sf::Color(14, 6, 20), true, true);
        WisdomUI::Theme::DrawCrispText(window, font, "Delete '" + projectToDelete + "'?", 16, deleteModalBounds.left + deleteModalBounds.width / 2.f, deleteModalBounds.top + 84.f, WisdomUI::Theme::TextPrimary, sf::Color(14, 6, 20), true, true);
        WisdomUI::Theme::DrawCrispText(window, font, "This will permanently remove the file archive.", 13, deleteModalBounds.left + deleteModalBounds.width / 2.f, deleteModalBounds.top + 118.f, WisdomUI::Theme::TextSecondary, sf::Color(14, 6, 20), true, true);

        WisdomUI::Theme::DrawSunsetButton(window, confirmBtnBounds, "Delete Forever", font, 15, false, confirmBtnBounds.contains(mPos), true, 1.0f);
        WisdomUI::Theme::DrawSunsetButton(window, cancelBtnBounds, "Keep Project", font, 15, false, cancelBtnBounds.contains(mPos), false, 1.0f);
    }
}