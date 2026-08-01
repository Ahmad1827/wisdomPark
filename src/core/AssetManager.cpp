#include "AssetManager.h"
#include <algorithm>
#include <iostream>

AssetManager::AssetManager() {}

void AssetManager::init(const std::string& projectPath) {
    rootPath = std::filesystem::absolute(projectPath + "/Assets").string();

    std::filesystem::create_directories(rootPath + "/Images");
    std::filesystem::create_directories(rootPath + "/Audio");
    std::filesystem::create_directories(rootPath + "/Fonts");
    std::filesystem::create_directories(rootPath + "/Brushes");
    std::filesystem::create_directories(rootPath + "/Patterns");
    std::filesystem::create_directories(rootPath + "/AI");
    scanAssets();
}

void AssetManager::scanAssets() {
    assets.clear();
    try {
        if (!std::filesystem::exists(rootPath)) return;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(rootPath)) {
            if (entry.is_regular_file()) {
                AssetRecord record;
                record.filepath = entry.path().string();
                record.filename = entry.path().filename().string();
                record.extension = entry.path().extension().string();
                std::transform(record.extension.begin(), record.extension.end(), record.extension.begin(), ::tolower);
                record.id = record.filepath;
                record.type = determineType(record.extension);
                record.fileSize = std::filesystem::file_size(entry);
                record.lastModified = decltype(record.lastModified)();
                record.isFavorite = false;
                record.thumbnailLoaded = false;
                assets.push_back(record);
            }
        }
    }
    catch (...) {
        // Suppress OS-level file permission crashes
    }
}

AssetType AssetManager::determineType(const std::string& ext) {
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga" || ext == ".gif" || ext == ".webp") return AssetType::Image;
    if (ext == ".wav" || ext == ".ogg" || ext == ".mp3" || ext == ".flac") return AssetType::Audio;
    if (ext == ".ttf" || ext == ".otf") return AssetType::Font;
    return AssetType::Unknown;
}

void AssetManager::importAssets(const std::vector<std::string>& filePaths) {
    for (const auto& path : filePaths) {
        try {
            std::filesystem::path p(path);
            AssetType t = determineType(p.extension().string());
            std::string subDir = "/Unknown";
            if (t == AssetType::Image) subDir = "/Images";
            else if (t == AssetType::Audio) subDir = "/Audio";
            else if (t == AssetType::Font) subDir = "/Fonts";

            std::string dest = rootPath + subDir + "/" + p.filename().string();

            if (std::filesystem::absolute(path) != std::filesystem::absolute(dest)) {
                copyFileToProject(path, dest);
                recentIds.push_back(dest);
            }
        }
        catch (...) {
            // Suppress invalid filepath parsing crashes
        }
    }
    scanAssets();
}

void AssetManager::copyFileToProject(const std::string& src, const std::string& dest) {
    try {
        std::filesystem::copy_file(src, dest, std::filesystem::copy_options::overwrite_existing);
    }
    catch (...) {}
}

void AssetManager::createFolder(const std::string& folderName) {
    std::filesystem::create_directories(rootPath + "/" + folderName);
}

std::vector<AssetRecord*> AssetManager::getAssetsByCategory(AssetType type) {
    std::vector<AssetRecord*> result;
    for (auto& a : assets) {
        if (a.type == type) result.push_back(&a);
    }
    return result;
}

std::vector<AssetRecord*> AssetManager::getFavorites() {
    std::vector<AssetRecord*> result;
    for (auto& a : assets) {
        if (a.isFavorite) result.push_back(&a);
    }
    return result;
}

std::vector<AssetRecord*> AssetManager::getRecent() {
    std::vector<AssetRecord*> result;
    for (const auto& id : recentIds) {
        for (auto& a : assets) {
            if (a.id == id) result.push_back(&a);
        }
    }
    return result;
}

void AssetManager::toggleFavorite(const std::string& id) {
    for (auto& a : assets) {
        if (a.id == id) {
            a.isFavorite = !a.isFavorite;
            break;
        }
    }
}

void AssetManager::deleteAsset(const std::string& id) {
    try {
        std::filesystem::remove(id);
        scanAssets();
    }
    catch (...) {}
}

AssetRecord* AssetManager::getAsset(const std::string& id) {
    for (auto& a : assets) {
        if (a.id == id) return &a;
    }
    return nullptr;
}

void AssetManager::requestThumbnail(AssetRecord* record) {
    if (!record || record->thumbnailLoaded) return;
    if (record->type == AssetType::Image) {
        sf::Image img;
        if (img.loadFromFile(record->filepath)) {
            record->thumbnail.loadFromImage(img);
            record->thumbnailLoaded = true;
        }
    }
}

std::string AssetManager::getAssetsPath() const {
    return rootPath;
}