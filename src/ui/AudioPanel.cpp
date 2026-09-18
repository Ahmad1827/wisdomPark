#include "AudioPanel.h"
#include "../UI/UITheme.h"
#include <algorithm>
#include <cmath>
#include <filesystem>

AudioPanel::AudioPanel() : position(1240.f, 75.f), size(630.f, 360.f), isVisible(false) {}

AudioPanel::~AudioPanel() {}

void AudioPanel::init() {
    font.loadFromFile("assets/font.otf");
    position = sf::Vector2f(1240.f, 75.f);

    tracks.clear();
    AudioTrack defaultTrack;
    defaultTrack.name = "Track 1";
    tracks.push_back(defaultTrack);
}

void AudioPanel::toggle() {
    isVisible = !isVisible;
}

bool AudioPanel::getIsVisible() const {
    return isVisible;
}

void AudioPanel::update(float dt) {}

void AudioPanel::generateWaveform(AudioClip& clip, float w, float h) {
    clip.waveformRender.setPrimitiveType(sf::Lines);
    clip.waveformRender.clear();

    if (w <= 1.0f || w > 30000.f) {
        clip.needsWaveformUpdate = false;
        return;
    }

    for (float x = 0; x < w; x += 2.0f) {
        float pseudoRandomAmp = std::sin(x * 0.1f) * std::cos(x * 0.05f) * (h / 2.5f);
        if (x > w * 0.8f) pseudoRandomAmp *= (1.0f - (x - w * 0.8f) / (w * 0.2f));

        sf::Color waveCol = sf::Color(255, 154, 118, 160);
        clip.waveformRender.append(sf::Vertex(sf::Vector2f(x, h / 2.0f - pseudoRandomAmp), waveCol));
        clip.waveformRender.append(sf::Vertex(sf::Vector2f(x, h / 2.0f + pseudoRandomAmp), waveCol));
    }
    clip.needsWaveformUpdate = false;
}

void AudioPanel::drawTooltip(sf::RenderWindow& window, const std::string& text, sf::Vector2f pos) {
    sf::Text tipText(text, font, 13);
    sf::FloatRect bounds = tipText.getLocalBounds();

    float padX = 10.f;
    float padY = 6.f;
    float w = bounds.width + padX * 2.f;
    float h = bounds.height + padY * 2.f + 4.f;

    float x = pos.x - w - 12.f;
    float y = pos.y + 14.f;

    if (x < 10.f) x = pos.x + 16.f;
    if (y + h > 1070.f) y = pos.y - h - 6.f;

    sf::RectangleShape bg(sf::Vector2f(w, h));
    bg.setPosition(x, y);
    bg.setFillColor(sf::Color(18, 12, 24, 250));
    bg.setOutlineThickness(1.5f);
    bg.setOutlineColor(WisdomUI::Theme::Gold);

    tipText.setPosition(x + padX, y + padY - 2.f);
    tipText.setFillColor(sf::Color::White);

    window.draw(bg);
    window.draw(tipText);
}

void AudioPanel::draw(sf::RenderWindow& window) {
    if (!isVisible) return;

    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    std::string hoveredTooltip = "";

    sf::FloatRect panelBounds(position.x, position.y, size.x, size.y);
    WisdomUI::Theme::DrawSunsetPanel(window, panelBounds, 1.0f);

    sf::FloatRect headerGrip(position.x + 8.f, position.y + 6.f, size.x - 16.f, 32.f);
    sf::RectangleShape gripBg(sf::Vector2f(headerGrip.width, headerGrip.height));
    gripBg.setPosition(headerGrip.left, headerGrip.top);
    gripBg.setFillColor(WisdomUI::Theme::SunsetDeepDark);
    gripBg.setOutlineThickness(1.f);
    gripBg.setOutlineColor(WisdomUI::Theme::SunsetPlum);
    window.draw(gripBg);

    WisdomUI::Theme::DrawCrispText(window, font, ":: AUDIO TIMELINE ENGINE ::", 16, headerGrip.left + headerGrip.width / 2.0f, headerGrip.top + headerGrip.height / 2.0f, WisdomUI::Theme::SunsetAmber, sf::Color(14, 6, 20), true, true);

    scanBtnBounds = sf::FloatRect(position.x + 14.f, position.y + 44.f, 180.f, 30.f);
    bool hovScan = scanBtnBounds.contains(mPos);
    if (hovScan) hoveredTooltip = "Scan assets/audio directory for audio files";
    WisdomUI::Theme::DrawSunsetButton(window, scanBtnBounds, "Scan assets/audio", font, 14, false, hovScan, true, 1.0f);

    closeBtnBounds = sf::FloatRect(position.x + size.x - 38.f, position.y + 7.f, 30.f, 30.f);
    bool hovClose = closeBtnBounds.contains(mPos);
    if (hovClose) hoveredTooltip = "Close Audio Panel";
    WisdomUI::Theme::DrawSunsetButton(window, closeBtnBounds, "X", font, 14, false, hovClose, false, 1.0f);

    float trackY = position.y + 84.f;
    for (size_t i = 0; i < tracks.size(); ++i) {
        sf::FloatRect trackBgRect(position.x + 12.f, trackY, size.x - 24.f, 70.f);
        sf::RectangleShape trackBg(sf::Vector2f(trackBgRect.width, trackBgRect.height));
        trackBg.setPosition(trackBgRect.left, trackBgRect.top);
        trackBg.setFillColor(WisdomUI::Theme::SunsetDeepDark);
        trackBg.setOutlineThickness(1.f);
        trackBg.setOutlineColor(WisdomUI::Theme::SunsetPlum);
        window.draw(trackBg);

        WisdomUI::Theme::DrawCrispText(window, font, tracks[i].name, 14, trackBgRect.left + 12.f, trackBgRect.top + 26.f, WisdomUI::Theme::SunsetAmber);

        sf::FloatRect timelineArea(trackBgRect.left + 98.f, trackBgRect.top + 6.f, trackBgRect.width - 108.f, 58.f);
        sf::RectangleShape tArea(sf::Vector2f(timelineArea.width, timelineArea.height));
        tArea.setPosition(timelineArea.left, timelineArea.top);
        tArea.setFillColor(WisdomUI::Theme::SunsetSkyTop);
        window.draw(tArea);

        if (timelineArea.contains(mPos)) hoveredTooltip = "Audio timeline track channel";

        for (auto& clip : tracks[i].clips) {
            float pixelsPerFrame = 4.0f;
            float clipX = timelineArea.left + (clip.startFrame * pixelsPerFrame);
            float clipW = (clip.endFrame - clip.startFrame) * pixelsPerFrame;
            float renderWidth = std::clamp(clipW, 10.f, timelineArea.width - 4.f);

            sf::FloatRect clipRect(clipX, timelineArea.top + 5.f, renderWidth, 48.f);
            if (clipRect.contains(mPos)) hoveredTooltip = "Audio clip: " + clip.name;

            sf::RectangleShape clipBg(sf::Vector2f(renderWidth, 48.f));
            clipBg.setPosition(clipX, timelineArea.top + 5.f);
            clipBg.setFillColor(WisdomUI::Theme::SunsetPlum);
            clipBg.setOutlineThickness(1.f);
            clipBg.setOutlineColor(WisdomUI::Theme::SunsetCoral);
            window.draw(clipBg);

            if (clip.needsWaveformUpdate) {
                generateWaveform(clip, renderWidth, 48.f);
            }

            sf::Transform t;
            t.translate(clipX, timelineArea.top + 5.f);
            window.draw(clip.waveformRender, t);
        }
        trackY += 78.f;
    }

    if (!hoveredTooltip.empty()) {
        drawTooltip(window, hoveredTooltip, mPos);
    }
}

bool AudioPanel::handleEvent(const sf::Event& event, sf::Vector2f mousePos) {
    if (!isVisible) return false;

    sf::FloatRect headerGrip(position.x, position.y, size.x, 34.f);

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (headerGrip.contains(mousePos)) {
            isDraggingPanel = true;
            dragOffset = mousePos - position;
            return true;
        }
        return sf::FloatRect(position.x, position.y, size.x, size.y).contains(mousePos);
    }
    else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        isDraggingPanel = false;
    }
    else if (event.type == sf::Event::MouseMoved && isDraggingPanel) {
        position = mousePos - dragOffset;
        position.x = std::clamp(position.x, 56.f, 1920.f - size.x);
        position.y = std::clamp(position.y, 40.f, 1080.f - size.y);
    }
    return sf::FloatRect(position.x, position.y, size.x, size.y).contains(mousePos);
}

std::string AudioPanel::handleClick(sf::Vector2f mousePos, int currentFrame) {
    if (!isVisible) return "";

    if (closeBtnBounds.contains(mousePos)) {
        toggle();
        return "closed";
    }

    if (scanBtnBounds.contains(mousePos)) {
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

    cleanClip.startFrame = currentFrame;
    cleanClip.endFrame = currentFrame + 60;

    cleanClip.needsWaveformUpdate = true;
    cleanClip.soundLoadFailed = true;

    if (tracks.empty()) {
        AudioTrack t;
        t.name = "Track 1";
        tracks.push_back(t);
    }

    tracks[0].clips.push_back(std::move(cleanClip));
}

void AudioPanel::updatePlayback(int currentFrame, float fps, bool isPlaying) {}
void AudioPanel::stopAll() {}