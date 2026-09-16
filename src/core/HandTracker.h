#pragma once
#include <SFML/Window/WindowHandle.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <memory>
class HandTracker {
public:
    static HandTracker& getInstance();

    bool start(sf::WindowHandle targetHwnd = 0);
    void stop();
    bool isRunning() const;
    bool updateTexture(sf::Texture& texture);

private:
    HandTracker();
    ~HandTracker();

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};