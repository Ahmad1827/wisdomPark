#pragma once
#include <SFML/Graphics/Color.hpp>

namespace StudioUI {

namespace Theme {

// Wisdom Park Dark Theme Palette
inline const sf::Color MainBackground{28, 30, 34};
inline const sf::Color PanelBackground{38, 40, 45};
inline const sf::Color InspectorBackground{43, 45, 50};
inline const sf::Color ViewportBackground{24, 25, 28};
inline const sf::Color BorderColor{60, 63, 70};
inline const sf::Color HoverColor{70, 74, 84};
inline const sf::Color ActiveColor{52, 55, 65};

// Wisdom Park Accent Purple
inline const sf::Color AccentColor{88, 101, 242};
inline const sf::Color AccentHoverColor{108, 119, 250};

// Text Hierarchy
inline const sf::Color TextPrimary{220, 220, 225};
inline const sf::Color TextSecondary{150, 153, 160};
inline const sf::Color TextMuted{100, 103, 110};

// Sizing & Spacing Standards
inline constexpr float HeaderHeight = 28.0f;
inline constexpr float ToolbarHeight = 36.0f;
inline constexpr float StatusBarHeight = 24.0f;
inline constexpr float BorderThickness = 1.0f;
inline constexpr float CornerRadius = 0.0f; // Native desktop square panels

} // namespace Theme

} // namespace StudioUI