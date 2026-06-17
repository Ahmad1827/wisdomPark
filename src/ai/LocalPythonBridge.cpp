#include "LocalPythonBridge.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <algorithm>

std::vector<std::string> LocalPythonBridge::executeAndReadBlueprint(const std::string& providerName, const std::string& apiKey, const std::string& prompt) {
    std::vector<std::string> blueprint;
    std::unique_ptr<AIProvider> provider;

    if (providerName == "openai") provider = std::make_unique<OpenAIProvider>();
    else if (providerName == "gemini") provider = std::make_unique<GeminiProvider>();
    else if (providerName == "anthropic") provider = std::make_unique<AnthropicProvider>();
    else if (providerName == "openrouter") provider = std::make_unique<OpenRouterProvider>();
    else return blueprint;

    std::string command = provider->generateCommand(prompt, apiKey);
    int result = std::system(command.c_str());

    if (result != 0) {
        std::cerr << "Python Bridge Execution Failed!" << std::endl;
        return blueprint;
    }

    std::ifstream file("temp_blueprint.txt");
    if (!file.is_open()) {
        std::cerr << "Failed to open temp_blueprint.txt" << std::endl;
        return blueprint;
    }

    std::string line;
    while (std::getline(file, line)) {
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
        line.erase(std::remove(line.begin(), line.end(), ' '), line.end());
        if (!line.empty()) {
            blueprint.push_back(line);
        }
    }
    file.close();
    return blueprint;
}