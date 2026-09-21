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
        if (entry.is_directory() && entry.path().extension() == ".wpk") {
            std::string metaPath = entry.path().string() + "/meta.json";
            if (fs::exists(metaPath)) {
                std::ifstream file(metaPath);
                ProjectMetadata meta;
                meta.path = entry.path().string();

                std::string line;
                if (std::getline(file, meta.name) && std::getline(file, line)) meta.width = std::stoi(line);
                if (std::getline(file, line)) meta.height = std::stoi(line);
                if (std::getline(file, line)) meta.fps = std::stoi(line);
                if (std::getline(file, line)) meta.frameCount = std::stoi(line);
                if (std::getline(file, meta.lastModified)) {}
                for (int i = 0; i < 5; ++i) std::getline(file, line);
                if (std::getline(file, line)) meta.isPixelMode = (line == "1");

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

bool ProjectManager::createNewProject(const std::string& name, int width, int height, int fps, bool isPixelMode, Canvas& canvas) {
    std::string projPath = projectsDir + "/" + name + ".wpk";
    if (fs::exists(projPath)) return false;

    fs::create_directory(projPath);
    fs::create_directory(projPath + "/layers");

    canvas.initCustom(width, height);
    return saveProject(name, canvas, fps, isPixelMode);
}

bool ProjectManager::saveProject(const std::string& name, Canvas& canvas, int fps, bool isPixelMode) {
    std::string projPath = projectsDir + "/" + name + ".wpk";
    return saveProjectAs(projPath, name, canvas, fps, isPixelMode);
}

bool ProjectManager::saveProjectAs(const std::string& path, const std::string& name, Canvas& canvas, int fps, bool isPixelMode) {
    if (!fs::exists(path)) {
        fs::create_directory(path);
        fs::create_directory(path + "/layers");
    }

    std::ofstream metaFile(path + "/meta.json");
    if (!metaFile.is_open()) return false;

    sf::Vector2u size = canvas.getCanvasSize();
    metaFile << name << "\n"
        << size.x << "\n"
        << size.y << "\n"
        << fps << "\n"
        << canvas.getFrameCount() << "\n"
        << getCurrentTime() << "\n"
        << canvas.isOnionSkinEnabled() << "\n"
        << canvas.getOnionSkinPrevOpacity() << "\n"
        << canvas.getOnionSkinNextOpacity() << "\n"
        << canvas.getOnionSkinPrevCount() << "\n"
        << canvas.getOnionSkinNextCount() << "\n"
        << (isPixelMode ? "1" : "0") << "\n";
    metaFile.close();

    // 1. Vault Thumbnail (Composite vectors + static textures)
    if (canvas.getFrameCount() > 0) {
        sf::RenderTexture composite;
        sf::ContextSettings ctx;
        ctx.antialiasingLevel = isPixelMode ? 0 : 8;
        if (!composite.create(size.x, size.y, ctx)) {
            composite.create(size.x, size.y);
        }
        composite.setSmooth(!isPixelMode);
        composite.clear(sf::Color::White);

        const Frame* f0 = canvas.getFrameReadOnly(0);
        if (f0) {
            for (size_t l = 0; l < f0->layers.size(); ++l) {
                const auto& layer = f0->layers[l];
                if (layer.visible && layer.texture) {
                    layer.texture->display();
                    sf::Sprite s(layer.texture->getTexture());
                    s.setColor(sf::Color(255, 255, 255, static_cast<sf::Uint8>(layer.opacity * 255.0f)));
                    composite.draw(s);
                }
                if (!isPixelMode) {
                    for (const auto& vs : canvas.getVectorStrokes()) {
                        if (vs.frame == 0 && vs.layer == static_cast<int>(l) && !vs.isErase) {
                            composite.draw(vs.mesh);
                        }
                    }
                }
            }
        }
        composite.display();
        composite.getTexture().copyToImage().saveToFile(path + "/thumb.png");
    }

    // 2. Save vector geometry in normal mode
    if (!isPixelMode) {
        std::ofstream vFile(path + "/vector_strokes.dat");
        if (vFile.is_open()) {
            const auto& strokes = canvas.getVectorStrokes();
            vFile << strokes.size() << "\n";
            for (const auto& vs : strokes) {
                vFile << vs.frame << " " << vs.layer << " " << (vs.isErase ? 1 : 0) << " " << vs.mesh.getVertexCount() << "\n";
                for (size_t i = 0; i < vs.mesh.getVertexCount(); ++i) {
                    const auto& v = vs.mesh[i];
                    vFile << v.position.x << " " << v.position.y << " "
                        << static_cast<int>(v.color.r) << " "
                        << static_cast<int>(v.color.g) << " "
                        << static_cast<int>(v.color.b) << " "
                        << static_cast<int>(v.color.a) << "\n";
                }
            }
            vFile.close();
        }
    }

    // 3. Save layers (write clean transparent PNGs for vector layers to clear old raster ghosts)
    for (size_t f = 0; f < canvas.getFrameCount(); ++f) {
        const Frame* frame = canvas.getFrameReadOnly(static_cast<int>(f));
        if (!frame) continue;

        for (size_t l = 0; l < frame->layers.size(); ++l) {
            if (!frame->layers[l].persistent || f == 0) {
                std::string imgPath = path + "/layers/f" + std::to_string(f) + "_l" + std::to_string(l) + ".png";
                if (frame->layers[l].texture) {
                    if (!isPixelMode && !frame->layers[l].isImageResource) {
                        sf::Image cleanImg;
                        cleanImg.create(size.x, size.y, sf::Color::Transparent);
                        cleanImg.saveToFile(imgPath);
                    }
                    else {
                        frame->layers[l].texture->display();
                        sf::Image img = frame->layers[l].texture->getTexture().copyToImage();
                        img.saveToFile(imgPath);
                    }
                }
            }
        }

        std::ofstream layerMeta(path + "/f" + std::to_string(f) + "_layers.txt");
        for (size_t l = 0; l < frame->layers.size(); ++l) {
            layerMeta << frame->layers[l].name << "|"
                << frame->layers[l].visible << "|"
                << frame->layers[l].locked << "|"
                << frame->layers[l].opacity << "|"
                << static_cast<int>(frame->layers[l].blendMode) << "|"
                << frame->layers[l].persistent << "|"
                << frame->layers[l].colorTag << "\n";
        }
        layerMeta.close();
    }

    canvas.cleanVectorLayers();
    return true;
}

bool ProjectManager::loadProject(const std::string& path, Canvas& canvas, int& outFps, bool& outIsPixelMode) {
    std::string metaPath = path + "/meta.json";
    if (!fs::exists(metaPath)) return false;

    std::ifstream file(metaPath);
    std::string projName, line, lastMod;
    int width = 1280, height = 720, frames = 1;
    bool onionOn = false;
    float onionP = 89.25f, onionN = 89.25f;
    int opc = 1, onc = 1;
    outIsPixelMode = false;

    if (!std::getline(file, projName)) return false;
    if (std::getline(file, line)) width = std::stoi(line);
    if (std::getline(file, line)) height = std::stoi(line);
    if (std::getline(file, line)) outFps = std::stoi(line);
    if (std::getline(file, line)) frames = std::stoi(line);
    if (std::getline(file, lastMod)) {}
    if (std::getline(file, line)) onionOn = (line == "1");
    if (std::getline(file, line)) onionP = std::stof(line);
    if (std::getline(file, line)) onionN = std::stof(line);
    if (std::getline(file, line)) opc = std::stoi(line);
    if (std::getline(file, line)) onc = std::stoi(line);
    if (std::getline(file, line)) outIsPixelMode = (line == "1");
    file.close();

    canvas.setPixelMode(outIsPixelMode);
    canvas.initCustom(width, height);
    canvas.clearAllFrames();
    canvas.setOnionSkin(onionOn, onionP, onionN);
    canvas.setOnionSkinCounts(opc, onc);

    for (int f = 0; f < frames; ++f) {
        if (f > 0) canvas.addFrame(f - 1);

        std::ifstream layerMeta(path + "/f" + std::to_string(f) + "_layers.txt");
        if (!layerMeta.is_open()) continue;

        std::string lLine;
        int l = 0;

        while (std::getline(layerMeta, lLine)) {
            while (!lLine.empty() && (lLine.back() == '\r' || lLine.back() == '\n' || lLine.back() == ' ')) {
                lLine.pop_back();
            }
            if (lLine.empty()) continue;

            size_t p1 = lLine.find('|');
            size_t p2 = lLine.find('|', p1 + 1);
            size_t p3 = lLine.find('|', p2 + 1);
            size_t p4 = lLine.find('|', p3 + 1);
            size_t p5 = lLine.find('|', p4 + 1);
            size_t p6 = lLine.find('|', p5 + 1);

            if (p1 == std::string::npos || p4 == std::string::npos) continue;

            while (l >= static_cast<int>(canvas.getFrame(f)->layers.size())) {
                canvas.addLayer(f, "Layer");
            }

            std::string lName = lLine.substr(0, p1);
            bool lVis = (lLine.substr(p1 + 1, p2 - p1 - 1) == "1");
            bool lLock = (lLine.substr(p2 + 1, p3 - p2 - 1) == "1");
            float lOpac = std::stof(lLine.substr(p3 + 1, p4 - p3 - 1));
            int lBlend = std::stoi(lLine.substr(p4 + 1, p5 - p4 - 1));
            bool lPers = false;
            int lTag = 0;

            if (p5 != std::string::npos && p6 != std::string::npos) {
                lPers = (lLine.substr(p5 + 1, p6 - p5 - 1) == "1");
                lTag = std::stoi(lLine.substr(p6 + 1));
            }

            canvas.setLayerProperties(f, l, lName, lVis, lLock, lOpac, static_cast<BlendMode>(lBlend));
            canvas.getFrame(f)->layers[l].persistent = lPers;
            canvas.getFrame(f)->layers[l].colorTag = lTag;
            canvas.getFrame(f)->layers[l].texture->setSmooth(!outIsPixelMode);

            if (lPers && f > 0) {
                canvas.getFrame(f)->layers[l].texture = canvas.getFrame(0)->layers[l].texture;
            }
            else {
                std::string imgPath = path + "/layers/f" + std::to_string(f) + "_l" + std::to_string(l) + ".png";
                if (fs::exists(imgPath)) {
                    sf::Image img;
                    if (img.loadFromFile(imgPath)) {
                        sf::Texture tex;
                        tex.setSmooth(!outIsPixelMode);
                        if (tex.loadFromImage(img)) {
                            auto target = canvas.getFrame(f)->layers[l].texture;
                            target->setSmooth(!outIsPixelMode);
                            target->clear(sf::Color::Transparent);
                            sf::Sprite spr(tex);
                            target->draw(spr, sf::RenderStates(sf::BlendNone));
                            target->display();
                        }
                    }
                }
            }
            l++;
        }
    }

    // Load vector geometry and wipe any contaminated raster stroke data
    canvas.clearVectorStrokes();
    if (!outIsPixelMode) {
        std::string vPath = path + "/vector_strokes.dat";
        if (fs::exists(vPath)) {
            std::ifstream vFile(vPath);
            size_t strokeCount = 0;
            if (vFile >> strokeCount) {
                std::vector<VectorStroke> loadedStrokes;
                for (size_t s = 0; s < strokeCount; ++s) {
                    VectorStroke vs;
                    int isEraseInt = 0;
                    size_t vertCount = 0;
                    vFile >> vs.frame >> vs.layer >> isEraseInt >> vertCount;
                    vs.isErase = (isEraseInt != 0);
                    vs.mesh.setPrimitiveType(sf::Triangles);

                    for (size_t i = 0; i < vertCount; ++i) {
                        float vx, vy;
                        int r, g, b, a;
                        vFile >> vx >> vy >> r >> g >> b >> a;
                        sf::Vertex v(sf::Vector2f(vx, vy), sf::Color(r, g, b, a));
                        vs.mesh.append(v);
                    }
                    loadedStrokes.push_back(std::move(vs));
                }
                canvas.setVectorStrokes(loadedStrokes);
                canvas.cleanVectorLayers();
            }
            vFile.close();
        }
    }

    return true;
}

bool ProjectManager::deleteProject(const std::string& name) {
    std::string projPath = projectsDir + "/" + name + ".wpk";
    if (fs::exists(projPath)) {
        std::error_code ec;
        fs::remove_all(projPath, ec);
        return !ec;
    }
    return false;
}

bool ProjectManager::duplicateProject(const std::string& sourceName, const std::string& newName) {
    std::string srcPath = projectsDir + "/" + sourceName + ".wpk";
    std::string dstPath = projectsDir + "/" + newName + ".wpk";

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