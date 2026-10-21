#include "ChatPanel.h"
#include "UITheme.h"
#include "../ai/AIProvider.h"
#include "json.hpp"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <regex>
#include <thread>

namespace {
    const unsigned int kTextSize = 15;
    const float kLineHeight = 20.f;
    const float kPad = 12.f;
    const float kMessageGap = 12.f;
    const unsigned int kInputMaxLength = 1200;
    const int kInputMaxLines = 5;
    const char* kModelFile = "chat_model.txt";
    // Every finished run, so the estimate learns how long Claude really takes on this computer
    const char* kTimingsFile = "chat_timings.jsonl";
    // The plan usage Claude Code last reported, and how fast runs use it up
    const char* kUsageFile = "chat_usage.json";
    const char* kProbeFile = "chat_usage_probe.json";

    // The colours of Claude Code in a dark terminal
    const sf::Color kBg(21, 21, 21);
    const sf::Color kEdge(58, 58, 58);
    const sf::Color kText(232, 232, 232);
    const sf::Color kDim(138, 138, 138);
    const sf::Color kClaude(217, 119, 87);
    const sf::Color kUserBg(44, 44, 44);
    const sf::Color kError(255, 107, 107);
    const sf::Color kBoxEdge(112, 112, 112);
    const sf::Color kHover(48, 48, 48);

    struct Model {
        const char* label;
        const char* id;
        // How long it takes next to Opus on the same request, before any runs have been timed
        float speed;
    };
    const Model kModels[] = {
        { "Default", "", 1.0f },
        { "Opus 5.5", "claude-opus-5-5", 1.0f },
        { "Sonnet 5.5", "claude-sonnet-5-5", 0.65f },
        { "Haiku 5.5", "claude-haiku-5-5", 0.4f },
        { "Fable 5.1", "claude-fable-5-1", 1.0f },
    };
    const int kModelCount = static_cast<int>(sizeof(kModels) / sizeof(kModels[0]));

    // How finely a big area is drawn: the longest side Claude draws it at. Finer takes longer.
    struct Detail {
        const char* label;
        int side;
    };
    const Detail kDetails[] = {
        { "low", 160 },
        { "medium", 320 },
        { "high", 640 },
        { "full", 0 },
    };
    const int kDetailCount = static_cast<int>(sizeof(kDetails) / sizeof(kDetails[0]));

    std::string lower(std::string s) {
        for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
    }

    // "~40s", "~2m 10s", "~12m"
    std::string roughTime(float seconds) {
        int s = static_cast<int>(std::round(seconds));
        if (s < 55) return "~" + std::to_string(std::max(5, (s + 2) / 5 * 5)) + "s";
        if (s < 600) {
            s = (s + 5) / 10 * 10;
            return "~" + std::to_string(s / 60) + "m" + (s % 60 ? " " + std::to_string(s % 60) + "s" : "");
        }
        return "~" + std::to_string((s + 30) / 60) + "m";
    }

    // "0:42", "12:05"
    std::string clockTime(float seconds) {
        int s = static_cast<int>(seconds);
        char buf[16];
        snprintf(buf, sizeof(buf), "%d:%02d", s / 60, s % 60);
        return buf;
    }

    sf::String utf8(const std::string& s) { return sf::String::fromUtf8(s.begin(), s.end()); }
}

ChatPanel::ChatPanel() = default;

void ChatPanel::init() {
    // A terminal font when Windows has one; the app's own font otherwise
    if (!m_font.loadFromFile("C:/Windows/Fonts/CascadiaMono.ttf") && !m_font.loadFromFile("C:/Windows/Fonts/consola.ttf")) {
        m_font.loadFromFile("assets/Jersey10-Regular.ttf");
    }
    m_charW = m_font.getGlyph('M', kTextSize, false).advance;

    // Claude Code's spinner and mark, where the font has them
    const sf::Uint32 stars[] = { 0x00B7, 0x2722, 0x2733, 0x2736, 0x273B, 0x273D };
    bool haveStars = true;
    for (sf::Uint32 c : stars) haveStars = haveStars && m_font.hasGlyph(c);
    if (haveStars) {
        for (sf::Uint32 c : stars) m_spinner += c;
        for (int i = 4; i >= 1; --i) m_spinner += stars[i];
    }
    else m_spinner = "|/-\\";
    m_mark = m_font.hasGlyph(0x273B) ? sf::String(static_cast<sf::Uint32>(0x273B)) : sf::String("*");

    // The model on the first line, the detail on the second (added later, so it may be missing)
    std::ifstream modelIn(kModelFile);
    std::string saved;
    if (std::getline(modelIn, saved)) {
        for (int i = 0; i < kModelCount; ++i) {
            if (saved == kModels[i].id || (i == 0 && saved == "default")) m_model = i;
        }
    }
    if (std::getline(modelIn, saved)) {
        for (int i = 0; i < kDetailCount; ++i) {
            if (saved == kDetails[i].label) m_detail = i;
        }
    }

    try {
        std::ifstream usageIn(kUsageFile, std::ios::binary);
        if (usageIn) {
            nlohmann::json u = nlohmann::json::parse(usageIn);
            m_usage.fiveHour = u.value("five_hour", -1.f);
            m_usage.fiveHourResets = u.value("five_hour_resets", 0LL);
            m_usage.week = u.value("week", -1.f);
            m_usage.weekResets = u.value("week_resets", 0LL);
            m_usage.percentSum = u.value("percent_sum", 0.0);
            m_usage.costSum = u.value("cost_sum", 0.0);
            if (u.contains("models")) {
                for (auto& [model, v] : u["models"].items()) m_usage.perModel[model] = { v.value("cost", 0.0), v.value("seconds", 0.0) };
            }
        }
    }
    catch (...) {}

    std::ifstream timingsIn(kTimingsFile, std::ios::binary);
    std::string line;
    while (std::getline(timingsIn, line)) {
        try {
            nlohmann::json t = nlohmann::json::parse(line);
            m_timings.push_back({ t.value("model", ""), t.value("estimate", 0.f), t.value("seconds", 0.f) });
        }
        catch (...) {}
    }
}

void ChatPanel::setContext(const std::string& folderLabel, bool hasSprite, unsigned int width, unsigned int height, const std::string& areaLabel, bool scaled) {
    m_folderLabel = folderLabel;
    m_areaLabel = areaLabel;
    m_scaled = scaled;
    m_hasSprite = hasSprite;
    m_canvasW = width;
    m_canvasH = height;
}

std::string ChatPanel::modelId() const { return kModels[m_model].id; }

int ChatPanel::detailSide() const { return kDetails[m_detail].side; }

void ChatPanel::setMarking(bool marking, const std::string& markLabel) {
    m_marking = marking;
    m_markLabel = markLabel;
}

void ChatPanel::setModel(int index) {
    m_model = std::clamp(index, 0, kModelCount - 1);
    saveChoices();
}

void ChatPanel::setDetail(int index) {
    m_detail = std::clamp(index, 0, kDetailCount - 1);
    saveChoices();
}

void ChatPanel::saveChoices() const {
    std::ofstream out(kModelFile);
    out << (m_model == 0 ? "default" : kModels[m_model].id) << "\n" << kDetails[m_detail].label << "\n";
}

ChatPanel::Job ChatPanel::estimate(const std::string& message, int model) const {
    Job job;
    std::string m = lower(message);
    auto has = [&](std::initializer_list<const char*> words) {
        for (const char* w : words) if (m.find(w) != std::string::npos) return true;
        return false;
    };

    // How many frames the message asks for: a number of frames, an animation, or a single drawing
    std::smatch found;
    static const std::regex counted("(\\d+)\\s*-?\\s*frames?");
    static const std::regex spelled("\\b(two|three|four|five|six|seven|eight|nine|ten|eleven|twelve)\\s*-?\\s*frames?");
    const char* numbers[] = { "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve" };
    bool animated = has({ "walk", "run", "jump", "attack", "idle", "animat", "cycle", "loop", "swing", "blink", "wave" });
    if (std::regex_search(m, found, counted)) job.frames = std::atoi(found[1].str().c_str());
    else if (std::regex_search(m, found, spelled)) {
        for (int i = 0; i < 11; ++i) if (found[1] == numbers[i]) job.frames = i + 2;
    }
    else job.frames = animated ? 6 : 1;
    job.frames = std::clamp(job.frames, 1, 32);

    bool drawing = animated || has({ "draw", "make", "create", "add", "change", "turn", "give", "paint", "colo", "fix",
        "redo", "move", "frame", "sprite", "bigger", "smaller", "remove", "shade", "outline", "darker", "lighter", "put" });
    float sizeFactor = 1.f + std::max(m_canvasW, m_canvasH) / 256.f;
    if (!drawing) { job.kind = "chat"; job.seconds = 12.f; }
    else if (!m_hasSprite) { job.kind = "new"; job.seconds = (90.f + 18.f * job.frames) * sizeFactor; }
    else { job.kind = "edit"; job.seconds = (45.f + 14.f * job.frames) * sizeFactor; }
    job.seconds *= kModels[model].speed;

    // Scaled by how the last runs went against their estimates: this model's own when it has any
    std::vector<float> ratios;
    for (int pass = 0; pass < 2 && ratios.empty(); ++pass) {
        for (auto it = m_timings.rbegin(); it != m_timings.rend() && ratios.size() < 8; ++it) {
            if ((pass == 1 || it->model == kModels[model].id) && it->estimate > 0.f) ratios.push_back(it->seconds / it->estimate);
        }
    }
    if (!ratios.empty()) {
        std::nth_element(ratios.begin(), ratios.begin() + ratios.size() / 2, ratios.end());
        job.seconds *= std::clamp(ratios[ratios.size() / 2], 0.3f, 4.f);
    }

    // Time turns into cost at the rate this model's runs have gone (all models' when it has none yet), and
    // cost into a share of the 5-hour limit at the rate the limit has moved against it
    double cost = 0.0, seconds = 0.0;
    auto own = m_usage.perModel.find(kModels[model].id);
    if (own != m_usage.perModel.end() && own->second.second > 0.0) { cost = own->second.first; seconds = own->second.second; }
    else for (const auto& [id, cs] : m_usage.perModel) { cost += cs.first; seconds += cs.second; }
    if (seconds > 0.0 && m_usage.costSum > 0.0 && m_usage.percentSum > 0.0) {
        job.percent = static_cast<float>(job.seconds * (cost / seconds) * (m_usage.percentSum / m_usage.costSum));
    }
    return job;
}

void ChatPanel::finishRun(bool success) {
    if (success && m_runEstimate > 0.f && m_busyTime > 2.f) {
        m_timings.push_back({ m_runModel, m_runEstimate, m_busyTime });
        std::ofstream out(kTimingsFile, std::ios::app | std::ios::binary);
        out << nlohmann::json{ {"model", m_runModel}, {"estimate", m_runEstimate}, {"seconds", m_busyTime} }.dump() << "\n";
    }
    m_runEstimate = 0.f;
}

std::string ChatPanel::reportUsage(float fiveHour, long long fiveHourResets, float week, long long weekResets, double cost) {
    if (fiveHour < 0.f) return "";
    float seconds = m_busyTime;
    if (cost > 0.0 && seconds > 2.f) {
        auto& cs = m_usage.perModel[m_runModel];
        cs.first += cost;
        cs.second += seconds;
    }
    // What this run used is how far the limit moved since the last run reported it, in the same 5-hour
    // window. Claude used anywhere else meanwhile counts too, and the limit moves in whole percents.
    float used = -1.f;
    if (m_usage.fiveHour >= 0.f && fiveHourResets == m_usage.fiveHourResets) used = std::max(0.f, fiveHour - m_usage.fiveHour) * 100.f;
    if (used >= 0.f && cost > 0.0) {
        m_usage.percentSum += used;
        m_usage.costSum += cost;
    }
    m_usage.fiveHour = fiveHour;
    m_usage.fiveHourResets = fiveHourResets;
    m_usage.week = week;
    m_usage.weekResets = weekResets;
    saveUsage();

    std::string now = std::to_string(static_cast<int>(std::round(fiveHour * 100.f))) + "%";
    std::string weekNow = week >= 0.f ? ", week " + std::to_string(static_cast<int>(std::round(week * 100.f))) + "%" : "";
    if (used < 0.f) return "5-hour limit now at " + now + weekNow;
    std::string usedText = used < 1.f ? "under 1%" : "~" + std::to_string(static_cast<int>(std::round(used))) + "%";
    return "Used " + usedText + " of your 5-hour limit (now " + now + weekNow + ")";
}

void ChatPanel::probeUsage() {
    m_probe->store(1);
    std::shared_ptr<std::atomic<int>> state = m_probe;
    std::thread([state]() {
        std::error_code ec;
        std::filesystem::remove(kProbeFile, ec);
        int exitCode = 1;
        bool ran = runHiddenProcess("python scripts/sprite/claude_usage.py", exitCode);
        state->store(ran && exitCode == 0 ? 2 : 3);
    }).detach();
}

void ChatPanel::saveUsage() const {
    nlohmann::json models = nlohmann::json::object();
    for (const auto& [id, cs] : m_usage.perModel) models[id] = { {"cost", cs.first}, {"seconds", cs.second} };
    nlohmann::json u = {
        {"five_hour", m_usage.fiveHour}, {"five_hour_resets", m_usage.fiveHourResets},
        {"week", m_usage.week}, {"week_resets", m_usage.weekResets},
        {"percent_sum", m_usage.percentSum}, {"cost_sum", m_usage.costSum}, {"models", models}
    };
    std::ofstream out(kUsageFile, std::ios::binary);
    out << u.dump(1) << "\n";
}

// "5h limit 42% (resets 20:00), week 5%", from the last run; a window that has reset since shows as such
std::string ChatPanel::usageLine() const {
    int probe = m_probe->load();
    if (probe == 1) return "5h limit: checking...";
    if (m_usage.fiveHour < 0.f) return probe == 3 ? "5h limit: unknown (is Claude Code set up?)" : "5h limit: ...";
    long long now = static_cast<long long>(std::time(nullptr));
    std::string line;
    if (now >= m_usage.fiveHourResets) line = "5h limit reset";
    else {
        std::time_t t = static_cast<std::time_t>(m_usage.fiveHourResets);
        std::tm local{};
        localtime_s(&local, &t);
        char at[8];
        std::strftime(at, sizeof(at), "%H:%M", &local);
        line = "5h limit " + std::to_string(static_cast<int>(std::round(m_usage.fiveHour * 100.f))) + "% (resets " + at + ")";
    }
    if (m_usage.week >= 0.f && now < m_usage.weekResets) line += ", week " + std::to_string(static_cast<int>(std::round(m_usage.week * 100.f))) + "%";
    return line;
}

void ChatPanel::addMessage(Role role, const std::string& utf8Text) {
    m_messages.push_back({ role, utf8(utf8Text) });
    m_scroll = 0.f;
}

void ChatPanel::clearMessages() {
    m_messages.clear();
    m_scroll = 0.f;
}

void ChatPanel::loadHistory(const std::string& chatFile) {
    clearMessages();
    std::ifstream in(chatFile, std::ios::binary);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        try {
            nlohmann::json entry = nlohmann::json::parse(line);
            std::string role = entry.value("role", "");
            std::string text = entry.value("text", "");
            Role r = role == "artist" ? Role::Artist : role == "claude" ? Role::Claude : Role::Error;
            addMessage(r, text);
        }
        catch (...) {
            // A line cut short (the app closed mid-write) is skipped rather than losing the rest
        }
    }
}

std::string ChatPanel::takeAction() {
    std::string a = m_pendingAction;
    m_pendingAction.clear();
    return a;
}

std::string ChatPanel::takeMessage() {
    std::string m = m_pendingMessage;
    m_pendingMessage.clear();
    return m;
}

std::string ChatPanel::inputUtf8() const {
    std::basic_string<sf::Uint8> bytes = m_input.toUtf8();
    return std::string(bytes.begin(), bytes.end());
}

void ChatPanel::send() {
    std::string text = inputUtf8();
    size_t start = text.find_first_not_of(" \t\n");
    // Nothing but spaces is not a message
    if (start == std::string::npos) return;
    if (text[start] == '/') {
        runCommand(text.substr(start));
        m_input.clear();
        m_caret = 0;
        return;
    }
    if (m_busy) return;
    Job job = estimate(text, m_model);
    m_runModel = kModels[m_model].id;
    m_runEstimate = job.seconds;
    m_pendingMessage = text;
    m_pendingAction = "send";
    m_input.clear();
    m_caret = 0;
}

void ChatPanel::runCommand(const std::string& command) {
    size_t space = command.find_first_of(" \t");
    std::string name = lower(command.substr(0, space));
    std::string arg;
    if (space != std::string::npos) {
        size_t a = command.find_first_not_of(" \t", space);
        if (a != std::string::npos) arg = lower(command.substr(a, command.find_last_not_of(" \t\n") + 1 - a));
    }

    if (name == "/clear" || name == "/new") {
        if (m_busy) addMessage(Role::Note, "Claude is still working. Wait for it, or press Esc to stop it.");
        else m_pendingAction = "new_chat";
    }
    else if (name == "/model") {
        if (arg.empty()) { m_pickerOpen = true; return; }
        for (int i = 0; i < kModelCount; ++i) {
            if (lower(kModels[i].label).find(arg) != std::string::npos || arg == kModels[i].id) {
                setModel(i);
                addMessage(Role::Note, std::string("Model set to ") + kModels[i].label);
                return;
            }
        }
        addMessage(Role::Note, "No model called \"" + arg + "\". Try opus, sonnet, haiku, fable or default.");
    }
    else if (name == "/area") m_pendingAction = "mark_area";
    else if (name == "/detail") {
        for (int i = 0; i < kDetailCount; ++i) {
            if (arg == kDetails[i].label) {
                setDetail(i);
                addMessage(Role::Note, std::string("Detail set to ") + kDetails[i].label);
                return;
            }
        }
        addMessage(Role::Note, "Detail is low, medium, high or full: how finely Claude draws a big canvas. Finer takes longer.");
    }
    else if (name == "/help") {
        addMessage(Role::Note, "/model  pick the model Claude runs on\n/area   mark where on the canvas Claude draws\n/detail low, medium, high or full: how finely a big canvas is drawn\n/clear  start a new conversation\n"
            "Enter sends, Shift+Enter adds a line, Esc stops Claude while it works. Under the box: how long the typed "
            "request should take and how much of your 5-hour limit it should use (?% until two replies have shown how fast "
            "Claude uses it); both learn from your runs.");
    }
    else addMessage(Role::Note, "Unknown command " + name + ". /help lists them.");
}

std::vector<sf::String> ChatPanel::wrap(const sf::String& text, unsigned int size, float maxWidth) const {
    std::vector<sf::String> lines;
    // Glyph advances added up: laying out an sf::Text for every word tried made the panel crawl
    auto widthOf = [&](const sf::String& s) {
        float w = 0.f;
        for (size_t i = 0; i < s.getSize(); ++i) {
            if (i > 0) w += m_font.getKerning(s[i - 1], s[i], size);
            w += m_font.getGlyph(s[i], size, false).advance;
        }
        return w;
    };

    sf::String line, word;
    auto flushWord = [&]() {
        if (word.isEmpty()) return;
        sf::String tryLine = line.isEmpty() ? word : line + " " + word;
        if (widthOf(tryLine) <= maxWidth) { line = tryLine; word.clear(); return; }
        if (!line.isEmpty()) { lines.push_back(line); line.clear(); }
        // A word wider than the line is broken wherever it runs out of room
        while (widthOf(word) > maxWidth && word.getSize() > 1) {
            size_t cut = word.getSize() - 1;
            while (cut > 1 && widthOf(word.substring(0, cut)) > maxWidth) --cut;
            lines.push_back(word.substring(0, cut));
            word = word.substring(cut);
        }
        line = word;
        word.clear();
    };
    for (size_t i = 0; i < text.getSize(); ++i) {
        sf::Uint32 ch = text[i];
        if (ch == '\n') { flushWord(); lines.push_back(line); line.clear(); }
        else if (ch == ' ' || ch == '\t') flushWord();
        else if (ch != '\r') word += ch;
    }
    flushWord();
    if (!line.isEmpty() || lines.empty()) lines.push_back(line);
    return lines;
}

void ChatPanel::update(float dt, bool focusMode, bool isOpen, bool busy, const std::string& status) {
    m_isOpen = isOpen && !focusMode;
    // The clock starts over with each run and keeps its time after it, until finishRun has read it
    if (busy && !m_busy) m_busyTime = 0.f;
    if (busy) m_busyTime += dt;
    m_busy = busy;
    m_status = status;
    m_caretBlink += dt;
    if (!m_isOpen) { m_inputFocused = false; m_pickerOpen = false; }

    // The limits show from the moment the chat opens: without a reading from this 5-hour window, Claude Code is asked
    long long now = static_cast<long long>(std::time(nullptr));
    if (m_isOpen && !m_busy && m_probe->load() == 0 && (m_usage.fiveHour < 0.f || now >= m_usage.fiveHourResets)) probeUsage();
    if (m_probe->load() == 2) {
        try {
            std::ifstream in(kProbeFile, std::ios::binary);
            nlohmann::json w = nlohmann::json::parse(in);
            if (w.contains("five_hour")) {
                m_usage.fiveHour = w["five_hour"].value("utilization", -1.f);
                m_usage.fiveHourResets = w["five_hour"].value("resetsAt", 0LL);
            }
            if (w.contains("seven_day")) {
                m_usage.week = w["seven_day"].value("utilization", -1.f);
                m_usage.weekResets = w["seven_day"].value("resetsAt", 0LL);
            }
            saveUsage();
        }
        catch (...) {}
        // Asked again only once this reading's window has reset
        m_probe->store(m_usage.fiveHour >= 0.f ? 0 : 3);
    }

    float topBarsH = WisdomUI::Theme::TopBarHeight + WisdomUI::Theme::OptionsBarHeight;
    float statusBarH = WisdomUI::Theme::StatusBarHeight;
    float tabDockW = WisdomUI::Theme::ToolDockWidth;
    float targetX = m_isOpen ? 1920.f - tabDockW - m_width : 1920.f;
    m_currentX += (targetX - m_currentX) * 18.f * dt;
    float x = std::floor(m_currentX);
    float bottom = 1080.f - statusBarH;
    float innerW = m_width - kPad * 2.f;

    m_panelBounds = sf::FloatRect(x, topBarsH, m_width, bottom - topBarsH);
    m_headerBounds = sf::FloatRect(x + kPad, topBarsH + kPad, innerW, 14.f + 4.f * kLineHeight);
    m_closeBounds = sf::FloatRect(m_headerBounds.left + innerW - 28.f, m_headerBounds.top + 4.f, 24.f, 24.f);

    // The prompt box grows with what is typed, up to a few lines, and the conversation gives up the room
    m_footerBounds = sf::FloatRect(x + kPad, bottom - kPad - kLineHeight, innerW, kLineHeight);
    float textW = innerW - 16.f - 2.f * m_charW;
    int inputLines = std::clamp(static_cast<int>(wrap(m_input, kTextSize, textW).size()), 1, kInputMaxLines);
    float inputH = 14.f + inputLines * kLineHeight;
    m_inputBounds = sf::FloatRect(x + kPad, m_footerBounds.top - 6.f - inputH, innerW, inputH);
    float statusH = m_busy ? 2.f * kLineHeight + 8.f : 0.f;
    float listTop = m_headerBounds.top + m_headerBounds.height + 12.f;
    m_listBounds = sf::FloatRect(x + kPad, listTop, innerW, m_inputBounds.top - statusH - 10.f - listTop);

    std::string label = kModels[m_model].label;
    m_modelBounds = sf::FloatRect(m_footerBounds.left, m_footerBounds.top, (label.size() + 3) * m_charW, kLineHeight);
    // Next to the model: the area Claude draws in, or the way to mark one
    m_areaText = m_marking ? "[drag on canvas]" : !m_markLabel.empty() ? "[" + m_markLabel + " x]" : "[area]";
    m_areaBounds = sf::FloatRect(m_modelBounds.left + m_modelBounds.width + m_charW, m_footerBounds.top, m_areaText.size() * m_charW + 8.f, kLineHeight);
    // The detail only shows when the area is too big to draw pixel for pixel; a click goes to the next one
    m_detailText = std::string("[") + kDetails[m_detail].label + "]";
    m_detailBounds = sf::FloatRect(m_areaBounds.left + m_areaBounds.width + m_charW * 0.5f, m_footerBounds.top, m_detailText.size() * m_charW + 8.f, kLineHeight);
    // While Claude works a stop button takes the right of the footer, where the estimate was
    float stopW = 8.f * m_charW + 12.f;
    m_stopBounds = sf::FloatRect(m_footerBounds.left + m_footerBounds.width - stopW, m_footerBounds.top - 1.f, stopW, kLineHeight + 2.f);
    float pickerH = (kModelCount + 1) * kLineHeight + 12.f;
    m_pickerBounds = sf::FloatRect(m_footerBounds.left, m_footerBounds.top - 4.f - pickerH, innerW, pickerH);
}

void ChatPanel::draw(sf::RenderWindow& window) {
    if (m_currentX >= 1919.f) return;
    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    auto text = [&](const sf::String& s, float x, float y, sf::Color c) {
        sf::Text t(s, m_font, kTextSize);
        t.setFillColor(c);
        t.setPosition(std::floor(x), std::floor(y + 1.f));
        window.draw(t);
        return t;
    };
    auto rect = [&](sf::FloatRect r, sf::Color fill, sf::Color edge = sf::Color::Transparent) {
        sf::RectangleShape s(sf::Vector2f(r.width, r.height));
        s.setPosition(std::floor(r.left), std::floor(r.top));
        s.setFillColor(fill);
        if (edge.a) { s.setOutlineThickness(-1.f); s.setOutlineColor(edge); }
        window.draw(s);
    };
    // Claude Code's "⏺" in front of a reply
    auto bullet = [&](float x, float y, sf::Color c) {
        sf::CircleShape dot(3.5f);
        dot.setFillColor(c);
        dot.setPosition(std::floor(x + m_charW * 0.5f - 3.5f), std::floor(y + kLineHeight / 2.f - 2.5f));
        window.draw(dot);
    };
    // And its "⎿" in front of a note under it
    auto elbow = [&](float x, float y, sf::Color c) {
        rect(sf::FloatRect(x + m_charW * 0.5f, y + 3.f, 1.f, 8.f), c);
        rect(sf::FloatRect(x + m_charW * 0.5f, y + 10.f, m_charW, 1.f), c);
    };

    rect(m_panelBounds, kBg);
    rect(sf::FloatRect(m_panelBounds.left, m_panelBounds.top, 1.f, m_panelBounds.height), kEdge);

    // The welcome box Claude Code opens with
    const sf::FloatRect& hb = m_headerBounds;
    rect(hb, sf::Color::Transparent, kClaude);
    float hx = hb.left + 12.f;
    float hy = hb.top + 7.f;
    text(m_mark, hx, hy, kClaude);
    text("Claude Code", hx + 2.f * m_charW, hy, kText);
    text(std::string(kModels[m_model].label) + "  " + (m_areaLabel.empty() ? std::string("/help for commands") : m_areaLabel), hx + 2.f * m_charW, hy + kLineHeight, kDim);
    std::string folder = m_folderLabel;
    size_t maxChars = static_cast<size_t>((hb.width - 24.f) / m_charW) - 7;
    if (folder.size() > maxChars) folder = "..." + folder.substr(folder.size() - maxChars + 3);
    text(utf8("cwd: " + folder), hx + 2.f * m_charW, hy + 2.f * kLineHeight, kDim);
    std::string limits = usageLine();
    if (!limits.empty()) text(limits, hx + 2.f * m_charW, hy + 3.f * kLineHeight, kDim);
    bool closeHover = m_closeBounds.contains(mPos);
    if (closeHover) rect(m_closeBounds, kHover);
    text("x", m_closeBounds.left + (m_closeBounds.width - m_charW) / 2.f, m_closeBounds.top + 1.f, closeHover ? kText : kDim);

    // The conversation, newest at the bottom. Only what falls inside the list area is drawn.
    sf::View oldView = window.getView();
    sf::FloatRect list = m_listBounds;
    sf::Vector2f viewSize = oldView.getSize();
    sf::Vector2f viewTopLeft = oldView.getCenter() - viewSize / 2.f;
    sf::FloatRect vp = oldView.getViewport();
    sf::View clip(list);
    clip.setViewport(sf::FloatRect(vp.left + (list.left - viewTopLeft.x) / viewSize.x * vp.width,
        vp.top + (list.top - viewTopLeft.y) / viewSize.y * vp.height,
        list.width / viewSize.x * vp.width, list.height / viewSize.y * vp.height));
    window.setView(clip);

    float indent = 2.f * m_charW;
    float textMaxW = list.width - indent - 10.f;
    float totalH = 0.f;
    struct Laid { const Message* msg; const std::vector<sf::String>* lines; float h; };
    std::vector<Laid> laid;
    for (const auto& m : m_messages) {
        if (m.wrappedWidth != textMaxW) {
            m.lines = wrap(m.text, kTextSize, textMaxW);
            m.wrappedWidth = textMaxW;
            sf::Color ink = m.role == Role::Error ? kError : m.role == Role::Note ? kDim : kText;
            m.texts.clear();
            for (const auto& ln : m.lines) {
                m.texts.emplace_back(ln, m_font, kTextSize);
                m.texts.back().setFillColor(ink);
            }
        }
        Laid l{ &m, &m.lines, 0.f };
        l.h = l.lines->size() * kLineHeight + (m.role == Role::Artist ? 6.f : 0.f);
        totalH += l.h + kMessageGap;
        laid.push_back(std::move(l));
    }
    m_maxScroll = std::max(0.f, totalH - list.height);
    m_scroll = std::clamp(m_scroll, 0.f, m_maxScroll);

    if (m_messages.empty()) {
        const char* tips[] = {
            "Tips for getting started:",
            "",
            "1. Ask for a sprite: \"draw a knight facing right\"",
            "2. Then animate it: \"now 6 frames of him walking\"",
            "3. Claude draws on a layer called Claude; your layers are never touched",
            "4. Ctrl+Z undoes a whole reply. Keep drawing while Claude works",
        };
        float y = list.top + 4.f;
        for (const char* tip : tips) {
            for (const auto& ln : wrap(sf::String(tip), kTextSize, list.width - indent)) {
                text(ln, list.left, y, tip[0] == 'T' ? kText : kDim);
                y += kLineHeight;
            }
        }
    }

    float y = list.top + list.height - totalH + m_scroll;
    for (const auto& l : laid) {
        if (y + l.h >= list.top && y <= list.top + list.height) {
            Role role = l.msg->role;
            float ty = y;
            if (role == Role::Artist) {
                rect(sf::FloatRect(list.left, y, list.width, l.h), kUserBg);
                ty += 3.f;
                text(">", list.left + 4.f, ty, kDim);
            }
            else if (role == Role::Claude) bullet(list.left + 4.f, ty, kText);
            else if (role == Role::Error) bullet(list.left + 4.f, ty, kError);
            else elbow(list.left + 4.f, ty, kDim);
            for (sf::Text& t : l.msg->texts) {
                // Lines scrolled out of the list are skipped
                if (ty + kLineHeight >= list.top && ty <= list.top + list.height) {
                    t.setPosition(std::floor(list.left + 4.f + indent), std::floor(ty + 1.f));
                    window.draw(t);
                }
                ty += kLineHeight;
            }
        }
        y += l.h + kMessageGap;
    }
    window.setView(oldView);

    // While Claude works: the spinner, what it is doing, and how long it has been at it
    if (m_busy) {
        float sy = m_inputBounds.top - 8.f - 2.f * kLineHeight;
        sf::Uint32 star = m_spinner[static_cast<size_t>(m_busyTime * 8.f) % m_spinner.getSize()];
        text(sf::String(star), m_listBounds.left + 4.f, sy, kClaude);
        std::string status = m_status.empty() ? std::string("Thinking") : m_status;
        size_t room = static_cast<size_t>((m_listBounds.width - 4.f - indent) / m_charW) - 1;
        if (status.size() + 3 > room) status = status.substr(0, room - 3);
        sf::String line = utf8(status) + (m_font.hasGlyph(0x2026) ? sf::String(static_cast<sf::Uint32>(0x2026)) : sf::String("..."));
        text(line, m_listBounds.left + 4.f + indent, sy, kClaude);
        std::string timing = "(" + clockTime(m_busyTime);
        if (m_runEstimate > 0.f) timing += (m_busyTime > m_runEstimate ? " - expected " : " of ") + roughTime(m_runEstimate);
        timing += " - esc to interrupt)";
        text(timing, m_listBounds.left + 4.f + indent, sy + kLineHeight, kDim);
    }

    // The prompt box
    rect(m_inputBounds, sf::Color::Transparent, m_inputFocused ? kBoxEdge : kEdge);
    float tx = m_inputBounds.left + 8.f;
    float ly = m_inputBounds.top + 7.f;
    text(">", tx, ly, m_inputFocused ? kText : kDim);
    tx += indent;
    if (m_input.isEmpty()) text("Try \"draw a knight facing right\"", tx + (m_inputFocused ? m_charW : 0.f), ly, kDim);
    // Wrap with the caret counted, so it lands on the right line; only the last few lines show
    std::vector<sf::String> lines = wrap(m_input, kTextSize, m_inputBounds.width - 16.f - indent);
    int first = std::max(0, static_cast<int>(lines.size()) - kInputMaxLines);
    size_t consumed = 0;
    bool caretDrawn = false;
    for (int i = 0; i < static_cast<int>(lines.size()); ++i) {
        size_t len = lines[i].getSize();
        if (i >= first) {
            sf::Text t = text(lines[i], tx, ly, kText);
            if (m_inputFocused && !caretDrawn && m_caret <= consumed + len) {
                // A terminal's block cursor, with the letter under it in reverse
                sf::Vector2f cp = t.findCharacterPos(m_caret - consumed);
                if (std::fmod(m_caretBlink, 1.f) < 0.6f) {
                    rect(sf::FloatRect(cp.x, ly + 1.f, m_charW, kLineHeight - 2.f), kText);
                    if (m_caret < m_input.getSize() && m_input[m_caret] != '\n') text(sf::String(m_input[m_caret]), cp.x, ly, kBg);
                }
                caretDrawn = true;
            }
            ly += kLineHeight;
        }
        // The wrap dropped the space (or line break) where it broke the line
        consumed += len + 1;
    }

    // Under the box: the model on the left, and on the right how long the typed request should take
    bool modelHover = m_modelBounds.contains(mPos) || m_pickerOpen;
    text(kModels[m_model].label, m_footerBounds.left + 4.f, m_footerBounds.top, modelHover ? kText : kDim);
    sf::ConvexShape arrow(3);
    float ax = m_footerBounds.left + 4.f + (std::string(kModels[m_model].label).size() + 1) * m_charW;
    float ay = m_footerBounds.top + kLineHeight / 2.f - 1.f;
    arrow.setPoint(0, sf::Vector2f(ax, ay - 2.f));
    arrow.setPoint(1, sf::Vector2f(ax + 7.f, ay - 2.f));
    arrow.setPoint(2, sf::Vector2f(ax + 3.5f, ay + 2.5f));
    arrow.setFillColor(modelHover ? kText : kDim);
    window.draw(arrow);

    bool areaHover = m_areaBounds.contains(mPos);
    if (areaHover) rect(m_areaBounds, kHover);
    text(m_areaText, m_areaBounds.left + 4.f, m_footerBounds.top, m_marking || !m_markLabel.empty() ? kClaude : areaHover ? kText : kDim);
    bool detailHover = m_detailBounds.contains(mPos);
    if (detailHover) rect(m_detailBounds, kHover);
    text(m_detailText, m_detailBounds.left + 4.f, m_footerBounds.top, detailHover && m_scaled ? kText : kDim);
    float leftEnd = m_detailBounds.left + m_detailBounds.width - m_footerBounds.left;

    std::string typed = inputUtf8();
    size_t start = typed.find_first_not_of(" \t\n");
    std::string right;
    sf::Color rightInk = kDim;
    if (m_busy) right = "";
    else if (start == std::string::npos) right = leftEnd + 22.f * m_charW < m_footerBounds.width ? "type for time & usage" : "";
    else if (typed[start] == '/') right = "command";
    else {
        Job job = estimate(typed, m_model);
        std::string what = job.kind == "chat" ? "reply only"
            : job.kind == "new" ? (job.frames > 1 ? "new sprite, " + std::to_string(job.frames) + " frames" : "new sprite")
            : (job.frames > 1 ? std::to_string(job.frames) + " frames" : "edit");
        // "?%" until two finished runs have shown how fast Claude uses the limit
        std::string cost = job.percent < 0.f ? " ?%" : job.percent < 1.f ? " <1%" : " ~" + std::to_string(static_cast<int>(std::round(job.percent))) + "%";
        right = roughTime(job.seconds) + cost + " - " + what;
        float room = m_footerBounds.width - leftEnd - 12.f;
        if (right.size() * m_charW > room) right = roughTime(job.seconds) + cost;
        rightInk = kClaude;
    }
    text(right, m_footerBounds.left + m_footerBounds.width - right.size() * m_charW - 4.f, m_footerBounds.top, rightInk);
    if (m_busy) {
        bool stopHover = m_stopBounds.contains(mPos);
        rect(m_stopBounds, stopHover ? sf::Color(70, 30, 30) : sf::Color::Transparent, stopHover ? kError : kBoxEdge);
        float sx = m_stopBounds.left + 8.f;
        rect(sf::FloatRect(sx, m_stopBounds.top + kLineHeight / 2.f - 3.f, 8.f, 8.f), stopHover ? kError : kText);
        text("Stop", sx + 2.f * m_charW, m_footerBounds.top, stopHover ? kError : kText);
    }

    // The model picker, like /model in Claude Code: each row says how long the typed request would take on it
    if (m_pickerOpen) {
        const sf::FloatRect& pb = m_pickerBounds;
        rect(pb, kBg, kBoxEdge);
        text("Select model", pb.left + 10.f, pb.top + 5.f, kClaude);
        for (int i = 0; i < kModelCount; ++i) {
            float ry = pb.top + 5.f + (i + 1) * kLineHeight;
            sf::FloatRect row(pb.left + 2.f, ry, pb.width - 4.f, kLineHeight);
            if (row.contains(mPos)) rect(row, kHover);
            bool chosen = i == m_model;
            if (chosen) text(">", pb.left + 10.f, ry, kClaude);
            text(kModels[i].label, pb.left + 10.f + indent, ry, chosen ? kClaude : kText);
            Job rowJob = estimate(typed, i);
            std::string rowCost = rowJob.percent < 0.f ? "  ?%" : rowJob.percent < 1.f ? "  <1%" : "  ~" + std::to_string(static_cast<int>(std::round(rowJob.percent))) + "%";
            std::string hint = (start != std::string::npos && typed[start] != '/') ? roughTime(rowJob.seconds) + rowCost
                : (i == 0 ? "your Claude Code setting" : "");
            text(hint, pb.left + pb.width - 10.f - hint.size() * m_charW, ry, kDim);
        }
    }
}

bool ChatPanel::handleEvent(const sf::Event& event, sf::Vector2f mousePos) {
    if (!m_isOpen) return false;

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (m_pickerOpen) {
            m_pickerOpen = false;
            if (m_pickerBounds.contains(mousePos)) {
                int row = static_cast<int>((mousePos.y - m_pickerBounds.top - 5.f) / kLineHeight) - 1;
                if (row >= 0 && row < kModelCount) setModel(row);
                return true;
            }
            if (m_modelBounds.contains(mousePos)) return true;
        }
        if (!m_panelBounds.contains(mousePos)) {
            m_inputFocused = false;
            return false;
        }
        m_inputFocused = m_inputBounds.contains(mousePos);
        if (m_closeBounds.contains(mousePos)) m_pendingAction = "close";
        else if (m_busy && m_stopBounds.contains(mousePos)) m_pendingAction = "stop";
        else if (m_modelBounds.contains(mousePos)) m_pickerOpen = true;
        else if (m_areaBounds.contains(mousePos)) m_pendingAction = "mark_area";
        else if (m_detailBounds.contains(mousePos)) {
            if (m_scaled) setDetail((m_detail + 1) % kDetailCount);
            else addMessage(Role::Note, "Detail is for canvases over 256px: low, medium, high or full sets how finely Claude draws them. "
                "This one is small enough to draw pixel for pixel.");
        }
        return true;
    }
    if (event.type == sf::Event::MouseWheelScrolled && m_panelBounds.contains(mousePos)) {
        m_scroll = std::clamp(m_scroll + event.mouseWheelScroll.delta * 40.f, 0.f, m_maxScroll);
        return true;
    }
    if (event.type == sf::Event::MouseButtonReleased || event.type == sf::Event::MouseMoved) {
        return m_panelBounds.contains(mousePos) && event.type != sf::Event::MouseMoved;
    }

    // The picker takes the arrows, Enter and Esc while it is open
    if (m_pickerOpen && event.type == sf::Event::KeyPressed) {
        switch (event.key.code) {
        case sf::Keyboard::Up: setModel((m_model + kModelCount - 1) % kModelCount); break;
        case sf::Keyboard::Down: setModel((m_model + 1) % kModelCount); break;
        case sf::Keyboard::Enter:
        case sf::Keyboard::Escape: m_pickerOpen = false; break;
        default: break;
        }
        return true;
    }
    if (m_pickerOpen && event.type == sf::Event::TextEntered) return true;
    // Esc interrupts Claude while it works, as in the terminal, wherever the focus is
    if (m_busy && event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
        m_pendingAction = "stop";
        return true;
    }

    if (!m_inputFocused) return false;
    if (event.type == sf::Event::TextEntered) {
        sf::Uint32 ch = event.text.unicode;
        if (ch == '\b') {
            if (m_caret > 0) m_input.erase(--m_caret);
        }
        else if (ch >= 32 && ch != 127 && m_input.getSize() < kInputMaxLength) {
            m_input.insert(m_caret++, sf::String(ch));
        }
        m_caretBlink = 0.f;
        return true;
    }
    if (event.type == sf::Event::KeyPressed) {
        m_caretBlink = 0.f;
        switch (event.key.code) {
        case sf::Keyboard::Enter:
            // Shift+Enter starts a new line; Enter alone sends
            if (event.key.shift) { if (m_input.getSize() < kInputMaxLength) m_input.insert(m_caret++, sf::String(static_cast<sf::Uint32>('\n'))); }
            else send();
            break;
        case sf::Keyboard::Escape: m_inputFocused = false; break;
        case sf::Keyboard::Left: if (m_caret > 0) --m_caret; break;
        case sf::Keyboard::Right: if (m_caret < m_input.getSize()) ++m_caret; break;
        case sf::Keyboard::Home: m_caret = 0; break;
        case sf::Keyboard::End: m_caret = m_input.getSize(); break;
        case sf::Keyboard::Delete: if (m_caret < m_input.getSize()) m_input.erase(m_caret); break;
        case sf::Keyboard::V:
            if (event.key.control) {
                sf::String pasted = sf::Clipboard::getString();
                for (size_t i = 0; i < pasted.getSize() && m_input.getSize() < kInputMaxLength; ++i) {
                    sf::Uint32 ch = pasted[i];
                    if (ch == '\r') continue;
                    if (ch < 32 && ch != '\n') ch = ' ';
                    m_input.insert(m_caret++, sf::String(ch));
                }
            }
            break;
        default: break;
        }
        // Every key goes to the box while it has focus, so tool shortcuts do not fire while typing
        return true;
    }
    if (event.type == sf::Event::KeyReleased) return true;
    return false;
}
