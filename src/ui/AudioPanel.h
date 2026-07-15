#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <string>
#include <vector>
#include <memory>

struct AudioClip {
    std::string id;
    std::string name;
    std::string filePath;
    std::shared_ptr<sf::SoundBuffer> buffer;
    std::shared_ptr<sf::Sound> sound;

    float volume = 100.0f;
    float pitch = 1.0f;
    float pan = 0.0f;

    int startFrame = 0;
    int endFrame = 0;
    int trimStartFrame = 0;

    bool isMuted = false;
    bool isLooping = false;

    sf::VertexArray waveformRender;
    bool needsWaveformUpdate = true;
};

struct AudioTrack {
    std::string name;
    bool isMuted = false;
    bool isSolo = false;
    float trackVolume = 100.0f;
    std::vector<AudioClip> clips;
};

class AudioPanel {
private:
    sf::RectangleShape background;
    sf::RectangleShape header;
    sf::Text titleText;
    sf::Font font;

    sf::RectangleShape importBtn;
    sf::Text importBtnText;

    sf::RectangleShape closeBtn;
    sf::Text closeBtnText;

    std::vector<AudioTrack> tracks;

    float currentY;
    float targetY;
    float height;
    bool isVisible;

    int selectedTrackIndex;
    int selectedClipIndex;

    void generateWaveform(AudioClip& clip, float width, float height);

public:
    AudioPanel();
    ~AudioPanel();
    void init();
    void update(float dt);
    void draw(sf::RenderWindow& window);

    bool handleEvent(const sf::Event& event, sf::Vector2f mousePos);
    std::string handleClick(sf::Vector2f mousePos);

    void toggle();
    bool getIsVisible() const;

    void addAudioClip(const std::string& path, int currentFrame);
    void updatePlayback(int currentFrame, float fps, bool isPlaying);
    void stopAll();
};