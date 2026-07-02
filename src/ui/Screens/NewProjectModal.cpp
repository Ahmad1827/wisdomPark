#include "NewProjectModal.h"

NewProjectModal::NewProjectModal() : isOpen(false) {}

void NewProjectModal::init() {
    font.loadFromFile("assets/font.otf");

    overlay.setSize(sf::Vector2f(1920.f, 1080.f));
    overlay.setFillColor(sf::Color(0, 0, 0, 200));

    modalBg.setSize(sf::Vector2f(800.f, 400.f));
    modalBg.setPosition(1920.f / 2.f - 400.f, 1080.f / 2.f - 200.f);
    modalBg.setFillColor(sf::Color(25, 25, 30, 255));
    modalBg.setOutlineThickness(2.f);
    modalBg.setOutlineColor(sf::Color(100, 100, 110, 100));

    title.setFont(font);
    title.setString("Choose Project Type");
    title.setCharacterSize(24);
    title.setFillColor(sf::Color::White);
    title.setPosition(modalBg.getPosition().x + 30.f, modalBg.getPosition().y + 30.f);

    closeBtn.setSize(sf::Vector2f(100.f, 40.f));
    closeBtn.setPosition(modalBg.getPosition().x + 670.f, modalBg.getPosition().y + 30.f);
    closeBtn.setFillColor(sf::Color(50, 50, 60));

    closeText.setFont(font);
    closeText.setString("Cancel");
    closeText.setCharacterSize(16);
    closeText.setFillColor(sf::Color::White);
    closeText.setPosition(closeBtn.getPosition().x + 25.f, closeBtn.getPosition().y + 10.f);

    normalBtn.setSize(sf::Vector2f(320.f, 200.f));
    normalBtn.setPosition(modalBg.getPosition().x + 50.f, modalBg.getPosition().y + 120.f);
    normalBtn.setFillColor(sf::Color(30, 30, 40));
    normalBtn.setOutlineThickness(2.f);
    normalBtn.setOutlineColor(sf::Color(100, 100, 120));

    normalText.setFont(font);
    normalText.setString("Normal Mode");
    normalText.setCharacterSize(22);
    normalText.setFillColor(sf::Color::White);
    normalText.setPosition(normalBtn.getPosition().x + 80.f, normalBtn.getPosition().y + 50.f);

    normalDesc.setFont(font);
    normalDesc.setString("1920x1080 Animation\nSmooth Brushes\nAnti-Aliasing");
    normalDesc.setCharacterSize(14);
    normalDesc.setFillColor(sf::Color(180, 180, 180));
    normalDesc.setPosition(normalBtn.getPosition().x + 80.f, normalBtn.getPosition().y + 100.f);

    pixelBtn.setSize(sf::Vector2f(320.f, 200.f));
    pixelBtn.setPosition(modalBg.getPosition().x + 430.f, modalBg.getPosition().y + 120.f);
    pixelBtn.setFillColor(sf::Color(30, 30, 40));
    pixelBtn.setOutlineThickness(2.f);
    pixelBtn.setOutlineColor(sf::Color(100, 100, 120));

    pixelText.setFont(font);
    pixelText.setString("Pixel Art Mode");
    pixelText.setCharacterSize(22);
    pixelText.setFillColor(sf::Color::White);
    pixelText.setPosition(pixelBtn.getPosition().x + 80.f, pixelBtn.getPosition().y + 50.f);

    pixelDesc.setFont(font);
    pixelDesc.setString("64x64 Sprite Sheet\nHard Edges\nSnap & Tile Tools");
    pixelDesc.setCharacterSize(14);
    pixelDesc.setFillColor(sf::Color(180, 180, 180));
    pixelDesc.setPosition(pixelBtn.getPosition().x + 80.f, pixelBtn.getPosition().y + 100.f);
}

void NewProjectModal::open() { isOpen = true; }
void NewProjectModal::close() { isOpen = false; }
bool NewProjectModal::getIsOpen() const { return isOpen; }

void NewProjectModal::updateHover(sf::Vector2f mousePos) {
    if (!isOpen) return;
    closeBtn.setFillColor(closeBtn.getGlobalBounds().contains(mousePos) ? sf::Color(80, 80, 90) : sf::Color(50, 50, 60));
    normalBtn.setFillColor(normalBtn.getGlobalBounds().contains(mousePos) ? sf::Color(50, 50, 70) : sf::Color(30, 30, 40));
    pixelBtn.setFillColor(pixelBtn.getGlobalBounds().contains(mousePos) ? sf::Color(50, 50, 70) : sf::Color(30, 30, 40));
}

std::string NewProjectModal::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    if (!isOpen || event.type != sf::Event::MouseButtonPressed || event.mouseButton.button != sf::Mouse::Left) return "";

    sf::Vector2f mousePos(static_cast<float>(sf::Mouse::getPosition(window).x), static_cast<float>(sf::Mouse::getPosition(window).y));
    if (closeBtn.getGlobalBounds().contains(mousePos)) { close(); return "cancel"; }
    if (normalBtn.getGlobalBounds().contains(mousePos)) { close(); return "create_normal"; }
    if (pixelBtn.getGlobalBounds().contains(mousePos)) { close(); return "create_pixel"; }
    return "";
}

void NewProjectModal::draw(sf::RenderWindow& window) {
    if (!isOpen) return;
    window.draw(overlay);
    window.draw(modalBg);
    window.draw(title);
    window.draw(closeBtn);
    window.draw(closeText);

    window.draw(normalBtn);
    window.draw(normalText);
    window.draw(normalDesc);

    window.draw(pixelBtn);
    window.draw(pixelText);
    window.draw(pixelDesc);
}