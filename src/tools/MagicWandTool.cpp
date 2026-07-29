#include "MagicWandTool.h"
#include "../core/ExportManager.h"
#include <cmath>

MagicWandTool::MagicWandTool(Canvas& canvas, Timeline& timeline)
    : m_canvas(canvas), m_timeline(timeline), m_tolerance(10),
    m_contiguous(true), m_sampleAllLayers(false), m_requestColorPanelOpen(false) {
    m_lastPrimaryColor = canvas.getPrimaryColor();
}

void MagicWandTool::Initialize() {
    m_font.loadFromFile("assets/font.otf");
    m_panelBg.setSize(sf::Vector2f(220.f, 210.f));
    m_panelBg.setFillColor(sf::Color(25, 25, 30, 240));
    m_panelBg.setOutlineThickness(1.f);
    m_panelBg.setOutlineColor(sf::Color(100, 100, 110));
}

void MagicWandTool::SetBounds(const sf::FloatRect& bounds) {
    m_bounds = bounds;
    m_panelBg.setPosition(bounds.left + 110.f, bounds.top + 60.f);
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
    sf::Vector2i mousePosI = sf::Mouse::getPosition(window);
    sf::Vector2f mousePos = window.mapPixelToCoords(mousePosI);

    if (m_panelBg.getGlobalBounds().contains(mousePos)) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {

            if (m_colorBoxRect.contains(mousePos)) {
                m_requestColorPanelOpen = true;
                return;
            }

            float y = mousePos.y - m_panelBg.getPosition().y;
            if (y > 40 && y < 70) {
                if (mousePos.x < m_panelBg.getPosition().x + 110) m_tolerance = std::max(0, m_tolerance - 5);
                else m_tolerance = std::min(255, m_tolerance + 5);
            }
            else if (y > 80 && y < 110) m_contiguous = !m_contiguous;
            else if (y > 120 && y < 150) m_sampleAllLayers = !m_sampleAllLayers;
        }
        return;
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button != sf::Mouse::Left) return;

    sf::Vector2f viewPos = m_canvas.getInverseTransform().transformPoint(mousePos);
    float scaleX = static_cast<float>(m_canvas.getCanvasSize().x) / m_canvas.getDrawArea().width;
    float scaleY = static_cast<float>(m_canvas.getCanvasSize().y) / m_canvas.getDrawArea().height;
    sf::Vector2i logicalPos(static_cast<int>((viewPos.x - m_canvas.getDrawArea().left) * scaleX),
        static_cast<int>((viewPos.y - m_canvas.getDrawArea().top) * scaleY));

    if (m_canvas.getDrawArea().contains(viewPos)) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {

            auto mask = extractSelectionMask(logicalPos);
            int w = m_canvas.getCanvasSize().x;
            int h = m_canvas.getCanvasSize().y;

            std::vector<std::vector<sf::Vector2f>> polygons;
            std::vector<bool> visited(w * h, false);

            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    if (mask[y * w + x] && !visited[y * w + x]) {

                        std::queue<sf::Vector2i> q;
                        q.push({ x, y });
                        visited[y * w + x] = true;
                        while (!q.empty()) {
                            sf::Vector2i p = q.front(); q.pop();
                            sf::Vector2i n[4] = { {p.x + 1, p.y}, {p.x - 1, p.y}, {p.x, p.y + 1}, {p.x, p.y - 1} };
                            for (auto& i : n) {
                                if (i.x >= 0 && i.x < w && i.y >= 0 && i.y < h) {
                                    if (mask[i.y * w + i.x] && !visited[i.y * w + i.x]) {
                                        visited[i.y * w + i.x] = true;
                                        q.push(i);
                                    }
                                }
                            }
                        }

                        auto poly = traceBoundary(mask, w, h, { x, y });
                        if (!poly.empty()) polygons.push_back(poly);
                    }
                }
            }

            if (!polygons.empty()) {
                std::vector<sf::Vector2f> unified;
                for (const auto& poly : polygons) {
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

                m_canvas.commitSelection(m_timeline.getCurrentFrame());
                m_canvas.getSelectionManager().clearSelection();
                m_canvas.getSelectionManager().startLasso(unified[0], m_canvas.getCanvasSize());
                for (size_t i = 1; i < unified.size(); ++i) {
                    m_canvas.getSelectionManager().addLassoPoint(unified[i], m_canvas.getCanvasSize());
                }
                m_canvas.getSelectionManager().endLasso();
            }
            else {
                m_canvas.commitSelection(m_timeline.getCurrentFrame());
                m_canvas.getSelectionManager().clearSelection();
            }
        }
    }
}

void MagicWandTool::Update(float deltaTime, const sf::RenderWindow& window) {
    m_canvas.updateTransform(deltaTime, m_bounds);

    sf::Color currentPrimary = m_canvas.getPrimaryColor();
    if (currentPrimary != m_lastPrimaryColor) {
        if (m_canvas.getSelectionManager().isActive()) {
            auto* tex = m_canvas.getActiveRenderTexture(m_timeline.getCurrentFrame());
            if (tex) {
                m_canvas.saveUndoState();
                sf::Image img = tex->getTexture().copyToImage();
                unsigned int w = std::min(img.getSize().x, m_canvas.getCanvasSize().x);
                unsigned int h = std::min(img.getSize().y, m_canvas.getCanvasSize().y);
                for (unsigned int y = 0; y < h; ++y) {
                    for (unsigned int x = 0; x < w; ++x) {
                        if (m_canvas.getSelectionManager().isPointInsideSelection(sf::Vector2f(static_cast<float>(x), static_cast<float>(y)))) {
                            img.setPixel(x, y, currentPrimary);
                        }
                    }
                }
                sf::Texture newTex; newTex.loadFromImage(img);
                tex->clear(sf::Color::Transparent);
                tex->draw(sf::Sprite(newTex), sf::RenderStates(sf::BlendNone));
                tex->display();
            }
        }
        m_lastPrimaryColor = currentPrimary;
    }
}

void MagicWandTool::Render(sf::RenderWindow& window) {
    m_canvas.draw(window, m_timeline.getCurrentFrame(), m_timeline.isPlaying(), sf::RenderStates::Default);
    drawPropertiesPanel(window);
}

void MagicWandTool::drawPropertiesPanel(sf::RenderWindow& window) {
    window.draw(m_panelBg);

    sf::Text title("MAGIC WAND", m_font, 14);
    title.setPosition(m_panelBg.getPosition().x + 10.f, m_panelBg.getPosition().y + 10.f);
    title.setFillColor(sf::Color(255, 200, 100));
    window.draw(title);

    float y = m_panelBg.getPosition().y + 40.f;
    sf::Text tol("Tolerance: " + std::to_string(m_tolerance), m_font, 12);
    tol.setPosition(m_panelBg.getPosition().x + 20.f, y + 5.f);
    tol.setFillColor(sf::Color::White);
    window.draw(tol);

    sf::Text tMinus("[-]", m_font, 14); tMinus.setPosition(m_panelBg.getPosition().x + 130.f, y + 3.f); tMinus.setFillColor(sf::Color(200, 50, 50));
    sf::Text tPlus("[+]", m_font, 14); tPlus.setPosition(m_panelBg.getPosition().x + 160.f, y + 3.f); tPlus.setFillColor(sf::Color(50, 200, 50));
    window.draw(tMinus); window.draw(tPlus);

    y += 40.f;
    sf::Text contT("Contiguous: " + std::string(m_contiguous ? "ON" : "OFF"), m_font, 12);
    contT.setPosition(m_panelBg.getPosition().x + 20.f, y + 5.f);
    contT.setFillColor(m_contiguous ? sf::Color::Green : sf::Color::Red);
    window.draw(contT);

    y += 40.f;
    sf::Text sampT("Sample: " + std::string(m_sampleAllLayers ? "All Layers" : "Current"), m_font, 12);
    sampT.setPosition(m_panelBg.getPosition().x + 20.f, y + 5.f);
    sampT.setFillColor(sf::Color(200, 200, 200));
    window.draw(sampT);

    y += 40.f;
    sf::Text fillTxt("Change Into:", m_font, 12);
    fillTxt.setPosition(m_panelBg.getPosition().x + 20.f, y + 5.f);
    fillTxt.setFillColor(sf::Color(200, 200, 200));
    window.draw(fillTxt);

    m_colorBoxRect = sf::FloatRect(m_panelBg.getPosition().x + 110.f, y, 40.f, 24.f);
    sf::RectangleShape colorBox(sf::Vector2f(m_colorBoxRect.width, m_colorBoxRect.height));
    colorBox.setPosition(m_colorBoxRect.left, m_colorBoxRect.top);
    colorBox.setFillColor(m_canvas.getPrimaryColor());
    colorBox.setOutlineThickness(1.f);
    colorBox.setOutlineColor(sf::Color(200, 200, 200));
    window.draw(colorBox);
}