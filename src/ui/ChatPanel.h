#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

// The chat with Claude in the right dock, drawn like Claude Code in a terminal: the conversation so far,
// a prompt box for the next message, the model Claude runs on and how long the typed request should take.
// The panel only collects and shows text; UIManager sends the messages and puts the frames Claude draws
// on the timeline.
class ChatPanel {
public:
    enum class Role { Artist, Claude, Error, Note };
    struct Message {
        Role role;
        sf::String text;
    };

    ChatPanel();
    void init();
    void update(float dt, bool focusMode, bool isOpen, bool busy, const std::string& status);
    void draw(sf::RenderWindow& window);

    // What the estimate depends on besides the message: whether Claude already has a sprite going in this
    // project, and the canvas size. folderLabel is shown in the header like a terminal's working folder.
    void setContext(const std::string& folderLabel, bool hasSprite, unsigned int width, unsigned int height);

    // True when the event was meant for the panel (a click inside it, or typing while its box has focus)
    bool handleEvent(const sf::Event& event, sf::Vector2f mousePos);
    // "send", "stop", "new_chat" or "close" after a click, Enter or a slash command, then empty until the next one
    std::string takeAction();
    // The message typed when "send" came out; the box is cleared
    std::string takeMessage();
    // The Claude Code model id to run on, empty for whatever Claude Code itself is set to
    std::string modelId() const;
    // Called when the reply to the last message is back; a run that finished teaches the estimate
    void finishRun(bool success);

    bool isTyping() const { return m_inputFocused && m_isOpen; }
    bool containsPoint(sf::Vector2f p) const { return m_isOpen && m_panelBounds.contains(p); }
    void focusInput() { m_inputFocused = true; }

    void addMessage(Role role, const std::string& utf8Text);
    void clearMessages();
    // Reads the conversation the bridge keeps in <assistant folder>/chat.jsonl
    void loadHistory(const std::string& chatFile);

private:
    // What a message asks for, as far as timing goes
    struct Job {
        std::string kind;   // "chat", "new" or "edit"
        int frames = 0;
        float seconds = 0.f;
    };
    // One finished run: what was estimated and how long it really took
    struct Timing {
        std::string model;
        float estimate;
        float seconds;
    };

    sf::Font m_font;
    float m_charW = 9.f;
    sf::String m_spinner;
    sf::String m_mark;
    std::vector<Message> m_messages;
    sf::String m_input;
    size_t m_caret = 0;
    bool m_inputFocused = false;
    bool m_isOpen = false;
    bool m_busy = false;
    std::string m_status;
    float m_busyTime = 0.f;
    float m_caretBlink = 0.f;

    std::string m_folderLabel;
    bool m_hasSprite = false;
    unsigned int m_canvasW = 32;
    unsigned int m_canvasH = 32;

    int m_model = 0;
    bool m_pickerOpen = false;
    std::vector<Timing> m_timings;
    // The run in flight: its model and what it was estimated to take
    std::string m_runModel;
    float m_runEstimate = 0.f;

    float m_currentX = 1920.f;
    float m_width = 420.f;
    sf::FloatRect m_panelBounds;
    sf::FloatRect m_headerBounds;
    sf::FloatRect m_listBounds;
    sf::FloatRect m_inputBounds;
    sf::FloatRect m_footerBounds;
    sf::FloatRect m_modelBounds;
    sf::FloatRect m_pickerBounds;
    sf::FloatRect m_closeBounds;
    sf::FloatRect m_stopBounds;

    // How far the conversation is scrolled up from its newest message, in pixels
    float m_scroll = 0.f;
    float m_maxScroll = 0.f;

    std::string m_pendingAction;
    std::string m_pendingMessage;

    std::vector<sf::String> wrap(const sf::String& text, unsigned int size, float maxWidth) const;
    void send();
    void runCommand(const std::string& command);
    void setModel(int index);
    Job estimate(const std::string& message, int model) const;
    std::string inputUtf8() const;
};
