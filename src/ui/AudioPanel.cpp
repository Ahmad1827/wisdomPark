#include "AudioPanel.h"
#include "../core/NativeDialogs.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <filesystem>

AudioPanel::AudioPanel() : currentY(1080.f), targetY(1080.f), height(300.f), isVisible(false), selectedTrackIndex(-1), selectedClipIndex(-1) {}

AudioPanel::~AudioPanel() {}

void AudioPanel::init() {
    if (!font.loadFromFile("assets/font.otf")) {}

    background.setSize(sf::Vector2f(1920.f, height));
    background.setFillColor(sf::Color(20, 20, 25, 245));
    background.setOutlineThickness(2.f);
    background.setOutlineColor(sf::Color(80, 40, 120, 200));

    header.setSize(sf::Vector2f(1920.f, 40.f));
    header.setFillColor(sf::Color(30, 25, 40, 255));

    titleText.setFont(font);
    titleText.setString("Audio Sequence Editor (Hardware Safe Mode)");
    titleText.setCharacterSize(18);
    titleText.setFillColor(sf::Color::White);
    titleText.setPosition(20.f, 10.f);

    importBtn.setSize(sf::Vector2f(180.f, 26.f));
    importBtn.setFillColor(sf::Color(60, 120, 80));
    importBtn.setPosition(470.f, 7.f);

    importBtnText.setFont(font);
    importBtnText.setString("Scan assets/audio");
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

    tracks.clear();
    AudioTrack defaultTrack;
    defaultTrack.name = "Track 1";
    tracks.push_back(defaultTrack);
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

    importBtn.setPosition(470.f, currentY + 7.f);
    importBtnText.setPosition(470.f + 90.f, currentY + 20.f);

    closeBtn.setPosition(1920.f - 60.f, currentY + 7.f);
    closeBtnText.setPosition(1920.f - 40.f, currentY + 20.f);
}

void AudioPanel::generateWaveform(AudioClip& clip, float w, float h) {
    clip.waveformRender.setPrimitiveType(sf::Lines);
    clip.waveformRender.clear();

    if (w <= 1.0f || w > 30000.f) {
        clip.needsWaveformUpdate = false;
        return;
    }

    // Procedural fake waveform generator (requires no memory buffer or decoding)
    for (float x = 0; x < w; x += 2.0f) {
        float pseudoRandomAmp = std::sin(x * 0.1f) * std::cos(x * 0.05f) * (h / 2.5f);
        if (x > w * 0.8f) pseudoRandomAmp *= (1.0f - (x - w * 0.8f) / (w * 0.2f));

        sf::Color waveCol = sf::Color(100, 200, 255, 140);
        clip.waveformRender.append(sf::Vertex(sf::Vector2f(x, h / 2.0f - pseudoRandomAmp), waveCol));
        clip.waveformRender.append(sf::Vertex(sf::Vector2f(x, h / 2.0f + pseudoRandomAmp), waveCol));
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

            float renderWidth = std::min(15000.f, std::max(10.f, clipW));

            sf::RectangleShape clipBg(sf::Vector2f(renderWidth, 40.f));
            clipBg.setPosition(clipX, trackY + 10.f);
            clipBg.setFillColor(sf::Color(80, 40, 120, 100));
            clipBg.setOutlineThickness(1.f);
            clipBg.setOutlineColor(sf::Color(160, 100, 255));
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
    if (!isVisible || !background.getGlobalBounds().contains(mousePos)) return false;
    return true;
}

std::string AudioPanel::handleClick(sf::Vector2f mousePos, int currentFrame) {
    if (!isVisible) return "";

    if (closeBtn.getGlobalBounds().contains(mousePos)) {
        toggle();
        return "closed";
    }

    if (importBtn.getGlobalBounds().contains(mousePos)) {
        scanAudioDirectory(currentFrame);
        return "imported";
    }

    return "";
}

void AudioPanel::scanAudioDirectory(int currentFrame) {
    std::string path = "assets/audio";
    if (!std::filesystem::exists(path)) {
        std::filesystem::create_directories(path);
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext == ".wav" || ext == ".ogg" || ext == ".flac" || ext == ".mp3") {
            std::string fullPath = entry.path().string();
            bool alreadyLoaded = false;
            for (auto& t : tracks) {
                for (auto& c : t.clips) {
                    if (c.filePath == fullPath) alreadyLoaded = true;
                }
            }
            if (!alreadyLoaded) {
                addAudioClip(fullPath, currentFrame);
            }
        }
    }
}

void AudioPanel::addAudioClip(const std::string& path, int currentFrame) {
    AudioClip cleanClip;
    cleanClip.filePath = path;

    std::string filename = path;
    size_t slashPos = filename.find_last_of("/\\");
    if (slashPos != std::string::npos) filename = filename.substr(slashPos + 1);
    cleanClip.name = filename;

    // Hardcode a default duration (5 seconds = 60 frames) so it looks good on the timeline
    cleanClip.startFrame = currentFrame;
    cleanClip.endFrame = currentFrame + 60;

    cleanClip.needsWaveformUpdate = true;
    cleanClip.soundLoadFailed = true; // Permanently blocks hardware access

    if (tracks.empty()) {
        AudioTrack t;
        t.name = "Track 1";
        tracks.push_back(t);
    }

    tracks[0].clips.push_back(std::move(cleanClip));
}

bool AudioPanel::ensureSoundCreated(AudioClip& clip) {
    return false; // Safely shut down all OpenAL hardware calls
}

void AudioPanel::updatePlayback(int currentFrame, float fps, bool isPlaying) {
    // Bypassed for Safe Mode
}

void AudioPanel::stopAll() {
    // Bypassed for Safe Mode
}