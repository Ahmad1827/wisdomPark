#include "ProjectBrowser.h"
#include "../UITheme.h"
#include "MenuLayouts.h"
#include "MenuWidgets.h"
#include <algorithm>
#include <cmath>

using WisdomUI::Theme;
using WisdomUI::Animation;
using namespace MenuWidgets;

namespace {

    const int kColumns = 4;
    const float kCardGap = 20.0f;
    const float kCardHeight = 340.0f;

    float cardWidth() {
        return (MenuLayout::kContentWidth - kCardGap * static_cast<float>(kColumns - 1)) / static_cast<float>(kColumns);
    }

    // Padded so card glows and shadows are not cut off by the clip.
    sf::FloatRect listArea() {
        return sf::FloatRect(MenuLayout::kContentLeft - 8.0f, MenuLayout::kContentTop - 8.0f,
            MenuLayout::kContentWidth + 16.0f, MenuLayout::kContentBottom - MenuLayout::kContentTop + 16.0f);
    }

    sf::FloatRect cardBounds(size_t index, float scroll) {
        float col = static_cast<float>(index % kColumns);
        float row = static_cast<float>(index / kColumns);
        return sf::FloatRect(MenuLayout::kContentLeft + col * (cardWidth() + kCardGap),
            MenuLayout::kContentTop + row * (kCardHeight + kCardGap) - scroll, cardWidth(), kCardHeight);
    }

    sf::FloatRect deleteButton(const sf::FloatRect& card) {
        return sf::FloatRect(card.left + card.width - 50.0f, card.top + 14.0f, 36.0f, 36.0f);
    }

}

ProjectBrowser::ProjectBrowser()
    : pm(nullptr), showDeleteConfirm(false), scrollOffset(0.0f), maxScroll(0.0f) {}

void ProjectBrowser::init(ProjectManager* projectManager) {
    pm = projectManager;
    font.loadFromFile("assets/Jersey10-Regular.ttf");

    const float right = MenuLayout::kContentLeft + MenuLayout::kContentWidth;
    backBtnBounds = MenuLayout::BackButton();
    newProjectBtnBounds = sf::FloatRect(right - 210.0f, 54.0f, 210.0f, 46.0f);
    openFileBtnBounds = sf::FloatRect(right - 210.0f - 14.0f - 190.0f, 54.0f, 190.0f, 46.0f);

    deleteModalBounds = sf::FloatRect(1920.f / 2.f - 280.f, 1080.f / 2.f - 150.f, 560.f, 300.f);
    confirmBtnBounds = sf::FloatRect(deleteModalBounds.left + 36.f, deleteModalBounds.top + 200.f, 230.f, 56.f);
    cancelBtnBounds = sf::FloatRect(deleteModalBounds.left + 294.f, deleteModalBounds.top + 200.f, 230.f, 56.f);

    refreshList();
}

void ProjectBrowser::refreshList() {
    if (pm) projects = pm->getRecentProjects();
    for (auto& project : projects) {
        project.thumbnail.setSmooth(!project.isPixelMode);
    }
    scrollOffset = 0.0f;
}

void ProjectBrowser::updateHover(sf::Vector2f mousePos) {}

void ProjectBrowser::handleScroll(float delta) {
    scrollOffset = std::clamp(scrollOffset - delta * 70.0f, 0.0f, maxScroll);
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

    // Cards scrolled out of the list area are not clickable.
    if (!listArea().contains(mousePos)) return "";

    for (size_t i = 0; i < projects.size(); ++i) {
        sf::FloatRect card = cardBounds(i, scrollOffset);
        if (!card.contains(mousePos)) continue;

        if (deleteButton(card).contains(mousePos)) {
            projectToDelete = projects[i].name;
            showDeleteConfirm = true;
            return "";
        }
        outMeta = projects[i];
        return "load_project";
    }

    return "";
}

void ProjectBrowser::draw(sf::RenderWindow& window, float time) {
    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    bool modalOpen = showDeleteConfirm;

    Theme::DrawSunsetButton(window, openFileBtnBounds, "BROWSE DISK", font, 15, false, !modalOpen && openFileBtnBounds.contains(mPos), false, 1.0f);

    // New Project is the main action here, so it gets the filled amber treatment.
    {
        const sf::FloatRect& b = newProjectBtnBounds;
        float t = Theme::AnimateHover(b, !modalOpen && b.contains(mPos));
        if (t > 0.01f) {
            sf::ConvexShape glow = Theme::ChamferedRect(b.left - 3.0f, b.top - 3.0f, b.width + 6.0f, b.height + 6.0f, 6.0f);
            glow.setFillColor(Theme::WithAlpha(Theme::SunsetGold, 60.0f * t));
            window.draw(glow);
        }
        sf::ConvexShape body = Theme::ChamferedRect(b.left, b.top, b.width, b.height, 4.0f);
        body.setFillColor(Theme::Mix(Theme::SunsetAmber, Theme::SunsetGold, t));
        body.setOutlineThickness(1.0f);
        body.setOutlineColor(Theme::SunsetGlow);
        window.draw(body);
        Theme::DrawCrispText(window, font, "+  NEW PROJECT", 15, b.left + b.width * 0.5f, b.top + b.height * 0.5f, Theme::SunsetDeepDark, sf::Color::Transparent, true, true);
    }

    sf::FloatRect list = listArea();

    if (projects.empty()) {
        float a = Animation::EaseOutCubic((time - 0.1f) / 0.4f);
        float cx = MenuLayout::kContentLeft + MenuLayout::kContentWidth * 0.5f;
        Theme::DrawCrispText(window, font, "NO PROJECTS YET", 30, cx, 520.0f, faded(Theme::SunsetAmber, a), faded(kTextShadow, a), true, true);
        Theme::DrawCrispText(window, font, "Create one with New Project, or open a .wpk file with Browse Disk.", 17, cx, 570.0f, faded(Theme::TextSecondary, a), sf::Color::Transparent, true, true);
        maxScroll = 0.0f;
    }
    else {
        float rows = std::ceil(static_cast<float>(projects.size()) / static_cast<float>(kColumns));
        float contentH = rows * (kCardHeight + kCardGap) - kCardGap + 16.0f;
        maxScroll = std::max(0.0f, contentH - list.height);
        scrollOffset = std::clamp(scrollOffset, 0.0f, maxScroll);

        bool mouseInList = !modalOpen && list.contains(mPos);
        sf::View savedView = window.getView();
        window.setView(clipView(window, list));

        for (size_t i = 0; i < projects.size(); ++i) {
            sf::FloatRect bounds = cardBounds(i, scrollOffset);
            if (bounds.top + bounds.height < list.top || bounds.top > list.top + list.height) continue;

            float a = Animation::EaseOutCubic((time - 0.06f - 0.04f * static_cast<float>(std::min<size_t>(i, 12))) / 0.4f);
            if (a <= 0.0f) continue;

            const ProjectMetadata& project = projects[i];
            sf::FloatRect delBounds = deleteButton(bounds);
            bool overDelete = mouseInList && delBounds.contains(mPos);
            float t = Theme::AnimateHover(bounds, mouseInList && bounds.contains(mPos));

            sf::FloatRect card = shifted(bounds, 0.0f, 18.0f * (1.0f - a) - 4.0f * t);
            drawCard(window, card, t, a);

            // Thumbnail
            sf::FloatRect thumbArea(card.left + 12.0f, card.top + 12.0f, card.width - 24.0f, 214.0f);
            drawCheckerboard(window, thumbArea, 16.0f, a);

            sf::Vector2u texSize = project.thumbnail.getSize();
            if (texSize.x > 0 && texSize.y > 0) {
                sf::Sprite thumb(project.thumbnail);
                float scale = std::min(thumbArea.width / static_cast<float>(texSize.x), thumbArea.height / static_cast<float>(texSize.y));
                thumb.setScale(scale, scale);
                thumb.setPosition(std::floor(thumbArea.left + (thumbArea.width - texSize.x * scale) * 0.5f),
                    std::floor(thumbArea.top + (thumbArea.height - texSize.y * scale) * 0.5f));
                thumb.setColor(faded(sf::Color::White, a));
                window.draw(thumb);
            }
            else {
                Theme::DrawCrispText(window, font, "NO PREVIEW", 14, thumbArea.left + thumbArea.width * 0.5f, thumbArea.top + thumbArea.height * 0.5f, faded(sf::Color(110, 96, 124), a), sf::Color::Transparent, true, true);
            }

            sf::RectangleShape thumbFrame(sf::Vector2f(thumbArea.width, thumbArea.height));
            thumbFrame.setPosition(thumbArea.left, thumbArea.top);
            thumbFrame.setFillColor(sf::Color::Transparent);
            thumbFrame.setOutlineThickness(1.0f);
            thumbFrame.setOutlineColor(faded(Theme::Mix(Theme::SunsetPlum, Theme::SunsetGold, t), a));
            window.draw(thumbFrame);

            // Details
            std::string name = project.name;
            std::replace(name.begin(), name.end(), '_', ' ');
            if (name.length() > 24) name = name.substr(0, 22) + "..";
            Theme::DrawCrispText(window, font, name, 20, card.left + 16.0f, card.top + 254.0f, faded(Theme::Mix(Theme::TextPrimary, Theme::SunsetGold, t), a), faded(kTextShadow, a), false, true);

            const char* kind = project.isPixelMode ? "PIXEL ART" : "ILLUSTRATION";
            Theme::DrawCrispText(window, font, kind, 14, card.left + 16.0f, card.top + 286.0f, faded(project.isPixelMode ? Theme::SunsetAmber : Theme::SunsetPeach, a), sf::Color::Transparent, false, true);

            std::string size = std::to_string(project.width) + " x " + std::to_string(project.height);
            float kindW = Theme::MeasureText(font, kind, 14);
            Theme::DrawCrispText(window, font, size, 14, card.left + 16.0f + kindW + 18.0f, card.top + 286.0f, faded(Theme::TextSecondary, a), sf::Color::Transparent, false, true);

            Theme::DrawCrispText(window, font, project.lastModified, 13, card.left + 16.0f, card.top + 314.0f, faded(Theme::TextMuted, a), sf::Color::Transparent, false, true);

            // Delete stays quiet until the card is hovered.
            if (a > 0.6f && (t > 0.05f || overDelete)) {
                sf::FloatRect del = shifted(delBounds, 0.0f, card.top - bounds.top);
                Theme::DrawSunsetButton(window, del, "X", font, 15, false, overDelete, true, 1.0f);
            }
        }

        window.setView(savedView);

        drawScrollbar(window, sf::FloatRect(MenuLayout::kContentLeft + MenuLayout::kContentWidth + 10.0f, MenuLayout::kContentTop, 6.0f, MenuLayout::kContentBottom - MenuLayout::kContentTop),
            scrollOffset, maxScroll, list.height / contentH);
    }

    if (showDeleteConfirm) {
        sf::RectangleShape modalOverlay(sf::Vector2f(1920.f, 1080.f));
        modalOverlay.setFillColor(sf::Color(10, 4, 16, 215));
        window.draw(modalOverlay);

        Theme::DrawSunsetPanel(window, deleteModalBounds, 1.0f);

        float cx = deleteModalBounds.left + deleteModalBounds.width / 2.f;
        Theme::DrawCrispText(window, font, "DELETE PROJECT", 26, cx, deleteModalBounds.top + 44.f, Theme::SunsetCoral, kTextShadow, true, true);
        Theme::DrawCrispText(window, font, "Delete '" + projectToDelete + "'?", 18, cx, deleteModalBounds.top + 98.f, Theme::TextPrimary, kTextShadow, true, true);
        Theme::DrawCrispText(window, font, "This permanently removes the project from disk.", 15, cx, deleteModalBounds.top + 136.f, Theme::TextSecondary, kTextShadow, true, true);

        Theme::DrawSunsetButton(window, confirmBtnBounds, "Delete Forever", font, 17, false, confirmBtnBounds.contains(mPos), true, 1.0f);
        Theme::DrawSunsetButton(window, cancelBtnBounds, "Cancel", font, 17, false, cancelBtnBounds.contains(mPos), false, 1.0f);
    }
}
