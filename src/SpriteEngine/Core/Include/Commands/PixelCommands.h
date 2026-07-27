#pragma once
#include "Commands/ICommand.h"
#include "DataModels/SourceTexture.h"
#include <vector>
#include <cstdint>
#include <memory>

namespace StudioCore {

class PixelRegionCommand : public ICommand {
public:
    PixelRegionCommand(std::shared_ptr<SourceTexture> texture,
                       const std::vector<uint8_t>& oldPixels,
                       const std::vector<uint8_t>& newPixels)
        : m_texture(texture), m_oldPixels(oldPixels), m_newPixels(newPixels) {}

    void Execute() override {
        if (!m_texture) return;
        m_texture->SetPixels(m_newPixels);
    }

    void Undo() override {
        if (!m_texture) return;
        m_texture->SetPixels(m_oldPixels);
    }

private:
    std::shared_ptr<SourceTexture> m_texture;
    std::vector<uint8_t> m_oldPixels;
    std::vector<uint8_t> m_newPixels;
};

}