#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

struct AudioClip {
    std::string name;
    std::string filePath;
    int startFrame = 0;
    int endFrame = 60;
    bool needsWaveformUpdate = true;
    bool soundLoadFailed = true;
    sf::VertexArray waveformRender;
};

struct AudioTrack {
    std::string name;
    std::vector<AudioClip> clips;
};

class AudioPanel {
private:
    sf::Font font;
    sf::Vector2f position;
    sf::Vector2f size;

    bool isDraggingPanel = false;
    sf::Vector2f dragOffset;

    bool isVisible = false;
    std::vector<AudioTrack> tracks;

    sf::FloatRect scanBtnBounds;
    sf::FloatRect closeBtnBounds;

    void generateWaveform(AudioClip& clip, float w, float h);

public:
    AudioPanel();
    ~AudioPanel();

    void init();
    void toggle();
    bool getIsVisible() const;
    void update(float dt);
    void draw(sf::RenderWindow& window);
    bool handleEvent(const sf::Event& event, sf::Vector2f mousePos);
    std::string handleClick(sf::Vector2f mousePos, int currentFrame);

    void scanAudioDirectory(int currentFrame);
    void addAudioClip(const std::string& path, int currentFrame);
    void updatePlayback(int currentFrame, float fps, bool isPlaying);
    void stopAll();
};