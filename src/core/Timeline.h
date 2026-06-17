#pragma once
#include <SFML/Graphics.hpp>

class Timeline {
private:
    int currentFrame;
    bool playing;
    sf::Clock playClock;
    float timePerFrame;
    size_t totalFrames;

public:
    Timeline();

    void update(size_t frameCount);
    void togglePlayback();
    void stopPlayback();

    void nextFrame();
    void prevFrame();
    void setFrame(int frame);

    int getCurrentFrame() const;
    bool isPlaying() const;
};