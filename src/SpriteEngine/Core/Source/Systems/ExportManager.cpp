#include "Systems/ExportManager.h"
#include "Processing/AtlasPacker.h"
#include "DataModels/Project.h"
#include "DataModels/AnimationGroup.h"
#include "json.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace StudioCore {

sf::Image ExportManager::GeneratePreview(const Project& project, int padding) const {
    return AtlasPacker::PackUniformGrid(project, padding).image;
}

bool ExportManager::ExportPNG(const Project& project, const std::string& filePath, int padding) const {
    if (filePath.empty()) return false;

    fs::path exportPath(filePath);
    if (exportPath.has_parent_path()) {
        fs::create_directories(exportPath.parent_path());
    }

    AtlasResult result = AtlasPacker::PackUniformGrid(project, padding);
    if (result.image.getSize().x == 0 || result.image.getSize().y == 0) {
        std::cerr << "[X] Export failed: Generated atlas grid size is 0x0." << std::endl;
        return false;
    }

    if (!result.image.saveToFile(exportPath.string())) {
        std::cerr << "[X] Failed to save PNG to: " << exportPath.string() << std::endl;
        return false;
    }

    fs::path parentDir = exportPath.parent_path();
    fs::path atlasJsonPath = parentDir.empty() ? "atlas.json" : parentDir / "atlas.json";
    fs::path animJsonPath = parentDir.empty() ? "animations.json" : parentDir / "animations.json";

    json atlasJson = json::array();
    for (const auto& s : result.sprites) {
        atlasJson.push_back({
            {"id", s.id},
            {"name", s.name},
            {"x", s.x},
            {"y", s.y},
            {"width", s.width},
            {"height", s.height},
            {"pivot", {{"x", s.pivotX}, {"y", s.pivotY}}},
            {"baseline", s.baseline}
        });
    }
    std::ofstream atlasFile(atlasJsonPath.string());
    atlasFile << atlasJson.dump(4);

    json animJson = json::array();
    for (const auto& a : project.GetAnimationGroups()) {
        animJson.push_back({
            {"name", a->GetName()},
            {"fps", a->GetFPS()},
            {"looping", a->IsLooping()},
            {"frames", a->GetFrames()}
        });
    }
    std::ofstream animFile(animJsonPath.string());
    animFile << animJson.dump(4);

    std::cout << "[✓] Export successful!" << std::endl;
    std::cout << "    - PNG: " << exportPath.string() << std::endl;
    std::cout << "    - Metadata: " << atlasJsonPath.string() << std::endl;
    std::cout << "    - Animations: " << animJsonPath.string() << std::endl;

    return true;
}

}