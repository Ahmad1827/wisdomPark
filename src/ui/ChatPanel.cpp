#include "ChatPanel.h"
#include "UITheme.h"
#include "json.hpp"
#include <algorithm>
#include <cmath>
#include <fstream>

namespace {
    const unsigned int kTextSize = 18;
    const float kLineHeight = 21.f;
    const float kBubblePad = 9.f;
    const float kBubbleGap = 10.f;
    const unsigned int kInputMaxLength = 1200;
    const int kInputMaxLines = 5;
}

ChatPanel::ChatPanel() = default;

void ChatPanel::init() {
    m_font.loadFromFile("assets/Jersey10-Regular.ttf");
}

void ChatPanel::addMessage(Role role, const std::string& utf8Text) {
    m_messages.push_back({ role, sf::String::fromUtf8(utf8Text.begin(), utf8Text.end()) });
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

void ChatPanel::send() {
    if (m_busy) return;
    std::basic_string<sf::Uint8> utf8 = m_input.toUtf8();
    std::string text(utf8.begin(), utf8.end());
    // Nothing but spaces is not a message
    if (text.find_first_not_of(" \t") == std::string::npos) return;
    m_pendingMessage = text;
    m_pendingAction = "send";
    m_input.clear();
    m_caret = 0;
}

std::vector<sf::String> ChatPanel::wrap(const sf::String& text, unsigned int size, float maxWidth) const {
    std::vector<sf::String> lines;
    sf::Text probe("", m_font, size);
    auto widthOf = [&](const sf::String& s) { probe.setString(s); return probe.getLocalBounds().width; };

    sf::String line, word;
    auto flushWord = [&]() {
        if (word.isEmpty()) return;
        sf::String tryLine = line.isEmpty() ? word : line + " " + word;
        if (widthOf(tryLine) <= maxWidth) { line = tryLine; word.clear(); return; }
        if (!line.isEmpty()) { lines.push_back(line); line.clear(); }
        // A word wider than the bubble is broken wherever it runs out of room
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
    m_busy = busy;
    m_status = status;
    m_busyTime = busy ? m_busyTime + dt : 0.f;
    m_caretBlink += dt;
    if (!m_isOpen) m_inputFocused = false;

    float topBarsH = WisdomUI::Theme::TopBarHeight + WisdomUI::Theme::OptionsBarHeight;
    float statusBarH = WisdomUI::Theme::StatusBarHeight;
    float tabDockW = WisdomUI::Theme::ToolDockWidth;
    float targetX = m_isOpen ? 1920.f - tabDockW - m_width : 1920.f;
    m_currentX += (targetX - m_currentX) * 18.f * dt;
    float x = std::floor(m_currentX);
    float bottom = 1080.f - statusBarH;

    m_panelBounds = sf::FloatRect(x, topBarsH, m_width, bottom - topBarsH);
    m_headerBounds = sf::FloatRect(x, topBarsH, m_width, 40.f);
    m_closeBounds = sf::FloatRect(x + m_width - 36.f, topBarsH + 6.f, 28.f, 28.f);
    m_newChatBounds = sf::FloatRect(x + m_width - 132.f, topBarsH + 6.f, 90.f, 28.f);

    // The box grows with what is typed, up to a few lines, and the conversation gives up the room
    float innerW = m_width - 24.f - 76.f;
    int inputLines = std::clamp(static_cast<int>(wrap(m_input, kTextSize, innerW - 16.f).size()), 1, kInputMaxLines);
    float inputH = 18.f + inputLines * kLineHeight;
    float statusH = m_busy ? 30.f : 0.f;
    m_inputBounds = sf::FloatRect(x + 12.f, bottom - 12.f - inputH, innerW, inputH);
    m_sendBounds = sf::FloatRect(x + m_width - 12.f - 68.f, bottom - 12.f - inputH, 68.f, inputH);
    float listTop = topBarsH + 48.f;
    m_listBounds = sf::FloatRect(x + 8.f, listTop, m_width - 16.f, m_inputBounds.top - statusH - 10.f - listTop);
}

void ChatPanel::draw(sf::RenderWindow& window) {
    if (m_currentX >= 1919.f) return;
    using namespace WisdomUI;
    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));

    sf::RectangleShape bg(sf::Vector2f(m_panelBounds.width, m_panelBounds.height));
    bg.setPosition(m_panelBounds.left, m_panelBounds.top);
    bg.setFillColor(Theme::SunsetSkyTop);
    bg.setOutlineThickness(-1.f);
    bg.setOutlineColor(Theme::Border);
    window.draw(bg);

    sf::RectangleShape header(sf::Vector2f(m_headerBounds.width, m_headerBounds.height));
    header.setPosition(m_headerBounds.left, m_headerBounds.top);
    header.setFillColor(Theme::SunsetDeepDark);
    window.draw(header);
    Theme::DrawCrispText(window, m_font, "CLAUDE CHAT", 20, m_headerBounds.left + 16.f, m_headerBounds.top + 20.f, Theme::SunsetAmber, sf::Color(14, 6, 20), false, true);
    Theme::DrawSunsetButton(window, m_newChatBounds, "New Chat", m_font, 15, false, m_newChatBounds.contains(mPos) && !m_busy, false, 1.0f);
    Theme::DrawSunsetButton(window, m_closeBounds, "X", m_font, 15, false, m_closeBounds.contains(mPos), true, 1.0f);

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

    float bubbleMaxW = list.width * 0.82f;
    float totalH = 0.f;
    struct Laid { const Message* msg; std::vector<sf::String> lines; float w, h; };
    std::vector<Laid> laid;
    for (const auto& m : m_messages) {
        Laid l{ &m, wrap(m.text, kTextSize, bubbleMaxW - kBubblePad * 2.f), 0.f, 0.f };
        sf::Text probe("", m_font, kTextSize);
        for (const auto& ln : l.lines) { probe.setString(ln); l.w = std::max(l.w, probe.getLocalBounds().width); }
        l.w += kBubblePad * 2.f;
        l.h = l.lines.size() * kLineHeight + kBubblePad * 2.f;
        totalH += l.h + kBubbleGap;
        laid.push_back(std::move(l));
    }
    m_maxScroll = std::max(0.f, totalH - list.height);
    m_scroll = std::clamp(m_scroll, 0.f, m_maxScroll);

    if (m_messages.empty()) {
        const char* hint = "Talk to Claude about your sprite.\n\nTry: \"draw a knight facing right\", then \"now 6 frames of him walking\".\n\nWhat Claude draws lands on a layer called Claude, and your own layers are never touched. Ctrl+Z undoes a whole reply. You can keep drawing while Claude works.";
        auto lines = wrap(sf::String(hint), 17, list.width - 24.f);
        float y = list.top + 16.f;
        for (const auto& ln : lines) {
            sf::Text t(ln, m_font, 17);
            t.setFillColor(Theme::TextMuted);
            t.setPosition(std::floor(list.left + 12.f), std::floor(y));
            window.draw(t);
            y += 20.f;
        }
    }

    float y = list.top + list.height - totalH + m_scroll;
    for (const auto& l : laid) {
        bool mine = l.msg->role == Role::Artist;
        float bx = mine ? list.left + list.width - l.w - 4.f : list.left + 4.f;
        if (y + l.h >= list.top && y <= list.top + list.height) {
            sf::RectangleShape bubble(sf::Vector2f(l.w, l.h));
            bubble.setPosition(std::floor(bx), std::floor(y));
            sf::Color fill = mine ? Theme::SunsetPlum : Theme::SunsetDeepDark;
            sf::Color edge = mine ? Theme::SunsetViolet : Theme::Border;
            sf::Color ink = Theme::TextPrimary;
            if (l.msg->role == Role::Error) { fill = Theme::DangerIdle; edge = Theme::SunsetCoral; }
            if (l.msg->role == Role::Note) { fill = sf::Color(0, 0, 0, 0); edge = sf::Color(0, 0, 0, 0); ink = Theme::TextMuted; }
            bubble.setFillColor(fill);
            bubble.setOutlineThickness(-1.f);
            bubble.setOutlineColor(edge);
            window.draw(bubble);
            float ty = y + kBubblePad - 3.f;
            for (const auto& ln : l.lines) {
                sf::Text t(ln, m_font, kTextSize);
                t.setFillColor(ink);
                t.setPosition(std::floor(bx + kBubblePad), std::floor(ty));
                window.draw(t);
                ty += kLineHeight;
            }
        }
        y += l.h + kBubbleGap;
    }
    window.setView(oldView);

    if (m_busy) {
        int dots = static_cast<int>(m_busyTime * 3.f) % 4;
        std::string line = (m_status.empty() ? std::string("Claude is working") : m_status) + std::string(dots, '.');
        Theme::DrawCrispText(window, m_font, line, 17, m_panelBounds.left + 16.f, m_inputBounds.top - 20.f, Theme::SunsetGold, sf::Color(0, 0, 0, 160), false, true);
    }

    // The message box
    sf::RectangleShape box(sf::Vector2f(m_inputBounds.width, m_inputBounds.height));
    box.setPosition(m_inputBounds.left, m_inputBounds.top);
    box.setFillColor(Theme::SunsetDeepDark);
    box.setOutlineThickness(-1.5f);
    box.setOutlineColor(m_inputFocused ? Theme::SunsetAmber : Theme::Border);
    window.draw(box);

    float textW = m_inputBounds.width - 16.f;
    if (m_input.isEmpty()) {
        Theme::DrawCrispText(window, m_font, m_busy ? "Claude is working..." : "Message Claude (Enter sends)", kTextSize, m_inputBounds.left + 8.f, m_inputBounds.top + m_inputBounds.height / 2.f, Theme::TextMuted, sf::Color::Transparent, false, true);
    }
    // Wrap with the caret marked, so it lands on the right line; only the last few lines show
    std::vector<sf::String> lines = wrap(m_input, kTextSize, textW);
    int first = std::max(0, static_cast<int>(lines.size()) - kInputMaxLines);
    float ly = m_inputBounds.top + 6.f;
    size_t consumed = 0;
    bool caretDrawn = false;
    for (int i = 0; i < static_cast<int>(lines.size()); ++i) {
        size_t len = lines[i].getSize();
        if (i >= first) {
            sf::Text t(lines[i], m_font, kTextSize);
            t.setFillColor(Theme::TextPrimary);
            t.setPosition(std::floor(m_inputBounds.left + 8.f), std::floor(ly));
            window.draw(t);
            if (m_inputFocused && !caretDrawn && m_caret <= consumed + len && std::fmod(m_caretBlink, 1.f) < 0.6f) {
                sf::Vector2f cp = t.findCharacterPos(m_caret - consumed);
                sf::RectangleShape caret(sf::Vector2f(2.f, kLineHeight - 2.f));
                caret.setPosition(std::floor(cp.x), std::floor(ly + 2.f));
                caret.setFillColor(Theme::SunsetGold);
                window.draw(caret);
                caretDrawn = true;
            }
            ly += kLineHeight;
        }
        // The wrap dropped the space (or line break) where it broke the line
        consumed += len + 1;
    }

    // While Claude works the button stops it instead
    bool canSend = !m_busy && !m_input.isEmpty();
    if (m_busy) Theme::DrawSunsetButton(window, m_sendBounds, "Stop", m_font, 16, false, m_sendBounds.contains(mPos), true, 1.0f);
    else Theme::DrawSunsetButton(window, m_sendBounds, "Send", m_font, 16, false, canSend && m_sendBounds.contains(mPos), canSend, 1.0f);
}

bool ChatPanel::handleEvent(const sf::Event& event, sf::Vector2f mousePos) {
    if (!m_isOpen) return false;

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        if (!m_panelBounds.contains(mousePos)) {
            m_inputFocused = false;
            return false;
        }
        m_inputFocused = m_inputBounds.contains(mousePos);
        if (m_closeBounds.contains(mousePos)) m_pendingAction = "close";
        else if (m_newChatBounds.contains(mousePos) && !m_busy) m_pendingAction = "new_chat";
        else if (m_sendBounds.contains(mousePos)) {
            if (m_busy) m_pendingAction = "stop";
            else send();
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
