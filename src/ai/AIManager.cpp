#include "AIManager.h"
#include <fstream>
#include <algorithm>
#include <thread>
#include <atomic>
#include <filesystem>

static std::atomic<bool> g_aiProcessing{ false };
static std::atomic<bool> g_aiFinished{ false };
static AIResult g_asyncResult;
static sf::Image g_asyncInputImage;

AIManager::AIManager() : aiEnabled(true) {}

void AIManager::init() {
    registerProvider(std::make_shared<GeminiProvider>());
    registerProvider(std::make_shared<OpenAIProvider>());
    registerProvider(std::make_shared<ClaudeProvider>());
    registerProvider(std::make_shared<OpenRouterProvider>());
    registerProvider(std::make_shared<OllamaProvider>());
    loadSettingsLocally();
}

void AIManager::abortTask() {
    taskAborted = true;
}

bool AIManager::isTaskAborted() {
    return taskAborted.load();
}

std::string AIManager::encryptKey(const std::string& key) {
    std::string encrypted = key;
    for (char& c : encrypted) c ^= 0x5A;
    return encrypted;
}

std::string AIManager::decryptKey(const std::string& encrypted) {
    std::string decrypted = encrypted;
    for (char& c : decrypted) c ^= 0x5A;
    return decrypted;
}

void AIManager::registerProvider(std::shared_ptr<AIProvider> provider) {
    if (provider) {
        providers[provider->getName()] = provider;
        if (apiKeys.find(provider->getName()) != apiKeys.end()) {
            provider->setApiKey(apiKeys[provider->getName()]);
        }
        if (activeProvider.empty()) activeProvider = provider->getName();
    }
}

void AIManager::setActiveProvider(const std::string& name) {
    if (providers.find(name) != providers.end()) activeProvider = name;
}

void AIManager::cycleProvider(int direction) {
    if (providers.empty()) return;
    std::vector<std::string> names = getAvailableProviders();
    auto it = std::find(names.begin(), names.end(), activeProvider);
    int idx = 0;
    if (it != names.end()) idx = std::distance(names.begin(), it);

    idx += direction;
    if (idx < 0) idx = names.size() - 1;
    if (idx >= static_cast<int>(names.size())) idx = 0;
    activeProvider = names[idx];
}

std::string AIManager::getActiveProvider() const { return activeProvider; }

std::vector<std::string> AIManager::getAvailableProviders() const {
    std::vector<std::string> names;
    for (const auto& pair : providers) names.push_back(pair.first);
    return names;
}

void AIManager::setApiKey(const std::string& providerName, const std::string& key) {
    apiKeys[providerName] = key;
    if (providers.find(providerName) != providers.end()) providers[providerName]->setApiKey(key);
    saveSettingsLocally();
}

std::string AIManager::getApiKey(const std::string& providerName) {
    if (apiKeys.find(providerName) != apiKeys.end()) return apiKeys[providerName];
    return "";
}

void AIManager::removeApiKey(const std::string& providerName) {
    apiKeys.erase(providerName);
    if (providers.find(providerName) != providers.end()) providers[providerName]->setApiKey("");
    saveSettingsLocally();
}

bool AIManager::testConnection(const std::string& providerName) {
    if (providers.find(providerName) != providers.end()) return providers[providerName]->testConnection();
    return false;
}

void AIManager::setAIEnabled(bool enabled) {
    aiEnabled = enabled;
    saveSettingsLocally();
}
void AIManager::resetAbortTask() {
    taskAborted = false;
}

bool AIManager::isAIEnabled() const { return aiEnabled; }

void AIManager::saveSettingsLocally() {
    std::ofstream out("ai_settings.dat", std::ios::binary);
    if (out.is_open()) {
        out << (aiEnabled ? "1" : "0") << "\n";
        out << activeProvider << "\n";
        out << apiKeys.size() << "\n";
        for (const auto& pair : apiKeys) {
            out << pair.first << "\n" << encryptKey(pair.second) << "\n";
        }
    }
}

void AIManager::loadSettingsLocally() {
    std::ifstream in("ai_settings.dat", std::ios::binary);
    if (in.is_open()) {
        std::string line;
        std::getline(in, line); aiEnabled = (line == "1");
        std::getline(in, activeProvider);
        std::getline(in, line);
        int keyCount = 0;
        try { keyCount = std::stoi(line); }
        catch (...) {}
        for (int i = 0; i < keyCount; ++i) {
            std::string pName, pKey;
            std::getline(in, pName); std::getline(in, pKey);
            apiKeys[pName] = decryptKey(pKey);
            if (providers.find(pName) != providers.end()) providers[pName]->setApiKey(apiKeys[pName]);
        }
    }
}

void AIManager::addHistory(const std::string& prompt, const std::string& negativePrompt) {
    AIPromptHistory h; h.prompt = prompt; h.negativePrompt = negativePrompt;
    h.providerName = activeProvider; h.timestamp = std::time(nullptr); h.isFavorite = false;
    history.insert(history.begin(), h);
    if (history.size() > 100) history.pop_back();
}

const std::vector<AIPromptHistory>& AIManager::getHistory() const { return history; }

static void asyncAIWorker(std::shared_ptr<AIProvider> provider, AIRequest request) {
    g_asyncResult = provider->process(request);
    if (AIManager::getInstance().isTaskAborted()) {
        AIManager::getInstance().resetAbortTask();
        g_asyncResult.success = false;
        g_asyncResult.errorMessage = "Task canceled by user.";

        // CRITICAL FIX: No 'return;' here! 
        // We let it fall through to the bottom of the function so the UI unlocks!
    }
    else {
        // Check if the external python file actually dropped the output asset
        if (std::filesystem::exists("temp_ai_output.png")) {
            sf::Image loadedResult;
            if (loadedResult.loadFromFile("temp_ai_output.png")) {
                g_asyncResult.resultImage = loadedResult;
                g_asyncResult.success = true;
            }
            else {
                g_asyncResult.success = false;
                g_asyncResult.errorMessage = "Output file corrupted or unreadable.";
            }
        }
        else {
            g_asyncResult.success = false;
            g_asyncResult.errorMessage = "Python script failed to drop asset.";
        }
    }
    // Check if the external python file actually dropped the output asset
    if (std::filesystem::exists("temp_ai_output.png")) {
        sf::Image loadedResult;
        if (loadedResult.loadFromFile("temp_ai_output.png")) {
            g_asyncResult.resultImage = loadedResult;
            g_asyncResult.success = true;
        }
        else {
            g_asyncResult.success = false;
            g_asyncResult.errorMessage = "Output file corrupted or unreadable.";
        }
    }
    else {
        g_asyncResult.success = false;
        // Provide clear diagnostic info to the user
        g_asyncResult.errorMessage = "Script failed to write 'temp_ai_output.png'. Check API Key / Console.";
    }

    g_aiFinished = true;
    g_aiProcessing = false;
}

AIResult AIManager::executeRequest(const AIRequest& request) {
    AIResult res;
    if (g_aiProcessing) {
        res.success = false;
        res.errorMessage = "A generation task is already active.";
        return res;
    }

    if (!aiEnabled || activeProvider.empty() || providers.find(activeProvider) == providers.end()) {
        res.success = false; res.errorMessage = "AI is disabled or no provider configured."; return res;
    }
    if (activeProvider != "Ollama (Local)" && (apiKeys.find(activeProvider) == apiKeys.end() || apiKeys[activeProvider].empty())) {
        res.success = false; res.errorMessage = "API key missing for provider: " + activeProvider; return res;
    }

    addHistory(request.prompt, request.negativePrompt);
    request.baseImage.saveToFile("temp_ai_input.png");
    g_asyncInputImage = request.baseImage;

    g_aiFinished = false;
    g_aiProcessing = true;

    std::thread worker(asyncAIWorker, providers[activeProvider], request);
    worker.detach();

    res.success = true;
    res.errorMessage = "PENDING";
    return res;
}

bool AIManager::isProcessingAsync() const {
    return g_aiProcessing;
}

bool AIManager::hasAsyncFinished() {
    if (g_aiFinished) {
        g_aiFinished = false;
        return true;
    }
    return false;
}

AIResult AIManager::getAsyncResult(sf::Image& outOriginalImage) {
    outOriginalImage = g_asyncInputImage;
    return g_asyncResult;
}