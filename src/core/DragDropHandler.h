#pragma once
#include <SFML/Graphics.hpp>
#include <windows.h>
#include <shellapi.h>
#include <string>
#include <vector>
#include <mutex>
#include <filesystem>
#include <algorithm>

struct DroppedFile {
    std::string path;
    sf::Vector2i mousePos;
};

class DragDropHandler {
private:
    static inline WNDPROC s_oldProc = nullptr;
    static inline std::vector<DroppedFile> s_dropQueue;
    static inline std::mutex s_mutex;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        if (msg == WM_DROPFILES) {
            HDROP hDrop = reinterpret_cast<HDROP>(wParam);
            POINT pt;
            DragQueryPoint(hDrop, &pt);

            UINT count = DragQueryFileA(hDrop, 0xFFFFFFFF, nullptr, 0);
            char filePath[MAX_PATH];

            std::lock_guard<std::mutex> lock(s_mutex);
            for (UINT i = 0; i < count; ++i) {
                if (DragQueryFileA(hDrop, i, filePath, MAX_PATH)) {
                    s_dropQueue.push_back({ std::string(filePath), sf::Vector2i(pt.x, pt.y) });
                }
            }

            DragFinish(hDrop);
            return 0;
        }
        return CallWindowProc(s_oldProc, hwnd, msg, wParam, lParam);
    }

public:
    static void Attach(sf::RenderWindow& window) {
        HWND hwnd = static_cast<HWND>(window.getSystemHandle());
        DragAcceptFiles(hwnd, TRUE);
        s_oldProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndProc)));
    }

    static std::vector<DroppedFile> PollDroppedFiles() {
        std::lock_guard<std::mutex> lock(s_mutex);
        std::vector<DroppedFile> pending = std::move(s_dropQueue);
        s_dropQueue.clear();
        return pending;
    }
};