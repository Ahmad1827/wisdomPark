#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "HandTracker.h"
#include <SFML/Graphics/Texture.hpp>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")s
#include <thread>
#include <atomic>
#include <sstream>
#include <vector>
#include <mutex>

struct HandTracker::Impl {
    std::atomic<bool> running{ false };
    std::thread workerThread;
    SOCKET sock{ INVALID_SOCKET };
    HWND hwnd{ NULL };
    PROCESS_INFORMATION procInfo{};
    bool processSpawned{ false };

    std::mutex frameMutex;
    std::vector<uint8_t> jpegBuffer;
    bool hasNewFrame{ false };

    void listenLoop();
};

HandTracker::HandTracker() : m_impl(std::make_unique<Impl>()) {}

HandTracker::~HandTracker() {
    stop();
}

HandTracker& HandTracker::getInstance() {
    static HandTracker instance;
    return instance;
}

bool HandTracker::isRunning() const {
    return m_impl->running.load();
}

bool HandTracker::updateTexture(sf::Texture& texture) {
    std::vector<uint8_t> temp;
    {
        std::lock_guard<std::mutex> lock(m_impl->frameMutex);
        if (!m_impl->hasNewFrame) return false;
        temp = m_impl->jpegBuffer;
        m_impl->hasNewFrame = false;
    }
    if (!temp.empty()) {
        return texture.loadFromMemory(temp.data(), temp.size());
    }
    return false;
}

bool HandTracker::start(sf::WindowHandle targetHwnd) {
    if (m_impl->running.load()) return true;

    m_impl->hwnd = targetHwnd ? static_cast<HWND>(targetHwnd) : GetActiveWindow();
    if (!m_impl->hwnd) {
        m_impl->hwnd = GetForegroundWindow();
    }

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        return false;
    }

    m_impl->sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_impl->sock == INVALID_SOCKET) {
        WSACleanup();
        return false;
    }

    DWORD timeout = 200;
    setsockopt(m_impl->sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
    serverAddr.sin_port = htons(5005);

    if (bind(m_impl->sock, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(m_impl->sock);
        m_impl->sock = INVALID_SOCKET;
        WSACleanup();
        return false;
    }

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    char cmd[] = "python scripts/hand_tracker.py";

    m_impl->processSpawned = CreateProcessA(
        NULL,
        cmd,
        NULL,
        NULL,
        FALSE,
        CREATE_NO_WINDOW,
        NULL,
        NULL,
        &si,
        &m_impl->procInfo
    );

    m_impl->running.store(true);
    m_impl->workerThread = std::thread(&HandTracker::Impl::listenLoop, m_impl.get());
    return true;
}

void HandTracker::stop() {
    if (!m_impl->running.load()) return;

    m_impl->running.store(false);

    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);

    if (m_impl->sock != INVALID_SOCKET) {
        closesocket(m_impl->sock);
        m_impl->sock = INVALID_SOCKET;
    }

    if (m_impl->workerThread.joinable()) {
        m_impl->workerThread.join();
    }

    if (m_impl->processSpawned) {
        TerminateProcess(m_impl->procInfo.hProcess, 0);
        CloseHandle(m_impl->procInfo.hProcess);
        CloseHandle(m_impl->procInfo.hThread);
        m_impl->processSpawned = false;
    }

    WSACleanup();
}

void HandTracker::Impl::listenLoop() {
    std::vector<char> buffer(65536);
    sockaddr_in clientAddr{};
    int clientLen = sizeof(clientAddr);

    int prevLeft = 0;
    int prevRight = 0;
    int prevZoom = 0;
    float zoomAnchorY = -1.0f;

    while (running.load()) {
        int bytes = recvfrom(sock, buffer.data(), static_cast<int>(buffer.size()), 0, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (bytes <= 0) {
            if (prevLeft == 1) {
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                prevLeft = 0;
            }
            if (prevRight == 1) {
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
                prevRight = 0;
            }
            prevZoom = 0;
            zoomAnchorY = -1.0f;
            continue;
        }

        if (bytes > 4 && memcmp(buffer.data(), "IMG:", 4) == 0) {
            std::lock_guard<std::mutex> lock(frameMutex);
            jpegBuffer.assign(reinterpret_cast<uint8_t*>(buffer.data() + 4), reinterpret_cast<uint8_t*>(buffer.data() + bytes));
            hasNewFrame = true;
            continue;
        }

        const char* posData = buffer.data();
        int posBytes = bytes;
        if (bytes > 4 && memcmp(buffer.data(), "POS:", 4) == 0) {
            posData = buffer.data() + 4;
            posBytes = bytes - 4;
        }

        std::string payload(posData, posBytes);
        std::stringstream ss(payload);
        std::string item;
        std::vector<std::string> tokens;
        while (std::getline(ss, item, ',')) {
            tokens.push_back(item);
        }

        if (tokens.empty()) continue;

        if (tokens[0] == "LOST") {
            if (prevLeft == 1) {
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                prevLeft = 0;
            }
            if (prevRight == 1) {
                mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
                prevRight = 0;
            }
            prevZoom = 0;
            zoomAnchorY = -1.0f;
            continue;
        }

        if (tokens.size() < 5) continue;

        float normX = std::stof(tokens[0]);
        float normY = std::stof(tokens[1]);
        int leftClick = std::stoi(tokens[2]);
        int rightClick = std::stoi(tokens[3]);
        int zoomActive = std::stoi(tokens[4]);

        if (hwnd && IsWindow(hwnd)) {
            RECT rect;
            GetClientRect(hwnd, &rect);
            POINT topLeft{ rect.left, rect.top };
            ClientToScreen(hwnd, &topLeft);

            int width = rect.right - rect.left;
            int height = rect.bottom - rect.top;

            int targetX = topLeft.x + static_cast<int>(normX * width);
            int targetY = topLeft.y + static_cast<int>(normY * height);

            if (zoomActive == 1) {
                if (prevZoom == 0 || zoomAnchorY < 0.0f) {
                    zoomAnchorY = normY;
                }
                float dy = normY - zoomAnchorY;
                if (dy < -0.035f) {
                    mouse_event(MOUSEEVENTF_WHEEL, 0, 0, WHEEL_DELTA, 0);
                    zoomAnchorY = normY;
                }
                else if (dy > 0.035f) {
                    mouse_event(MOUSEEVENTF_WHEEL, 0, 0, -WHEEL_DELTA, 0);
                    zoomAnchorY = normY;
                }
            }
            else {
                zoomAnchorY = -1.0f;
                SetCursorPos(targetX, targetY);

                if (leftClick == 1 && prevLeft == 0) {
                    mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                }
                else if (leftClick == 0 && prevLeft == 1) {
                    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
                }

                if (rightClick == 1 && prevRight == 0) {
                    mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, 0);
                }
                else if (rightClick == 0 && prevRight == 1) {
                    mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
                }

                prevLeft = leftClick;
                prevRight = rightClick;
            }
            prevZoom = zoomActive;
        }
    }
}