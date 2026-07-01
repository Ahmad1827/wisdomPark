#pragma once
#include <string>

class NativeDialogs {
public:
    static std::string saveFileDialog(const std::string& filter, const std::string& defaultExt, const std::string& defaultName = "");
    static std::string openFileDialog(const std::string& filter);
    static std::string selectFolderDialog();
};