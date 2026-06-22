#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

struct TimelineFrame {
    sf::Image thumbnail;
    int duration = 1;
    std::string label = "";
    sf::Color color = sf::Color::Transparent;
    std::string notes = "";
};

class Timeline {
private:
    std::vector<TimelineFrame> frames;
    int currentFrameIndex = 0;
    float zoomLevel = 1.0f;
    int loopStart = 0;
    int loopEnd = 0;
    bool isLoopingEnabled = false;
    int playbackStart = 0;
    int playbackEnd = 0;
    bool playing = false;
    float frameTimer = 0.0f;
    float fps = 12.0f;

public:
    Timeline();
    ~Timeline() = default;

    void update(float dt);

    void addFrame();
    void deleteFrame(int index);
    void duplicateFrame(int index);
    void swapFrames(int indexA, int indexB);
    void copyFrame(int index, TimelineFrame& clipboard);
    void pasteFrame(int index, const TimelineFrame& clipboard);

    void setFrame(int index);
    int getCurrentFrame() const;
    int getFrameCount() const;

    TimelineFrame& getFrameData(int index);
    const TimelineFrame& getFrameData(int index) const;

    void setFrameDuration(int index, int duration);
    void setFrameLabel(int index, const std::string& label);
    void setFrameColor(int index, const sf::Color& color);
    void setFrameNotes(int index, const std::string& notes);

    void setZoom(float zoom);
    float getZoom() const;
    void zoomIn();
    void zoomOut();

    void setLoopRange(int start, int end);
    void toggleLoop(bool enable);
    bool isLooping() const;

    void setPlaybackRange(int start, int end);
    void nextFrame();
    void prevFrame();

    bool isPlaying() const;
    void togglePlayback();

    void updateThumbnails(const std::vector<sf::Image>& currentLayerImages);
};