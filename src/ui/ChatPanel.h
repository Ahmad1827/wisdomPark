#pragma once
#include <SFML/Graphics.hpp>
#include <atomic>
#include <map>
#include <memory>
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
        // The text wrapped to the list and laid out, kept until the list's width changes: building an sf::Text
        // for every line every frame was most of what the open panel cost
        mutable std::vector<sf::String> lines;
        mutable std::vector<sf::Text> texts;
        mutable float wrappedWidth = -1.f;
    };

    ChatPanel();
    void init();
    void update(float dt, bool focusMode, bool isOpen, bool busy, const std::string& status);
    void draw(sf::RenderWindow& window);

    // What the estimate depends on besides the message: whether Claude already has a sprite going in this
    // project, and the size Claude will draw at. folderLabel is shown in the header like a terminal's working
    // folder; areaLabel says what Claude draws when it is not simply the whole canvas.
    // scaled says the area is big enough that the detail setting decides how finely Claude draws it.
    void setContext(const std::string& folderLabel, bool hasSprite, unsigned int width, unsigned int height, const std::string& areaLabel, bool scaled);

    // True when the event was meant for the panel (a click inside it, or typing while its box has focus)
    bool handleEvent(const sf::Event& event, sf::Vector2f mousePos);
    // "send", "stop", "new_chat", "mark_area" or "close" after a click, Enter or a slash command, then empty until the next one
    std::string takeAction();
    // The message typed when "send" came out; the box is cleared
    std::string takeMessage();
    // The Claude Code model id to run on, empty for whatever Claude Code itself is set to
    std::string modelId() const;
    // How finely Claude draws an area too big to draw pixel for pixel quickly: the longest side it draws at,
    // 0 for every pixel of the canvas
    int detailSide() const;
    // Called when the reply to the last message is back; a run that finished teaches the estimate
    void finishRun(bool success);
    // The plan usage Claude Code reported at the end of the last run (fractions of the 5-hour and weekly limits,
    // and when each resets), and what the run would have cost at API prices. Returns a line for the chat
    // saying how much of the limit the run used, or "" when Claude Code reported nothing.
    std::string reportUsage(float fiveHour, long long fiveHourResets, float week, long long weekResets, double cost);
    // The area button under the prompt box: whether the artist is dragging out an area on the canvas right
    // now, and the area marked so far ("" for none)
    void setMarking(bool marking, const std::string& markLabel);

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
        // Share of the 5-hour limit it should use, in percent; below 0 until runs have shown how fast it goes
        float percent = -1.f;
    };
    // The plan usage as last reported, and what runs have shown about how fast it goes
    struct Usage {
        float fiveHour = -1.f;
        long long fiveHourResets = 0;
        float week = -1.f;
        long long weekResets = 0;
        // Percent of the 5-hour limit used against API-price cost, over every run where both are known
        double percentSum = 0.0;
        double costSum = 0.0;
        // Per model: API-price cost and seconds over its runs, so a time estimate turns into a cost
        std::map<std::string, std::pair<double, double>> perModel;
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
    std::string m_areaLabel;
    bool m_marking = false;
    std::string m_markLabel;
    std::string m_areaText;
    bool m_scaled = false;
    int m_detail = 1;
    std::string m_detailText;
    bool m_hasSprite = false;
    unsigned int m_canvasW = 32;
    unsigned int m_canvasH = 32;

    int m_model = 0;
    bool m_pickerOpen = false;
    std::vector<Timing> m_timings;
    Usage m_usage;
    // Asking Claude Code where the limits stand when the chat opens without a recent reading:
    // 0 not asked, 1 asking, 2 answered, 3 failed (not asked again this session)
    std::shared_ptr<std::atomic<int>> m_probe = std::make_shared<std::atomic<int>>(0);
    void probeUsage();
    void saveUsage() const;
    std::string usageLine() const;
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
    sf::FloatRect m_areaBounds;
    sf::FloatRect m_detailBounds;
    sf::FloatRect m_pickerBounds;
    sf::FloatRect m_closeBounds;
    sf::FloatRect m_stopBounds;
    // The right of the footer: how long the typed request should take, on the footer's own row or, when it
    // would run into the buttons there, on a row of its own under them (the stop button goes there too)
    std::string m_footerRight;
    sf::Color m_footerRightInk;
    float m_footerRightRow = 0.f;

    // How far the conversation is scrolled up from its newest message, in pixels
    float m_scroll = 0.f;
    float m_maxScroll = 0.f;

    std::string m_pendingAction;
    std::string m_pendingMessage;

    std::vector<sf::String> wrap(const sf::String& text, unsigned int size, float maxWidth) const;
    void send();
    void runCommand(const std::string& command);
    void setModel(int index);
    void setDetail(int index);
    void saveChoices() const;
    Job estimate(const std::string& message, int model) const;
    std::string inputUtf8() const;
    std::string footerRightText(float room, sf::Color& ink) const;
};
