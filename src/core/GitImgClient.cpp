#include "GitImgClient.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winhttp.h>
#include <thread>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "winhttp.lib")

static bool parseUrl(const std::string& url, std::wstring& outHost, INTERNET_PORT& outPort, bool& outIsHttps) {
    std::string temp = url;
    outIsHttps = false;

    if (temp.rfind("https://", 0) == 0) {
        outIsHttps = true;
        temp = temp.substr(8);
    }
    else if (temp.rfind("http://", 0) == 0) {
        temp = temp.substr(7);
    }

    size_t slashPos = temp.find('/');
    if (slashPos != std::string::npos) {
        temp = temp.substr(0, slashPos);
    }

    size_t colonPos = temp.find(':');
    if (colonPos != std::string::npos) {
        std::string hostStr = temp.substr(0, colonPos);
        outHost = std::wstring(hostStr.begin(), hostStr.end());
        try {
            outPort = static_cast<INTERNET_PORT>(std::stoi(temp.substr(colonPos + 1)));
        }
        catch (...) {
            outPort = outIsHttps ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
        }
    }
    else {
        outHost = std::wstring(temp.begin(), temp.end());
        outPort = outIsHttps ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
    }

    return !outHost.empty();
}

static std::string urlEncode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;
    for (char c : value) {
        if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        }
        else if (c == ' ') {
            escaped << '+';
        }
        else {
            escaped << '%' << std::setw(2) << static_cast<int>(static_cast<unsigned char>(c));
        }
    }
    return escaped.str();
}

static std::string extractJsonToken(const std::string& json) {
    const std::string key = "\"token\"";
    size_t pos = json.find(key);
    if (pos == std::string::npos) return "";

    pos += key.length();
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == ':' || json[pos] == '\t')) pos++;
    if (pos < json.length() && json[pos] == '"') {
        pos++;
        size_t endPos = json.find('"', pos);
        if (endPos != std::string::npos) {
            return json.substr(pos, endPos - pos);
        }
    }
    return "";
}

GitImgClient::GitImgClient(std::string baseUrl)
    : m_baseUrl(std::move(baseUrl)) {
}

GitImgClient::~GitImgClient() {
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
    std::wstring host;
    INTERNET_PORT port = 80;
    bool isHttps = false;
    if (!parseUrl(m_baseUrl, host, port, isHttps)) return false;

    HINTERNET hSession = WinHttpOpen(L"WisdomPark/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) return false;

    HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), port, 0);
    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        return false;
    }

    DWORD flags = isHttps ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", L"/auth/login",
        NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    std::string jsonBody = "{\"username\":\"" + username + "\",\"password\":\"" + password + "\"}";
    std::wstring headers = L"Content-Type: application/json\r\n";

    BOOL sent = WinHttpSendRequest(hRequest, headers.c_str(), static_cast<DWORD>(headers.length()),
        const_cast<char*>(jsonBody.data()), static_cast<DWORD>(jsonBody.length()),
        static_cast<DWORD>(jsonBody.length()), 0);

    bool success = false;
    if (sent && WinHttpReceiveResponse(hRequest, NULL)) {
        DWORD statusCode = 0;
        DWORD size = sizeof(statusCode);
        WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &size, WINHTTP_NO_HEADER_INDEX);

        if (statusCode >= 200 && statusCode < 300) {
            std::string responseStr;
            DWORD bytesAvailable = 0;
            while (WinHttpQueryDataAvailable(hRequest, &bytesAvailable) && bytesAvailable > 0) {
                std::vector<char> buffer(bytesAvailable + 1, 0);
                DWORD bytesRead = 0;
                if (WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead)) {
                    responseStr.append(buffer.data(), bytesRead);
                }
            }
            std::string parsedToken = extractJsonToken(responseStr);
            if (!parsedToken.empty()) {
                setToken(parsedToken);
                success = true;
            }
        }
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return success;
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
        std::wstring host;
        INTERNET_PORT port = 80;
        bool isHttps = false;
        if (!parseUrl(baseUrl, host, port, isHttps)) {
            if (callback) callback(false);
            return;
        }

        HINTERNET hSession = WinHttpOpen(L"WisdomPark/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!hSession) {
            if (callback) callback(false);
            return;
        }

        HINTERNET hConnect = WinHttpConnect(hSession, host.c_str(), port, 0);
        if (!hConnect) {
            WinHttpCloseHandle(hSession);
            if (callback) callback(false);
            return;
        }

        std::string pathStr = "/api/push/" + repo + "/" + filename + "?msg=" + urlEncode(commitMsg);
        std::wstring wPath(pathStr.begin(), pathStr.end());

        DWORD flags = isHttps ? WINHTTP_FLAG_SECURE : 0;
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST", wPath.c_str(),
            NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
        if (!hRequest) {
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);
            if (callback) callback(false);
            return;
        }

        std::string authHdr = "Content-Type: application/octet-stream\r\nAuthorization: Bearer " + token + "\r\n";
        std::wstring wHeaders(authHdr.begin(), authHdr.end());

        BOOL sent = WinHttpSendRequest(hRequest, wHeaders.c_str(), static_cast<DWORD>(wHeaders.length()),
            const_cast<sf::Uint8*>(pngBytes.data()), static_cast<DWORD>(pngBytes.size()),
            static_cast<DWORD>(pngBytes.size()), 0);

        bool success = false;
        if (sent && WinHttpReceiveResponse(hRequest, NULL)) {
            DWORD statusCode = 0;
            DWORD size = sizeof(statusCode);
            WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &size, WINHTTP_NO_HEADER_INDEX);
            success = (statusCode >= 200 && statusCode < 300);
        }

        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        if (callback) {
            callback(success);
        }
        }).detach();
}