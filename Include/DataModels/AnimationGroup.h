#pragma once
#include <string>
#include <vector>

namespace StudioCore {

class AnimationGroup {
public:
    AnimationGroup(std::string id, std::string name);

    const std::string& GetId() const;
    
    void SetName(const std::string& name);
    const std::string& GetName() const;

    void SetFPS(float fps);
    float GetFPS() const;

    void SetLooping(bool looping);
    bool IsLooping() const;

    void SetFrames(const std::vector<std::string>& spriteIds);
    const std::vector<std::string>& GetFrames() const;

private:
    std::string m_id;
    std::string m_name;
    float m_fps{12.0f};
    bool m_looping{true};
    std::vector<std::string> m_frames;
};

}