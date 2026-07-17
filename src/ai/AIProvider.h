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
};

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
};