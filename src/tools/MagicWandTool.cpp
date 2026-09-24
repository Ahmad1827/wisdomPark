#include "MagicWandTool.h"
#include "../core/ExportManager.h"
#include "../UI/UITheme.h"
#include <cmath>
#include <algorithm>

MagicWandTool::MagicWandTool(Canvas& canvas, Timeline& timeline)
    : m_canvas(canvas), m_timeline(timeline), m_tolerance(10),
    m_contiguous(true), m_sampleAllLayers(false), m_isPanning(false),
    m_panelPos(64.f, 78.f), m_panelSize(290.f, 280.f), m_isDraggingPanel(false),
    m_requestColorPanelOpen(false) {
    m_lastPrimaryColor = canvas.getPrimaryColor();
}

void MagicWandTool::Initialize() {
    m_font.loadFromFile("assets/font.otf");
    m_panelPos = sf::Vector2f(64.f, 78.f);
}

void MagicWandTool::SetBounds(const sf::FloatRect& bounds) {
    m_bounds = bounds;
}

bool MagicWandTool::wantsColorPanelOpen() const { return m_requestColorPanelOpen; }
void MagicWandTool::clearColorPanelRequest() { m_requestColorPanelOpen = false; }

float MagicWandTool::getPerceptualDistance(sf::Color c1, sf::Color c2) {
    float r = std::abs(static_cast<float>(c1.r) - static_cast<float>(c2.r));
    float g = std::abs(static_cast<float>(c1.g) - static_cast<float>(c2.g));
    float b = std::abs(static_cast<float>(c1.b) - static_cast<float>(c2.b));
    float a = std::abs(static_cast<float>(c1.a) - static_cast<float>(c2.a));
    return std::max({ r, g, b, a });
}

std::vector<bool> MagicWandTool::extractSelectionMask(sf::Vector2i startPos) {
    int w = m_canvas.getCanvasSize().x;
    int h = m_canvas.getCanvasSize().y;
    std::vector<bool> mask(w * h, false);

    if (startPos.x < 0 || startPos.x >= w || startPos.y < 0 || startPos.y >= h) return mask;

    sf::Image img;
    if (m_sampleAllLayers) {
        img = ExportManager::flattenFrame(m_canvas, m_timeline.getCurrentFrame());
    }
    else {
        sf::RenderTexture scratch;
        if (m_canvas.renderLayerToTexture(m_timeline.getCurrentFrame(), m_canvas.getActiveLayer(), scratch)) {
            img = scratch.getTexture().copyToImage();
        }
        else {
            auto* tex = m_canvas.getActiveRenderTexture(m_timeline.getCurrentFrame());
            if (!tex) return mask;
            img = tex->getTexture().copyToImage();
        }
    }

    if (startPos.x >= static_cast<int>(img.getSize().x) || startPos.y >= static_cast<int>(img.getSize().y)) {
        return mask;
    }

    sf::Color targetCol = img.getPixel(startPos.x, startPos.y);

    auto unPreMult = [](const sf::Color& c) -> sf::Color {
        if (c.a == 0) return sf::Color(0, 0, 0, 0);
        int r = std::min(255, (static_cast<int>(c.r) * 255) / static_cast<int>(c.a));
        int g = std::min(255, (static_cast<int>(c.g) * 255) / static_cast<int>(c.a));
        int b = std::min(255, (static_cast<int>(c.b) * 255) / static_cast<int>(c.a));
        return sf::Color(r, g, b, c.a);
        };

    bool targetIsTransparent = (targetCol.a <= 15);
    sf::Color normTarget = unPreMult(targetCol);

    auto matchesTarget = [&](const sf::Color& c) -> bool {
        if (targetIsTransparent) {
            return c.a <= 15 + m_tolerance;
        }
        if (c.a <= 15) return false;
        sf::Color normC = unPreMult(c);
        float r = std::abs(static_cast<float>(normC.r) - static_cast<float>(normTarget.r));
        float g = std::abs(static_cast<float>(normC.g) - static_cast<float>(normTarget.g));
        float b = std::abs(static_cast<float>(normC.b) - static_cast<float>(normTarget.b));
        return std::max({ r, g, b }) <= static_cast<float>(m_tolerance);
        };

    const sf::Uint8* pixels = img.getPixelsPtr();

    if (m_contiguous) {
        std::vector<int> q;
        q.reserve(65536);
        q.push_back(startPos.y * w + startPos.x);
        mask[startPos.y * w + startPos.x] = true;

        size_t head = 0;
        while (head < q.size()) {
            int curr = q[head++];
            int cx = curr % w;
            int cy = curr / w;

            int nx[4] = { cx - 1, cx + 1, cx, cx };
            int ny[4] = { cy, cy, cy - 1, cy + 1 };

            for (int i = 0; i < 4; ++i) {
                int x = nx[i];
                int y = ny[i];
                if (x >= 0 && x < w && y >= 0 && y < h) {
                    int nIdx = y * w + x;
                    if (!mask[nIdx]) {
                        size_t pByte = static_cast<size_t>(nIdx) * 4;
                        sf::Color c(pixels[pByte], pixels[pByte + 1], pixels[pByte + 2], pixels[pByte + 3]);
                        if (matchesTarget(c)) {
                            mask[nIdx] = true;
                            q.push_back(nIdx);
                        }
                    }
                }
            }
        }
    }
    else {
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                size_t idx = (static_cast<size_t>(y) * w + x) * 4;
                sf::Color c(pixels[idx], pixels[idx + 1], pixels[idx + 2], pixels[idx + 3]);
                if (matchesTarget(c)) {
                    mask[y * w + x] = true;
                }
            }
        }
    }

    return mask;
}

std::vector<sf::Vector2f> MagicWandTool::traceBoundary(const std::vector<bool>& mask, int w, int h, sf::Vector2i startNode) {
    std::vector<sf::Vector2f> poly;
    sf::Vector2i curr = startNode;
    int dir = 0;

    sf::Vector2i start_curr = curr;
    int start_dir = dir;

    auto getMask = [&](int px, int py) {
        if (px < 0 || px >= w || py < 0 || py >= h) return false;
        return (bool)mask[py * w + px];
        };

    do {
        poly.push_back(sf::Vector2f(static_cast<float>(curr.x), static_cast<float>(curr.y)));

        int left_px = 0, left_py = 0, right_px = 0, right_py = 0;
        if (dir == 0) { right_px = curr.x; right_py = curr.y;   left_px = curr.x; left_py = curr.y - 1; }
        else if (dir == 1) { right_px = curr.x - 1; right_py = curr.y; left_px = curr.x; left_py = curr.y; }
        else if (dir == 2) { right_px = curr.x - 1; right_py = curr.y - 1; left_px = curr.x - 1; left_py = curr.y; }
        else if (dir == 3) { right_px = curr.x; right_py = curr.y - 1; left_px = curr.x - 1; left_py = curr.y - 1; }

        bool valL = getMask(left_px, left_py);
        bool valR = getMask(right_px, right_py);

        bool moved = false;
        if (valL) {
            dir = (dir + 3) % 4;
            moved = true;
        }
        else if (valR) {
            moved = true;
        }
        else {
            dir = (dir + 1) % 4;
            moved = false;
        }

        if (moved) {
            if (dir == 0) curr.x++;
            else if (dir == 1) curr.y++;
            else if (dir == 2) curr.x--;
            else if (dir == 3) curr.y--;
        }

        if (poly.size() > static_cast<size_t>(w * h * 4)) break;

    } while (curr != start_curr || dir != start_dir);

    if (!poly.empty()) {
        poly.push_back(poly.front());
    }
    return poly;
}

void MagicWandTool::HandleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    if (event.type == sf::Event::KeyPressed && (event.key.code == sf::Keyboard::Delete || event.key.code == sf::Keyboard::BackSpace)) {
        if (m_canvas.getSelectionManager().isActive()) {
            m_canvas.deleteSelection(m_timeline.getCurrentFrame());
        }
        return;
    }

    sf::Vector2i mousePosI = sf::Mouse::getPosition(window);
    sf::Vector2f mousePos = window.mapPixelToCoords(mousePosI);
    sf::FloatRect headerGrip(m_panelPos.x, m_panelPos.y, m_panelSize.x, 34.f);

    if (headerGrip.contains(mousePos) && event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
        m_isDraggingPanel = true;
        m_panelDragOffset = mousePos - m_panelPos;
        return;
    }

    if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        m_isDraggingPanel = false;
    }

    if (event.type == sf::Event::MouseMoved && m_isDraggingPanel) {
        m_panelPos = mousePos - m_panelDragOffset;
        m_panelPos.x = std::clamp(m_panelPos.x, 56.f, 1920.f - m_panelSize.x);
        m_panelPos.y = std::clamp(m_panelPos.y, 40.f, 1080.f - m_panelSize.y);
        return;
    }

    if (sf::FloatRect(m_panelPos.x, m_panelPos.y, m_panelSize.x, m_panelSize.y).contains(mousePos)) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            if (m_colorBoxRect.contains(mousePos)) {
                m_requestColorPanelOpen = true;
                return;
            }
            if (m_tolMinusRect.contains(mousePos)) {
                m_tolerance = std::max(0, m_tolerance - 5);
                return;
            }
            if (m_tolPlusRect.contains(mousePos)) {
                m_tolerance = std::min(255, m_tolerance + 5);
                return;
            }
            if (m_contigRect.contains(mousePos)) {
                m_contiguous = !m_contiguous;
                return;
            }
            if (m_sampleRect.contains(mousePos)) {
                m_sampleAllLayers = !m_sampleAllLayers;
                return;
            }
        }
        return;
    }

    if (event.type == sf::Event::MouseButtonPressed && (event.mouseButton.button == sf::Mouse::Right || event.mouseButton.button == sf::Mouse::Middle)) {
        if (event.mouseButton.button == sf::Mouse::Right && m_canvas.getSelectionManager().isActive()) {
            m_canvas.saveUndoState();
            m_canvas.commitSelection(m_timeline.getCurrentFrame());
            m_canvas.clearObjectSelection();
            return;
        }

        m_isPanning = true;
        m_lastPanPos = sf::Vector2f(static_cast<float>(mousePosI.x), static_cast<float>(mousePosI.y));
        return;
    }
    if (event.type == sf::Event::MouseButtonReleased && (event.mouseButton.button == sf::Mouse::Right || event.mouseButton.button == sf::Mouse::Middle)) {
        m_isPanning = false;
        return;
    }
    if (event.type == sf::Event::MouseMoved && m_isPanning) {
        sf::Vector2f currentPanPos(static_cast<float>(mousePosI.x), static_cast<float>(mousePosI.y));
        sf::Vector2f delta = currentPanPos - m_lastPanPos;
        m_canvas.pan(delta);
        m_lastPanPos = currentPanPos;
        return;
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button != sf::Mouse::Left) return;

    sf::Vector2f viewPos = m_canvas.getInverseTransform().transformPoint(mousePos);
    float scaleX = static_cast<float>(m_canvas.getCanvasSize().x) / m_canvas.getDrawArea().width;
    float scaleY = static_cast<float>(m_canvas.getCanvasSize().y) / m_canvas.getDrawArea().height;
    float lx = (viewPos.x - m_canvas.getDrawArea().left) * scaleX;
    float ly = (viewPos.y - m_canvas.getDrawArea().top) * scaleY;
    sf::Vector2i logicalPos(static_cast<int>(std::floor(lx)), static_cast<int>(std::floor(ly)));

    if (m_canvas.getDrawArea().contains(viewPos)) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            m_canvas.saveUndoState();

            auto mask = extractSelectionMask(logicalPos);
            int w = m_canvas.getCanvasSize().x;
            int h = m_canvas.getCanvasSize().y;

            std::vector<sf::Vector2i> exactPixels;
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    if (mask[y * w + x]) {
                        exactPixels.push_back({ x, y });
                    }
                }
            }

            if (exactPixels.empty()) {
                m_canvas.commitSelection(m_timeline.getCurrentFrame());
                m_canvas.clearObjectSelection();
                return;
            }

            int minX = w, maxX = 0, minY = h, maxY = 0;
            for (const auto& pt : exactPixels) {
                minX = std::min(minX, pt.x);
                maxX = std::max(maxX, pt.x);
                minY = std::min(minY, pt.y);
                maxY = std::max(maxY, pt.y);
            }

            std::vector<sf::FloatRect> subBoxes;
            subBoxes.push_back(sf::FloatRect(
                static_cast<float>(minX), static_cast<float>(minY),
                static_cast<float>(maxX - minX + 1), static_cast<float>(maxY - minY + 1)
            ));

            m_canvas.commitSelection(m_timeline.getCurrentFrame());
            m_canvas.getSelectionManager().setPixelSelection(exactPixels, m_canvas.getCanvasSize(), subBoxes);
        }
    }
}

void MagicWandTool::Update(float deltaTime, const sf::RenderWindow& window) {
    m_canvas.updateTransform(deltaTime, m_bounds);
    m_lastPrimaryColor = m_canvas.getPrimaryColor();
}

void MagicWandTool::Render(sf::RenderWindow& window) {
    sf::RenderStates canvasStates;
    canvasStates.transform = m_canvas.getTransform();
    m_canvas.draw(window, m_timeline.getCurrentFrame(), m_timeline.isPlaying(), canvasStates);

    drawPropertiesPanel(window);
}

void MagicWandTool::drawTooltip(sf::RenderWindow& window, const std::string& text, sf::Vector2f pos) {
    sf::Text tipText(text, m_font, 13);
    sf::FloatRect bounds = tipText.getLocalBounds();

    float padX = 10.f;
    float padY = 6.f;
    float w = bounds.width + padX * 2.f;
    float h = bounds.height + padY * 2.f + 4.f;

    float x = pos.x - w - 12.f;
    float y = pos.y + 14.f;

    if (x < 10.f) x = pos.x + 16.f;
    if (y + h > 1070.f) y = pos.y - h - 6.f;

    sf::RectangleShape bg(sf::Vector2f(w, h));
    bg.setPosition(x, y);
    bg.setFillColor(sf::Color(18, 12, 24, 250));
    bg.setOutlineThickness(1.5f);
    bg.setOutlineColor(WisdomUI::Theme::Gold);

    tipText.setPosition(x + padX, y + padY - 2.f);
    tipText.setFillColor(sf::Color::White);

    window.draw(bg);
    window.draw(tipText);
}

void MagicWandTool::drawPropertiesPanel(sf::RenderWindow& window) {
    sf::FloatRect panelBounds(m_panelPos.x, m_panelPos.y, m_panelSize.x, m_panelSize.y);
    WisdomUI::Theme::DrawSunsetPanel(window, panelBounds, 1.0f);

    sf::FloatRect headerGrip(m_panelPos.x + 8.f, m_panelPos.y + 6.f, m_panelSize.x - 16.f, 32.f);
    sf::RectangleShape gripBg(sf::Vector2f(headerGrip.width, headerGrip.height));
    gripBg.setPosition(headerGrip.left, headerGrip.top);
    gripBg.setFillColor(WisdomUI::Theme::SunsetDeepDark);
    gripBg.setOutlineThickness(1.f);
    gripBg.setOutlineColor(WisdomUI::Theme::SunsetPlum);
    window.draw(gripBg);

    WisdomUI::Theme::DrawCrispText(window, m_font, ":: MAGIC WAND ::", 15, headerGrip.left + headerGrip.width / 2.0f, headerGrip.top + headerGrip.height / 2.0f, WisdomUI::Theme::SunsetAmber, sf::Color(14, 6, 20), true, true);

    sf::Vector2f mPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
    std::string hoveredTooltip = "";

    float bx = m_panelPos.x;
    float y = m_panelPos.y + 48.f;

    WisdomUI::Theme::DrawCrispText(window, m_font, "Tolerance: " + std::to_string(m_tolerance), 14, bx + 16.f, y + 6.f, WisdomUI::Theme::TextPrimary);

    m_tolMinusRect = sf::FloatRect(bx + 165.f, y, 48.f, 28.f);
    m_tolPlusRect = sf::FloatRect(bx + 225.f, y, 48.f, 28.f);

    bool hovMinus = m_tolMinusRect.contains(mPos);
    bool hovPlus = m_tolPlusRect.contains(mPos);

    if (hovMinus) hoveredTooltip = "Decrease color match tolerance (-5)";
    if (hovPlus) hoveredTooltip = "Increase color match tolerance (+5)";

    WisdomUI::Theme::DrawSunsetButton(window, m_tolMinusRect, "-5", m_font, 13, false, hovMinus, false, 1.0f);
    WisdomUI::Theme::DrawSunsetButton(window, m_tolPlusRect, "+5", m_font, 13, false, hovPlus, true, 1.0f);
    y += 38.f;

    m_contigRect = sf::FloatRect(bx + 16.f, y, m_panelSize.x - 32.f, 32.f);
    bool hovContig = m_contigRect.contains(mPos);
    if (hovContig) hoveredTooltip = m_contiguous ? "Contiguous: Only select adjacent pixels" : "Non-contiguous: Select all matching pixels on canvas";
    WisdomUI::Theme::DrawSunsetButton(window, m_contigRect, m_contiguous ? "Contiguous: ON" : "Contiguous: OFF", m_font, 13, m_contiguous, hovContig, m_contiguous, 1.0f);
    y += 38.f;

    m_sampleRect = sf::FloatRect(bx + 16.f, y, m_panelSize.x - 32.f, 32.f);
    bool hovSample = m_sampleRect.contains(mPos);
    if (hovSample) hoveredTooltip = m_sampleAllLayers ? "Sample all visible layers combined" : "Sample only the currently active layer";
    WisdomUI::Theme::DrawSunsetButton(window, m_sampleRect, m_sampleAllLayers ? "Sample: All Layers" : "Sample: Current Layer", m_font, 13, m_sampleAllLayers, hovSample, m_sampleAllLayers, 1.0f);
    y += 42.f;

    WisdomUI::Theme::DrawCrispText(window, m_font, "Fill Color:", 14, bx + 16.f, y + 6.f, WisdomUI::Theme::TextSecondary);

    m_colorBoxRect = sf::FloatRect(bx + 120.f, y, m_panelSize.x - 136.f, 28.f);
    bool hovColor = m_colorBoxRect.contains(mPos);
    if (hovColor) hoveredTooltip = "Click to open Palette and pick fill color";

    sf::RectangleShape colorBox(sf::Vector2f(m_colorBoxRect.width, m_colorBoxRect.height));
    colorBox.setPosition(m_colorBoxRect.left, m_colorBoxRect.top);
    colorBox.setFillColor(m_canvas.getPrimaryColor());
    colorBox.setOutlineThickness(1.5f);
    colorBox.setOutlineColor(hovColor ? WisdomUI::Theme::SunsetGold : WisdomUI::Theme::SunsetAmber);
    window.draw(colorBox);

    if (!hoveredTooltip.empty() && !m_isDraggingPanel) {
        drawTooltip(window, hoveredTooltip, mPos);
    }
}