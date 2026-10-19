#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>

// The chat with Claude in the right dock: the conversation so far, a box to type the next message,
// and what Claude is doing while it works. The panel only collects and shows text; UIManager sends
// the messages and puts the frames Claude draws on the timeline.
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

    // True when the event was meant for the panel (a click inside it, or typing while its box has focus)
    bool handleEvent(const sf::Event& event, sf::Vector2f mousePos);
    // "send", "stop", "new_chat" or "close" after a click or Enter, then empty until the next one
    std::string takeAction();
    // The message typed when "send" came out; the box is cleared
    std::string takeMessage();

    bool isTyping() const { return m_inputFocused && m_isOpen; }
    bool containsPoint(sf::Vector2f p) const { return m_isOpen && m_panelBounds.contains(p); }
    void focusInput() { m_inputFocused = true; }

    void addMessage(Role role, const std::string& utf8Text);
    void clearMessages();
    // Reads the conversation the bridge keeps in <assistant folder>/chat.jsonl
    void loadHistory(const std::string& chatFile);

private:
    sf::Font m_font;
    std::vector<Message> m_messages;
    sf::String m_input;
    size_t m_caret = 0;
    bool m_inputFocused = false;
    bool m_isOpen = false;
    bool m_busy = false;
    std::string m_status;
    float m_busyTime = 0.f;
    float m_caretBlink = 0.f;

    float m_currentX = 1920.f;
    float m_width = 380.f;
    sf::FloatRect m_panelBounds;
    sf::FloatRect m_headerBounds;
    sf::FloatRect m_listBounds;
    sf::FloatRect m_inputBounds;
    sf::FloatRect m_sendBounds;
    sf::FloatRect m_newChatBounds;
    sf::FloatRect m_closeBounds;

    // How far the conversation is scrolled up from its newest message, in pixels
    float m_scroll = 0.f;
    float m_maxScroll = 0.f;

    std::string m_pendingAction;
    std::string m_pendingMessage;

    std::vector<sf::String> wrap(const sf::String& text, unsigned int size, float maxWidth) const;
    void send();
};
