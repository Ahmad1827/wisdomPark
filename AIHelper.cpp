#include "AIHelper.h"
#include <algorithm>
#include <cmath>
#include <iostream>

AIHelper::AIHelper() : active(false), isGenerating(false), currentDrawIndex(0), isTrained(false) {
    mascot.setRadius(40);
    mascot.setPosition(1800, 950);
    mascot.setFillColor(sf::Color(0, 191, 255));
    mascot.setOutlineThickness(2);
    mascot.setOutlineColor(sf::Color::Transparent);
    grid.resize(width * height, 0);
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

bool AIHelper::isActive() const { return active; }
sf::FloatRect AIHelper::getBounds() const { return mascot.getGlobalBounds(); }
void AIHelper::draw(sf::RenderWindow& window) { window.draw(mascot); }
void AIHelper::clearGrid() { std::fill(grid.begin(), grid.end(), 0); }

void AIHelper::trainOnDataset(const std::string& filename) {
    datasetTemplates.clear();
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cout << "FAILED: Could not find " << filename << "\n";
        isTrained = false;
        return;
    }

    std::vector<std::string> currentTemplate;
    std::string line;

    while (std::getline(file, line)) {
        if (line.empty() || line.find_first_not_of("\r\n\t ") == std::string::npos) {
            if (!currentTemplate.empty()) {
                if (currentTemplate.size() == height) {
                    datasetTemplates.push_back(currentTemplate);
                }
                currentTemplate.clear();
            }
            continue;
        }
        currentTemplate.push_back(line);
    }

    if (!currentTemplate.empty() && currentTemplate.size() == height) {
        datasetTemplates.push_back(currentTemplate);
    }

    if (!datasetTemplates.empty()) {
        isTrained = true;
        std::cout << "SUCCESS: Loaded " << datasetTemplates.size() << " separate templates from " << filename << "\n";
    }
    else {
        isTrained = false;
        std::cout << "FAILED: File found, but no valid templates inside.\n";
    }
}

std::vector<std::string> AIHelper::generateDynamicBlueprint(std::mt19937& rng) {
    std::string emptyRow(width, '.');
    std::vector<std::string> blueprint(height, emptyRow);
    int center = (width / 2) - 1;
    int objectType = std::uniform_int_distribution<int>(0, 2)(rng);

    if (objectType == 0) { // Sword
        int bladeLen = std::uniform_int_distribution<int>(15, 28)(rng);
        int bladeW = std::uniform_int_distribution<int>(1, 4)(rng);
        int guardW = std::uniform_int_distribution<int>(4, 10)(rng);
        int handleLen = std::uniform_int_distribution<int>(5, 8)(rng);
        int startY = 4;
        for (int y = startY; y < startY + bladeLen; ++y) {
            for (int x = center - bladeW + 1; x <= center; ++x) {
                blueprint[y][x] = (std::uniform_int_distribution<int>(0, 100)(rng) > 10) ? ((x == center) ? 'X' : 'M') : '?';
            }
        }
        int guardY = startY + bladeLen;
        for (int y = guardY; y < guardY + 2; ++y) {
            for (int x = center - guardW + 1; x <= center; ++x) blueprint[y][x] = 'M';
        }
        int handleY = guardY + 2;
        for (int y = handleY; y < handleY + handleLen; ++y) {
            blueprint[y][center] = 'X'; blueprint[y][center - 1] = 'X';
        }
    }
    else if (objectType == 1) { // Tree
        int trunkH = std::uniform_int_distribution<int>(10, 20)(rng);
        int trunkW = std::uniform_int_distribution<int>(1, 3)(rng);
        int startY = height - 4 - trunkH;
        for (int y = startY; y < startY + trunkH; ++y) {
            for (int x = center - trunkW + 1; x <= center; ++x) blueprint[y][x] = 'M';
        }
        int leavesR = std::uniform_int_distribution<int>(8, 16)(rng);
        int leavesCY = startY;
        for (int y = leavesCY - leavesR; y <= leavesCY + leavesR; ++y) {
            for (int x = center - leavesR; x <= center; ++x) {
                if (y >= 0 && y < height && x >= 0) {
                    float dist = std::sqrt(std::pow(center - x, 2) + std::pow(leavesCY - y, 2));
                    if (dist <= leavesR) blueprint[y][x] = (std::uniform_int_distribution<int>(0, 100)(rng) > 25) ? 'M' : '?';
                }
            }
        }
    }
    else { // Potion
        int baseW = std::uniform_int_distribution<int>(6, 12)(rng);
        int baseH = std::uniform_int_distribution<int>(8, 16)(rng);
        int neckW = std::uniform_int_distribution<int>(2, 5)(rng);
        int neckH = std::uniform_int_distribution<int>(4, 8)(rng);
        int startY = height - 6 - baseH;
        for (int y = startY; y < startY + baseH; ++y) {
            for (int x = center - baseW + 1; x <= center; ++x) blueprint[y][x] = (std::uniform_int_distribution<int>(0, 100)(rng) > 15) ? 'M' : '?';
        }
        int neckY = startY - neckH;
        for (int y = neckY; y < startY; ++y) {
            for (int x = center - neckW + 1; x <= center; ++x) blueprint[y][x] = 'M';
        }
    }
    return blueprint;
}

void AIHelper::generateFromTemplate(std::mt19937& rng, const std::vector<std::string>& blueprint) {
    std::uniform_real_distribution<float> prob(0.0f, 1.0f);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            char cell = blueprint[y][x];
            if (cell == 'X') { grid[y * width + x] = 1; grid[y * width + (width - 1 - x)] = 1; }
            else if (cell == '?') { if (prob(rng) > 0.4f) grid[y * width + x] = 1; }
            else if (cell == 'M') {
                if (x < width / 2 && prob(rng) > 0.15f) {
                    grid[y * width + x] = 1; grid[y * width + (width - 1 - x)] = 1;
                }
            }
        }
    }
}

void AIHelper::applyShading() {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (grid[y * width + x] == 1) {
                if (y > 0 && grid[(y - 1) * width + x] == 0) grid[y * width + x] = 2;
                else if (y < height - 1 && grid[(y + 1) * width + x] == 0) grid[y * width + x] = 3;
            }
        }
    }
}

void AIHelper::applyOutline() {
    std::vector<int> tempGrid = grid;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (grid[y * width + x] > 0) {
                if (y > 0 && tempGrid[(y - 1) * width + x] == 0) tempGrid[(y - 1) * width + x] = 4;
                if (y < height - 1 && tempGrid[(y + 1) * width + x] == 0) tempGrid[(y + 1) * width + x] = 4;
                if (x > 0 && tempGrid[y * width + (x - 1)] == 0) tempGrid[y * width + (x - 1)] = 4;
                if (x < width - 1 && tempGrid[y * width + (x + 1)] == 0) tempGrid[y * width + (x + 1)] = 4;
            }
        }
    }
    grid = tempGrid;
}

void AIHelper::startGeneratingComplexArt(sf::FloatRect bounds) {
    isGenerating = true;
    currentDrawIndex = 0;
    currentBounds = bounds;
    std::random_device rd;
    std::mt19937 rng(rd());

    std::vector<std::vector<sf::Color>> thematicPalettes = {
        {sf::Color(77, 51, 25), sf::Color(115, 77, 38), sf::Color(38, 26, 13)},
        {sf::Color(140, 146, 153), sf::Color(190, 196, 204), sf::Color(90, 95, 102)},
        {sf::Color(34, 139, 34), sf::Color(50, 205, 50), sf::Color(0, 100, 0)},
        {sf::Color(178, 34, 34), sf::Color(220, 20, 60), sf::Color(139, 0, 0)},
        {sf::Color(65, 105, 225), sf::Color(100, 149, 237), sf::Color(0, 0, 139)}
    };

    std::uniform_int_distribution<int> paletteDist(0, thematicPalettes.size() - 1);
    int selectedPalette = paletteDist(rng);

    baseColor = thematicPalettes[selectedPalette][0];
    lightColor = thematicPalettes[selectedPalette][1];
    darkColor = thematicPalettes[selectedPalette][2];

    clearGrid();

    if (isTrained) {
        std::uniform_int_distribution<int> templateDist(0, datasetTemplates.size() - 1);
        const auto& selectedTemplate = datasetTemplates[templateDist(rng)];

        std::uniform_real_distribution<float> prob(0.0f, 1.0f);

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width / 2; ++x) {
                char cell = selectedTemplate[y][x];
                bool shouldDraw = false;

                if (cell == 'X' || cell == 'M') {
                    if (prob(rng) < 0.92f) shouldDraw = true;
                }
                else {
                    int neighbors = 0;
                    if (x > 0 && selectedTemplate[y][x - 1] == 'X') neighbors++;
                    if (x < width - 1 && selectedTemplate[y][x + 1] == 'X') neighbors++;
                    if (y > 0 && selectedTemplate[y - 1][x] == 'X') neighbors++;
                    if (y < height - 1 && selectedTemplate[y + 1][x] == 'X') neighbors++;

                    if (neighbors > 0 && prob(rng) < 0.12f) shouldDraw = true;
                }

                if (shouldDraw) {
                    grid[y * width + x] = 1;
                    grid[y * width + (width - 1 - x)] = 1;
                }
            }
        }
    }
    else {
        std::vector<std::string> blueprint = generateDynamicBlueprint(rng);
        generateFromTemplate(rng, blueprint);
    }

    applyShading();
    applyOutline();

    drawOrder.clear();
    drawOrder.resize(width * height);
    for (int i = 0; i < width * height; ++i) drawOrder[i] = i;
    std::shuffle(drawOrder.begin(), drawOrder.end(), rng);
}

void AIHelper::update(sf::RenderTexture& canvas) {
    if (!isGenerating) return;
    int pixelsPerFrame = 40;
    float pixelSize = 10.0f;
    float startX = currentBounds.left + (currentBounds.width - (width * pixelSize)) / 2.0f;
    float startY = currentBounds.top + (currentBounds.height - (height * pixelSize)) / 2.0f;

    for (int i = 0; i < pixelsPerFrame; ++i) {
        if (currentDrawIndex >= width * height) {
            isGenerating = false;
            toggle();
            break;
        }
        int actualIndex = drawOrder[currentDrawIndex];
        int cell = grid[actualIndex];
        if (cell > 0) {
            sf::RectangleShape pixel(sf::Vector2f(pixelSize, pixelSize));
            pixel.setPosition(startX + (actualIndex % width * pixelSize), startY + (actualIndex / width * pixelSize));
            if (cell == 1) pixel.setFillColor(baseColor);
            else if (cell == 2) pixel.setFillColor(lightColor);
            else if (cell == 3) pixel.setFillColor(darkColor);
            else if (cell == 4) pixel.setFillColor(sf::Color(30, 30, 30));
            canvas.draw(pixel);
        }
        currentDrawIndex++;
    }
    canvas.display();
}