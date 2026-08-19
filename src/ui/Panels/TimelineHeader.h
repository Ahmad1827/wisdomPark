#pragma once
#include <SFML/Graphics.hpp>
#include <functional>
#include <string>
#include "../UIAnimation.h"

namespace WisdomUI {

    class TimelineHeader {
    public:
        TimelineHeader();
        void Initialize(const sf::Font& font,
            std::function<void()> onTogglePlay,
            std::function<void()> onAddFrame,
            std::function<void()> onDuplicateFrame,
            std::function<void()> onDeleteFrame,
            std::function<void()> onToggleOnion);

        void SetBounds(const sf::FloatRect& bounds);
        void SyncState(bool isPlaying, int currentFrame, int totalFrames, float fps, bool onionEnabled);
        bool HandleEvent(const sf::Event& event, const sf::RenderWindow& window);
        void Update(float deltaTime, const sf::Vector2f& mousePos);
        void Render(sf::RenderWindow& window);

    private:
        sf::FloatRect m_bounds;
        sf::Font m_font;
        bool m_isPlaying{ false };
        int m_currentFrame{ 0 };
        int m_totalFrames{ 1 };
        float m_fps{ 12.0f };
        bool m_onionEnabled{ false };

        std::function<void()> m_onTogglePlay;
        std::function<void()> m_onAddFrame;
        std::function<void()> m_onDuplicateFrame;
        std::function<void()> m_onDeleteFrame;
        std::function<void()> m_onToggleOnion;

        sf::FloatRect m_playBtnBounds;
        sf::FloatRect m_addBtnBounds;
        sf::FloatRect m_dupBtnBounds;
        sf::FloatRect m_delBtnBounds;
        sf::FloatRect m_onionBtnBounds;

        float m_playHover{ 0.0f };
        float m_addHover{ 0.0f };
        float m_dupHover{ 0.0f };
        float m_delHover{ 0.0f };
        float m_onionHover{ 0.0f };
    };

}