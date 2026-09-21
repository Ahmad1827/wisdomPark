#include "ProjectBrowser.h"
#include "../UITheme.h"
#include <algorithm>

ProjectBrowser::ProjectBrowser()
    : pm(nullptr), showDeleteConfirm(false), scrollOffset(0.0f), maxScroll(0.0f) {}

void ProjectBrowser::init(ProjectManager* projectManager) {
    pm = projectManager;
    font.loadFromFile("assets/font.otf");

    containerBounds = sf::FloatRect(160.f, 50.f, 1600.f, 980.f);
    backBtnBounds = sf::FloatRect(containerBounds.left + 32.f, containerBounds.top + 24.f, 150.f, 48.f);

    openFileBtnBounds = sf::FloatRect(containerBounds.left + containerBounds.width - 410.f, containerBounds.top + 24.f, 180.f, 48.f);
    newProjectBtnBounds = sf::FloatRect(containerBounds.left + containerBounds.width - 210.f, containerBounds.top + 24.f, 180.f, 48.f);

    deleteModalBounds = sf::FloatRect(1920.f / 2.f - 280.f, 1080.f / 2.f - 150.f, 560.f, 300.f);
    confirmBtnBounds = sf::FloatRect(deleteModalBounds.left + 36.f, deleteModalBounds.top + 200.f, 230.f, 56.f);
    cancelBtnBounds = sf::FloatRect(deleteModalBounds.left + 294.f, deleteModalBounds.top + 200.f, 230.f, 56.f);

    refreshList();
}

void ProjectBrowser::refreshList() {
    if (pm) projects = pm->getRecentProjects();
}

void ProjectBrowser::updateHover(sf::Vector2f mousePos) {}

void ProjectBrowser::handleScroll(float delta) {
    scrollOffset = std::clamp(scrollOffset - delta * 60.0f, 0.0f, maxScroll);
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

    WisdomUI::Theme::DrawSunsetButton(window, backBtnBounds, "< BACK", font, 18, false, backBtnBounds.contains(mPos), false, 1.0f);

    WisdomUI::Theme::DrawCrispText(window, font, "ARCHIVE VAULT", 38, containerBounds.left + 210.f, containerBounds.top + 24.f, WisdomUI::Theme::SunsetGold, sf::Color(14, 6, 20));

    WisdomUI::Theme::DrawSunsetButton(window, openFileBtnBounds, "Browse Disk", font, 18, false, openFileBtnBounds.contains(mPos), false, 1.0f);
    WisdomUI::Theme::DrawSunsetButton(window, newProjectBtnBounds, "+ New Canvas", font, 18, false, newProjectBtnBounds.contains(mPos), true, 1.0f);

    sf::RectangleShape divider(sf::Vector2f(containerBounds.width - 64.f, 2.f));
    divider.setPosition(containerBounds.left + 32.f, containerBounds.top + 84.f);
    divider.setFillColor(WisdomUI::Theme::SunsetPlum);
    window.draw(divider);

    cardBoundsList.clear();
    deleteBtnsList.clear();

    float startX = containerBounds.left + 36.f;
    float startY = containerBounds.top + 104.f - scrollOffset;
    float cardWidth = (containerBounds.width - 92.f) / 2.f;
    float cardHeight = 136.f;

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

        sf::FloatRect thumbRect(cx + 14.f, cy + 14.f, 156.f, 108.f);
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
            WisdomUI::Theme::DrawCrispText(window, font, "NO PREVIEW", 14, thumbRect.left + thumbRect.width / 2.f, thumbRect.top + thumbRect.height / 2.f, WisdomUI::Theme::TextSecondary, sf::Color::Transparent, true, true);
        }

        sf::Color titleColor = isHov ? WisdomUI::Theme::SunsetGold : WisdomUI::Theme::SunsetAmber;
        WisdomUI::Theme::DrawCrispText(window, font, projects[i].name, 22, cx + 186.f, cy + 18.f, titleColor, sf::Color(14, 6, 20));

        std::string modeBadge = projects[i].isPixelMode ? "PIXEL ART" : "RGBA CANV";
        WisdomUI::Theme::DrawCrispText(window, font, modeBadge, 16, cx + 186.f, cy + 54.f, WisdomUI::Theme::SunsetPeach);

        std::string resStr = std::to_string(projects[i].width) + " x " + std::to_string(projects[i].height) + " px";
        WisdomUI::Theme::DrawCrispText(window, font, resStr, 16, cx + 306.f, cy + 54.f, WisdomUI::Theme::TextPrimary);

        WisdomUI::Theme::DrawCrispText(window, font, "MODIFIED: " + projects[i].lastModified, 15, cx + 186.f, cy + 92.f, WisdomUI::Theme::TextSecondary);

        sf::FloatRect delRect(cx + cardWidth - 58.f, cy + 14.f, 44.f, 44.f);
        deleteBtnsList.push_back(delRect);
        bool delHov = delRect.contains(mPos);

        WisdomUI::Theme::DrawSunsetButton(window, delRect, "X", font, 18, false, delHov, true, 1.0f);
    }

    float totalContentH = (std::ceil(static_cast<float>(projects.size()) / 2.f) * (cardHeight + 18.f));
    maxScroll = std::max(0.0f, totalContentH - (containerBounds.height - 120.f));

    if (showDeleteConfirm) {
        sf::RectangleShape modalOverlay(sf::Vector2f(1920.f, 1080.f));
        modalOverlay.setFillColor(sf::Color(10, 4, 16, 225));
        window.draw(modalOverlay);

        WisdomUI::Theme::DrawSunsetPanel(window, deleteModalBounds, 1.0f);

        WisdomUI::Theme::DrawCrispText(window, font, "DELETE PROJECT", 24, deleteModalBounds.left + deleteModalBounds.width / 2.f, deleteModalBounds.top + 34.f, WisdomUI::Theme::SunsetCoral, sf::Color(14, 6, 20), true, true);
        WisdomUI::Theme::DrawCrispText(window, font, "Delete '" + projectToDelete + "'?", 18, deleteModalBounds.left + deleteModalBounds.width / 2.f, deleteModalBounds.top + 88.f, WisdomUI::Theme::TextPrimary, sf::Color(14, 6, 20), true, true);
        WisdomUI::Theme::DrawCrispText(window, font, "This will permanently remove the archive.", 15, deleteModalBounds.left + deleteModalBounds.width / 2.f, deleteModalBounds.top + 124.f, WisdomUI::Theme::TextSecondary, sf::Color(14, 6, 20), true, true);

        WisdomUI::Theme::DrawSunsetButton(window, confirmBtnBounds, "Delete Forever", font, 18, false, confirmBtnBounds.contains(mPos), true, 1.0f);
        WisdomUI::Theme::DrawSunsetButton(window, cancelBtnBounds, "Cancel", font, 18, false, cancelBtnBounds.contains(mPos), false, 1.0f);
    }
}