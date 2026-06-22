#include <SFML/Audio.hpp>
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>

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
    AudioManager() = default;
    ~AudioManager() = default;

    bool importAudio(const std::string& trackName, const std::string& path) {
        AudioTrack track;
        if (!track.buffer.loadFromFile(path)) {
            return false;
        }
        track.sound.setBuffer(track.buffer);
        track.filePath = path;
        track.name = trackName;
        track.volume = 100.0f;
        track.isMuted = false;
        track.offsetTime = 0.0f;
        tracks[trackName] = track;
        return true;
    }

    void removeTrack(const std::string& trackName) {
        if (tracks.find(trackName) != tracks.end()) {
            tracks[trackName].sound.stop();
            tracks.erase(trackName);
        }
    }

    void playTrack(const std::string& trackName) {
        if (tracks.find(trackName) != tracks.end() && !tracks[trackName].isMuted && !globalMute) {
            tracks[trackName].sound.play();
        }
    }

    void pauseTrack(const std::string& trackName) {
        if (tracks.find(trackName) != tracks.end()) {
            tracks[trackName].sound.pause();
        }
    }

    void stopTrack(const std::string& trackName) {
        if (tracks.find(trackName) != tracks.end()) {
            tracks[trackName].sound.stop();
        }
    }

    void playAll() {
        for (auto& pair : tracks) {
            if (!pair.second.isMuted && !globalMute) {
                pair.second.sound.play();
            }
        }
    }

    void pauseAll() {
        for (auto& pair : tracks) {
            pair.second.sound.pause();
        }
    }

    void stopAll() {
        for (auto& pair : tracks) {
            pair.second.sound.stop();
        }
    }

    void setTrackVolume(const std::string& trackName, float volume) {
        if (tracks.find(trackName) != tracks.end()) {
            tracks[trackName].volume = std::max(0.0f, std::min(volume, 100.0f));
            if (!tracks[trackName].isMuted && !globalMute) {
                tracks[trackName].sound.setVolume(tracks[trackName].volume * (masterVolume / 100.0f));
            }
        }
    }

    void muteTrack(const std::string& trackName, bool mute) {
        if (tracks.find(trackName) != tracks.end()) {
            tracks[trackName].isMuted = mute;
            if (mute) {
                tracks[trackName].sound.setVolume(0.0f);
            }
            else {
                tracks[trackName].sound.setVolume(tracks[trackName].volume * (masterVolume / 100.0f));
            }
        }
    }

    void setMasterVolume(float volume) {
        masterVolume = std::max(0.0f, std::min(volume, 100.0f));
        for (auto& pair : tracks) {
            if (!pair.second.isMuted && !globalMute) {
                pair.second.sound.setVolume(pair.second.volume * (masterVolume / 100.0f));
            }
        }
    }

    void toggleGlobalMute(bool mute) {
        globalMute = mute;
        for (auto& pair : tracks) {
            if (globalMute || pair.second.isMuted) {
                pair.second.sound.setVolume(0.0f);
            }
            else {
                pair.second.sound.setVolume(pair.second.volume * (masterVolume / 100.0f));
            }
        }
    }

    void syncWithAnimation(float currentPlaybackTime) {
        for (auto& pair : tracks) {
            if (pair.second.sound.getStatus() == sf::Sound::Playing) {
                float expectedTime = currentPlaybackTime - pair.second.offsetTime;
                if (expectedTime >= 0.0f && expectedTime < pair.second.buffer.getDuration().asSeconds()) {
                    float delta = pair.second.sound.getPlayingOffset().asSeconds() - expectedTime;
                    if (std::abs(delta) > 0.05f) {
                        pair.second.sound.setPlayingOffset(sf::seconds(expectedTime));
                    }
                }
            }
        }
    }

    std::vector<int16_t> generateWaveformDisplayData(const std::string& trackName, size_t sampleCount) {
        std::vector<int16_t> renderedWaveform;
        if (tracks.find(trackName) == tracks.end()) {
            return renderedWaveform;
        }
        const sf::SoundBuffer& buf = tracks[trackName].buffer;
        uint64_t totalSamples = buf.getSampleCount();
        if (totalSamples == 0 || sampleCount == 0) return renderedWaveform;

        uint64_t chunk = totalSamples / sampleCount;
        if (chunk == 0) chunk = 1;
        const int16_t* samples = buf.getSamples();

        for (size_t i = 0; i < sampleCount; ++i) {
            int maxVal = 0;
            for (uint64_t j = 0; j < chunk && (i * chunk + j) < totalSamples; ++j) {
                int currentSample = static_cast<int>(samples[i * chunk + j]);
                int absVal = std::abs(currentSample);
                maxVal = std::max(maxVal, absVal);
            }
            renderedWaveform.push_back(static_cast<int16_t>(maxVal));
        }
        return renderedWaveform;
    }
};