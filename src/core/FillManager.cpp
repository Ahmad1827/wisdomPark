#include "FillManager.h"
#include <cmath>
#include <stack>
#include <algorithm>

bool FillManager::colorMatches(const sf::Color& target, const sf::Color& test, float tolerance) const {
    if (tolerance <= 0.0f) {
        return target.r == test.r && target.g == test.g && target.b == test.b && target.a == test.a;
    }

    float diffR = std::abs(static_cast<float>(target.r) - static_cast<float>(test.r));
    float diffG = std::abs(static_cast<float>(target.g) - static_cast<float>(test.g));
    float diffB = std::abs(static_cast<float>(target.b) - static_cast<float>(test.b));
    float diffA = std::abs(static_cast<float>(target.a) - static_cast<float>(test.a));

    float totalDiff = (diffR + diffG + diffB + diffA) / (4.0f * 255.0f);
    return totalDiff <= tolerance;
}

void FillManager::fillScanline(sf::Image& image, int x, int y, const sf::Color& targetColor, const sf::Color& replacementColor, float tolerance) {
    if (colorMatches(targetColor, replacementColor, 0.0f)) return;

    int width = static_cast<int>(image.getSize().x);
    int height = static_cast<int>(image.getSize().y);

    if (x < 0 || x >= width || y < 0 || y >= height) return;

    if (!colorMatches(image.getPixel(x, y), targetColor, tolerance)) return;

    std::vector<bool> visited(width * height, false);
    std::stack<sf::Vector2i> stack;
    stack.push(sf::Vector2i(x, y));

    while (!stack.empty()) {
        sf::Vector2i pt = stack.top();
        stack.pop();

        int cx = pt.x;
        int cy = pt.y;

        int linearIndex = cy * width + cx;
        if (visited[linearIndex]) continue;

        while (cx > 0 && colorMatches(image.getPixel(cx - 1, cy), targetColor, tolerance)) {
            cx--;
        }

        bool spanAbove = false;
        bool spanBelow = false;

        while (cx < width && colorMatches(image.getPixel(cx, cy), targetColor, tolerance)) {
            int currentLinearIndex = cy * width + cx;
            if (!visited[currentLinearIndex]) {
                image.setPixel(cx, cy, replacementColor);
                visited[currentLinearIndex] = true;
            }

            if (cy > 0) {
                bool matchAbove = colorMatches(image.getPixel(cx, cy - 1), targetColor, tolerance);
                int aboveIndex = (cy - 1) * width + cx;
                if (!spanAbove && matchAbove && !visited[aboveIndex]) {
                    stack.push(sf::Vector2i(cx, cy - 1));
                    spanAbove = true;
                }
                else if (spanAbove && !matchAbove) {
                    spanAbove = false;
                }
            }

            if (cy < height - 1) {
                bool matchBelow = colorMatches(image.getPixel(cx, cy + 1), targetColor, tolerance);
                int belowIndex = (cy + 1) * width + cx;
                if (!spanBelow && matchBelow && !visited[belowIndex]) {
                    stack.push(sf::Vector2i(cx, cy + 1));
                    spanBelow = true;
                }
                else if (spanBelow && !matchBelow) {
                    spanBelow = false;
                }
            }
            cx++;
        }
    }
}

void FillManager::execute(sf::Image& image, sf::Vector2i startPoint, const sf::Color& targetColor, const sf::Color& replacementColor, float tolerance, bool contiguous) {
    if (colorMatches(targetColor, replacementColor, 0.0f)) return;

    if (contiguous) {
        fillScanline(image, startPoint.x, startPoint.y, targetColor, replacementColor, tolerance);
    }
    else {
        int width = static_cast<int>(image.getSize().x);
        int height = static_cast<int>(image.getSize().y);
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                if (colorMatches(image.getPixel(x, y), targetColor, tolerance)) {
                    image.setPixel(x, y, replacementColor);
                }
            }
        }
    }
}