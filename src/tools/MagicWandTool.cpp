#include "MagicWandTool.h"
#include "../core/ExportManager.h"
#include <cmath>

MagicWandTool::MagicWandTool(Canvas& canvas, Timeline& timeline)
    : m_canvas(canvas), m_timeline(timeline), m_tolerance(10),
    m_contiguous(true), m_sampleAllLayers(false), m_isPanning(false) {
}

void MagicWandTool::Initialize() {
    m_font.loadFromFile("assets/font.otf");
    m_panelBg.setSize(sf::Vector2f(220.f, 180.f));
    m_panelBg.setFillColor(sf::Color(25, 25, 30, 240));
    m_panelBg.setOutlineThickness(1.f);
    m_panelBg.setOutlineColor(sf::Color(100, 100, 110));
}

void MagicWandTool::SetBounds(const sf::FloatRect& bounds) {
    m_bounds = bounds;
    m_panelBg.setPosition(bounds.left + 110.f, bounds.top + 60.f);
}

float MagicWandTool::getPerceptualDistance(sf::Color c1, sf::Color c2) {
    // Fixed: Properly isolates alpha! Black and Transparent are no longer identical.
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

std::vector<sf::Vector2f> MagicWandTool::traceBoundary(const std::vector<bool>& mask, int w, int h) {
    sf::Vector2i startNode(-1, -1);

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (mask[y * w + x]) {
                startNode = { x, y };
                break;
            }
        }
        if (startNode.x != -1) break;
    }

    if (startNode.x == -1) return {};

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

    poly.push_back(sf::Vector2f(static_cast<float>(curr.x), static_cast<float>(curr.y)));
    return poly;
}

void MagicWandTool::HandleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    sf::Vector2i mousePosI = sf::Mouse::getPosition(window);
    sf::Vector2f mousePos = window.mapPixelToCoords(mousePosI);

    if (m_panelBg.getGlobalBounds().contains(mousePos)) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
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

    // Completely fixed camera panning (uses raw screen pixels)
    if (event.type == sf::Event::MouseButtonPressed && (event.mouseButton.button == sf::Mouse::Right || event.mouseButton.button == sf::Mouse::Middle)) {
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
    sf::Vector2i logicalPos(static_cast<int>((viewPos.x - m_canvas.getDrawArea().left) * scaleX),
        static_cast<int>((viewPos.y - m_canvas.getDrawArea().top) * scaleY));

    if (m_canvas.getDrawArea().contains(viewPos)) {
        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            auto mask = extractSelectionMask(logicalPos);
            auto poly = traceBoundary(mask, m_canvas.getCanvasSize().x, m_canvas.getCanvasSize().y);

            if (!poly.empty()) {
                m_canvas.commitSelection(m_timeline.getCurrentFrame());
                m_canvas.getSelectionManager().clearSelection();

                m_canvas.getSelectionManager().startLasso(poly[0], m_canvas.getCanvasSize());
                for (size_t i = 1; i < poly.size(); ++i) {
                    m_canvas.getSelectionManager().addLassoPoint(poly[i], m_canvas.getCanvasSize());
                }
                m_canvas.getSelectionManager().endLasso();
            }
        }
    }
}

void MagicWandTool::Update(float deltaTime, const sf::RenderWindow& window) {
    m_canvas.updateTransform(deltaTime, m_bounds);
}

void MagicWandTool::Render(sf::RenderWindow& window) {
    sf::RenderStates canvasStates;
    canvasStates.transform = m_canvas.getTransform();
    m_canvas.draw(window, m_timeline.getCurrentFrame(), m_timeline.isPlaying(), canvasStates);

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
}