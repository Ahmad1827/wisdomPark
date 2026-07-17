#include "AIProvider.h"
#include <cstdlib>

bool GeminiProvider::testConnection() { return !apiKey.empty(); }

std::string GeminiProvider::getName() const { return "Gemini"; }

AIResult GeminiProvider::process(const AIRequest& request) {
    AIResult res;
    std::string cmd = "python scripts/run_ai.py --provider gemini --key \"" + apiKey + "\" --prompt \"" + request.prompt + "\"";
    int exitCode = std::system(cmd.c_str());
    res.success = (exitCode == 0);
    return res;
}

bool OpenAIProvider::testConnection() { return !apiKey.empty(); }

std::string OpenAIProvider::getName() const { return "OpenAI"; }

AIResult OpenAIProvider::process(const AIRequest& request) {
    AIResult res;
    std::string cmd = "python scripts/run_ai.py --provider openai --key \"" + apiKey + "\" --prompt \"" + request.prompt + "\"";
    int exitCode = std::system(cmd.c_str());
    res.success = (exitCode == 0);
    return res;
}

bool ClaudeProvider::testConnection() { return !apiKey.empty(); }

std::string ClaudeProvider::getName() const { return "Claude"; }

AIResult ClaudeProvider::process(const AIRequest& request) {
    AIResult res;
    std::string cmd = "python scripts/run_ai.py --provider claude --key \"" + apiKey + "\" --prompt \"" + request.prompt + "\"";
    int exitCode = std::system(cmd.c_str());
    res.success = (exitCode == 0);
    return res;
}

bool OpenRouterProvider::testConnection() { return !apiKey.empty(); }

std::string OpenRouterProvider::getName() const { return "OpenRouter"; }

AIResult OpenRouterProvider::process(const AIRequest& request) {
    AIResult res;
    std::string cmd = "python scripts/run_ai.py --provider openrouter --key \"" + apiKey + "\" --prompt \"" + request.prompt + "\"";
    int exitCode = std::system(cmd.c_str());
    res.success = (exitCode == 0);
    return res;
}

bool OllamaProvider::testConnection() { return true; }

std::string OllamaProvider::getName() const { return "Ollama (Local)"; }

AIResult OllamaProvider::process(const AIRequest& request) {
    AIResult res;
    std::string cmd = "python scripts/run_ai.py --provider ollama --key \"none\" --prompt \"" + request.prompt + "\"";
    int exitCode = std::system(cmd.c_str());
    res.success = (exitCode == 0);
    return res;
}