#pragma once
#include <memory>
#include <string>
#include <vector>
#include <SFML/Graphics/Image.hpp>
#include "Processing/SpriteDetector.h"
#include "DataModels/SpriteDefinition.h"
#include "Commands/BatchCommands.h"
#include "Systems/AnimationBuilder.h"

namespace StudioCore {

class WorkspaceManager;
class Project;
class SourceTexture;
class BackgroundJobQueue;
class CommandHistory;
class PlaybackEngine;
class ExportManager;

class StudioEngineFacade {
public:
    StudioEngineFacade();
    ~StudioEngineFacade();

    void Initialize();
    void Update(float deltaTime); 
    void CreateProject();
    bool IsProjectActive() const;
    bool ImportImage(const std::string& filePath, std::string& outErrorMessage);

    bool SaveProject(const std::string& filePath) const;
    bool LoadProject(const std::string& filePath, std::string& outErrorMessage);
    
    std::vector<ProposedAnimation> BuildAnimationsByRow();
    void CommitProposedAnimations(const std::vector<ProposedAnimation>& animations);
    void DeleteSpriteWithPixels(const std::string& spriteId);
    std::shared_ptr<Project> GetCurrentProject() const;
    std::shared_ptr<const SourceTexture> GetCurrentTexture() const;
    bool HasTexture() const;

    void RunAutoDetection(const DetectionConfig& config);
    void CancelDetection();
    bool IsDetectionRunning() const;
    float GetDetectionProgress() const;

    void Undo();
    void Redo();
    void EditPivot(const std::vector<std::string>& spriteIds, Point newPivot);
    void EditBaseline(const std::vector<std::string>& spriteIds, float newBaseline);

    void CreateAnimation(const std::string& name);
    void DeleteAnimation(const std::string& id);
    void ModifyAnimationFrames(const std::string& id, const std::vector<std::string>& newFrames);
    void EditAnimationSettings(const std::string& id, const std::string& newName, float fps, bool looping);

    void ToggleAutoAlign();
    bool IsAutoAlignEnabled() const;

    sf::Image GenerateExportPreview(int padding = 8, bool keepOriginalResolution = false) const;
    bool ExportPNG(const std::string& filePath, int padding = 8, bool keepOriginalResolution = false) const;

    PlaybackEngine& GetPlaybackEngine();
    const PlaybackEngine& GetPlaybackEngine() const;
    std::shared_ptr<WorkspaceManager> GetWorkspace() const;

    void ExecuteBatchOperation(const std::vector<std::string>& spriteIds, BatchOp op);
    void ExecuteAlignSprites(const std::vector<std::string>& spriteIds, AlignOp op);
    void RepackFrames();
    void FlipHorizontal();
    void MergeOverlappingSprites();
    void RemoveArtifacts(int targetX, int targetY);
    void RemoveArtifactsArea(int x, int y, int width, int height);
    void RemoveColorGlobal(int targetX, int targetY, float tolerance = 25.0f);
    void CleanCurrentTexture();
    void DeleteArea(int x, int y, int width, int height);
    void FillInternalHoles(int targetX, int targetY);
    void MergeSelectedSprites(const std::vector<std::string>& selectedIds);
    void FillTransparencyArea(int x, int y, int width, int height);
    bool ExportIndividualSprites(const std::string& outputFolder, const std::string& baseName = "sprite") const;
private:
    std::shared_ptr<WorkspaceManager> m_workspace;
    std::unique_ptr<BackgroundJobQueue> m_jobQueue;
    std::unique_ptr<CommandHistory> m_commandHistory;
    std::unique_ptr<PlaybackEngine> m_playbackEngine;
    std::unique_ptr<ExportManager> m_exportManager;
    int m_animIdCounter{1};
};

}