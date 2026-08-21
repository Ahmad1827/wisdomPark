#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <SFML/Graphics.hpp>

enum class AssetType { Image, Audio, Font, Brush, Pattern, AI, Unknown };

struct AssetRecord {
    std::string id;
    std::string filename;
    std::string filepath;
    std::string extension;
    AssetType type;
    size_t fileSize;
    std::time_t lastModified;
    bool isFavorite;
    sf::Texture thumbnail;
    bool thumbnailLoaded;
};

class AssetManager {
public:
    void removeAsset(const std::string& id);
    AssetManager();
    void init(const std::string& projectPath);
    void scanAssets();
    void importAssets(const std::vector<std::string>& filePaths);
    void createFolder(const std::string& folderName);
    std::vector<AssetRecord*> getAssetsByCategory(AssetType type);
    std::vector<AssetRecord*> getFavorites();
    std::vector<AssetRecord*> getRecent();
    void toggleFavorite(const std::string& id);
    void deleteAsset(const std::string& id);
    AssetRecord* getAsset(const std::string& id);
    void requestThumbnail(AssetRecord* record);
    std::string getAssetsPath() const;

private:
    std::string rootPath;
    std::vector<AssetRecord> assets;
    std::vector<std::string> recentIds;
    AssetType determineType(const std::string& ext);
    void copyFileToProject(const std::string& src, const std::string& dest);
};