#include "LocalPythonBridge.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <algorithm>

std::vector<std::string> LocalPythonBridge::executeAndReadBlueprint(const std::string& providerName, const std::string& apiKey, const std::string& prompt) {
    std::vector<std::string> blueprint;

    std::string safePrompt = prompt;
    std::replace(safePrompt.begin(), safePrompt.end(), '"', '\'');

    std::string command = "python scripts/run_ai.py --provider " + providerName + " --key " + apiKey + " --prompt \"" + safePrompt + "\"";

    int result = std::system(command.c_str());

    if (result != 0) {
        std::cerr << "Python Bridge Execution Failed!" << std::endl;
        return blueprint;
    }

    std::ifstream file("temp_blueprint.txt");
    if (!file.is_open()) return blueprint;

    std::string line;
    while (std::getline(file, line)) {
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
        line.erase(std::remove(line.begin(), line.end(), ' '), line.end());
        if (!line.empty()) blueprint.push_back(line);
    }
    file.close();
    return blueprint;
}