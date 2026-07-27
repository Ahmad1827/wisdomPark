#include "Processing/SpriteDetector.h"
#include "DataModels/SourceTexture.h"
#include "DataModels/SpriteDefinition.h"
#include <unordered_map>
#include <algorithm>
#include <cmath>

namespace StudioCore {

class UnionFind {
public:
    UnionFind(int size) : parent(size) {
        for (int i = 0; i < size; ++i) parent[i] = i;
    }
    int Find(int i) {
        if (parent[i] == i) return i;
        return parent[i] = Find(parent[i]);
    }
    void Unite(int i, int j) {
        int rootI = Find(i);
        int rootJ = Find(j);
        if (rootI != rootJ) {
            if (rootI < rootJ) parent[rootJ] = rootI;
            else parent[rootI] = rootJ;
        }
    }
private:
    std::vector<int> parent;
};

struct ComponentStats {
    int minX{999999}, minY{999999};
    int maxX{-1}, maxY{-1};
    int pixelCount{0};
    double sumX{0}, sumY{0};
};

std::vector<std::shared_ptr<SpriteDefinition>> SpriteDetector::Detect(
    const SourceTexture& texture,
    const DetectionConfig& config,
    std::atomic<float>& progress,
    std::atomic<bool>& cancelToken
) {
    std::vector<std::shared_ptr<SpriteDefinition>> results;
    int width = texture.GetWidth();
    int height = texture.GetHeight();
    const auto& pixels = texture.GetPixels();

    std::vector<bool> mask(width * height, false);
    for (int i = 0; i < width * height; ++i) {
        if (cancelToken) return {};
        size_t idx = i * 4;
        
        if (config.mode == DetectionMode::AlphaThreshold) {
            mask[i] = (pixels[idx + 3] >= config.alphaThreshold);
        } else {
            float dr = pixels[idx] - config.targetColor[0];
            float dg = pixels[idx + 1] - config.targetColor[1];
            float db = pixels[idx + 2] - config.targetColor[2];
            float dist = std::sqrt(dr*dr + dg*dg + db*db);
            mask[i] = (dist > config.colorTolerance && pixels[idx + 3] > 0);
        }
    }
    progress = 0.2f;

    std::vector<int> labels(width * height, 0);
    int nextLabel = 1;
    UnionFind uf(width * height / 4 + 1);

    for (int y = 0; y < height; ++y) {
        if (cancelToken) return {};
        for (int x = 0; x < width; ++x) {
            if (!mask[y * width + x]) continue;

            std::vector<int> neighborLabels;
            if (x > 0 && labels[y * width + (x - 1)] > 0) 
                neighborLabels.push_back(labels[y * width + (x - 1)]);
            if (y > 0 && labels[(y - 1) * width + x] > 0) 
                neighborLabels.push_back(labels[(y - 1) * width + x]);

            if (config.use8WayConnectivity && y > 0) {
                if (x > 0 && labels[(y - 1) * width + (x - 1)] > 0)
                    neighborLabels.push_back(labels[(y - 1) * width + (x - 1)]);
                if (x < width - 1 && labels[(y - 1) * width + (x + 1)] > 0)
                    neighborLabels.push_back(labels[(y - 1) * width + (x + 1)]);
            }

            if (neighborLabels.empty()) {
                labels[y * width + x] = nextLabel;
                uf.Find(nextLabel); 
                nextLabel++;
            } else {
                int minLabel = neighborLabels[0];
                for (int l : neighborLabels) minLabel = std::min(minLabel, l);
                labels[y * width + x] = minLabel;
                for (int l : neighborLabels) uf.Unite(minLabel, l);
            }
        }
        progress = 0.2f + 0.3f * ((float)y / height);
    }

    std::unordered_map<int, ComponentStats> statsMap;
    for (int y = 0; y < height; ++y) {
        if (cancelToken) return {};
        for (int x = 0; x < width; ++x) {
            int l = labels[y * width + x];
            if (l > 0) {
                int root = uf.Find(l);
                auto& s = statsMap[root];
                s.minX = std::min(s.minX, x);
                s.minY = std::min(s.minY, y);
                s.maxX = std::max(s.maxX, x);
                s.maxY = std::max(s.maxY, y);
                s.pixelCount++;
                s.sumX += x;
                s.sumY += y;
            }
        }
        progress = 0.5f + 0.4f * ((float)y / height);
    }

    int idCounter = 1;
    for (const auto& kv : statsMap) {
        if (cancelToken) return {};
        const auto& s = kv.second;
        
        int bw = (s.maxX - s.minX) + 1;
        int bh = (s.maxY - s.minY) + 1;

        if (bw >= config.minSpriteSize && bh >= config.minSpriteSize &&
            bw <= config.maxSpriteSize && bh <= config.maxSpriteSize) {
            
            Rect rect{
                std::max(0, s.minX - config.padding),
                std::max(0, s.minY - config.padding),
                std::min(width - s.minX, bw + config.padding * 2),
                std::min(height - s.minY, bh + config.padding * 2)
            };

            auto def = std::make_shared<SpriteDefinition>("sprite_" + std::to_string(idCounter++), rect);
            def->SetPixelCount(s.pixelCount);
            def->SetCenter(Point{static_cast<float>(s.sumX / s.pixelCount), static_cast<float>(s.sumY / s.pixelCount)});
            results.push_back(def);
        }
    }

    progress = 1.0f;
    return results;
}

}