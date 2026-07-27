#include "StudioEngineFacade.h"
#include "Systems/WorkspaceManager.h"
#include "Systems/BackgroundJobQueue.h"
#include "Systems/PlaybackEngine.h"
#include "Systems/ExportManager.h"
#include "Systems/ProjectManager.h"
#include "Commands/CommandHistory.h"
#include "Commands/EditMetadataCommand.h"
#include "Commands/AnimationCommands.h"
#include "Processing/ImageLoader.h"
#include "DataModels/Project.h"
#include "Commands/BatchCommands.h"
#include "Commands/PixelCommands.h"
#include "DataModels/SpriteDefinition.h"
#include "DataModels/SourceTexture.h"
#include <iostream>
#include "Commands/MoveSpriteCommand.h"
#include "Commands/MoveSpriteWithPixelsCommand.h"

namespace StudioCore {

StudioEngineFacade::StudioEngineFacade() = default;
StudioEngineFacade::~StudioEngineFacade() = default;

void StudioEngineFacade::Initialize() {
    m_workspace = std::make_shared<WorkspaceManager>();
    m_jobQueue = std::make_unique<BackgroundJobQueue>();
    m_commandHistory = std::make_unique<CommandHistory>();
    m_playbackEngine = std::make_unique<PlaybackEngine>();
    m_exportManager = std::make_unique<ExportManager>();
}

void StudioEngineFacade::Update(float deltaTime) {
    if (m_jobQueue && m_jobQueue->HasResults()) {
        auto results = m_jobQueue->ConsumeResults();
        if (IsProjectActive()) {
            for (auto& sprite : results) {
                m_workspace->GetActiveProject()->AddSprite(*sprite); // <-- Added dereference here
            }
        }
    }
    if (IsProjectActive() && m_playbackEngine) {
        m_playbackEngine->Update(deltaTime, m_workspace->GetActiveProject().get());
    }
}

void StudioEngineFacade::CreateProject() {
    if (m_workspace) m_workspace->CreateNewProject();
    if (m_commandHistory) m_commandHistory->Clear();
    if (m_playbackEngine) m_playbackEngine->Stop();
    m_animIdCounter = 1;
}

bool StudioEngineFacade::IsProjectActive() const {
    return m_workspace && m_workspace->HasActiveProject();
}

bool StudioEngineFacade::ImportImage(const std::string& filePath, std::string& outErrorMessage) {
    if (!IsProjectActive()) return false;
    auto texture = ImageLoader::LoadFromFile(filePath, outErrorMessage);
    if (!texture) return false;
    m_workspace->GetActiveProject()->SetTexture(std::move(texture));
    m_workspace->GetActiveProject()->SetImagePath(filePath);
    return true;
}

bool StudioEngineFacade::SaveProject(const std::string& filePath) const {
    if (!IsProjectActive()) return false;
    return ProjectManager::SaveProject(*GetCurrentProject(), filePath);
}

bool StudioEngineFacade::LoadProject(const std::string& filePath, std::string& outErrorMessage) {
    auto proj = ProjectManager::LoadProject(filePath, outErrorMessage);
    if (!proj) return false;

    // Use std::const_pointer_cast to pass the loaded texture to SetTexture cleanly
    m_workspace->CreateNewProject();
    auto tex = std::const_pointer_cast<SourceTexture>(proj->GetTexture());
    m_workspace->GetActiveProject()->SetTexture(tex);
    m_workspace->GetActiveProject()->SetImagePath(proj->GetImagePath());

    for (const auto& s : proj->GetSprites()) {
        m_workspace->GetActiveProject()->AddSprite(*s);
    }
    for (const auto& a : proj->GetAnimationGroups()) {
        m_workspace->GetActiveProject()->AddAnimationGroup(std::make_shared<AnimationGroup>(*a));
    }

    if (m_commandHistory) m_commandHistory->Clear();
    if (m_playbackEngine) m_playbackEngine->Stop();

    return true;
}

std::shared_ptr<Project> StudioEngineFacade::GetCurrentProject() const {
    if (IsProjectActive()) return m_workspace->GetActiveProject();
    return nullptr;
}

std::shared_ptr<const SourceTexture> StudioEngineFacade::GetCurrentTexture() const {
    auto p = GetCurrentProject();
    if (p) return p->GetTexture();
    return nullptr;
}

bool StudioEngineFacade::HasTexture() const {
    return GetCurrentTexture() != nullptr;
}

void StudioEngineFacade::RunAutoDetection(const DetectionConfig& config) {
    if (!HasTexture() || m_jobQueue->IsRunning()) return;
    std::shared_ptr<const SourceTexture> tex = GetCurrentTexture();
    m_jobQueue->StartJob([tex, config](std::atomic<float>& p, std::atomic<bool>& c) {
        return SpriteDetector::Detect(*tex, config, p, c);
    });
}

void StudioEngineFacade::CancelDetection() {
    if (m_jobQueue) m_jobQueue->Cancel();
}

bool StudioEngineFacade::IsDetectionRunning() const {
    return m_jobQueue && m_jobQueue->IsRunning();
}

float StudioEngineFacade::GetDetectionProgress() const {
    return m_jobQueue ? m_jobQueue->GetProgress() : 0.0f;
}

void StudioEngineFacade::Undo() {
    if (m_commandHistory) {
        m_commandHistory->Undo();
    }
}

void StudioEngineFacade::Redo() {
    if (m_commandHistory) {
        m_commandHistory->Redo();
    }
}

void StudioEngineFacade::EditPivot(const std::vector<std::string>& spriteIds, Point newPivot) {
    if (!IsProjectActive() || spriteIds.empty()) return;
    auto cmd = std::make_unique<EditMetadataCommand>(GetCurrentProject(), spriteIds, EditType::Pivot, newPivot, 0.0f);
    m_commandHistory->ExecuteCommand(std::move(cmd));
}

void StudioEngineFacade::EditBaseline(const std::vector<std::string>& spriteIds, float newBaseline) {
    if (!IsProjectActive() || spriteIds.empty()) return;
    auto cmd = std::make_unique<EditMetadataCommand>(GetCurrentProject(), spriteIds, EditType::Baseline, Point{0,0}, newBaseline);
    m_commandHistory->ExecuteCommand(std::move(cmd));
}

void StudioEngineFacade::CreateAnimation(const std::string& name) {
    if (!IsProjectActive()) return;
    std::string id = "anim_" + std::to_string(m_animIdCounter++);
    auto cmd = std::make_unique<CreateAnimationCommand>(GetCurrentProject(), id, name);
    m_commandHistory->ExecuteCommand(std::move(cmd));
}

void StudioEngineFacade::DeleteAnimation(const std::string& id) {
    if (!IsProjectActive()) return;
    if (m_playbackEngine->GetActiveAnimation() == id) m_playbackEngine->Stop();
    auto cmd = std::make_unique<DeleteAnimationCommand>(GetCurrentProject(), id);
    m_commandHistory->ExecuteCommand(std::move(cmd));
}

void StudioEngineFacade::ModifyAnimationFrames(const std::string& id, const std::vector<std::string>& newFrames) {
    if (!IsProjectActive()) return;
    auto cmd = std::make_unique<ModifyFramesCommand>(GetCurrentProject(), id, newFrames);
    m_commandHistory->ExecuteCommand(std::move(cmd));
}

void StudioEngineFacade::EditAnimationSettings(const std::string& id, const std::string& newName, float fps, bool looping) {
    if (!IsProjectActive()) return;
    auto cmd = std::make_unique<EditAnimationSettingsCommand>(GetCurrentProject(), id, newName, fps, looping);
    m_commandHistory->ExecuteCommand(std::move(cmd));
}

void StudioEngineFacade::ToggleAutoAlign() {
    if (m_playbackEngine) {
        m_playbackEngine->SetAutoAlign(!m_playbackEngine->IsAutoAlignEnabled());
    }
}

bool StudioEngineFacade::IsAutoAlignEnabled() const {
    return m_playbackEngine ? m_playbackEngine->IsAutoAlignEnabled() : false;
}

sf::Image StudioEngineFacade::GenerateExportPreview(int padding) const {
    if (!IsProjectActive() || !m_exportManager) return sf::Image();
    return m_exportManager->GeneratePreview(*GetCurrentProject(), padding);
}

bool StudioEngineFacade::ExportPNG(const std::string& filePath, int padding) const {
    if (!IsProjectActive() || !m_exportManager) return false;
    return m_exportManager->ExportPNG(*GetCurrentProject(), filePath, padding);
}

PlaybackEngine& StudioEngineFacade::GetPlaybackEngine() {
    return *m_playbackEngine;
}

const PlaybackEngine& StudioEngineFacade::GetPlaybackEngine() const {
    return *m_playbackEngine;
}

std::shared_ptr<WorkspaceManager> StudioEngineFacade::GetWorkspace() const {
    return m_workspace;
}

void StudioEngineFacade::ExecuteBatchOperation(const std::vector<std::string>& spriteIds, BatchOp op) {
    if (!IsProjectActive() || spriteIds.empty()) return;
    auto cmd = std::make_unique<BatchOperationCommand>(GetCurrentProject(), spriteIds, op);
    m_commandHistory->ExecuteCommand(std::move(cmd));
}

void StudioEngineFacade::ExecuteAlignSprites(const std::vector<std::string>& spriteIds, AlignOp op) {
    if (!IsProjectActive() || spriteIds.empty()) return;
    auto cmd = std::make_unique<AlignSpritesCommand>(GetCurrentProject(), spriteIds, op);
    m_commandHistory->ExecuteCommand(std::move(cmd));
}

// Implementation for Facade:
std::vector<ProposedAnimation> StudioEngineFacade::BuildAnimationsByRow() {
    auto proj = GetCurrentProject();
    if (!proj) return {};
    
    AnimationBuilder builder;
    return builder.DetectByRows(proj->GetSprites());
}

void StudioEngineFacade::CommitProposedAnimations(const std::vector<ProposedAnimation>& animations) {
    auto proj = GetCurrentProject();
    if (!proj) return;

    for (const auto& prop : animations) {
        std::string animId = "anim_" + prop.name;
        
        auto group = std::make_shared<AnimationGroup>(animId, prop.name);
        group->SetFPS(static_cast<float>(prop.fps));
        group->SetLooping(prop.isLooping);
        
        auto idsToAdd = prop.spriteIds;
        if (prop.reverseOrder) {
            std::reverse(idsToAdd.begin(), idsToAdd.end());
        }

        // Set all frames at once using the built-in method
        group->SetFrames(idsToAdd);

        proj->AddAnimationGroup(group);
    }
}



void StudioEngineFacade::DeleteSpriteWithPixels(const std::string& spriteId) {
    auto proj = GetCurrentProject();
    if (!proj) return;

    auto constTexture = proj->GetTexture();
    if (!constTexture) return;

    auto texture = std::const_pointer_cast<SourceTexture>(constTexture);
    if (!texture || !texture->IsValid()) return;

    auto sprite = proj->GetSpriteById(spriteId);
    if (!sprite) return;

    auto rect = sprite->GetSourceRect();
    int imgWidth = texture->GetWidth();
    int imgHeight = texture->GetHeight();

    sf::Image currentCanvas;
    currentCanvas.create(imgWidth, imgHeight, texture->GetPixels().data());
    sf::Image erasedCanvas = currentCanvas;

    sf::IntRect srcRect(static_cast<int>(rect.x), static_cast<int>(rect.y),
                        static_cast<int>(rect.width), static_cast<int>(rect.height));

    for (int y = srcRect.top; y < srcRect.top + srcRect.height; ++y) {
        for (int x = srcRect.left; x < srcRect.left + srcRect.width; ++x) {
            if (x >= 0 && x < imgWidth && y >= 0 && y < imgHeight) {
                erasedCanvas.setPixel(x, y, sf::Color::Transparent);
            }
        }
    }

    std::vector<uint8_t> oldPixels = texture->GetPixels();
    std::vector<uint8_t> newPixels(erasedCanvas.getPixelsPtr(), erasedCanvas.getPixelsPtr() + (imgWidth * imgHeight * 4));

    auto pixelCmd = std::make_unique<PixelRegionCommand>(texture, oldPixels, newPixels);
    pixelCmd->Execute();

    proj->RemoveSprite(spriteId);

    if (m_commandHistory) {
        m_commandHistory->ExecuteCommand(std::move(pixelCmd));
    }
}

}