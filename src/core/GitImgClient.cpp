#include "GitImgClient.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <winhttp.h>
#include <thread>
#include <sstream>
#include <iomanip>
#include <fstream>

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

struct DlgData {
    HWND hFirst = NULL;
    HWND hSecond = NULL;
    bool ok = false;
    std::string first;
    std::string second;
};

static LRESULT CALLBACK GitImgDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    DlgData* data = reinterpret_cast<DlgData*>(GetWindowLongPtrA(hwnd, GWLP_USERDATA));
    if (msg == WM_COMMAND) {
        int id = LOWORD(wParam);
        if (id == 1 && data) {
            char buf1[256] = { 0 };
            char buf2[256] = { 0 };
            if (data->hFirst) GetWindowTextA(data->hFirst, buf1, sizeof(buf1));
            if (data->hSecond) GetWindowTextA(data->hSecond, buf2, sizeof(buf2));
            data->first = buf1;
            data->second = buf2;
            data->ok = true;
            DestroyWindow(hwnd);
            return 0;
        }
        else if (id == 2) {
            if (data) data->ok = false;
            DestroyWindow(hwnd);
            return 0;
        }
    }
    else if (msg == WM_CLOSE) {
        if (data) data->ok = false;
        DestroyWindow(hwnd);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

static void registerDlgClass() {
    static bool registered = false;
    if (registered) return;
    WNDCLASSA wc = { 0 };
    wc.lpfnWndProc = GitImgDlgProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "GitImgInputDlgClass";
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassA(&wc);
    registered = true;
}

bool GitImgClient::loadSavedCredentials(std::string& outUser, std::string& outPass) {
    std::ifstream file("gitimg_auth.cfg");
    if (!file.is_open()) return false;
    if (std::getline(file, outUser) && std::getline(file, outPass)) {
        return !outUser.empty() && !outPass.empty();
    }
    return false;
}

void GitImgClient::saveCredentials(const std::string& user, const std::string& pass) {
    std::ofstream file("gitimg_auth.cfg");
    if (file.is_open()) {
        file << user << "\n" << pass << "\n";
    }
}

void GitImgClient::clearSavedCredentials() {
    std::remove("gitimg_auth.cfg");
}

bool GitImgClient::promptCredentials(std::string& outUser, std::string& outPass) {
    registerDlgClass();
    DlgData data;
    int w = 340;
    int h = 185;
    int scrW = GetSystemMetrics(SM_CXSCREEN);
    int scrH = GetSystemMetrics(SM_CYSCREEN);

    HWND hwnd = CreateWindowExA(WS_EX_TOPMOST | WS_EX_DLGMODALFRAME, "GitImgInputDlgClass", "GitImg Authentication",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        (scrW - w) / 2, (scrH - h) / 2, w, h, NULL, NULL, GetModuleHandle(NULL), NULL);
    if (!hwnd) return false;

    SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&data));

    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    HWND lblU = CreateWindowA("STATIC", "Username:", WS_CHILD | WS_VISIBLE, 20, 20, 75, 20, hwnd, NULL, NULL, NULL);
    data.hFirst = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", outUser.c_str(), WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 100, 18, 200, 22, hwnd, NULL, NULL, NULL);

    HWND lblP = CreateWindowA("STATIC", "Password:", WS_CHILD | WS_VISIBLE, 20, 52, 75, 20, hwnd, NULL, NULL, NULL);
    data.hSecond = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_PASSWORD | ES_AUTOHSCROLL, 100, 50, 200, 22, hwnd, NULL, NULL, NULL);

    HWND btnOk = CreateWindowA("BUTTON", "Login", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 70, 95, 85, 28, hwnd, (HMENU)1, NULL, NULL);
    HWND btnCancel = CreateWindowA("BUTTON", "Cancel", WS_CHILD | WS_VISIBLE, 175, 95, 85, 28, hwnd, (HMENU)2, NULL, NULL);

    SendMessageA(lblU, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessageA(data.hFirst, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessageA(lblP, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessageA(data.hSecond, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessageA(btnOk, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessageA(btnCancel, WM_SETFONT, (WPARAM)hFont, TRUE);

    SetFocus(outUser.empty() ? data.hFirst : data.hSecond);

    MSG msg;
    while (IsWindow(hwnd) && GetMessageA(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN) {
            if (msg.wParam == VK_RETURN) {
                SendMessageA(hwnd, WM_COMMAND, 1, 0);
                break;
            }
            else if (msg.wParam == VK_ESCAPE) {
                SendMessageA(hwnd, WM_COMMAND, 2, 0);
                break;
            }
        }
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    if (data.ok) {
        outUser = data.first;
        outPass = data.second;
        return !outUser.empty() && !outPass.empty();
    }
    return false;
}

bool GitImgClient::promptCommitMessage(std::string& outMsg) {
    registerDlgClass();
    DlgData data;
    int w = 360;
    int h = 150;
    int scrW = GetSystemMetrics(SM_CXSCREEN);
    int scrH = GetSystemMetrics(SM_CYSCREEN);

    HWND hwnd = CreateWindowExA(WS_EX_TOPMOST | WS_EX_DLGMODALFRAME, "GitImgInputDlgClass", "Commit Artwork",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        (scrW - w) / 2, (scrH - h) / 2, w, h, NULL, NULL, GetModuleHandle(NULL), NULL);
    if (!hwnd) return false;

    SetWindowLongPtrA(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&data));

    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    HWND lbl = CreateWindowA("STATIC", "Commit Message:", WS_CHILD | WS_VISIBLE, 20, 15, 200, 18, hwnd, NULL, NULL, NULL);
    data.hFirst = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "Pushed from WisdomPark", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 20, 36, 305, 22, hwnd, NULL, NULL, NULL);

    HWND btnOk = CreateWindowA("BUTTON", "Push", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 80, 72, 85, 28, hwnd, (HMENU)1, NULL, NULL);
    HWND btnCancel = CreateWindowA("BUTTON", "Cancel", WS_CHILD | WS_VISIBLE, 185, 72, 85, 28, hwnd, (HMENU)2, NULL, NULL);

    SendMessageA(lbl, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessageA(data.hFirst, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessageA(btnOk, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessageA(btnCancel, WM_SETFONT, (WPARAM)hFont, TRUE);

    SendMessageA(data.hFirst, EM_SETSEL, 0, -1);
    SetFocus(data.hFirst);

    MSG msg;
    while (IsWindow(hwnd) && GetMessageA(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN) {
            if (msg.wParam == VK_RETURN) {
                SendMessageA(hwnd, WM_COMMAND, 1, 0);
                break;
            }
            else if (msg.wParam == VK_ESCAPE) {
                SendMessageA(hwnd, WM_COMMAND, 2, 0);
                break;
            }
        }
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    if (data.ok) {
        outMsg = data.first;
        if (outMsg.empty()) {
            outMsg = "Update artwork";
        }
        return true;
    }
    return false;
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