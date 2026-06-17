#pragma once
#include <string>

class AIProvider {
public:
    virtual ~AIProvider() = default;
    virtual std::string getProviderName() const = 0;
    virtual std::string generateCommand(const std::string& prompt, const std::string& apiKey) const = 0;
};

class OpenAIProvider : public AIProvider {
public:
    std::string getProviderName() const override;
    std::string generateCommand(const std::string& prompt, const std::string& apiKey) const override;
};

class GeminiProvider : public AIProvider {
public:
    std::string getProviderName() const override;
    std::string generateCommand(const std::string& prompt, const std::string& apiKey) const override;
};

class AnthropicProvider : public AIProvider {
public:
    std::string getProviderName() const override;
    std::string generateCommand(const std::string& prompt, const std::string& apiKey) const override;
};

class OpenRouterProvider : public AIProvider {
public:
    std::string getProviderName() const override;
    std::string generateCommand(const std::string& prompt, const std::string& apiKey) const override;
};