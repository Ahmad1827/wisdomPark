#pragma once
#include <SFML/Graphics.hpp>

namespace WisdomUI {

    struct Theme {
        // Theme Park & Fantasy Workshop Palette
        static inline const sf::Color Background = sf::Color(24, 20, 16, 185); // Translucent warm park shadow
        static inline const sf::Color Panel = sf::Color(42, 32, 24, 220); // Warm mahogany workshop panel
        static inline const sf::Color PanelHover = sf::Color(65, 48, 36, 235);
        static inline const sf::Color Border = sf::Color(175, 125, 45);    // Antique park brass / gold
        static inline const sf::Color BorderHighlight = sf::Color(250, 205, 80);    // Bright carousel gold
        static inline const sf::Color Accent = sf::Color(190, 55, 40);     // Carnival ruby red highlight
        static inline const sf::Color AccentHover = sf::Color(225, 75, 55);     // Glowing attraction ruby
        static inline const sf::Color Gold = sf::Color(255, 215, 90);    // Golden park sign accent
        static inline const sf::Color TextPrimary = sf::Color(255, 245, 225);   // Warm ivory parchment
        static inline const sf::Color TextSecondary = sf::Color(205, 175, 130);   // Antique sand text
        static inline const sf::Color Transparent = sf::Color(0, 0, 0, 0);

        // Layout Metrics
        static inline const float TopBarHeight = 36.0f;
        static inline const float OptionsBarHeight = 32.0f;
        static inline const float ToolDockWidth = 48.0f;
        static inline const float RightDockWidth = 280.0f;
        static inline const float TimelineHeight = 200.0f;
        static inline const float StatusBarHeight = 24.0f;
        static inline const float BorderThickness = 1.0f;
    };

}