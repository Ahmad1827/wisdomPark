#pragma once
#include <string>

namespace StudioCore {

struct ExportSettings {
    int cellWidth{32};
    int cellHeight{32};
    int padding{0};
    int margin{0};
    int columns{0};
    int rows{0};
    std::string format{"PNG"};
};

}