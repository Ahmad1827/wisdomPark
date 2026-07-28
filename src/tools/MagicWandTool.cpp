#include "MagicWandTool.h"
#include "../core/ExportManager.h"
#include <cmath>

MagicWandTool::MagicWandTool(Canvas& canvas, Timeline& timeline)
    : m_canvas(canvas), m_timeline(timeline), m_tolerance(32), m_blendMode(SelectionBlendMode::Replace),
    m_contiguous(true), m_sampleAllLayers(false), m_antiAlias(false), m_feather(0),
    m_lastHoverPos(-1, -1), m_isPanning(false) {
}

void MagicWandTool::Initialize() {
    m_font.loadFromFile("assets/font.otf");
    m_panelBg.setSize(sf::Vector2f(220.f, 380.f));
    m_panelBg.setFillColor(sf::Color(25, 25, 30, 240));
    m_panelBg.setOutlineThickness(1.f);
    m_panelBg.setOutlineColor(sf::Color(100, 100, 110));
}

void MagicWandTool::SetBounds(const sf::FloatRect& bounds) {
    m_bounds = bounds;
    m_panelBg.setPosition(bounds.left + 110.f, bounds.top + 60.f);
}

float MagicWandTool::getPerceptualDistance(sf::Color c1, sf::Color c2) {
    long rmean = ((long)c1.r + (long)c2.r) / 2;
    long r = (long)c1.r - (long)c2.r;
    long g = (long)c1.g - (long)c2.g;
    long b = (long)c1.b - (long)c2.b;
    float distSq = (((512 + rmean) * r * r) >> 8) + 4 * g * g + (((767 - rmean) * b * b) >> 8);
    return std::sqrt(distSq) / 765.0f * 255.0f;
}

void MagicWandTool::growMask(std::vector<bool>& mask, int w, int h, int amount) {
    if (amount <= 0) return;
    for (int i = 0; i < amount; ++i) {
        std::vector<bool> temp = mask;
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                if (mask[y * w + x]) {
                    if (x > 0) temp[y * w + (x - 1)] = true;
                    if (x < w - 1) temp[y * w + (x + 1)] = true;
                    if (y > 0) temp[(y - 1) * w + x] = true;
                    if (y < h - 1) temp[(y + 1) * w + x] = true;
                }
            }
        }
        mask = temp;
    }
}

void MagicWandTool::shrinkMask(std::vector<bool>& mask, int w, int h, int amount) {
    if (amount <= 0) return;
    for (int i = 0; i < amount; ++i) {
        std::vector<bool> temp = mask;
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                if (mask[y * w + x]) {
                    if (x == 0 || x == w - 1 || y == 0 || y == h - 1 ||
                        !mask[y * w + (x - 1)] || !mask[y * w + (x + 1)] ||
                        !mask[(y - 1) * w + x] || !mask[(y + 1) * w + x]) {
                        temp[y * w + x] = false;
                    }
                }
            }
        }
        mask = temp;
    }
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
        auto* tex = m_canvas.getActiveRenderTexture(m_timeline.getCurrentFrame());
        if (!tex) return mask;
        img = tex->getTexture().copyToImage();
    }

    sf::Color targetCol = img.getPixel(startPos.x, startPos.y);
    const sf::Uint8* pixels = img.getPixelsPtr();

    if (m_contiguous) {
        std::queue<sf::Vector2i> q;
        q.push(startPos);
        mask[startPos.y * w + startPos.x] = true;

        while (!q.empty()) {
            sf::Vector2i p = q.front();
            q.pop();

            sf::Vector2i neighbors[4] = {
                {p.x - 1, p.y}, {p.x + 1, p.y}, {p.x, p.y - 1}, {p.x, p.y + 1}
            };

            for (auto& n : neighbors) {
                if (n.x >= 0 && n.x < w && n.y >= 0 && n.y < h) {
                    if (!mask[n.y * w + n.x]) {
                        size_t idx = (n.y * w + n.x) * 4;
                        sf::Color c(pixels[idx], pixels[idx + 1], pixels[idx + 2], pixels[idx + 3]);
                        if (getPerceptualDistance(c, targetCol) <= m_tolerance) {
                            mask[n.y * w + n.x] = true;
                            q.push(n);
                        }
                    }
                }
            }
        }
    }
    else {
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                size_t idx = (y * w + x) * 4;
                sf::Color c(pixels[idx], pixels[idx + 1], pixels[idx + 2], pixels[idx + 3]);
                if (getPerceptualDistance(c, targetCol) <= m_tolerance) {
                    mask[y * w + x] = true;
                }
            }
        }
    }

    return mask;
}

std::vector<std::vector<sf::Vector2f>> MagicWandTool::generateContours(const std::vector<bool>& mask, int w, int h) {
    struct Edge { sf::Vector2i p1, p2; };
    std::vector<Edge> edges;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (mask[y * w + x]) {
                if (y == 0 || !mask[(y - 1) * w + x]) edges.push_back({ {x, y}, {x + 1, y} });
                if (y == h - 1 || !mask[(y + 1) * w + x]) edges.push_back({ {x + 1, y + 1}, {x, y + 1} });
                if (x == 0 || !mask[y * w + (x - 1)]) edges.push_back({ {x, y + 1}, {x, y} });
                if (x == w - 1 || !mask[y * w + (x + 1)]) edges.push_back({ {x + 1, y}, {x + 1, y + 1} });
            }
        }
    }

    std::map<std::pair<int, int>, std::vector<sf::Vector2i>> edgeMap;
    for (auto& e : edges) {
        edgeMap[{e.p1.x, e.p1.y}].push_back(e.p2);
    }

    std::vector<std::vector<sf::Vector2f>> contours;
    while (!edgeMap.empty()) {
        auto it = edgeMap.begin();
        sf::Vector2i start = { it->first.first, it->first.second };
        sf::Vector2i curr = start;

        std::vector<sf::Vector2f> contour;
        while (true) {
            contour.push_back(sf::Vector2f(static_cast<float>(curr.x), static_cast<float>(curr.y)));

            auto nextIt = edgeMap.find({ curr.x, curr.y });
            if (nextIt == edgeMap.end() || nextIt->second.empty()) break;

            sf::Vector2i next = nextIt->second.back();
            nextIt->second.pop_back();
            if (nextIt->second.empty()) edgeMap.erase(nextIt);

            curr = next;
            if (curr == start) break;
        }

        if (contour.size() > 2) {
            contours.push_back(contour);
        }
    }

    return contours;
}

void MagicWandTool::smoothContours(std::vector<std::vector<sf::Vector2f>>& contours) {
    for (auto& contour : contours) {
        std::vector<sf::Vector2f> smoothed;
        for (size_t i = 0; i < contour.size(); ++i) {
            sf::Vector2f p0 = contour[(i == 0) ? contour.size() - 1 : i - 1];
            sf::Vector2f p1 = contour[i];
            sf::Vector2f p2 = contour[(i + 1) % contour.size()];
            smoothed.push_back(p0 * 0.25f + p1 * 0.5f + p2 * 0.25f);
        }
        contour = smoothed;
    }
}

std::vector<sf::Vector2f> MagicWandTool::bridgeContours(const std::vector<std::vector<sf::Vector2f>>& contours) {
    std::vector<sf::Vector2f> unified;
    if (contours.empty()) return unified;

    for (const auto& poly : contours) {
        if (unified.empty()) {
            unified = poly;
        }
        else {
            unified.push_back(unified.back());
            unified.push_back(poly.front());
            unified.insert(unified.end(), poly.begin(), poly.end());
            unified.push_back(poly.front());
            unified.push_back(unified[unified.size() - poly.size() - 3]);
        }
    }
    return unified;
}

void MagicWandTool::applyWandSelection(const std::vector<bool>& newMask) {
    int w = m_canvas.getCanvasSize().x;
    int h = m_canvas.getCanvasSize().y;

    std::vector<bool> globalMask(w * h, false);
    if (m_blendMode != SelectionBlendMode::Replace) {
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                globalMask[y * w + x] = m_canvas.getSelectionManager().isPointInsideSelection(sf::Vector2f(x, y));
            }
        }
    }

    for (int i = 0; i < w * h; ++i) {
        if (m_blendMode == SelectionBlendMode::Replace) globalMask[i] = newMask[i];
        else if (m_blendMode == SelectionBlendMode::Add) globalMask[i] = globalMask[i] || newMask[i];
        else if (m_blendMode == SelectionBlendMode::Subtract) globalMask[i] = globalMask[i] && !newMask[i];
        else if (m_blendMode == SelectionBlendMode::Intersect) globalMask[i] = globalMask[i] && newMask[i];
    }

    if (m_feather > 0) {
        growMask(globalMask, w, h, m_feather);
    }

    auto contours = generateContours(globalMask, w, h);

    if (m_antiAlias && !m_canvas.getPixelMode()) {
        smoothContours(contours);
        smoothContours(contours);
    }

    m_canvas.commitSelection(m_timeline.getCurrentFrame());
    m_canvas.getSelectionManager().clearSelection();

    auto unified = bridgeContours(contours);
    if (!unified.empty()) {
        m_canvas.getSelectionManager().startLasso(unified[0], sf::Vector2u(w, h));
        for (size_t i = 1; i < unified.size(); ++i) {
            m_canvas.getSelectionManager().addLassoPoint(unified[i], sf::Vector2u(w, h));
        }
        m_canvas.getSelectionManager().endLasso();
    }
}

void MagicWandTool::updatePreview(sf::Vector2i pos) {
    if (pos == m_lastHoverPos) return;
    m_lastHoverPos = pos;
    m_previewContours.clear();

    auto mask = extractSelectionMask(pos);
    if (m_feather > 0) growMask(mask, m_canvas.getCanvasSize().x, m_canvas.getCanvasSize().y, m_feather);
    m_previewContours = generateContours(mask, m_canvas.getCanvasSize().x, m_canvas.getCanvasSize().y);
    if (m_antiAlias && !m_canvas.getPixelMode()) smoothContours(m_previewContours);
}

void MagicWandTool::HandleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    sf::Vector2i mousePosI = sf::Mouse::getPosition(window);
    sf::Vector2f mousePos = window.mapPixelToCoords(mousePosI);

    if (m_panelBg.getGlobalBounds().contains(mousePos)) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            float y = mousePos.y - m_panelBg.getPosition().y;

            if (y > 40 && y < 70) m_blendMode = SelectionBlendMode::Replace;
            else if (y > 70 && y < 100) m_blendMode = SelectionBlendMode::Add;
            else if (y > 100 && y < 130) m_blendMode = SelectionBlendMode::Subtract;
            else if (y > 130 && y < 160) m_blendMode = SelectionBlendMode::Intersect;

            else if (y > 170 && y < 190) m_tolerance = std::max(0, m_tolerance - 5);
            else if (y > 190 && y < 210) m_tolerance = std::min(255, m_tolerance + 5);

            else if (y > 220 && y < 250) m_contiguous = !m_contiguous;
            else if (y > 250 && y < 280) m_sampleAllLayers = !m_sampleAllLayers;
            else if (y > 280 && y < 310) m_antiAlias = !m_antiAlias;
            else if (y > 310 && y < 340) {
                if (mousePos.x < m_panelBg.getPosition().x + 110) m_feather = std::max(0, m_feather - 1);
                else m_feather = std::min(50, m_feather + 1);
            }
            m_lastHoverPos = sf::Vector2i(-1, -1);
        }
        return;
    }

    if (event.type == sf::Event::MouseButtonPressed && (event.mouseButton.button == sf::Mouse::Right || event.mouseButton.button == sf::Mouse::Middle)) {
        m_isPanning = true;
        m_lastPanPos = mousePos;
        return;
    }
    if (event.type == sf::Event::MouseButtonReleased && (event.mouseButton.button == sf::Mouse::Right || event.mouseButton.button == sf::Mouse::Middle)) {
        m_isPanning = false;
        return;
    }
    if (event.type == sf::Event::MouseMoved && m_isPanning) {
        sf::Vector2f delta = mousePos - m_lastPanPos;
        m_canvas.pan(delta);
        m_lastPanPos = mousePos;
        return;
    }

    sf::Vector2f viewPos = m_canvas.getInverseTransform().transformPoint(mousePos);
    float scaleX = static_cast<float>(m_canvas.getCanvasSize().x) / m_canvas.getDrawArea().width;
    float scaleY = static_cast<float>(m_canvas.getCanvasSize().y) / m_canvas.getDrawArea().height;
    sf::Vector2i logicalPos(static_cast<int>((viewPos.x - m_canvas.getDrawArea().left) * scaleX),
        static_cast<int>((viewPos.y - m_canvas.getDrawArea().top) * scaleY));

    if (m_canvas.getDrawArea().contains(viewPos)) {
        if (event.type == sf::Event::MouseMoved) {
            updatePreview(logicalPos);
        }
        else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            auto mask = extractSelectionMask(logicalPos);
            applyWandSelection(mask);
            m_previewContours.clear();
        }
    }
    else {
        m_previewContours.clear();
        m_lastHoverPos = sf::Vector2i(-1, -1);
    }
}

void MagicWandTool::Update(float deltaTime, const sf::RenderWindow& window) {
    m_canvas.updateTransform(deltaTime, m_bounds);
}

void MagicWandTool::Render(sf::RenderWindow& window) {
    m_canvas.draw(window, m_timeline.getCurrentFrame(), m_timeline.isPlaying(), sf::RenderStates::Default);

    if (!m_previewContours.empty()) {
        sf::Transform innerTransform;
        innerTransform.translate(std::round(m_canvas.getDrawArea().left), std::round(m_canvas.getDrawArea().top));
        float scaleX = m_canvas.getDrawArea().width / static_cast<float>(m_canvas.getCanvasSize().x);
        float scaleY = m_canvas.getDrawArea().height / static_cast<float>(m_canvas.getCanvasSize().y);
        innerTransform.scale(scaleX, scaleY);

        sf::RenderStates states;
        states.transform = m_canvas.getTransform() * innerTransform;

        for (const auto& contour : m_previewContours) {
            sf::VertexArray lines(sf::LineStrip, contour.size() + 1);
            for (size_t i = 0; i < contour.size(); ++i) {
                lines[i].position = contour[i];
                lines[i].color = sf::Color::Cyan;
            }
            lines[contour.size()].position = contour[0];
            lines[contour.size()].color = sf::Color::Cyan;
            window.draw(lines, states);
        }
    }

    drawPropertiesPanel(window);
}

void MagicWandTool::drawPropertiesPanel(sf::RenderWindow& window) {
    window.draw(m_panelBg);

    sf::Text title("MAGIC WAND", m_font, 14);
    title.setPosition(m_panelBg.getPosition().x + 10.f, m_panelBg.getPosition().y + 10.f);
    title.setFillColor(sf::Color(255, 200, 100));
    window.draw(title);

    std::vector<std::pair<std::string, SelectionBlendMode>> modes = {
        {"Replace Selection", SelectionBlendMode::Replace},
        {"Add to Selection", SelectionBlendMode::Add},
        {"Subtract Selection", SelectionBlendMode::Subtract},
        {"Intersect Selection", SelectionBlendMode::Intersect}
    };

    float y = m_panelBg.getPosition().y + 40.f;
    for (const auto& m : modes) {
        sf::Text t(m.first, m_font, 12);
        t.setPosition(m_panelBg.getPosition().x + 20.f, y + 5.f);
        if (m_blendMode == m.second) {
            sf::RectangleShape activeBg(sf::Vector2f(180.f, 24.f));
            activeBg.setPosition(m_panelBg.getPosition().x + 10.f, y);
            activeBg.setFillColor(sf::Color(0, 122, 204, 180));
            window.draw(activeBg);
            t.setFillColor(sf::Color::White);
        }
        else {
            t.setFillColor(sf::Color(200, 200, 200));
        }
        window.draw(t);
        y += 30.f;
    }

    y += 10.f;
    sf::Text tol("Tolerance: " + std::to_string(m_tolerance), m_font, 12);
    tol.setPosition(m_panelBg.getPosition().x + 20.f, y + 5.f);
    tol.setFillColor(sf::Color::White);
    window.draw(tol);

    sf::Text tMinus("[-]", m_font, 14); tMinus.setPosition(m_panelBg.getPosition().x + 130.f, y + 3.f); tMinus.setFillColor(sf::Color(200, 50, 50));
    sf::Text tPlus("[+]", m_font, 14); tPlus.setPosition(m_panelBg.getPosition().x + 160.f, y + 3.f); tPlus.setFillColor(sf::Color(50, 200, 50));
    window.draw(tMinus); window.draw(tPlus);

    y += 30.f;
    sf::Text contT("Contiguous: " + std::string(m_contiguous ? "ON" : "OFF"), m_font, 12);
    contT.setPosition(m_panelBg.getPosition().x + 20.f, y + 5.f);
    contT.setFillColor(m_contiguous ? sf::Color::Green : sf::Color::Red);
    window.draw(contT);

    y += 30.f;
    sf::Text sampT("Sample: " + std::string(m_sampleAllLayers ? "All Layers" : "Current"), m_font, 12);
    sampT.setPosition(m_panelBg.getPosition().x + 20.f, y + 5.f);
    sampT.setFillColor(sf::Color(200, 200, 200));
    window.draw(sampT);

    y += 30.f;
    sf::Text aaT("Anti-Alias: " + std::string(m_antiAlias ? "ON" : "OFF"), m_font, 12);
    aaT.setPosition(m_panelBg.getPosition().x + 20.f, y + 5.f);
    aaT.setFillColor(m_antiAlias ? sf::Color::Green : sf::Color::Red);
    window.draw(aaT);

    y += 30.f;
    sf::Text feathT("Feather/Grow: " + std::string(m_feather > 0 ? "+" : "") + std::to_string(m_feather), m_font, 12);
    feathT.setPosition(m_panelBg.getPosition().x + 20.f, y + 5.f);
    feathT.setFillColor(sf::Color::White);
    window.draw(feathT);

    sf::Text fMinus("[-]", m_font, 14); fMinus.setPosition(m_panelBg.getPosition().x + 130.f, y + 3.f); fMinus.setFillColor(sf::Color(200, 50, 50));
    sf::Text fPlus("[+]", m_font, 14); fPlus.setPosition(m_panelBg.getPosition().x + 160.f, y + 3.f); fPlus.setFillColor(sf::Color(50, 200, 50));
    window.draw(fMinus); window.draw(fPlus);
}