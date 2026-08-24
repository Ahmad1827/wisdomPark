#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <string>
#include <cstdlib>
#include <fstream>

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#define popen _popen
#define pclose _pclose
#endif

namespace StudioUI {

class NativeFileDialog {
private:
    static bool IsWSL() {
        std::ifstream versionFile("/proc/version");
        if (!versionFile.is_open()) return false;
        std::string line;
        std::getline(versionFile, line);
        return line.find("microsoft") != std::string::npos || line.find("WSL") != std::string::npos;
    }

    static std::string ExecuteCommand(const std::string& cmd) {
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return "";

        char buffer[128];
        std::string result = "";
        while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            result += buffer;
        }
        pclose(pipe);

        while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
            result.pop_back();
        }
        return result;
    }

public:
    static std::string OpenFileDialog(const std::string& filterName = "PNG Image (*.png)") {
#ifdef _WIN32
        char szFile[260] = {0};
        OPENFILENAMEA ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "PNG Images (*.png)\0*.png\0All Files (*.*)\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if (GetOpenFileNameA(&ofn) == TRUE) {
            return std::string(ofn.lpstrFile);
        }
        return "";
#else
        std::string startPath = IsWSL() ? "/mnt/c/Users/" : "";
        std::string command = "zenity --file-selection --filename=\"" + startPath + "\" --file-filter=\"PNG Image | *.png\" 2>/dev/null";
        return ExecuteCommand(command);
#endif
    }

    static std::string SaveFileDialog(const std::string& defaultName = "sprite_sheet.png") {
#ifdef _WIN32
        char szFile[260] = {0};
        strncpy_s(szFile, sizeof(szFile), defaultName.c_str(), _TRUNCATE);

        OPENFILENAMEA ofn;
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "PNG Image (*.png)\0*.png\0All Files (*.*)\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

        if (GetSaveFileNameA(&ofn) == TRUE) {
            std::string path = ofn.lpstrFile;
            if (path.find(".png") == std::string::npos) path += ".png";
            return path;
        }
        return "";
#else
        std::string startPath = IsWSL() ? "/mnt/c/Users/" + defaultName : defaultName;
        std::string command = "zenity --file-selection --save --confirm-overwrite --filename=\"" + startPath + "\" --file-filter=\"PNG Image | *.png\" 2>/dev/null";
        std::string path = ExecuteCommand(command);
        if (!path.empty() && path.find(".png") == std::string::npos) {
            path += ".png";
        }
        return path;
#endif
    }
};

}