#include "Systems/PlaybackEngine.h"
#include "DataModels/Project.h"
#include "DataModels/AnimationGroup.h"
#include "DataModels/SpriteDefinition.h"
#include "Processing/SpriteAligner.h"

namespace StudioCore {

    
void PlaybackEngine::Update(float deltaTime, const Project* project) {
    if (!m_isPlaying || !project || m_activeAnimationId.empty()) {
        return;
    }

    auto anim = project->GetAnimationById(m_activeAnimationId);
    if (!anim || anim->GetFrames().empty()) {
        m_currentFrameIndex = 0;
        return;
    }

    float frameDuration = 1.0f / anim->GetFPS();
    m_playbackTimer += deltaTime;

    while (m_playbackTimer >= frameDuration) {
        m_playbackTimer -= frameDuration;
        m_currentFrameIndex++;

        if (m_currentFrameIndex >= static_cast<int>(anim->GetFrames().size())) {
            if (anim->IsLooping()) {
                m_currentFrameIndex = 0;
            } else {
                m_currentFrameIndex = static_cast<int>(anim->GetFrames().size()) - 1;
                m_isPlaying = false;
            }
        }
    }
}

void PlaybackEngine::Play() {
    m_isPlaying = true;
}

void PlaybackEngine::Pause() {
    m_isPlaying = false;
}

void PlaybackEngine::Stop() {
    m_isPlaying = false;
    m_currentFrameIndex = 0;
    m_playbackTimer = 0.0f;
}

void PlaybackEngine::SetActiveAnimation(const std::string& animId) {
    if (m_activeAnimationId != animId) {
        m_activeAnimationId = animId;
        Stop();
    }
}

const std::string& PlaybackEngine::GetActiveAnimation() const {
    return m_activeAnimationId;
}

bool PlaybackEngine::IsPlaying() const {
    return m_isPlaying;
}

int PlaybackEngine::GetCurrentFrameIndex() const {
    return m_currentFrameIndex;
}

std::shared_ptr<SpriteDefinition> PlaybackEngine::GetCurrentSprite(const Project* project) const {
    if (!project || m_activeAnimationId.empty()) return nullptr;
    
    auto anim = project->GetAnimationById(m_activeAnimationId);
    if (!anim || anim->GetFrames().empty()) return nullptr;
    
    int index = m_currentFrameIndex;
    if (index >= static_cast<int>(anim->GetFrames().size())) {
        index = static_cast<int>(anim->GetFrames().size()) - 1;
    }
    
    return project->GetSpriteById(anim->GetFrames()[index]);
}

void PlaybackEngine::SetAutoAlign(bool enabled) {
    m_autoAlignEnabled = enabled;
}

bool PlaybackEngine::IsAutoAlignEnabled() const {
    return m_autoAlignEnabled;
}

AlignmentResult PlaybackEngine::GetCurrentAlignment(const Project* project) const {
    auto sprite = GetCurrentSprite(project);
    if (sprite) {
        return SpriteAligner::ComputeAlignment(*sprite);
    }
    return AlignmentResult();
}

}