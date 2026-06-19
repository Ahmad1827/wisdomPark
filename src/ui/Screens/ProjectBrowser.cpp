#include "ProjectBrowser.h"

ProjectBrowser::ProjectBrowser() : projManager(nullptr) {}

void ProjectBrowser::init(ProjectManager* pm) {
    projManager = pm;
    font.loadFromFile("assets/font.otf");

    overlay.setSize(sf::Vector2f(1920.f, 1080.f));
    overlay.setFillColor(sf::Color(0, 0, 0, 200));

    modalBg.setSize(sf::Vector2f(1200.f, 800.f));
    modalBg.setPosition(1920.f / 2.f - 600.f, 1080.f / 2.f - 400.f);
    modalBg.setFillColor(sf::Color(20, 20, 24, 255));
    modalBg.setOutlineThickness(1.f);
    modalBg.setOutlineColor(sf::Color(100, 100, 110, 100));

    title.setFont(font);
    title.setString("Recent Projects");
    title.setCharacterSize(36);
    title.setFillColor(sf::Color::White);
    title.setPosition(modalBg.getPosition().x + 50.f, modalBg.getPosition().y + 40.f);

    newProjectBtn.setSize(sf::Vector2f(200.f, 50.f));
    newProjectBtn.setPosition(modalBg.getPosition().x + 950.f, modalBg.getPosition().y + 40.f);
    newProjectBtn.setFillColor(sf::Color(0, 122, 204));

    newProjectText.setFont(font);
    newProjectText.setString("+ New Project");
    newProjectText.setCharacterSize(20);
    newProjectText.setFillColor(sf::Color::White);
    newProjectText.setPosition(newProjectBtn.getPosition().x + 25.f, newProjectBtn.getPosition().y + 12.f);

    refresh();
}

void ProjectBrowser::refresh() {
    cards.clear();
    if (!projManager) return;

    auto metas = projManager->getRecentProjects();

    float startX = modalBg.getPosition().x + 50.f;
    float startY = modalBg.getPosition().y + 120.f;
    float x = startX;
    float y = startY;

    for (const auto& meta : metas) {
        ProjectCard card;
        card.meta = meta;
        card.background.setSize(sf::Vector2f(250.f, 220.f));
        card.background.setPosition(x, y);
        card.background.setFillColor(sf::Color(30, 30, 35));

        if (meta.thumbnail.getSize().x > 0) {
            card.thumbnail.setTexture(meta.thumbnail);
            float scale = 230.f / std::max(meta.thumbnail.getSize().x, meta.thumbnail.getSize().y);
            card.thumbnail.setScale(scale, scale);
            card.thumbnail.setPosition(x + 10.f, y + 10.f);
        }

        card.nameText.setFont(font);
        card.nameText.setString(meta.name);
        card.nameText.setCharacterSize(18);
        card.nameText.setFillColor(sf::Color::White);
        card.nameText.setPosition(x + 10.f, y + 170.f);

        card.detailText.setFont(font);
        card.detailText.setString(meta.lastModified + " | " + std::to_string(meta.frameCount) + " frames");
        card.detailText.setCharacterSize(12);
        card.detailText.setFillColor(sf::Color(150, 150, 150));
        card.detailText.setPosition(x + 10.f, y + 195.f);

        cards.push_back(card);

        x += 280.f;
        if (x > modalBg.getPosition().x + 1000.f) {
            x = startX;
            y += 250.f;
        }
    }
}

void ProjectBrowser::updateHover(sf::Vector2f mousePos) {
    for (auto& card : cards) {
        card.isHovered = card.background.getGlobalBounds().contains(mousePos);
        card.background.setFillColor(card.isHovered ? sf::Color(45, 45, 55) : sf::Color(30, 30, 35));
    }

    if (newProjectBtn.getGlobalBounds().contains(mousePos)) {
        newProjectBtn.setFillColor(sf::Color(0, 142, 224));
    }
    else {
        newProjectBtn.setFillColor(sf::Color(0, 122, 204));
    }
}

void ProjectBrowser::draw(sf::RenderWindow& window) {
    window.draw(overlay);
    window.draw(modalBg);
    window.draw(title);
    window.draw(newProjectBtn);
    window.draw(newProjectText);

    for (const auto& card : cards) {
        window.draw(card.background);
        if (card.meta.thumbnail.getSize().x > 0) window.draw(card.thumbnail);
        window.draw(card.nameText);
        window.draw(card.detailText);
    }
}

std::string ProjectBrowser::handleClick(sf::Vector2f mousePos, ProjectMetadata& outMeta) {
    if (newProjectBtn.getGlobalBounds().contains(mousePos)) {
        return "new_project";
    }

    for (const auto& card : cards) {
        if (card.background.getGlobalBounds().contains(mousePos)) {
            outMeta = card.meta;
            return "load_project";
        }
    }
    return "";
}