#include "AIProvider.h"

std::string OpenAIProvider::getProviderName() const { return "openai"; }
std::string OpenAIProvider::generateCommand(const std::string& prompt, const std::string& apiKey) const {
    return "python scripts/brain.py \"openai\" \"" + apiKey + "\" \"" + prompt + "\"";
}

std::string GeminiProvider::getProviderName() const { return "gemini"; }
std::string GeminiProvider::generateCommand(const std::string& prompt, const std::string& apiKey) const {
    return "python scripts/brain.py \"gemini\" \"" + apiKey + "\" \"" + prompt + "\"";
}

std::string AnthropicProvider::getProviderName() const { return "anthropic"; }
std::string AnthropicProvider::generateCommand(const std::string& prompt, const std::string& apiKey) const {
    return "python scripts/brain.py \"anthropic\" \"" + apiKey + "\" \"" + prompt + "\"";
}

std::string OpenRouterProvider::getProviderName() const { return "openrouter"; }
std::string OpenRouterProvider::generateCommand(const std::string& prompt, const std::string& apiKey) const {
    return "python scripts/brain.py \"openrouter\" \"" + apiKey + "\" \"" + prompt + "\"";
}