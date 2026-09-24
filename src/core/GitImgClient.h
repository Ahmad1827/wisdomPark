#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <functional>
#include <mutex>

struct GitImgCommit {
    std::string hash;
    std::string shortHash;
    std::string dateFormatted;
    std::string message;
};

class GitImgClient {
public:
    GitImgClient(std::string baseUrl = "http://localhost:8080");
    ~GitImgClient();

    void setBaseUrl(const std::string& baseUrl);
    std::string getBaseUrl() const;

    void setToken(const std::string& token);
    std::string getToken() const;
    bool isAuthenticated() const;

    bool login(const std::string& username, const std::string& password);
    static std::string loadConfigUrl();
    static bool loadSavedCredentials(std::string& outUser, std::string& outPass);
    static void saveCredentials(const std::string& user, const std::string& pass);
    static void clearSavedCredentials();
    static bool promptCredentials(std::string& outUser, std::string& outPass);
    static bool promptCommitMessage(std::string& outMsg);
    void pushAsync(const sf::Image& image,
        const std::string& repo,
        const std::string& filename,
        const std::string& commitMsg,
        std::function<void(bool)> callback);

    static std::vector<GitImgCommit> getCommitHistory(std::string repo = "");
    static bool checkoutCommit(const std::string& commitHash);
    static bool downloadCommitImage(const std::string& commitHash, sf::Image& outImage, bool isThumb = false);

private:
    std::string m_baseUrl;
    std::string m_token;
    mutable std::mutex m_tokenMutex;
};