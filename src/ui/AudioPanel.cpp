#include "AudioPanel.h"
#include "../core/NativeDialogs.h"
#include <iostream>
#include <cmath>
#include <algorithm>

AudioPanel::AudioPanel() : currentY(1080.f), targetY(1080.f), height(300.f), isVisible(false), selectedTrackIndex(-1), selectedClipIndex(-1) {}

AudioPanel::~AudioPanel() {
    stopAll();
}

void AudioPanel::init() {
    if (!font.loadFromFile("assets/font.otf")) {
        std::cout << "Warning: AudioPanel could not load font.otf" << std::endl;
    }

    background.setSize(sf::Vector2f(1920.f, height));
    background.setFillColor(sf::Color(20, 20, 25, 245));
    background.setOutlineThickness(2.f);
    background.setOutlineColor(sf::Color(80, 40, 120, 200));

    header.setSize(sf::Vector2f(1920.f, 40.f));
    header.setFillColor(sf::Color(30, 25, 40, 255));

    titleText.setFont(font);
    titleText.setString("Audio Sequence Editor");
    titleText.setCharacterSize(18);
    titleText.setFillColor(sf::Color::White);
    titleText.setPosition(20.f, 10.f);

    importBtn.setSize(sf::Vector2f(120.f, 26.f));
    importBtn.setFillColor(sf::Color(60, 120, 80));
    importBtn.setPosition(240.f, 7.f);

    importBtnText.setFont(font);
    importBtnText.setString("Import Audio");
    importBtnText.setCharacterSize(14);
    importBtnText.setFillColor(sf::Color::White);
    sf::FloatRect b1 = importBtnText.getLocalBounds();
    importBtnText.setOrigin(b1.left + b1.width / 2.f, b1.top + b1.height / 2.f);

    closeBtn.setSize(sf::Vector2f(40.f, 26.f));
    closeBtn.setFillColor(sf::Color(150, 50, 50));
    closeBtn.setPosition(1920.f - 60.f, 7.f);

    closeBtnText.setFont(font);
    closeBtnText.setString("X");
    closeBtnText.setCharacterSize(16);
    closeBtnText.setFillColor(sf::Color::White);
    sf::FloatRect b3 = closeBtnText.getLocalBounds();
    closeBtnText.setOrigin(b3.left + b3.width / 2.f, b3.top + b3.height / 2.f);
}

void AudioPanel::toggle() {
    isVisible = !isVisible;
    if (isVisible) targetY = 1080.f - height;
    else targetY = 1080.f;
}

bool AudioPanel::getIsVisible() const {
    return isVisible;
}

void AudioPanel::update(float dt) {
    if (!isVisible && std::abs(currentY - targetY) < 1.0f) {
        currentY = targetY;
        return;
    }

    currentY += (targetY - currentY) * 12.0f * dt;

    background.setPosition(0.f, currentY);
    header.setPosition(0.f, currentY);
    titleText.setPosition(20.f, currentY + 10.f);

    importBtn.setPosition(240.f, currentY + 7.f);
    importBtnText.setPosition(240.f + 60.f, currentY + 20.f);

    closeBtn.setPosition(1920.f - 60.f, currentY + 7.f);
    closeBtnText.setPosition(1920.f - 40.f, currentY + 20.f);
}

void AudioPanel::generateWaveform(AudioClip& clip, float w, float h) {
    if (!clip.buffer) return;

    const sf::Int16* samples = clip.buffer->getSamples();
    std::size_t count = clip.buffer->getSampleCount();
    int channels = clip.buffer->getChannelCount();

    clip.waveformRender.setPrimitiveType(sf::Lines);
    clip.waveformRender.clear();

    // Memory Failsafe: Prevent drawing if data is invalid or width is completely absurd
    if (count == 0 || channels == 0 || w <= 0.0f || w > 30000.f || !samples) {
        clip.needsWaveformUpdate = false;
        return;
    }

    std::size_t step = count / static_cast<std::size_t>(w);
    if (step < channels) step = channels;

    for (float x = 0; x < w; x += 1.0f) {
        std::size_t idx = static_cast<std::size_t>(x) * step;

        // Ensure we always align to a valid sample chunk so we don't bleed across channels
        idx -= (idx % channels);

        if (idx >= count) break;

        float val = static_cast<float>(samples[idx]) / 32768.0f;
        float yExt = (val * h / 2.0f);

        sf::Color waveCol = sf::Color(100, 200, 255, 180);
        clip.waveformRender.append(sf::Vertex(sf::Vector2f(x, h / 2.0f - yExt), waveCol));
        clip.waveformRender.append(sf::Vertex(sf::Vector2f(x, h / 2.0f + yExt), waveCol));
    }
    clip.needsWaveformUpdate = false;
}

void AudioPanel::draw(sf::RenderWindow& window) {
    if (currentY >= 1080.f) return;

    window.draw(background);
    window.draw(header);
    window.draw(titleText);

    window.draw(importBtn);
    window.draw(importBtnText);

    window.draw(closeBtn);
    window.draw(closeBtnText);

    float trackY = currentY + 50.f;
    for (size_t i = 0; i < tracks.size(); ++i) {
        sf::RectangleShape trackBg(sf::Vector2f(1920.f, 60.f));
        trackBg.setPosition(0.f, trackY);
        trackBg.setFillColor(i % 2 == 0 ? sf::Color(25, 25, 30, 200) : sf::Color(35, 35, 40, 200));
        window.draw(trackBg);

        sf::Text tName(tracks[i].name, font, 14);
        tName.setPosition(10.f, trackY + 20.f);
        tName.setFillColor(sf::Color(150, 150, 150));
        window.draw(tName);

        sf::RectangleShape timelineArea(sf::Vector2f(1700.f, 50.f));
        timelineArea.setPosition(200.f, trackY + 5.f);
        timelineArea.setFillColor(sf::Color(15, 15, 15, 150));
        window.draw(timelineArea);

        for (auto& clip : tracks[i].clips) {
            float pixelsPerFrame = 5.0f;
            float clipX = 200.f + (clip.startFrame * pixelsPerFrame);
            float clipW = (clip.endFrame - clip.startFrame) * pixelsPerFrame;

            // Failsafe Width Clamp to prevent sf::RectangleShape allocation crashes
            float renderWidth = std::min(15000.f, std::max(10.f, clipW));

            sf::RectangleShape clipBg(sf::Vector2f(renderWidth, 40.f));
            clipBg.setPosition(clipX, trackY + 10.f);
            clipBg.setFillColor(sf::Color(50, 100, 150, 100));
            clipBg.setOutlineThickness(1.f);
            clipBg.setOutlineColor(sf::Color(100, 200, 255));
            window.draw(clipBg);

            if (clip.needsWaveformUpdate) {
                generateWaveform(clip, renderWidth, 40.f);
            }

            sf::Transform t;
            t.translate(clipX, trackY + 10.f);
            window.draw(clip.waveformRender, t);
        }
        trackY += 65.f;
    }
}

bool AudioPanel::handleEvent(const sf::Event& event, sf::Vector2f mousePos) {
    if (!isVisible) return false;
    if (!background.getGlobalBounds().contains(mousePos)) return false;
    return true;
}

std::string AudioPanel::handleClick(sf::Vector2f mousePos) {
    if (!isVisible) return "";

    if (closeBtn.getGlobalBounds().contains(mousePos)) {
        toggle();
        return "closed";
    }

    if (importBtn.getGlobalBounds().contains(mousePos)) {
        std::string file = NativeDialogs::openFileDialog("Audio Files\0*.wav;*.mp3;*.ogg;*.flac\0All Files\0*.*\0");
        if (!file.empty()) {
            addAudioClip(file, 0);
            return "imported";
        }
    }

    return "";
}

void AudioPanel::addAudioClip(const std::string& path, int currentFrame) {
    try {
        auto buf = std::make_shared<sf::SoundBuffer>();
        if (buf->loadFromFile(path)) {
            AudioClip clip;
            clip.filePath = path;
            clip.name = path.substr(path.find_last_of("/\\") + 1);
            clip.buffer = buf;
            clip.sound = std::make_shared<sf::Sound>(*buf);
            clip.startFrame = currentFrame;

            float durationSec = buf->getDuration().asSeconds();
            // Sanity check to prevent NaN frame bugs
            if (std::isnan(durationSec) || durationSec <= 0.0f) durationSec = 1.0f;

            clip.endFrame = currentFrame + static_cast<int>(durationSec * 12.0f);

            if (tracks.empty()) {
                AudioTrack t;
                t.name = "Track 1";
                tracks.push_back(t);
            }
            tracks[0].clips.push_back(clip);
        }
    }
    catch (const std::exception& e) {
        std::cout << "Safely caught audio loading exception: " << e.what() << std::endl;
    }
    catch (...) {
        std::cout << "Safely caught unknown audio loading exception." << std::endl;
    }
}

void AudioPanel::updatePlayback(int currentFrame, float fps, bool isPlaying) {
    if (tracks.empty()) return;

    if (!isPlaying) {
        stopAll();
        return;
    }

    for (auto& track : tracks) {
        if (track.isMuted) continue;
        for (auto& clip : track.clips) {
            if (!clip.sound) continue;

            if (currentFrame == clip.startFrame) {
                if (clip.sound->getStatus() != sf::SoundSource::Playing) {
                    clip.sound->setVolume(clip.volume * (track.trackVolume / 100.f));
                    clip.sound->setPitch(clip.pitch);
                    clip.sound->play();
                }
            }
            else if (currentFrame > clip.endFrame || currentFrame < clip.startFrame) {
                if (clip.sound->getStatus() == sf::SoundSource::Playing) {
                    clip.sound->stop();
                }
            }
        }
    }
}

void AudioPanel::stopAll() {
    if (tracks.empty()) return;
    for (auto& track : tracks) {
        for (auto& clip : track.clips) {
            if (clip.sound && clip.sound->getStatus() == sf::SoundSource::Playing) {
                clip.sound->stop();
            }
        }
    }
}