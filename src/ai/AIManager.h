#pragma once
#include "AIProvider.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <atomic>



struct AIPromptHistory {
    std::string prompt;
    std::string negativePrompt;
    std::string providerName;
    std::time_t timestamp;
    bool isFavorite;
};

class AIManager {
private:
    std::atomic<bool> taskAborted{ false };
    std::map<std::string, std::shared_ptr<AIProvider>> providers;
    std::map<std::string, std::string> apiKeys;
    std::string activeProvider;
    bool aiEnabled;
    std::vector<AIPromptHistory> history;

    AIManager();
    std::string encryptKey(const std::string& key);
    std::string decryptKey(const std::string& encrypted);

public:
    void abortTask();
    void resetAbortTask();
    bool isTaskAborted();
    static AIManager& getInstance() {
        static AIManager instance;
        return instance;
    }

    void init();
    void registerProvider(std::shared_ptr<AIProvider> provider);
    void setActiveProvider(const std::string& name);
    void cycleProvider(int direction);
    std::string getActiveProvider() const;
    std::vector<std::string> getAvailableProviders() const;

    void setApiKey(const std::string& providerName, const std::string& key);
    std::string getApiKey(const std::string& providerName);
    void removeApiKey(const std::string& providerName);

    bool testConnection(const std::string& providerName);

    void setAIEnabled(bool enabled);
    bool isAIEnabled() const;

    void saveSettingsLocally();
    void loadSettingsLocally();

    void addHistory(const std::string& prompt, const std::string& negativePrompt);
    const std::vector<AIPromptHistory>& getHistory() const;

    AIResult executeRequest(const AIRequest& request);

    bool isProcessingAsync() const;
    bool hasAsyncFinished();
    AIResult getAsyncResult(sf::Image& outOriginalImage);
};