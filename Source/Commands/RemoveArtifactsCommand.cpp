#include "Commands/RemoveArtifactsCommand.h"
#include "DataModels/Project.h"
#include "DataModels/SourceTexture.h"
#include <queue>
#include <map>
#include <tuple>
#include <algorithm>
#include <cmath>

namespace StudioCore {

RemoveArtifactsCommand::RemoveArtifactsCommand(std::shared_ptr<Project> project, int startX, int startY)
    : m_project(project), m_startX(startX), m_startY(startY) {}

void RemoveArtifactsCommand::Execute() {
    if (!m_executed) {
        m_oldTexture = std::const_pointer_cast<SourceTexture>(m_project->GetTexture());
        if (!m_oldTexture) return;

        int w = m_oldTexture->GetWidth();
        int h = m_oldTexture->GetHeight();
        
        // Safety bounds check
        if (m_startX < 0 || m_startX >= w || m_startY < 0 || m_startY >= h) return;

        std::vector<uint8_t> pixels = m_oldTexture->GetPixels();
        std::vector<bool> mask(w * h, false);

        // 1. Sample the EXACT color of the pixel the user clicked on
        int clickIdx = (m_startY * w + m_startX) * 4;
        auto targetColor = std::make_tuple(pixels[clickIdx], pixels[clickIdx+1], pixels[clickIdx+2], pixels[clickIdx+3]);

        // If user accidentally clicked empty transparent space, abort safely.
        if (std::get<3>(targetColor) == 0) return;

        // Helper to calculate color difference
        auto colorDist = [](auto c1, auto c2) {
            return std::abs(std::get<0>(c1) - std::get<0>(c2)) +
                   std::abs(std::get<1>(c1) - std::get<1>(c2)) +
                   std::abs(std::get<2>(c1) - std::get<2>(c2));
        };

        std::queue<std::pair<int, int>> q;
        q.push({m_startX, m_startY});
        mask[m_startY * w + m_startX] = true;

        const int dx[] = {1, 1, 1, 0, -1, -1, -1, 0};
        const int dy[] = {-1, 0, 1, 1, 1, 0, -1, -1};
        
        // Define max size of the artifact so we don't accidentally erase a whole character
        int maxRadius = 45; 

        // 2. Flood-Fill Outward to capture the entire watermark
        while (!q.empty()) {
            auto [cx, cy] = q.front();
            q.pop();

            for (int i = 0; i < 8; ++i) {
                int nx = cx + dx[i];
                int ny = cy + dy[i];
                if (nx >= 0 && nx < w && ny >= 0 && ny < h && !mask[ny * w + nx]) {
                    // Stay within local radius
                    if (std::abs(nx - m_startX) <= maxRadius && std::abs(ny - m_startY) <= maxRadius) {
                        int nIdx = (ny * w + nx) * 4;
                        auto c = std::make_tuple(pixels[nIdx], pixels[nIdx+1], pixels[nIdx+2], pixels[nIdx+3]);
                        
                        // If color is similar to our clicked target (tolerance 60 for anti-aliasing)
                        if (std::get<3>(c) > 0 && colorDist(c, targetColor) < 60) {
                            mask[ny * w + nx] = true;
                            q.push({nx, ny});
                        }
                    }
                }
            }
        }

        // 3. Dilate the mask by 1 pixel to grab the soft/blended edges of the star
        std::vector<bool> dilatedMask = mask;
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                if (mask[y * w + x]) {
                    for (int i = 0; i < 8; ++i) {
                        int nx = x + dx[i];
                        int ny = y + dy[i];
                        if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                            dilatedMask[ny * w + nx] = true;
                        }
                    }
                }
            }
        }
        mask = dilatedMask;

        // 4. Mode-Filter Repair (Collapse colors from outside-in to preserve pixel scale)
        bool repairing = true;
        while (repairing) {
            repairing = false;
            std::vector<uint8_t> tempPixels = pixels;
            std::vector<bool> newMask = mask;

            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    if (mask[y * w + x]) {
                        std::map<std::tuple<uint8_t, uint8_t, uint8_t, uint8_t>, int> colorCounts;
                        int maxCount = 0;
                        std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> bestColor = {0, 0, 0, 0};
                        bool hasUnmaskedNeighbor = false;

                        // Check 5x5 local surrounding structure
                        for (int dy2 = -2; dy2 <= 2; ++dy2) {
                            for (int dx2 = -2; dx2 <= 2; ++dx2) {
                                int nx = x + dx2;
                                int ny = y + dy2;
                                if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
                                    if (!mask[ny * w + nx]) {
                                        hasUnmaskedNeighbor = true;
                                        int nIdx = (ny * w + nx) * 4;
                                        auto color = std::make_tuple(pixels[nIdx], pixels[nIdx+1], pixels[nIdx+2], pixels[nIdx+3]);
                                        if (std::get<3>(color) > 0) {
                                            colorCounts[color]++;
                                            if (colorCounts[color] > maxCount) {
                                                maxCount = colorCounts[color];
                                                bestColor = color;
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        if (hasUnmaskedNeighbor && maxCount > 0) {
                            int idx = (y * w + x) * 4;
                            tempPixels[idx]     = std::get<0>(bestColor);
                            tempPixels[idx + 1] = std::get<1>(bestColor);
                            tempPixels[idx + 2] = std::get<2>(bestColor);
                            tempPixels[idx + 3] = std::get<3>(bestColor);
                            newMask[y * w + x] = false;
                            repairing = true;
                        } else if (hasUnmaskedNeighbor) { // Fill with transparency if at canvas edge
                            int idx = (y * w + x) * 4;
                            tempPixels[idx + 3] = 0;
                            newMask[y * w + x] = false;
                            repairing = true;
                        }
                    }
                }
            }
            pixels = tempPixels;
            mask = newMask;
        }

        m_newTexture = std::make_shared<SourceTexture>(w, h, std::move(pixels));
        m_executed = true;
    }

    m_project->SetTexture(m_newTexture);
}

void RemoveArtifactsCommand::Undo() {
    if (m_project && m_oldTexture) {
        m_project->SetTexture(m_oldTexture);
    }
}

}