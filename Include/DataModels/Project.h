#pragma once
#include <string>
#include <vector>
#include <memory>
#include "SpriteDefinition.h"
#include "AnimationGroup.h"

namespace StudioCore {

class SourceTexture;

class Project {
public:
    Project();
    ~Project();

    void SetTexture(std::shared_ptr<SourceTexture> texture);
    std::shared_ptr<const SourceTexture> GetTexture() const;
    
    void SetImagePath(const std::string& path);
    std::string GetImagePath() const;

    void AddSprite(const SpriteDefinition& sprite);
    void RemoveSprite(const std::string& id);
    std::shared_ptr<SpriteDefinition> GetSpriteById(const std::string& id) const;
    const std::vector<std::shared_ptr<SpriteDefinition>>& GetSprites() const;

    void AddAnimationGroup(std::shared_ptr<AnimationGroup> group);
    void RemoveAnimationGroup(const std::string& id);
    std::shared_ptr<AnimationGroup> GetAnimationById(const std::string& id) const;
    const std::vector<std::shared_ptr<AnimationGroup>>& GetAnimationGroups() const;
    void SetSprites(const std::vector<std::shared_ptr<SpriteDefinition>>& newSprites);
private:
    std::shared_ptr<SourceTexture> m_texture;
    std::string m_imagePath;
    std::vector<std::shared_ptr<SpriteDefinition>> m_sprites;
    std::vector<std::shared_ptr<AnimationGroup>> m_animations;
};

}