#pragma once
#include <string>

namespace StudioCore {

struct WorkspacePreferences {
    int defaultFps{12};
    int defaultPadding{0};
    std::string theme{"Dark"};
    bool gridVisibility{true};
    std::string gridColor{"#FFFFFF"};
    int autoSaveInterval{300};
    int recentProjectLimit{10};
};

}