#define _CRT_SECURE_NO_WARNINGS
#include "ProjectManager.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

ProjectManager::ProjectManager() : projectsDir("projects") {
    ensureDirectoryExists();
}

void ProjectManager::ensureDirectoryExists() {
    if (!fs::exists(projectsDir)) {
        fs::create_directory(projectsDir);
    }
}

std::string ProjectManager::getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::vector<ProjectMetadata> ProjectManager::getRecentProjects() {
    std::vector<ProjectMetadata> projects;
    ensureDirectoryExists();

    for (const auto& entry : fs::directory_iterator(projectsDir)) {
        if (entry.is_directory() && entry.path().extension() == ".wpark") {
            std::string metaPath = entry.path().string() + "/meta.json";
            if (fs::exists(metaPath)) {
                std::ifstream file(metaPath);
                ProjectMetadata meta;
                meta.path = entry.path().string();

                std::string line;
                if (std::getline(file, meta.name) &&
                    std::getline(file, line)) {
                    meta.width = std::stoi(line);
                }
                if (std::getline(file, line)) { meta.height = std::stoi(line); }
                if (std::getline(file, line)) { meta.fps = std::stoi(line); }
                if (std::getline(file, line)) { meta.frameCount = std::stoi(line); }
                if (std::getline(file, meta.lastModified)) {}

                std::string thumbPath = entry.path().string() + "/thumb.png";
                if (fs::exists(thumbPath)) {
                    meta.thumbnail.loadFromFile(thumbPath);
                }

                projects.push_back(meta);
            }
        }
    }

    std::sort(projects.begin(), projects.end(), [](const ProjectMetadata& a, const ProjectMetadata& b) {
        return a.lastModified > b.lastModified;
        });

    return projects;
}

bool ProjectManager::createNewProject(const std::string& name, int width, int height, int fps, Canvas& canvas) {
    std::string projPath = projectsDir + "/" + name + ".wpark";
    if (fs::exists(projPath)) return false;

    fs::create_directory(projPath);
    fs::create_directory(projPath + "/layers");

    canvas.initCustom(width, height);
    return saveProject(name, canvas, fps);
}

bool ProjectManager::saveProject(const std::string& name, Canvas& canvas, int fps) {
    std::string projPath = projectsDir + "/" + name + ".wpark";
    ensureDirectoryExists();
    if (!fs::exists(projPath)) {
        fs::create_directory(projPath);
        fs::create_directory(projPath + "/layers");
    }

    std::ofstream metaFile(projPath + "/meta.json");
    if (!metaFile.is_open()) return false;

    sf::Vector2u size = canvas.getCanvasSize();
    metaFile << name << "\n"
        << size.x << "\n"
        << size.y << "\n"
        << fps << "\n"
        << canvas.getFrameCount() << "\n"
        << getCurrentTime() << "\n";
    metaFile.close();

    if (canvas.getFrameCount() > 0) {
        sf::RenderTexture composite;
        composite.create(size.x, size.y);
        composite.clear(sf::Color::White);
        const Frame* f0 = canvas.getFrameReadOnly(0);
        for (const auto& l : f0->layers) {
            if (l.visible) {
                sf::Sprite s(l.texture->getTexture());
                s.setColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(l.opacity)));
                composite.draw(s);
            }
        }
        composite.display();
        composite.getTexture().copyToImage().saveToFile(projPath + "/thumb.png");
    }

    for (size_t f = 0; f < canvas.getFrameCount(); ++f) {
        const Frame* frame = canvas.getFrameReadOnly(static_cast<int>(f));
        if (!frame) continue;

        for (size_t l = 0; l < frame->layers.size(); ++l) {
            std::string imgPath = projPath + "/layers/f" + std::to_string(f) + "_l" + std::to_string(l) + ".png";
            sf::Image img = frame->layers[l].texture->getTexture().copyToImage();
            img.saveToFile(imgPath);
        }

        std::ofstream layerMeta(projPath + "/f" + std::to_string(f) + "_layers.txt");
        for (size_t l = 0; l < frame->layers.size(); ++l) {
            layerMeta << frame->layers[l].name << "|"
                << frame->layers[l].visible << "|"
                << frame->layers[l].locked << "|"
                << frame->layers[l].opacity << "|"
                << static_cast<int>(frame->layers[l].blendMode) << "\n";
        }
        layerMeta.close();
    }
    return true;
}

bool ProjectManager::loadProject(const std::string& name, Canvas& canvas, int& outFps) {
    std::string projPath = projectsDir + "/" + name + ".wpark";
    std::string metaPath = projPath + "/meta.json";
    if (!fs::exists(metaPath)) return false;

    std::ifstream file(metaPath);
    std::string projName, line, lastMod;
    int width, height, frames;

    std::getline(file, projName);
    std::getline(file, line); width = std::stoi(line);
    std::getline(file, line); height = std::stoi(line);
    std::getline(file, line); outFps = std::stoi(line);
    std::getline(file, line); frames = std::stoi(line);
    std::getline(file, lastMod);
    file.close();

    canvas.initCustom(width, height);
    canvas.clearAllFrames();

    for (int f = 0; f < frames; ++f) {
        if (f > 0) canvas.addFrame(f - 1);

        std::ifstream layerMeta(projPath + "/f" + std::to_string(f) + "_layers.txt");
        std::string lLine;
        int l = 0;

        while (std::getline(layerMeta, lLine)) {
            // Fixes the "addLayerToFrame is not a member of Canvas" error
            if (l > 1) canvas.addLayer(f, "Layer");

            size_t pos1 = lLine.find('|');
            size_t pos2 = lLine.find('|', pos1 + 1);
            size_t pos3 = lLine.find('|', pos2 + 1);
            size_t pos4 = lLine.find('|', pos3 + 1);

            if (pos1 != std::string::npos && pos4 != std::string::npos) {
                std::string lName = lLine.substr(0, pos1);
                bool lVis = (lLine.substr(pos1 + 1, pos2 - pos1 - 1) == "1");
                bool lLock = (lLine.substr(pos2 + 1, pos3 - pos2 - 1) == "1");
                float lOpac = std::stof(lLine.substr(pos3 + 1, pos4 - pos3 - 1));
                int lBlend = std::stoi(lLine.substr(pos4 + 1));

                canvas.setLayerProperties(f, l, lName, lVis, lLock, lOpac, static_cast<BlendMode>(lBlend));
            }

            std::string imgPath = projPath + "/layers/f" + std::to_string(f) + "_l" + std::to_string(l) + ".png";
            if (fs::exists(imgPath)) {
                sf::Texture tex;
                if (tex.loadFromFile(imgPath)) {
                    sf::Sprite spr(tex);
                    canvas.getFrame(f)->layers[l].texture->clear(sf::Color::Transparent);
                    canvas.getFrame(f)->layers[l].texture->draw(spr);
                    canvas.getFrame(f)->layers[l].texture->display();
                }
            }
            l++;
        }
    }
    return true;
}

bool ProjectManager::deleteProject(const std::string& name) {
    std::string projPath = projectsDir + "/" + name + ".wpark";
    if (fs::exists(projPath)) {
        return fs::remove_all(projPath) > 0;
    }
    return false;
}

bool ProjectManager::duplicateProject(const std::string& sourceName, const std::string& newName) {
    std::string srcPath = projectsDir + "/" + sourceName + ".wpark";
    std::string dstPath = projectsDir + "/" + newName + ".wpark";

    if (fs::exists(srcPath) && !fs::exists(dstPath)) {
        fs::copy(srcPath, dstPath, fs::copy_options::recursive);

        std::string metaPath = dstPath + "/meta.json";
        if (fs::exists(metaPath)) {
            std::ifstream in(metaPath);
            std::vector<std::string> lines;
            std::string line;
            while (std::getline(in, line)) lines.push_back(line);
            in.close();

            if (!lines.empty()) {
                lines[0] = newName;
                lines[5] = getCurrentTime();
                std::ofstream out(metaPath, std::ios::trunc);
                for (const auto& l : lines) out << l << "\n";
            }
        }
        return true;
    }
    return false;
}