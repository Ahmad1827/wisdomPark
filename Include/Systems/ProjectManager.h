#pragma once
#include <string>
#include <memory>

namespace StudioCore {

class Project;

class ProjectManager {
public:
    static bool SaveProject(const Project& project, const std::string& filePath);
    static std::shared_ptr<Project> LoadProject(const std::string& filePath, std::string& outError);
};

}