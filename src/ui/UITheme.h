#pragma once
#include <SFML/Graphics.hpp>
#include <string>

namespace WisdomUI {

    struct Theme {
        // Colors - "Dark Studio with subtle fantasy/magic accents"
        static inline const sf::Color Background = sf::Color(26, 28, 35);  // Deep slate
        static inline const sf::Color Panel = sf::Color(35, 38, 48);  // Clean panel surface
        static inline const sf::Color Border = sf::Color(50, 55, 70);  // Subtle separation
        static inline const sf::Color Accent = sf::Color(100, 110, 160); // Magic/Tool highlight
        static inline const sf::Color AccentHover = sf::Color(120, 130, 185);
        static inline const sf::Color TextPrimary = sf::Color(230, 235, 245);
        static inline const sf::Color TextSecondary = sf::Color(160, 165, 180);
        static inline const sf::Color Transparent = sf::Color(0, 0, 0, 0);

        // Layout Metrics
        static inline const float TopBarHeight = 36.0f;
        static inline const float OptionsBarHeight = 32.0f;
        static inline const float ToolDockWidth = 48.0f;
        static inline const float RightDockWidth = 280.0f;
        static inline const float TimelineHeight = 200.0f;
        static inline const float StatusBarHeight = 24.0f;

        // Styling
        static inline const float BorderThickness = 1.0f;
        static inline const float CornerRadius = 0.0f; // Kept sharp for pixel-art aesthetic
    };

}