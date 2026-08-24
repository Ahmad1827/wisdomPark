#include "Utils/NativeFileDialog.h"

#if defined(_WIN32)
#include <windows.h>
#include <commdlg.h>
#endif

std::string NativeFileDialog::OpenFileDialog(const std::string& filter) {
#if defined(_WIN32)
    char currentDir[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, currentDir);

    OPENFILENAMEA ofn;
    char szFile[MAX_PATH] = { 0 };
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "All Files (*.*)\0*.*\0Image Files (*.png;*.jpg)\0*.png;*.jpg\0\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

    std::string res = "";
    if (GetOpenFileNameA(&ofn)) {
        res = std::string(ofn.lpstrFile);
    }

    SetCurrentDirectoryA(currentDir);
    return res;
#else
    return "";
#endif
}

std::string NativeFileDialog::SaveFileDialog(const std::string& defaultName) {
#if defined(_WIN32)
    char currentDir[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, currentDir);

    OPENFILENAMEA ofn;
    char szFile[MAX_PATH] = { 0 };
    strcpy_s(szFile, defaultName.c_str());
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Sprite Sheet Studio (*.sps)\0*.sps\0All Files (*.*)\0*.*\0\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

    std::string res = "";
    if (GetSaveFileNameA(&ofn)) {
        res = std::string(ofn.lpstrFile);
    }

    SetCurrentDirectoryA(currentDir);
    return res;
#else
    return "";
#endif
}