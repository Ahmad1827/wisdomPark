#include "ParkSystem.h"
#include <filesystem>
#include <algorithm>

ParkSystem::ParkSystem()
    : m_boulevardY(940.0f),
    m_inspiration(100),
    m_incomeTimer(0.0f),
    m_selectedBuildingIndex(-1) {
}

bool ParkSystem::loadTexture(const std::string& key, const std::string& filename) {
    sf::Texture tex;
    std::string path = "assets/park/" + filename;
    if (!std::filesystem::exists(path)) {
        return false;
    }
    if (tex.loadFromFile(path)) {
        m_textures[key] = std::move(tex);
        return true;
    }
    return false;
}

void ParkSystem::init() {
    loadTexture("skyline", "skyline_back.png");
    loadTexture("boulevard", "boulevard.png");
    loadTexture("arcade", "building_arcade.png");
    loadTexture("booth", "building_booth.png");

    if (m_textures.find("skyline") != m_textures.end()) {
        m_skylineSprite.setTexture(m_textures["skyline"]);
        m_skylineSprite.setPosition(0.0f, m_boulevardY - 320.0f);
    }

    if (m_textures.find("boulevard") != m_textures.end()) {
        m_boulevardSprite.setTexture(m_textures["boulevard"]);
        m_boulevardSprite.setPosition(0.0f, m_boulevardY);
    }

    if (m_textures.find("visitor") != m_textures.end() || loadTexture("visitor", "visitor_sheet.png")) {
        m_visitorTexture = m_textures["visitor"];
    }

    addBuilding("arcade", sf::Vector2f(260.0f, m_boulevardY));
    addBuilding("booth", sf::Vector2f(600.0f, m_boulevardY));

    m_visitors.push_back({ sf::Vector2f(100.0f, m_boulevardY + 30.0f), 50.0f, 1, 0, 0.0f });
    m_visitors.push_back({ sf::Vector2f(800.0f, m_boulevardY + 45.0f), 40.0f, -1, 0, 0.0f });
    m_visitors.push_back({ sf::Vector2f(1300.0f, m_boulevardY + 25.0f), 55.0f, 1, 0, 0.0f });
}

void ParkSystem::addBuilding(const std::string& type, sf::Vector2f pos) {
    ParkBuilding b;
    b.id = static_cast<uint32_t>(m_buildings.size() + 1);
    b.type = type;
    b.level = 1;
    b.cost = (type == "arcade") ? 150 : 60;
    b.incomeRate = (type == "arcade") ? 5 : 2;

    if (m_textures.find(type) != m_textures.end()) {
        b.sprite.setTexture(m_textures[type]);
        sf::FloatRect bounds = b.sprite.getLocalBounds();
        b.sprite.setPosition(pos.x, pos.y - bounds.height);
        b.bounds = b.sprite.getGlobalBounds();
    }
    else {
        sf::Vector2f size = (type == "arcade") ? sf::Vector2f(160.0f, 140.0f) : sf::Vector2f(90.0f, 90.0f);
        b.bounds = sf::FloatRect(pos.x, pos.y - size.y, size.x, size.y);
    }

    m_buildings.push_back(b);
}

void ParkSystem::addInspiration(int amount) {
    m_inspiration += amount;
}

int ParkSystem::getInspiration() const {
    return m_inspiration;
}

void ParkSystem::upgradeSelectedBuilding() {
    if (m_selectedBuildingIndex < 0 || m_selectedBuildingIndex >= static_cast<int>(m_buildings.size())) {
        return;
    }
    auto& b = m_buildings[m_selectedBuildingIndex];
    int upgradeCost = b.cost * b.level;
    if (m_inspiration >= upgradeCost) {
        m_inspiration -= upgradeCost;
        b.level++;
        b.incomeRate = static_cast<int>(b.incomeRate * 1.6f);
    }
}

void ParkSystem::update(float dt) {
    m_incomeTimer += dt;
    if (m_incomeTimer >= 1.0f) {
        m_incomeTimer -= 1.0f;
        for (const auto& b : m_buildings) {
            m_inspiration += b.incomeRate;
        }
    }

    for (auto& v : m_visitors) {
        v.position.x += v.speed * static_cast<float>(v.direction) * dt;

        if (v.position.x > 1920.0f + 40.0f) {
            v.position.x = -40.0f;
        }
        else if (v.position.x < -40.0f) {
            v.position.x = 1920.0f + 40.0f;
        }

        v.animTimer += dt;
        if (v.animTimer >= 0.15f) {
            v.animTimer = 0.0f;
            v.animFrame = (v.animFrame + 1) % 4;
        }
    }
}

bool ParkSystem::handleEvent(const sf::Event& event, const sf::Vector2f& worldPos) {
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        for (size_t i = 0; i < m_buildings.size(); ++i) {
            if (m_buildings[i].bounds.contains(worldPos)) {
                m_selectedBuildingIndex = static_cast<int>(i);
                return true;
            }
        }
        m_selectedBuildingIndex = -1;
    }
    return false;
}

void ParkSystem::draw(sf::RenderTarget& target) {
    if (m_skylineSprite.getTexture()) {
        target.draw(m_skylineSprite);
    }
    else {
        sf::RectangleShape sky(sf::Vector2f(1920.0f, m_boulevardY));
        sky.setFillColor(sf::Color(14, 10, 20));
        target.draw(sky);
    }

    if (m_boulevardSprite.getTexture()) {
        target.draw(m_boulevardSprite);
    }
    else {
        sf::RectangleShape road(sf::Vector2f(1920.0f, 1080.0f - m_boulevardY));
        road.setPosition(0.0f, m_boulevardY);
        road.setFillColor(sf::Color(28, 20, 36));
        target.draw(road);
    }

    for (size_t i = 0; i < m_buildings.size(); ++i) {
        const auto& b = m_buildings[i];
        if (b.sprite.getTexture()) {
            target.draw(b.sprite);
        }
        else {
            sf::RectangleShape box(sf::Vector2f(b.bounds.width, b.bounds.height));
            box.setPosition(b.bounds.left, b.bounds.top);
            box.setFillColor(sf::Color(75, 45, 90));
            box.setOutlineThickness(1.0f);
            box.setOutlineColor(sf::Color(170, 80, 120));
            target.draw(box);
        }

        if (static_cast<int>(i) == m_selectedBuildingIndex) {
            sf::RectangleShape sel(sf::Vector2f(b.bounds.width + 6.0f, b.bounds.height + 6.0f));
            sel.setPosition(b.bounds.left - 3.0f, b.bounds.top - 3.0f);
            sel.setFillColor(sf::Color::Transparent);
            sel.setOutlineThickness(2.0f);
            sel.setOutlineColor(sf::Color(255, 195, 0));
            target.draw(sel);
        }
    }

    bool hasVisitorTex = (m_textures.find("visitor") != m_textures.end());
    for (const auto& v : m_visitors) {
        if (hasVisitorTex) {
            sf::Sprite spr(m_textures["visitor"]);
            spr.setTextureRect(sf::IntRect(v.animFrame * 16, 0, 16, 24));
            spr.setOrigin(8.0f, 24.0f);
            spr.setScale((v.direction < 0) ? -1.0f : 1.0f, 1.0f);
            spr.setPosition(v.position);
            target.draw(spr);
        }
        else {
            sf::RectangleShape body(sf::Vector2f(10.0f, 20.0f));
            body.setPosition(v.position.x, v.position.y - 20.0f);
            body.setFillColor(sf::Color(240, 140, 80));
            target.draw(body);
        }
    }
}