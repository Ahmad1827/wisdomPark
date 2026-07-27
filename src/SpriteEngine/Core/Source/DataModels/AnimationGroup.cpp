#include "DataModels/AnimationGroup.h"
#include <utility>

namespace StudioCore {

AnimationGroup::AnimationGroup(std::string id, std::string name)
    : m_id(std::move(id)), m_name(std::move(name)) {}

const std::string& AnimationGroup::GetId() const {
    return m_id;
}

void AnimationGroup::SetName(const std::string& name) {
    m_name = name;
}

const std::string& AnimationGroup::GetName() const {
    return m_name;
}

void AnimationGroup::SetFPS(float fps) {
    m_fps = fps;
}

float AnimationGroup::GetFPS() const {
    return m_fps;
}

void AnimationGroup::SetLooping(bool looping) {
    m_looping = looping;
}

bool AnimationGroup::IsLooping() const {
    return m_looping;
}

void AnimationGroup::SetFrames(const std::vector<std::string>& spriteIds) {
    m_frames = spriteIds;
}

const std::vector<std::string>& AnimationGroup::GetFrames() const {
    return m_frames;
}

}