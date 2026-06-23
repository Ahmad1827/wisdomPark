#pragma once
#include <SFML/Audio.hpp>
#include <string>
#include <vector>
#include <map>

struct AudioTrack {
    sf::SoundBuffer buffer;
    sf::Sound sound;
    std::string filePath;
    std::string name;
    float volume = 100.0f;
    bool isMuted = false;
    float offsetTime = 0.0f;
};

class AudioManager {
private:
    std::map<std::string, AudioTrack> tracks;
    float masterVolume = 100.0f;
    bool globalMute = false;

public:
    AudioManager();
    ~AudioManager() = default;

    bool importAudio(const std::string& trackName, const std::string& path);
    void removeTrack(const std::string& trackName);

    void playTrack(const std::string& trackName);
    void pauseTrack(const std::string& trackName);
    void stopTrack(const std::string& trackName);

    void playAll();
    void pauseAll();
    void stopAll();

    void setTrackVolume(const std::string& trackName, float volume);
    void muteTrack(const std::string& trackName, bool mute);

    void setMasterVolume(float volume);
    void toggleGlobalMute(bool mute);

    void syncWithAnimation(float currentPlaybackTime);
    std::vector<int16_t> generateWaveformDisplayData(const std::string& trackName, size_t sampleCount);
};