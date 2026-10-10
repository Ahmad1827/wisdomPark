#pragma once
#include <string>
#include <vector>
#include <SFML/Graphics.hpp>

enum class AIOperation {
    Generate,
    Edit,
    Variation,
    RemoveBackground,
    Upscale,
    Colorize,
    Inpaint,
    Outpaint,
    GenerateFrame
};

struct AIRequest {
    std::string prompt;
    std::string negativePrompt;
    AIOperation operation = AIOperation::Generate;
    sf::Image baseImage;
    sf::Image maskImage;
    bool hasMask = false;
    bool isPixelMode = false;
    int width = 0;
    int height = 0;
    float transparency = 1.0f;
    sf::Color primaryColor = sf::Color::Black;
};

// Stops the Claude bridge that is running, if any, along with everything it started.
void cancelClaudeArt();

struct AIResult {
    bool success = false;
    std::string errorMessage;
    sf::Image resultImage;
};

class AIProvider {
protected:
    std::string apiKey;
public:
    virtual ~AIProvider() = default;
    virtual void setApiKey(const std::string& key) { apiKey = key; }
    virtual bool testConnection() = 0;
    virtual AIResult process(const AIRequest& request) = 0;
    virtual std::string getName() const = 0;
    virtual bool requiresApiKey() const { return true; }
};

class GeminiProvider : public AIProvider {
public:
    bool testConnection() override;
    AIResult process(const AIRequest& request) override;
    std::string getName() const override;
};

class OpenAIProvider : public AIProvider {
public:
    bool testConnection() override;
    AIResult process(const AIRequest& request) override;
    std::string getName() const override;
};

class ClaudeProvider : public AIProvider {
public:
    bool testConnection() override;
    AIResult process(const AIRequest& request) override;
    std::string getName() const override;
};

// Claude through the user's own Claude Code login instead of an API key.
class ClaudeCodeProvider : public AIProvider {
public:
    bool testConnection() override;
    AIResult process(const AIRequest& request) override;
    std::string getName() const override;
    bool requiresApiKey() const override { return false; }
};

class OpenRouterProvider : public AIProvider {
public:
    bool testConnection() override;
    AIResult process(const AIRequest& request) override;
    std::string getName() const override;
};

class OllamaProvider : public AIProvider {
public:
    bool testConnection() override;
    AIResult process(const AIRequest& request) override;
    std::string getName() const override;
    bool requiresApiKey() const override { return false; }
};