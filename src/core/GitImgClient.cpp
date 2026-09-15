#include "GitImgClient.h"
#include <curl/curl.h>
#include <thread>
#include <nlohmann/json.hpp>

size_t GitImgClient::writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

GitImgClient::GitImgClient(std::string baseUrl)
    : m_baseUrl(std::move(baseUrl)) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

GitImgClient::~GitImgClient() {
    curl_global_cleanup();
}

void GitImgClient::setBaseUrl(const std::string& baseUrl) {
    m_baseUrl = baseUrl;
    while (!m_baseUrl.empty() && m_baseUrl.back() == '/') {
        m_baseUrl.pop_back();
    }
}

std::string GitImgClient::getBaseUrl() const {
    return m_baseUrl;
}

void GitImgClient::setToken(const std::string& token) {
    std::lock_guard<std::mutex> lock(m_tokenMutex);
    m_token = token;
}

std::string GitImgClient::getToken() const {
    std::lock_guard<std::mutex> lock(m_tokenMutex);
    return m_token;
}

bool GitImgClient::isAuthenticated() const {
    std::lock_guard<std::mutex> lock(m_tokenMutex);
    return !m_token.empty();
}

bool GitImgClient::login(const std::string& username, const std::string& password) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    std::string endpoint = m_baseUrl + "/auth/login";
    nlohmann::json reqBody;
    reqBody["username"] = username;
    reqBody["password"] = password;
    std::string jsonStr = reqBody.dump();

    std::string responseData;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(jsonStr.size()));
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    CURLcode res = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || httpCode < 200 || httpCode >= 300) {
        return false;
    }

    try {
        auto parsed = nlohmann::json::parse(responseData);
        if (parsed.contains("token") && parsed["token"].is_string()) {
            setToken(parsed["token"].get<std::string>());
            return true;
        }
    }
    catch (...) {
        return false;
    }

    return false;
}

void GitImgClient::pushAsync(const sf::Image& image,
    const std::string& repo,
    const std::string& filename,
    const std::string& commitMsg,
    std::function<void(bool)> callback) {
    std::vector<sf::Uint8> pngBytes;
    if (!image.saveToMemory(pngBytes, "png")) {
        if (callback) callback(false);
        return;
    }

    std::string token = getToken();
    std::string baseUrl = m_baseUrl;

    std::thread([baseUrl, token, pngBytes = std::move(pngBytes), repo, filename, commitMsg, callback]() {
        CURL* curl = curl_easy_init();
        if (!curl) {
            if (callback) callback(false);
            return;
        }

        char* escapedMsg = curl_easy_escape(curl, commitMsg.c_str(), static_cast<int>(commitMsg.length()));
        std::string queryMsg = escapedMsg ? escapedMsg : "";
        if (escapedMsg) curl_free(escapedMsg);

        std::string url = baseUrl + "/api/push/" + repo + "/" + filename + "?msg=" + queryMsg;

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/octet-stream");
        std::string authHeader = "Authorization: Bearer " + token;
        headers = curl_slist_append(headers, authHeader.c_str());

        std::string responseData;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, pngBytes.data());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(pngBytes.size()));
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseData);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

        CURLcode res = curl_easy_perform(curl);
        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        bool success = (res == CURLE_OK && httpCode >= 200 && httpCode < 300);
        if (callback) {
            callback(success);
        }
        }).detach();
}