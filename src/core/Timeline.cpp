#include "Timeline.h"

Timeline::Timeline() {
    addFrame();
}

void Timeline::update(float dt) {
    if (playing) {
        frameTimer += dt;
        if (frameTimer >= 1.0f / fps) {
            frameTimer = 0.0f;
            nextFrame();
        }
    }
}

void Timeline::addFrame() {
    TimelineFrame newFrame;
    newFrame.thumbnail.create(64, 64, sf::Color::Transparent);
    frames.push_back(newFrame);
    playbackEnd = static_cast<int>(frames.size()) - 1;
    loopEnd = playbackEnd;
}

void Timeline::deleteFrame(int index) {
    if (frames.size() <= 1 || index < 0 || index >= static_cast<int>(frames.size())) {
        return;
    }
    frames.erase(frames.begin() + index);
    if (currentFrameIndex >= static_cast<int>(frames.size())) {
        currentFrameIndex = static_cast<int>(frames.size()) - 1;
    }
    playbackEnd = static_cast<int>(frames.size()) - 1;
    loopEnd = std::min(loopEnd, playbackEnd);
}

void Timeline::duplicateFrame(int index) {
    if (index < 0 || index >= static_cast<int>(frames.size())) {
        return;
    }
    TimelineFrame duplicate = frames[index];
    frames.insert(frames.begin() + index + 1, duplicate);
    playbackEnd = static_cast<int>(frames.size()) - 1;
    loopEnd = playbackEnd;
}

void Timeline::swapFrames(int indexA, int indexB) {
    if (indexA < 0 || indexA >= static_cast<int>(frames.size()) ||
        indexB < 0 || indexB >= static_cast<int>(frames.size())) {
        return;
    }
    std::swap(frames[indexA], frames[indexB]);
}

void Timeline::copyFrame(int index, TimelineFrame& clipboard) {
    if (index >= 0 && index < static_cast<int>(frames.size())) {
        clipboard = frames[index];
    }
}

void Timeline::pasteFrame(int index, const TimelineFrame& clipboard) {
    if (index >= 0 && index < static_cast<int>(frames.size())) {
        frames[index] = clipboard;
    }
}

void Timeline::setFrame(int index) {
    if (index >= 0 && index < static_cast<int>(frames.size())) {
        currentFrameIndex = index;
    }
}

int Timeline::getCurrentFrame() const {
    return currentFrameIndex;
}

int Timeline::getFrameCount() const {
    return static_cast<int>(frames.size());
}

TimelineFrame& Timeline::getFrameData(int index) {
    return frames[index];
}

const TimelineFrame& Timeline::getFrameData(int index) const {
    return frames[index];
}

void Timeline::setFrameDuration(int index, int duration) {
    if (index >= 0 && index < static_cast<int>(frames.size())) {
        frames[index].duration = std::max(1, duration);
    }
}

void Timeline::setFrameLabel(int index, const std::string& label) {
    if (index >= 0 && index < static_cast<int>(frames.size())) {
        frames[index].label = label;
    }
}

void Timeline::setFrameColor(int index, const sf::Color& color) {
    if (index >= 0 && index < static_cast<int>(frames.size())) {
        frames[index].color = color;
    }
}

void Timeline::setFrameNotes(int index, const std::string& notes) {
    if (index >= 0 && index < static_cast<int>(frames.size())) {
        frames[index].notes = notes;
    }
}

void Timeline::setZoom(float zoom) {
    zoomLevel = std::max(0.1f, std::min(zoom, 5.0f));
}

float Timeline::getZoom() const {
    return zoomLevel;
}

void Timeline::zoomIn() {
    setZoom(zoomLevel + 0.1f);
}

void Timeline::zoomOut() {
    setZoom(zoomLevel - 0.1f);
}

void Timeline::setLoopRange(int start, int end) {
    if (start >= 0 && end >= start && end < static_cast<int>(frames.size())) {
        loopStart = start;
        loopEnd = end;
    }
}

void Timeline::toggleLoop(bool enable) {
    isLoopingEnabled = enable;
}

bool Timeline::isLooping() const {
    return isLoopingEnabled;
}

void Timeline::setPlaybackRange(int start, int end) {
    if (start >= 0 && end >= start && end < static_cast<int>(frames.size())) {
        playbackStart = start;
        playbackEnd = end;
    }
}

void Timeline::nextFrame() {
    int maxBound = isLoopingEnabled ? loopEnd : playbackEnd;
    int minBound = isLoopingEnabled ? loopStart : playbackStart;

    if (currentFrameIndex >= maxBound) {
        if (isLoopingEnabled) {
            currentFrameIndex = minBound;
        }
    }
    else {
        currentFrameIndex++;
    }
}

void Timeline::prevFrame() {
    int maxBound = isLoopingEnabled ? loopEnd : playbackEnd;
    int minBound = isLoopingEnabled ? loopStart : playbackStart;

    if (currentFrameIndex <= minBound) {
        if (isLoopingEnabled) {
            currentFrameIndex = maxBound;
        }
    }
    else {
        currentFrameIndex--;
    }
}

bool Timeline::isPlaying() const {
    return playing;
}

void Timeline::togglePlayback() {
    playing = !playing;
}

void Timeline::updateThumbnails(const std::vector<sf::Image>& currentLayerImages) {
    for (size_t i = 0; i < frames.size() && i < currentLayerImages.size(); ++i) {
        frames[i].thumbnail = currentLayerImages[i];
    }
}