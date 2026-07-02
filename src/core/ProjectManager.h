#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <SFML/Graphics.hpp>
#include "Canvas.h"

struct ProjectMetadata {
    std::string name = "Untitled";
    std::string path = "";
    int width = 1920;
    int height = 1080;
    int fps = 12;
    std::string lastModified = "";
    int frameCount = 0;
    bool isPixelMode = false;
    sf::Texture thumbnail;
};

class ProjectManager {
private:
    std::string projectsDir;

    void ensureDirectoryExists();
    std::string getCurrentTime();

public:
    ProjectManager();

    std::vector<ProjectMetadata> getRecentProjects();
    bool createNewProject(const std::string& name, int width, int height, int fps, bool isPixelMode, Canvas& canvas);
    bool saveProject(const std::string& name, Canvas& canvas, int fps, bool isPixelMode);
    bool saveProjectAs(const std::string& path, const std::string& name, Canvas& canvas, int fps, bool isPixelMode);
    bool loadProject(const std::string& path, Canvas& canvas, int& outFps, bool& outIsPixelMode);
    bool deleteProject(const std::string& name);
    bool duplicateProject(const std::string& sourceName, const std::string& newName);
};