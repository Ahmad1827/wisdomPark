#pragma once
#include <string>
#include <memory>
#include "Processing/SpriteAligner.h"

namespace StudioCore {

class Project;
class SpriteDefinition;

class PlaybackEngine {
public:
    PlaybackEngine() = default;

    void Update(float deltaTime, const Project* project);
    
    void SetAutoAlign(bool enabled);
    bool IsAutoAlignEnabled() const;
    AlignmentResult GetCurrentAlignment(const Project* project) const;

    void Play();
    void Pause();
    void Stop();
    
    void SetActiveAnimation(const std::string& animId);
    const std::string& GetActiveAnimation() const;

    bool IsPlaying() const;
    int GetCurrentFrameIndex() const;
    std::shared_ptr<SpriteDefinition> GetCurrentSprite(const Project* project) const;

private:
    std::string m_activeAnimationId;
    bool m_isPlaying{false};
    float m_playbackTimer{0.0f};
    int m_currentFrameIndex{0};
    bool m_autoAlignEnabled{false};
};

}