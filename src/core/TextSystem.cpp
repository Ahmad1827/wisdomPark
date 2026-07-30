#include "TextSystem.h"
#include "FontManager.h"
#include "Canvas.h"
#include <random>

std::string generateUUID() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    const char* v = "0123456789abcdef";
    std::string res;
    for (int i = 0; i < 16; i++) res += v[dis(gen)];
    return res;
}

void TextObject::render(sf::RenderTarget& target, bool isPixelMode, sf::RenderStates states) const {
    sf::Font* font = FontManager::getInstance().getFont(fontName);
    if (!font) {
        auto names = FontManager::getInstance().getFontNames();
        if (!names.empty()) font = FontManager::getInstance().getFont(names[0]);
        if (!font) return;
    }

    if (isPixelMode) {
        const_cast<sf::Texture&>(font->getTexture(size)).setSmooth(false);
    }
    else {
        const_cast<sf::Texture&>(font->getTexture(size)).setSmooth(true);
    }

    sf::Text sfText(text.isEmpty() ? " " : text, *font, size);
    sf::Uint32 style = sf::Text::Regular;
    if (bold) style |= sf::Text::Bold;
    if (italic) style |= sf::Text::Italic;
    if (underline) style |= sf::Text::Underlined;
    if (strikethrough) style |= sf::Text::StrikeThrough;
    sfText.setStyle(style);

    sfText.setLetterSpacing(letterSpacing);
    sfText.setLineSpacing(lineSpacing);

    sf::Color finalColor = color;
    finalColor.a = static_cast<sf::Uint8>((opacity / 100.f) * 255.f);
    sfText.setFillColor(finalColor);

    if (outline) {
        sfText.setOutlineThickness(outlineThickness);
        sf::Color outCol = outlineColor;
        outCol.a = finalColor.a;
        sfText.setOutlineColor(outCol);
    }

    sf::FloatRect bounds = sfText.getLocalBounds();
    sf::Vector2f origin(0.f, 0.f);
    if (!text.isEmpty()) {
        if (alignH == 1) origin.x = bounds.width / 2.f;
        else if (alignH == 2) origin.x = bounds.width;
        if (alignV == 1) origin.y = bounds.height / 2.f;
        else if (alignV == 2) origin.y = bounds.height;
    }
    sfText.setOrigin(origin);

    sfText.setPosition(isPixelMode ? sf::Vector2f(std::round(position.x), std::round(position.y)) : position);
    sfText.setRotation(rotation);
    sfText.setScale(scale);

    if (box && !text.isEmpty()) {
        sf::RectangleShape bg(sf::Vector2f(bounds.width + boxPadding * 2.f, bounds.height + boxPadding * 2.f));
        bg.setOrigin(origin + sf::Vector2f(boxPadding, boxPadding));
        bg.setPosition(sfText.getPosition());
        bg.setRotation(rotation);
        bg.setScale(scale);
        sf::Color bColor = boxColor;
        bColor.a = static_cast<sf::Uint8>((bColor.a / 255.f) * finalColor.a);
        bg.setFillColor(bColor);
        target.draw(bg, states);
    }

    if (shadow && !text.isEmpty()) {
        sf::Text shText = sfText;
        sf::Color shColor = shadowColor;
        shColor.a = static_cast<sf::Uint8>((shColor.a / 255.f) * finalColor.a);
        shText.setFillColor(shColor);
        shText.setOutlineThickness(0.f);
        shText.move(shadowOffsetX, shadowOffsetY);
        target.draw(shText, states);
    }

    if (!text.isEmpty()) {
        target.draw(sfText, states);
    }

    if (isEditing) {
        static sf::Clock blinkClock;
        if (blinkClock.getElapsedTime().asMilliseconds() % 1000 < 500) {
            float cursorHeight = std::max(2.0f, static_cast<float>(size) * 0.8f);
            sf::RectangleShape cursor(sf::Vector2f(1.f, cursorHeight));
            cursor.setFillColor(finalColor);

            sf::Vector2f curPos;
            if (text.isEmpty()) {
                curPos = sfText.getPosition();
                curPos.y += (static_cast<float>(size) - cursorHeight) / 2.0f;
            }
            else {
                curPos = sfText.findCharacterPos(text.getSize());
                curPos.y += (static_cast<float>(size) - cursorHeight) / 2.0f;
            }

            cursor.setPosition(curPos);
            cursor.setRotation(rotation);
            cursor.setScale(scale);
            target.draw(cursor, states);
        }
    }
}

sf::FloatRect TextObject::getBounds() const {
    sf::Font* font = FontManager::getInstance().getFont(fontName);
    if (!font) return sf::FloatRect();
    sf::Text sfText(text, *font, size);
    sfText.setPosition(position);
    sfText.setRotation(rotation);
    sfText.setScale(scale);
    sf::FloatRect local = sfText.getLocalBounds();
    sf::Transform t = sfText.getTransform();
    return t.transformRect(local);
}

void TextManager::init() {}

std::string TextManager::createText(int frame, int layer, sf::Vector2f pos) {
    saveUndoState(frame);
    TextObject t;
    t.id = generateUUID();
    t.position = pos;
    t.layerIndex = layer;
    auto names = FontManager::getInstance().getFontNames();
    if (!names.empty()) t.fontName = names[0];
    t.isEditing = true;
    m_frameTexts[frame].push_back(t);
    return t.id;
}

void TextManager::deleteText(int frame, const std::string& id) {
    saveUndoState(frame);
    auto& vec = m_frameTexts[frame];
    vec.erase(std::remove_if(vec.begin(), vec.end(), [&](const TextObject& o) { return o.id == id; }), vec.end());
}

TextObject* TextManager::getText(int frame, const std::string& id) {
    auto it = m_frameTexts.find(frame);
    if (it == m_frameTexts.end()) return nullptr;
    for (auto& o : it->second) {
        if (o.id == id) return &o;
    }
    return nullptr;
}

TextObject* TextManager::getEditingText() {
    for (auto& pair : m_frameTexts) {
        for (auto& o : pair.second) {
            if (o.isEditing) return &o;
        }
    }
    return nullptr;
}

void TextManager::clearEditingState() {
    for (auto& pair : m_frameTexts) {
        for (auto& o : pair.second) {
            o.isEditing = false;
        }
    }
}

void TextManager::render(sf::RenderTarget& target, int frame, int layer, bool isPixelMode, sf::RenderStates states, sf::Vector2u logicalSize) {
    // GPU Optimization: Early exit if layer has no text to prevent context switch stalls!
    bool hasText = false;
    auto it = m_frameTexts.find(frame);
    if (it != m_frameTexts.end()) {
        for (const auto& o : it->second) {
            if (o.layerIndex == layer) {
                hasText = true;
                break;
            }
        }
    }
    if (!hasText) return;

    if (m_renderTex.getSize() != logicalSize) {
        m_renderTex.create(logicalSize.x, logicalSize.y);
    }
    m_renderTex.clear(sf::Color::Transparent);

    for (const auto& o : it->second) {
        if (o.layerIndex == layer) o.render(m_renderTex, isPixelMode);
    }

    m_renderTex.display();
    sf::Sprite spr(m_renderTex.getTexture());
    target.draw(spr, states);
}

std::string TextManager::hitTest(int frame, int layer, sf::Vector2f pos) {
    auto it = m_frameTexts.find(frame);
    if (it != m_frameTexts.end()) {
        for (auto itObj = it->second.rbegin(); itObj != it->second.rend(); ++itObj) {
            if (itObj->layerIndex == layer && itObj->getBounds().contains(pos)) {
                return itObj->id;
            }
        }
    }
    return "";
}

void TextManager::rasterizeText(int frame, int layer, const std::string& id, Canvas& canvas) {
    TextObject* t = getText(frame, id);
    if (!t) return;
    canvas.saveUndoState();
    sf::RenderTexture* tex = canvas.getActiveRenderTexture(frame);
    if (tex) {
        t->isEditing = false;
        t->render(*tex, canvas.getPixelMode());
        tex->display();
        deleteText(frame, id);
    }
}

void TextManager::saveUndoState(int frame) {
    m_undoStack[frame].push_back(m_frameTexts[frame]);
    if (m_undoStack[frame].size() > 20) m_undoStack[frame].erase(m_undoStack[frame].begin());
    m_redoStack[frame].clear();
}

void TextManager::undo(int frame) {
    if (!m_undoStack[frame].empty()) {
        m_redoStack[frame].push_back(m_frameTexts[frame]);
        m_frameTexts[frame] = m_undoStack[frame].back();
        m_undoStack[frame].pop_back();
    }
}

void TextManager::redo(int frame) {
    if (!m_redoStack[frame].empty()) {
        m_undoStack[frame].push_back(m_frameTexts[frame]);
        m_frameTexts[frame] = m_redoStack[frame].back();
        m_redoStack[frame].pop_back();
    }
}