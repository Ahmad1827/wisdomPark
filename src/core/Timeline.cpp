#include "Timeline.h"

Timeline::Timeline() : currentFrame(0), playing(false), timePerFrame(1.0f / 12.0f), totalFrames(1) {}

void Timeline::update(size_t frameCount) {
    totalFrames = frameCount;
    if (playing) {
        if (playClock.getElapsedTime().asSeconds() >= timePerFrame) {
            currentFrame++;
            if (currentFrame >= totalFrames) {
                currentFrame = 0;
            }
            playClock.restart();
        }
    }
}

void Timeline::togglePlayback() {
    playing = !playing;
    if (playing) {
        currentFrame = 0;
        playClock.restart();
    }
    else {
        currentFrame = totalFrames > 0 ? totalFrames - 1 : 0;
    }
}

void Timeline::stopPlayback() {
    playing = false;
}

void Timeline::nextFrame() {
    if (!playing && currentFrame < totalFrames - 1) {
        currentFrame++;
    }
}

void Timeline::prevFrame() {
    if (!playing && currentFrame > 0) {
        currentFrame--;
    }
}

void Timeline::setFrame(int frame) {
    if (frame >= 0 && frame < totalFrames) {
        currentFrame = frame;
    }
}

int Timeline::getCurrentFrame() const { return currentFrame; }
bool Timeline::isPlaying() const { return playing; }