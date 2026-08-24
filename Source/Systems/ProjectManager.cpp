#include "Systems/ProjectManager.h"
#include "DataModels/Project.h"
#include "DataModels/AnimationGroup.h"
#include "DataModels/SourceTexture.h"
#include "Processing/ImageLoader.h"
#include "json.hpp"
#include <fstream>
#include <set>
#include <filesystem>
#include <iostream>
#include <cctype>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace StudioCore {

static const std::string b64_chars = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

static std::string base64_encode(const uint8_t* buf, size_t bufLen) {
    std::string ret;
    int i = 0;
    int j = 0;
    uint8_t char_array_3[3];
    uint8_t char_array_4[4];

    while (bufLen--) {
        char_array_3[i++] = *(buf++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; i < 4; i++) ret += b64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++) char_array_3[j] = '\0';
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        for (j = 0; j < i + 1; j++) ret += b64_chars[char_array_4[j]];
        while (i++ < 3) ret += '=';
    }
    return ret;
}

static std::vector<uint8_t> base64_decode(const std::string& encoded_string) {
    size_t in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    uint8_t char_array_4[4], char_array_3[3];
    std::vector<uint8_t> ret;

    auto is_base64 = [](uint8_t c) -> bool {
        return (std::isalnum(c) || (c == '+') || (c == '/'));
    };

    while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = static_cast<uint8_t>(b64_chars.find(char_array_4[i]));

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; i < 3; i++) ret.push_back(char_array_3[i]);
            i = 0;
        }
    }

    if (i) {
        for (j = 0; j < i; j++)
            char_array_4[j] = static_cast<uint8_t>(b64_chars.find(char_array_4[j]));

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);

        for (j = 0; j < i - 1; j++) ret.push_back(char_array_3[j]);
    }

    return ret;
}

bool ProjectManager::SaveProject(const Project& project, const std::string& filePath) {
    if (filePath.empty()) return false;

    try {
        fs::path p(filePath);
        if (p.has_parent_path()) {
            fs::create_directories(p.parent_path());
        }

        json j;
        j["imagePath"] = project.GetImagePath();

        auto texture = project.GetTexture();
        if (texture && texture->IsValid()) {
            json texObj;
            texObj["width"] = texture->GetWidth();
            texObj["height"] = texture->GetHeight();
            texObj["pixels"] = base64_encode(texture->GetPixels().data(), texture->GetPixels().size());
            j["embeddedTexture"] = texObj;
        }
        
        json spritesArray = json::array();
        for (const auto& s : project.GetSprites()) {
            const auto& rect = s->GetSourceRect();
            const auto& pivot = s->GetPivot();
            const auto& center = s->GetCenter();

            spritesArray.push_back({
                {"id", s->GetId()},
                {"x", rect.x},
                {"y", rect.y},
                {"width", rect.width},
                {"height", rect.height},
                {"pivotX", pivot.x},
                {"pivotY", pivot.y},
                {"baseline", s->GetBaseline()},
                {"pixelCount", s->GetPixelCount()},
                {"centerX", center.x},
                {"centerY", center.y}
            });
        }
        j["sprites"] = spritesArray;

        json animsArray = json::array();
        for (const auto& a : project.GetAnimationGroups()) {
            animsArray.push_back({
                {"id", a->GetId()},
                {"name", a->GetName()},
                {"fps", a->GetFPS()},
                {"looping", a->IsLooping()},
                {"frames", a->GetFrames()}
            });
        }
        j["animations"] = animsArray;

        std::ofstream file(p.string());
        if (!file.is_open()) {
            std::cerr << "[X] Failed to open path for writing: " << p.string() << std::endl;
            return false;
        }
        file << j.dump(4);
        std::cout << "[✓] Project file successfully written to: " << p.string() << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[X] Exception during project save: " << e.what() << std::endl;
        return false;
    }
}

std::shared_ptr<Project> ProjectManager::LoadProject(const std::string& filePath, std::string& outError) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        outError = "Could not open project file: " + filePath;
        return nullptr;
    }

    json j;
    try {
        file >> j;
    } catch (const json::parse_error& e) {
        outError = "Corrupted JSON project file.";
        return nullptr;
    }

    std::string imgPath = j.value("imagePath", "");
    auto project = std::make_shared<Project>();
    project->SetImagePath(imgPath);

    if (j.contains("embeddedTexture")) {
        int w = j["embeddedTexture"]["width"];
        int h = j["embeddedTexture"]["height"];
        std::string b64 = j["embeddedTexture"]["pixels"];
        std::vector<uint8_t> pixels = base64_decode(b64);
        auto texture = std::make_shared<SourceTexture>(w, h, std::move(pixels));
        project->SetTexture(texture);
    } else {
        std::ifstream imgCheck(imgPath);
        if (!imgCheck.good()) {
            outError = "Missing source image at stored path: " + imgPath;
            return nullptr;
        }
        auto texture = ImageLoader::LoadFromFile(imgPath, outError);
        if (!texture) return nullptr;
        project->SetTexture(std::move(texture));
    }

    std::set<std::string> seenIds;
    if (j.contains("sprites")) {
        for (const auto& s : j["sprites"]) {
            std::string id = s["id"];
            if (seenIds.find(id) != seenIds.end()) {
                outError = "Duplicate sprite ID detected: " + id;
                return nullptr;
            }
            seenIds.insert(id);

            Rect rect{s["x"], s["y"], s["width"], s["height"]};
            SpriteDefinition def(id, rect);
            def.SetPivot({s["pivotX"], s["pivotY"]});
            def.SetBaseline(s["baseline"]);
            
            if (s.contains("pixelCount")) {
                def.SetPixelCount(s["pixelCount"]);
            }
            if (s.contains("centerX") && s.contains("centerY")) {
                def.SetCenter({s["centerX"], s["centerY"]});
            }

            project->AddSprite(def);
        }
    }

    if (j.contains("animations")) {
        for (const auto& a : j["animations"]) {
            auto anim = std::make_shared<AnimationGroup>(a["id"], a["name"]);
            anim->SetFPS(a["fps"]);
            anim->SetLooping(a["looping"]);
            std::vector<std::string> frames = a["frames"];
            anim->SetFrames(frames);
            project->AddAnimationGroup(anim);
        }
    }

    return project;
}

}