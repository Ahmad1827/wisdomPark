#include "DataModels/Project.h"
#include <algorithm>

namespace StudioCore {

Project::Project() = default;
Project::~Project() = default;

void Project::SetTexture(std::shared_ptr<SourceTexture> texture) {
    m_texture = std::move(texture);
}

std::shared_ptr<const SourceTexture> Project::GetTexture() const {
    return m_texture;
}

void Project::SetImagePath(const std::string& path) {
    m_imagePath = path;
}

std::string Project::GetImagePath() const {
    return m_imagePath;
}

void Project::AddSprite(const SpriteDefinition& sprite) {
    m_sprites.push_back(std::make_shared<SpriteDefinition>(sprite));
}

void Project::RemoveSprite(const std::string& id) {
    m_sprites.erase(
        std::remove_if(m_sprites.begin(), m_sprites.end(),
            [&id](const std::shared_ptr<SpriteDefinition>& sprite) {
                return sprite && sprite->GetId() == id;
            }),
        m_sprites.end()
    );
}

std::shared_ptr<SpriteDefinition> Project::GetSpriteById(const std::string& id) const {
    for (const auto& s : m_sprites) {
        if (s->GetId() == id) return s;
    }
    return nullptr;
}

const std::vector<std::shared_ptr<SpriteDefinition>>& Project::GetSprites() const {
    return m_sprites;
}

void Project::AddAnimationGroup(std::shared_ptr<AnimationGroup> group) {
    m_animations.push_back(std::move(group));
}

void Project::RemoveAnimationGroup(const std::string& id) {
    m_animations.erase(std::remove_if(m_animations.begin(), m_animations.end(),
        [&id](const auto& a) { return a->GetId() == id; }), m_animations.end());
}

std::shared_ptr<AnimationGroup> Project::GetAnimationById(const std::string& id) const {
    for (const auto& a : m_animations) {
        if (a->GetId() == id) return a;
    }
    return nullptr;
}

const std::vector<std::shared_ptr<AnimationGroup>>& Project::GetAnimationGroups() const {
    return m_animations;
}

}