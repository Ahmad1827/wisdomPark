#include "AIProvider.h"
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <filesystem>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

// Starts a command, feeds it `input` on stdin and waits for it. Returns false if it could not be started.
static bool runWithStdin(const std::string& command, const std::string& input, int& exitCode) {
#if defined(_WIN32)
    // The app has no console of its own, so _popen would pop a cmd window over it; this keeps the child hidden
    SECURITY_ATTRIBUTES sa{ sizeof(sa), nullptr, TRUE };
    HANDLE stdinRead = nullptr, stdinWrite = nullptr;
    if (!CreatePipe(&stdinRead, &stdinWrite, &sa, 0)) return false;
    SetHandleInformation(stdinWrite, HANDLE_FLAG_INHERIT, 0);
    HANDLE nul = CreateFileA("NUL", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, nullptr);

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = stdinRead;
    si.hStdOutput = nul;
    si.hStdError = nul;

    PROCESS_INFORMATION pi{};
    std::string cmdLine = command;
    BOOL started = CreateProcessA(nullptr, cmdLine.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
    CloseHandle(stdinRead);
    if (nul != INVALID_HANDLE_VALUE) CloseHandle(nul);
    if (!started) {
        CloseHandle(stdinWrite);
        return false;
    }

    DWORD written = 0;
    WriteFile(stdinWrite, input.data(), static_cast<DWORD>(input.size()), &written, nullptr);
    CloseHandle(stdinWrite);

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    exitCode = static_cast<int>(code);
    return true;
#else
    FILE* pipe = popen(command.c_str(), "w");
    if (!pipe) return false;
    fwrite(input.data(), 1, input.size(), pipe);
    exitCode = pclose(pipe);
    return true;
#endif
}

// Runs scripts/claude_art.py, which finishes the drawing in temp_ai_input.png (or
// carries out request.prompt on it) and writes temp_ai_output.png. The job goes over stdin so the key and the user's
// prompt never end up on a command line.
static AIResult runClaudeArt(const std::string& backend, const std::string& apiKey, const AIRequest& request) {
    AIResult res;
    std::error_code ec;
    std::filesystem::remove("temp_ai_output.png", ec);
    std::filesystem::remove("temp_ai_error.txt", ec);
    std::filesystem::remove("temp_ai_status.txt", ec);

    char color[8];
    snprintf(color, sizeof(color), "#%02x%02x%02x", request.primaryColor.r, request.primaryColor.g, request.primaryColor.b);
    std::string job = backend + "\n" + apiKey + "\n" + color + "\n" + request.prompt;
    int exitCode = 1;
    if (!runWithStdin("python scripts/claude_art.py", job, exitCode)) {
        res.errorMessage = "Could not start Python. Is it installed and on PATH?";
        return res;
    }

    res.success = (exitCode == 0) && std::filesystem::exists("temp_ai_output.png");
    if (!res.success) {
        std::ifstream err("temp_ai_error.txt");
        std::stringstream ss;
        ss << err.rdbuf();
        res.errorMessage = ss.str();
        if (res.errorMessage.empty()) res.errorMessage = "Python or scripts/claude_art.py could not be run.";
    }
    return res;
}

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
    return runClaudeArt("api", apiKey, request);
}

bool ClaudeCodeProvider::testConnection() { return true; }

std::string ClaudeCodeProvider::getName() const { return "Claude Code"; }

AIResult ClaudeCodeProvider::process(const AIRequest& request) {
    return runClaudeArt("cli", "", request);
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