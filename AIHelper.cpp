#include "AIHelper.h"
#include <iostream>
#include <algorithm>
#include <iomanip>

AIHelper::AIHelper() : active(false) {
    mascot.setRadius(40);
    mascot.setPosition(1800, 950);
    mascot.setFillColor(sf::Color(0, 191, 255));
    mascot.setOutlineThickness(2);
    mascot.setOutlineColor(sf::Color::Transparent);
}

void AIHelper::toggle() {
    active = !active;
    if (active) {
        mascot.setFillColor(sf::Color(255, 215, 0));
        mascot.setOutlineColor(sf::Color::Black);
    }
    else {
        mascot.setFillColor(sf::Color(0, 191, 255));
        mascot.setOutlineColor(sf::Color::Transparent);
    }
}

bool AIHelper::isActive() const {
    return active;
}

sf::FloatRect AIHelper::getBounds() const {
    return mascot.getGlobalBounds();
}

void AIHelper::draw(sf::RenderWindow& window) {
    window.draw(mascot);
}

std::string AIHelper::determineMood(int r, int b, int g, int blk, int total) const {
    if (total == 0) return "Inexistent";

    int maxColor = std::max({ r, b, g, blk });

    if (maxColor == 0) return "Abstract / Transparent";
    if (maxColor == r) return "Pasionat / Agresiv (Dominanta Rosie)";
    if (maxColor == b) return "Calm / Melancolic (Dominanta Albastra)";
    if (maxColor == g) return "Natural / Echilibrat (Dominanta Verde)";

    return "Clasic / Intunecat (Dominanta Neagra)";
}

void AIHelper::analyzeFrame(const sf::RenderTexture& frame) {
    sf::Image img = frame.getTexture().copyToImage();
    int width = img.getSize().x;
    int height = 930;

    int totalArea = width * height;
    int totalDrawn = 0;
    int red = 0, blue = 0, green = 0, black = 0;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            sf::Color c = img.getPixel(x, y);
            if (c.a > 0 && c != sf::Color::White) {
                totalDrawn++;
                if (c == sf::Color::Red) red++;
                else if (c == sf::Color::Blue) blue++;
                else if (c == sf::Color::Green) green++;
                else if (c == sf::Color::Black) black++;
            }
        }
    }

    float coverage = (static_cast<float>(totalDrawn) / totalArea) * 100.0f;
    std::string mood = determineMood(red, blue, green, black, totalDrawn);

    std::cout << "\n=====================================" << std::endl;
    std::cout << "      WISDOM PARK AI CRITIC          " << std::endl;
    std::cout << "=====================================" << std::endl;

    if (totalDrawn == 0) {
        std::cout << "Panza este complet goala." << std::endl;
        std::cout << "Sugestie: Alege o unealta din parc si incepe!" << std::endl;
    }
    else {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Acoperire panza: " << coverage << "%" << std::endl;
        std::cout << "Pixeli desenati: " << totalDrawn << std::endl;
        std::cout << "Stil artistic:   " << mood << std::endl;
        std::cout << "-------------------------------------" << std::endl;
        std::cout << "Distributia culorilor:" << std::endl;
        std::cout << " - Rosu:  " << (static_cast<float>(red) / totalDrawn) * 100.0f << "%" << std::endl;
        std::cout << " - Albastru: " << (static_cast<float>(blue) / totalDrawn) * 100.0f << "%" << std::endl;
        std::cout << " - Verde: " << (static_cast<float>(green) / totalDrawn) * 100.0f << "%" << std::endl;
        std::cout << " - Negru: " << (static_cast<float>(black) / totalDrawn) * 100.0f << "%" << std::endl;
        std::cout << "-------------------------------------" << std::endl;

        if (coverage < 5.0f) {
            std::cout << "Verdict: O abordare minimalista si delicata." << std::endl;
        }
        else if (coverage < 30.0f) {
            std::cout << "Verdict: Compozitie echilibrata cu spatiu negativ bun." << std::endl;
        }
        else {
            std::cout << "Verdict: O lucrare densa si foarte complexa!" << std::endl;
        }
    }
    std::cout << "=====================================\n" << std::endl;
}