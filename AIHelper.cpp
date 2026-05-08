#include "AIHelper.h"
#include <algorithm>
#include <cmath>
#include <iostream>

AIHelper::AIHelper() : active(false), isGenerating(false), currentDrawIndex(0), isTrained(false), currentMemoryFrame(0), currentTheme("all"), currentTerrainDrawIndex(0) {
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

void AIHelper::setTheme(const std::string& theme) { currentTheme = theme; }
std::string AIHelper::getTheme() const { return currentTheme; }

void AIHelper::setFrame(int frameIndex) {
    if (isGenerating) {
        isGenerating = false;
        if (active) toggle();
    }
    frameMemory[currentMemoryFrame] = history;
    currentMemoryFrame = frameIndex;
    history = frameMemory[currentMemoryFrame];
}

void AIHelper::clearAllMemory() {
    history.clear();
    frameMemory.clear();
    currentMemoryFrame = 0;
}

void AIHelper::cancelSlowDraw() {
    isGenerating = false;
}

float AIHelper::getArtWidth() const {
    float GLOBAL_PIXEL_SIZE = 6.0f;
    if (isTrained && !history.empty()) {
        return width * (GLOBAL_PIXEL_SIZE * datasetTemplates[history.back().datasetIndex].scale);
    }
    return width * GLOBAL_PIXEL_SIZE;
}

float AIHelper::getArtHeight() const {
    float GLOBAL_PIXEL_SIZE = 6.0f;
    if (isTrained && !history.empty()) {
        return height * (GLOBAL_PIXEL_SIZE * datasetTemplates[history.back().datasetIndex].scale);
    }
    return height * GLOBAL_PIXEL_SIZE;
}

void AIHelper::stampOnCanvas(sf::RenderTexture& canvas, float drawX, float drawY) {
    float GLOBAL_PIXEL_SIZE = 6.0f;
    float pixelSize = GLOBAL_PIXEL_SIZE;

    if (isTrained && !history.empty()) {
        int currentIndex = history.back().datasetIndex;
        pixelSize = GLOBAL_PIXEL_SIZE * datasetTemplates[currentIndex].scale;
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int cell = grid[y * width + x];
            if (cell > 0) {
                sf::RectangleShape pixel(sf::Vector2f(pixelSize, pixelSize));
                pixel.setPosition(drawX + (x * pixelSize), drawY + (y * pixelSize));

                if (cell == 1) pixel.setFillColor(baseColor);
                else if (cell == 2) pixel.setFillColor(lightColor);
                else if (cell == 3) pixel.setFillColor(darkColor);
                else if (cell == 4) pixel.setFillColor(sf::Color(30, 30, 30));
                else if (cell == 5) pixel.setFillColor(sf::Color(30, 30, 30));
                else if (cell == 6) pixel.setFillColor(sf::Color(245, 245, 245));

                canvas.draw(pixel);
            }
        }
    }
}

void AIHelper::generatePath(sf::RenderTexture& canvas, sf::Vector2f start, sf::Vector2f end, sf::FloatRect bounds) {
    int nodeSize = 20;
    int gW = static_cast<int>(bounds.width / nodeSize);
    int gH = static_cast<int>(bounds.height / nodeSize);

    auto toGrid = [&](sf::Vector2f p) {
        return sf::Vector2i(
            std::clamp(static_cast<int>((p.x - bounds.left) / nodeSize), 0, gW - 1),
            std::clamp(static_cast<int>((p.y - bounds.top) / nodeSize), 0, gH - 1)
        );
        };

    sf::Vector2i startG = toGrid(start);
    sf::Vector2i endG = toGrid(end);

    std::vector<float> costs(gW * gH, 1.0f);
    for (const auto& item : history) {
        int minX = std::max(0, static_cast<int>((item.bounds.left - bounds.left) / nodeSize));
        int maxX = std::min(gW - 1, static_cast<int>((item.bounds.left + item.bounds.width - bounds.left) / nodeSize));
        int minY = std::max(0, static_cast<int>((item.bounds.top - bounds.top) / nodeSize));
        int maxY = std::min(gH - 1, static_cast<int>((item.bounds.top + item.bounds.height - bounds.top) / nodeSize));

        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                costs[y * gW + x] = 50.0f;
            }
        }
    }

    auto compare = [](Node* a, Node* b) { return a->f() > b->f(); };
    std::priority_queue<Node*, std::vector<Node*>, decltype(compare)> openSet(compare);

    std::vector<Node*> allNodes;
    Node* startNode = new Node{ startG.x, startG.y, 0, 0, nullptr };
    allNodes.push_back(startNode);
    openSet.push(startNode);

    std::map<int, float> closedSet;
    Node* goalNode = nullptr;

    while (!openSet.empty()) {
        Node* current = openSet.top();
        openSet.pop();

        if (current->x == endG.x && current->y == endG.y) {
            goalNode = current;
            break;
        }

        int id = current->y * gW + current->x;
        if (closedSet.count(id) && closedSet[id] <= current->g) continue;
        closedSet[id] = current->g;

        int dx[] = { 0, 0, 1, -1 };
        int dy[] = { 1, -1, 0, 0 };

        for (int i = 0; i < 4; ++i) {
            int nx = current->x + dx[i], ny = current->y + dy[i];
            if (nx >= 0 && nx < gW && ny >= 0 && ny < gH) {
                float moveCost = costs[ny * gW + nx];
                float newG = current->g + moveCost;
                float h = std::abs(nx - endG.x) + std::abs(ny - endG.y);
                Node* neighbor = new Node{ nx, ny, newG, h, current };
                allNodes.push_back(neighbor);
                openSet.push(neighbor);
            }
        }
    }

    if (goalNode) {
        Node* curr = goalNode;
        while (curr) {
            sf::CircleShape pathNode(nodeSize * 0.6f);
            pathNode.setOrigin(nodeSize * 0.3f, nodeSize * 0.3f);
            pathNode.setPosition(bounds.left + (curr->x * nodeSize) + nodeSize / 2, bounds.top + (curr->y * nodeSize) + nodeSize / 2);
            pathNode.setFillColor(sf::Color(139, 69, 19, 180));
            canvas.draw(pathNode);
            curr = curr->parent;
        }
    }

    for (auto n : allNodes) delete n;
    canvas.display();
}

void AIHelper::trainOnDataset(const std::string& filename) {
    datasetTemplates.clear();
    std::ifstream file(filename);
    if (!file.is_open()) {
        isTrained = false;
        return;
    }

    Template currentTemplate;
    std::string line;
    bool inPixels = false;

    while (std::getline(file, line)) {
        if (line.find("\"name\":") != std::string::npos) {
            size_t start = line.find_first_of(':') + 3;
            size_t end = line.find_last_of('"');
            currentTemplate.name = line.substr(start, end - start);
        }
        else if (line.find("\"category\":") != std::string::npos) {
            size_t start = line.find_first_of(':') + 3;
            size_t end = line.find_last_of('"');
            currentTemplate.category = line.substr(start, end - start);
        }
        else if (line.find("\"scale\":") != std::string::npos) {
            size_t start = line.find_first_of(':') + 1;
            currentTemplate.scale = std::stof(line.substr(start));
        }
        else if (line.find("\"width\":") != std::string::npos) {
            size_t start = line.find_first_of(':') + 1;
            currentTemplate.width = std::stoi(line.substr(start));
        }
        else if (line.find("\"height\":") != std::string::npos) {
            size_t start = line.find_first_of(':') + 1;
            currentTemplate.height = std::stoi(line.substr(start));
        }
        else if (line.find("\"pixels\": [") != std::string::npos) {
            inPixels = true;
            currentTemplate.pixels.clear();
        }
        else if (inPixels) {
            if (line.find("]") != std::string::npos) {
                inPixels = false;
                datasetTemplates.push_back(currentTemplate);
            }
            else {
                size_t start = line.find_first_of('"') + 1;
                size_t end = line.find_last_of('"');
                if (start != std::string::npos && end != std::string::npos && end > start) {
                    currentTemplate.pixels.push_back(line.substr(start, end - start));
                }
            }
        }
    }
    isTrained = !datasetTemplates.empty();
}

std::vector<std::string> AIHelper::generateDynamicBlueprint(std::mt19937& rng) {
    std::string emptyRow(width, '.');
    std::vector<std::string> blueprint(height, emptyRow);
    int center = (width / 2) - 1;
    int objectType = std::uniform_int_distribution<int>(0, 2)(rng);

    if (objectType == 0) {
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
    else if (objectType == 1) {
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
    else {
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

void AIHelper::generateTerrainPatch(std::mt19937& rng, sf::FloatRect itemBounds) {
    terrainWidth = 36;
    terrainHeight = 24;
    terrainPixelSize = 10.0f;
    terrainGrid.assign(terrainWidth * terrainHeight, 0);

    std::uniform_real_distribution<float> prob(0.0f, 1.0f);
    for (int i = 0; i < terrainWidth * terrainHeight; ++i) {
        if (prob(rng) > 0.40f) terrainGrid[i] = 1;
    }

    for (int pass = 0; pass < 3; ++pass) {
        std::vector<int> newGrid = terrainGrid;
        for (int y = 0; y < terrainHeight; ++y) {
            for (int x = 0; x < terrainWidth; ++x) {
                int neighbors = 0;
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        if (dx == 0 && dy == 0) continue;
                        int nx = x + dx, ny = y + dy;
                        if (nx >= 0 && nx < terrainWidth && ny >= 0 && ny < terrainHeight) {
                            neighbors += terrainGrid[ny * terrainWidth + nx];
                        }
                    }
                }
                if (neighbors > 4) newGrid[y * terrainWidth + x] = 1;
                else if (neighbors < 4) newGrid[y * terrainWidth + x] = 0;
            }
        }
        terrainGrid = newGrid;
    }

    std::vector<sf::Color> biomes = {
        sf::Color(107, 142, 35, 200),
        sf::Color(139, 69, 19, 200),
        sf::Color(70, 130, 180, 200),
        sf::Color(210, 180, 140, 200)
    };
    std::uniform_int_distribution<int> colorDist(0, biomes.size() - 1);
    terrainColor = biomes[colorDist(rng)];

    terrainX = itemBounds.left + (itemBounds.width / 2.0f) - ((terrainWidth * terrainPixelSize) / 2.0f);
    terrainY = itemBounds.top + itemBounds.height - ((terrainHeight * terrainPixelSize) * 0.75f);
}

std::string AIHelper::startGeneratingComplexArt(sf::FloatRect bounds, const sf::Image& currentCanvas, bool isAnimation) {
    if (isGenerating) return "";

    isGenerating = true;
    currentDrawIndex = 0;
    currentTerrainDrawIndex = 0;
    terrainGrid.clear();
    currentBounds = bounds;
    std::random_device rd;
    std::mt19937 rng(rd());

    long long rSum = 0, gSum = 0, bSum = 0;
    int validPixels = 0;

    for (unsigned int y = 0; y < currentCanvas.getSize().y; y += 10) {
        for (unsigned int x = 0; x < currentCanvas.getSize().x; x += 10) {
            sf::Color c = currentCanvas.getPixel(x, y);
            if (c.a > 20 && !(c.r > 240 && c.g > 240 && c.b > 240)) {
                rSum += c.r;
                gSum += c.g;
                bSum += c.b;
                validPixels++;
            }
        }
    }

    if (validPixels > 50) {
        int avgR = rSum / validPixels;
        int avgG = gSum / validPixels;
        int avgB = bSum / validPixels;

        baseColor = sf::Color(avgR, avgG, avgB);
        lightColor = sf::Color(std::min(255, avgR + 50), std::min(255, avgG + 50), std::min(255, avgB + 50));
        darkColor = sf::Color(std::max(0, avgR - 50), std::max(0, avgG - 50), std::max(0, avgB - 50));
    }
    else {
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
    }

    if (isTrained) {
        std::vector<int> templateIndices(datasetTemplates.size());
        for (size_t i = 0; i < datasetTemplates.size(); ++i) {
            templateIndices[i] = i;
        }
        std::shuffle(templateIndices.begin(), templateIndices.end(), rng);

        bool validSpot = false;
        int chosenIndex = -1;
        float finalItemWidth = 0;
        float finalItemHeight = 0;

        for (int idx : templateIndices) {
            const auto& testTemplate = datasetTemplates[idx];

            if (currentTheme != "all") {
                if (currentTheme == "clutter") {
                    if (testTemplate.category != "clutter" && testTemplate.category != "healing" && testTemplate.category != "status-cures" && testTemplate.category != "vitamins") {
                        continue;
                    }
                }
                else if (testTemplate.category != currentTheme) {
                    continue;
                }
            }

            float pixelSize = 6.0f * testTemplate.scale;
            float itemWidth = testTemplate.width * pixelSize;
            float itemHeight = testTemplate.height * pixelSize;

            if (itemWidth > bounds.width || itemHeight > bounds.height) {
                continue;
            }

            if (isAnimation) {
                validSpot = true;
                chosenIndex = idx;
                finalItemWidth = itemWidth;
                finalItemHeight = itemHeight;
                break;
            }

            bool isClutter = (testTemplate.category == "healing" || testTemplate.category == "status-cures" || testTemplate.category == "vitamins" || testTemplate.category == "clutter");

            std::vector<int> structures;
            if (isClutter) {
                for (size_t i = 0; i < history.size(); ++i) {
                    if (history[i].category != "healing" && history[i].category != "status-cures" && history[i].category != "vitamins" && history[i].category != "clutter") {
                        structures.push_back(i);
                    }
                }
            }

            if (history.empty()) {
                currentX = bounds.left + (bounds.width - itemWidth) / 2.0f;
                currentY = bounds.top + (bounds.height - itemHeight) / 2.0f;
                validSpot = true;
                chosenIndex = idx;
                finalItemWidth = itemWidth;
                finalItemHeight = itemHeight;
                break;
            }

            int attempts = 0;
            while (attempts < 300) {
                float testX = 0;
                float testY = 0;

                if (isClutter && !structures.empty()) {
                    std::uniform_int_distribution<int> sDist(0, structures.size() - 1);
                    sf::FloatRect sBounds = history[structures[sDist(rng)]].bounds;

                    float stackW = std::max(1.0f, sBounds.width * 0.6f);
                    float stackH = std::max(1.0f, sBounds.height * 0.6f);
                    float safeLeft = sBounds.left + (sBounds.width - stackW) / 2.0f;
                    float safeTop = sBounds.top + (sBounds.height - stackH) / 2.0f;

                    std::uniform_real_distribution<float> xDist(safeLeft, safeLeft + stackW);
                    std::uniform_real_distribution<float> yDist(safeTop, safeTop + stackH);

                    testX = std::clamp(xDist(rng), bounds.left, bounds.left + bounds.width - itemWidth);
                    testY = std::clamp(yDist(rng), bounds.top, bounds.top + bounds.height - itemHeight);
                }
                else {
                    std::uniform_real_distribution<float> xDist(bounds.left, bounds.left + bounds.width - itemWidth);
                    std::uniform_real_distribution<float> yDist(bounds.top, bounds.top + bounds.height - itemHeight);
                    testX = xDist(rng);
                    testY = yDist(rng);
                }

                sf::FloatRect newRect(testX - 5, testY - 5, itemWidth + 10, itemHeight + 10);
                bool intersects = false;

                for (const auto& pastItem : history) {
                    if (newRect.intersects(pastItem.bounds)) {
                        bool pastIsStructure = (pastItem.category != "healing" && pastItem.category != "status-cures" && pastItem.category != "vitamins" && pastItem.category != "clutter");

                        if (isClutter && pastIsStructure) {
                            continue;
                        }

                        intersects = true;
                        break;
                    }
                }

                if (!intersects) {
                    validSpot = true;
                    currentX = testX;
                    currentY = testY;
                    chosenIndex = idx;
                    finalItemWidth = itemWidth;
                    finalItemHeight = itemHeight;
                    break;
                }
                attempts++;
            }

            if (validSpot) break;
        }

        if (!validSpot) {
            isGenerating = false;
            return "ERROR: Canvas is too crowded or no items match theme!";
        }

        const auto& selectedTemplate = datasetTemplates[chosenIndex];
        height = selectedTemplate.height;
        width = selectedTemplate.width;

        sf::FloatRect finalBounds(currentX, currentY, finalItemWidth, finalItemHeight);
        PlacedItem newObj = { chosenIndex, selectedTemplate.category, finalBounds };
        history.push_back(newObj);

        bool isClutter = (selectedTemplate.category == "healing" || selectedTemplate.category == "status-cures" || selectedTemplate.category == "vitamins" || selectedTemplate.category == "clutter");
        if (!isClutter && !isAnimation) {
            generateTerrainPatch(rng, finalBounds);
        }

        grid.clear();
        grid.resize(width * height, 0);

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                char cell = selectedTemplate.pixels[y][x];
                if (cell == 'W') grid[y * width + x] = 6;
                else if (cell == 'H') grid[y * width + x] = 2;
                else if (cell == 'X') grid[y * width + x] = 1;
                else if (cell == 'S') grid[y * width + x] = 3;
                else if (cell == 'O') grid[y * width + x] = 5;
            }
        }
        applyOutline();
    }
    else {
        width = 48;
        height = 48;
        grid.clear();
        grid.resize(width * height, 0);

        std::vector<std::string> blueprint = generateDynamicBlueprint(rng);
        generateFromTemplate(rng, blueprint);
        applyShading();
        applyOutline();

        float GLOBAL_PIXEL_SIZE = 6.0f;
        float itemWidth = width * GLOBAL_PIXEL_SIZE;
        float itemHeight = height * GLOBAL_PIXEL_SIZE;

        currentX = bounds.left + (bounds.width - itemWidth) / 2.0f;
        currentY = bounds.top + (bounds.height - itemHeight) / 2.0f;

        if (!isAnimation) {
            sf::FloatRect finalBounds(currentX, currentY, itemWidth, itemHeight);
            generateTerrainPatch(rng, finalBounds);
        }
    }

    drawOrder.clear();
    drawOrder.resize(width * height);
    for (int i = 0; i < width * height; ++i) drawOrder[i] = i;
    std::shuffle(drawOrder.begin(), drawOrder.end(), rng);

    return "";
}

void AIHelper::update(sf::RenderTexture& canvas) {
    if (!isGenerating) return;

    if (currentTerrainDrawIndex < terrainGrid.size()) {
        int pixelsPerFrame = (terrainWidth * terrainHeight) / 10;
        if (pixelsPerFrame < 5) pixelsPerFrame = 5;

        for (int i = 0; i < pixelsPerFrame; ++i) {
            if (currentTerrainDrawIndex >= terrainGrid.size()) break;

            if (terrainGrid[currentTerrainDrawIndex] == 1) {
                int x = currentTerrainDrawIndex % terrainWidth;
                int y = currentTerrainDrawIndex / terrainWidth;

                sf::RectangleShape pixel(sf::Vector2f(terrainPixelSize, terrainPixelSize));
                pixel.setPosition(terrainX + (x * terrainPixelSize), terrainY + (y * terrainPixelSize));
                pixel.setFillColor(terrainColor);
                canvas.draw(pixel);
            }
            currentTerrainDrawIndex++;
        }

        if (currentTerrainDrawIndex < terrainGrid.size()) {
            canvas.display();
            return;
        }
    }

    float GLOBAL_PIXEL_SIZE = 6.0f;
    float pixelSize = GLOBAL_PIXEL_SIZE;

    if (isTrained && !history.empty()) {
        int currentIndex = history.back().datasetIndex;
        pixelSize = GLOBAL_PIXEL_SIZE * datasetTemplates[currentIndex].scale;
    }

    int pixelsPerFrame = (width * height) / 40;
    if (pixelsPerFrame < 10) pixelsPerFrame = 10;

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
            pixel.setPosition(currentX + (actualIndex % width * pixelSize), currentY + (actualIndex / width * pixelSize));

            if (cell == 1) pixel.setFillColor(baseColor);
            else if (cell == 2) pixel.setFillColor(lightColor);
            else if (cell == 3) pixel.setFillColor(darkColor);
            else if (cell == 4) pixel.setFillColor(sf::Color(30, 30, 30));
            else if (cell == 5) pixel.setFillColor(sf::Color(30, 30, 30));
            else if (cell == 6) pixel.setFillColor(sf::Color(245, 245, 245));

            canvas.draw(pixel);
        }
        currentDrawIndex++;
    }
    canvas.display();
}