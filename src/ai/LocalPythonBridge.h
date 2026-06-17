#pragma once
#include <string>
#include <vector>
#include <memory>
#include "AIProvider.h"

class LocalPythonBridge {
public:
    static std::vector<std::string> executeAndReadBlueprint(const std::string& providerName, const std::string& apiKey, const std::string& prompt);
};