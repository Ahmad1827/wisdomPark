#pragma once
#include <SFML/Graphics.hpp>
#include <string>

namespace WisdomUI {

    class Icons {
    public:
        static void Draw(sf::RenderWindow& window, const std::string& id, sf::Vector2f pos, float size, sf::Color color) {
            float s = size / 16.0f;

            auto drawPixel = [&](float x, float y, float w = 1.0f, float h = 1.0f) {
                sf::RectangleShape r(sf::Vector2f(w * s, h * s));
                r.setPosition(pos.x + x * s, pos.y + y * s);
                r.setFillColor(color);
                window.draw(r);
                };

            if (id == "brush") {
                drawPixel(10, 1, 4, 2);
                drawPixel(8, 3, 4, 3);
                drawPixel(6, 6, 4, 3);
                drawPixel(4, 9, 3, 3);
                drawPixel(2, 12, 3, 3);
                drawPixel(1, 14, 2, 2);
            }
            else if (id == "pencil") {
                drawPixel(11, 2, 3, 3);
                drawPixel(9, 5, 3, 3);
                drawPixel(7, 7, 3, 3);
                drawPixel(5, 9, 3, 3);
                drawPixel(3, 11, 3, 3);
                drawPixel(1, 14, 2, 2);
            }
            else if (id == "eraser") {
                drawPixel(6, 2, 8, 4);
                drawPixel(4, 6, 8, 4);
                drawPixel(2, 10, 8, 4);
                drawPixel(2, 12, 6, 2);
            }
            else if (id == "fill") {
                drawPixel(4, 2, 6, 2);
                drawPixel(3, 4, 8, 6);
                drawPixel(5, 10, 5, 2);
                drawPixel(12, 8, 2, 2);
                drawPixel(11, 11, 3, 4);
            }
            else if (id == "select") {
                drawPixel(2, 2, 12, 2);
                drawPixel(2, 12, 12, 2);
                drawPixel(2, 4, 2, 8);
                drawPixel(12, 4, 2, 8);
            }
            else if (id == "magic_wand") {
                drawPixel(11, 2, 3, 3);
                drawPixel(8, 5, 3, 3);
                drawPixel(5, 8, 3, 3);
                drawPixel(2, 11, 3, 3);
                drawPixel(6, 1, 2, 2);
                drawPixel(13, 7, 2, 2);
                drawPixel(1, 6, 2, 2);
            }
            else if (id == "shapes") {
                drawPixel(2, 2, 6, 6);
                drawPixel(8, 8, 6, 6);
            }
            else if (id == "text") {
                drawPixel(2, 2, 12, 3);
                drawPixel(6, 5, 4, 9);
                drawPixel(4, 13, 8, 2);
            }
            else if (id == "gradient") {
                drawPixel(2, 2, 12, 3);
                drawPixel(2, 6, 12, 2);
                drawPixel(2, 9, 12, 2);
                drawPixel(2, 12, 12, 2);
            }
            else if (id == "symmetry") {
                drawPixel(7, 1, 2, 14);
                drawPixel(3, 4, 3, 3);
                drawPixel(10, 4, 3, 3);
                drawPixel(2, 8, 4, 4);
                drawPixel(10, 8, 4, 4);
            }
            else if (id == "perspective") {
                drawPixel(2, 2, 12, 2);
                drawPixel(4, 12, 8, 2);
                drawPixel(3, 4, 2, 8);
                drawPixel(11, 4, 2, 8);
            }
            else if (id == "ai_gen") {
                drawPixel(7, 1, 2, 5);
                drawPixel(7, 10, 2, 5);
                drawPixel(1, 7, 5, 2);
                drawPixel(10, 7, 5, 2);
                drawPixel(6, 6, 4, 4);
            }
            else if (id == "layers") {
                drawPixel(2, 2, 12, 3);
                drawPixel(1, 6, 14, 3);
                drawPixel(2, 10, 12, 3);
            }
            else if (id == "palette") {
                drawPixel(4, 2, 8, 2);
                drawPixel(2, 4, 12, 8);
                drawPixel(4, 12, 8, 2);
                drawPixel(4, 4, 2, 2);
                drawPixel(8, 4, 2, 2);
                drawPixel(10, 7, 2, 2);
                drawPixel(6, 9, 2, 2);
            }
            else if (id == "properties") {
                drawPixel(2, 3, 12, 2);
                drawPixel(4, 2, 3, 4);
                drawPixel(2, 8, 12, 2);
                drawPixel(9, 7, 3, 4);
                drawPixel(2, 13, 12, 2);
                drawPixel(6, 12, 3, 4);
            }
            else if (id == "assets") {
                drawPixel(2, 3, 5, 2);
                drawPixel(2, 5, 12, 8);
                drawPixel(4, 7, 8, 4);
            }
            else if (id == "audio") {
                drawPixel(8, 2, 4, 2);
                drawPixel(8, 4, 2, 7);
                drawPixel(5, 9, 4, 3);
                drawPixel(12, 2, 2, 6);
            }
            else if (id == "timeline") {
                drawPixel(2, 4, 12, 8);
                drawPixel(4, 2, 2, 2);
                drawPixel(7, 2, 2, 2);
                drawPixel(10, 2, 2, 2);
                drawPixel(5, 6, 6, 4);
            }
            else if (id == "undo") {
                drawPixel(3, 5, 2, 4);
                drawPixel(5, 3, 2, 3);
                drawPixel(5, 8, 2, 3);
                drawPixel(7, 4, 6, 2);
                drawPixel(11, 6, 2, 6);
            }
            else if (id == "redo") {
                drawPixel(11, 5, 2, 4);
                drawPixel(9, 3, 2, 3);
                drawPixel(9, 8, 2, 3);
                drawPixel(3, 4, 6, 2);
                drawPixel(3, 6, 2, 6);
            }
            else if (id == "save") {
                drawPixel(2, 2, 12, 12);
                drawPixel(4, 2, 8, 4);
                drawPixel(4, 8, 8, 5);
            }
            else if (id == "fullscreen") {
                drawPixel(2, 2, 4, 2);
                drawPixel(2, 2, 2, 4);
                drawPixel(10, 2, 4, 2);
                drawPixel(12, 2, 2, 4);
                drawPixel(2, 12, 4, 2);
                drawPixel(2, 10, 2, 4);
                drawPixel(10, 12, 4, 2);
                drawPixel(12, 10, 2, 4);
            }
            else {
                drawPixel(2, 2, 12, 12);
            }
        }
    };

}