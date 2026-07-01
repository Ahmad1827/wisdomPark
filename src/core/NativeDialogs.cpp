#include "NativeDialogs.h"

#ifdef _WIN32
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>
#include <iostream>

std::string NativeDialogs::saveFileDialog(const std::string& filter, const std::string& defaultExt, const std::string& defaultName) {
    OPENFILENAMEA ofn;
    char szFile[260];

    strncpy_s(szFile, sizeof(szFile), defaultName.c_str(), _TRUNCATE);

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = filter.c_str();
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrDefExt = defaultExt.c_str();
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    if (GetSaveFileNameA(&ofn) == TRUE) {
        return std::string(ofn.lpstrFile);
    }
    return "";
}

std::string NativeDialogs::openFileDialog(const std::string& filter) {
    OPENFILENAMEA ofn;
    char szFile[260] = { 0 };

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = filter.c_str();
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA(&ofn) == TRUE) {
        return std::string(ofn.lpstrFile);
    }
    return "";
}

std::string NativeDialogs::selectFolderDialog() {
    BROWSEINFOA bi = { 0 };
    bi.lpszTitle = "Select Folder";
    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
    if (pidl != 0) {
        char path[MAX_PATH];
        if (SHGetPathFromIDListA(pidl, path)) {
            return std::string(path);
        }
    }
    return "";
}

#else

std::string NativeDialogs::saveFileDialog(const std::string& filter, const std::string& defaultExt, const std::string& defaultName) {
    return "";
}

std::string NativeDialogs::openFileDialog(const std::string& filter) {
    return "";
}

std::string NativeDialogs::selectFolderDialog() {
    return "";
}

#endif