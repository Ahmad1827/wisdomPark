#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <functional>
#include <mutex>

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

private:
    std::string m_baseUrl;
    std::string m_token;
    mutable std::mutex m_tokenMutex;
};