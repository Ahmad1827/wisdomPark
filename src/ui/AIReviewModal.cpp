#include "AIReviewModal.h"

AIReviewModal::AIReviewModal() : isOpen(false) {}

void AIReviewModal::init() {
    font.loadFromFile("assets/font.otf");

    overlay.setSize(sf::Vector2f(1920.f, 1080.f));
    overlay.setFillColor(sf::Color(0, 0, 0, 200));

    modalBg.setSize(sf::Vector2f(1400.f, 800.f));
    modalBg.setOrigin(700.f, 400.f);
    modalBg.setPosition(960.f, 540.f);
    modalBg.setFillColor(sf::Color(30, 30, 35));
    modalBg.setOutlineThickness(2.f);
    modalBg.setOutlineColor(sf::Color(100, 150, 255));

    titleText.setFont(font);
    titleText.setString("AI Generation Review");
    titleText.setCharacterSize(24);
    titleText.setFillColor(sf::Color::White);
    titleText.setPosition(290.f, 160.f);

    originalView.setSize(sf::Vector2f(600.f, 600.f));
    originalView.setPosition(310.f, 220.f);
    originalView.setFillColor(sf::Color(20, 20, 20));
    originalView.setOutlineThickness(1.f);

    resultView.setSize(sf::Vector2f(600.f, 600.f));
    resultView.setPosition(1010.f, 220.f);
    resultView.setFillColor(sf::Color(20, 20, 20));
    resultView.setOutlineThickness(1.f);

    acceptNewLayerBtn.setSize(sf::Vector2f(200.f, 50.f));
    acceptNewLayerBtn.setFillColor(sf::Color(50, 180, 50));
    acceptNewLayerBtn.setPosition(1010.f, 840.f);

    acceptNewLayerText.setFont(font);
    acceptNewLayerText.setString("Accept as New Layer");
    acceptNewLayerText.setCharacterSize(16);
    acceptNewLayerText.setFillColor(sf::Color::White);
    acceptNewLayerText.setPosition(1025.f, 855.f);

    replaceLayerBtn.setSize(sf::Vector2f(180.f, 50.f));
    replaceLayerBtn.setFillColor(sf::Color(180, 150, 50));
    replaceLayerBtn.setPosition(1230.f, 840.f);

    replaceLayerText.setFont(font);
    replaceLayerText.setString("Replace Layer");
    replaceLayerText.setCharacterSize(16);
    replaceLayerText.setFillColor(sf::Color::White);
    replaceLayerText.setPosition(1260.f, 855.f);

    newProjectBtn.setSize(sf::Vector2f(180.f, 50.f));
    newProjectBtn.setFillColor(sf::Color(50, 100, 200));
    newProjectBtn.setPosition(1430.f, 840.f);

    newProjectText.setFont(font);
    newProjectText.setString("New Project");
    newProjectText.setCharacterSize(16);
    newProjectText.setFillColor(sf::Color::White);
    newProjectText.setPosition(1470.f, 855.f);

    rejectBtn.setSize(sf::Vector2f(150.f, 50.f));
    rejectBtn.setFillColor(sf::Color(180, 50, 50));
    rejectBtn.setPosition(310.f, 840.f);

    rejectText.setFont(font);
    rejectText.setString("Reject");
    rejectText.setCharacterSize(16);
    rejectText.setFillColor(sf::Color::White);
    rejectText.setPosition(355.f, 855.f);
}

void AIReviewModal::open(const sf::Image& originalImg, const sf::Image& resultImg) {
    isOpen = true;
    originalSavedImage = originalImg;
    resultSavedImage = resultImg;

    if (originalImg.getSize().x > 0 && originalImg.getSize().y > 0) {
        originalTexture.loadFromImage(originalImg);
        originalView.setTexture(&originalTexture);
    }
    if (resultImg.getSize().x > 0 && resultImg.getSize().y > 0) {
        resultTexture.loadFromImage(resultImg);
        resultView.setTexture(&resultTexture);
    }
}

void AIReviewModal::close() {
    isOpen = false;
    originalView.setTexture(nullptr);
    resultView.setTexture(nullptr);
}

bool AIReviewModal::getIsOpen() const {
    return isOpen;
}

const sf::Image& AIReviewModal::getResultImage() const {
    return resultSavedImage;
}

void AIReviewModal::draw(sf::RenderWindow& window) {
    if (!isOpen) return;
    window.draw(overlay);
    window.draw(modalBg);
    window.draw(titleText);
    window.draw(originalView);
    window.draw(resultView);
    window.draw(acceptNewLayerBtn);
    window.draw(acceptNewLayerText);
    window.draw(replaceLayerBtn);
    window.draw(replaceLayerText);
    window.draw(newProjectBtn);
    window.draw(newProjectText);
    window.draw(rejectBtn);
    window.draw(rejectText);
}

std::string AIReviewModal::handleEvent(const sf::Event& event, sf::Vector2f mousePos) {
    if (!isOpen) return "";
    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (acceptNewLayerBtn.getGlobalBounds().contains(mousePos)) {
            close();
            return "accept_new";
        }
        if (replaceLayerBtn.getGlobalBounds().contains(mousePos)) {
            close();
            return "accept_replace";
        }
        if (newProjectBtn.getGlobalBounds().contains(mousePos)) {
            close();
            return "accept_project";
        }
        if (rejectBtn.getGlobalBounds().contains(mousePos)) {
            close();
            return "reject";
        }
    }
    return "consumed";
}