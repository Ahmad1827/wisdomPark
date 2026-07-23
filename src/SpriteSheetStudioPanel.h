#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include "StudioEngineFacade.h"
#include "Panels/Toolbar.h"
#include "Panels/PreviewViewport.h"
#include "Panels/AnimationPanel.h"
#include "Panels/WorkspaceEnvironment.h"
#include "Panels/ExportPreviewPanel.h"
#include "Panels/AnimationBuilderPanel.h"
#ifdef LoadImage
#undef LoadImage
#endif
namespace StudioUI {

class SpriteSheetStudioPanel {
public:
    SpriteSheetStudioPanel();
    ~SpriteSheetStudioPanel();

    // The clean public interface for host applications
    void Initialize();
    void HandleEvent(const sf::Event& event, const sf::RenderWindow& window);
    void Update(float deltaTime, const sf::RenderWindow& window);
    void Render(sf::RenderWindow& window);

    void SetBounds(const sf::FloatRect& bounds);
    void OnActivated() { m_isActive = true; }
    void OnDeactivated() { m_isActive = false; }

private:
    void LoadImage(const std::string& filePath);

    StudioCore::StudioEngineFacade m_engine;
    sf::FloatRect m_bounds;
    bool m_isActive = true;

    bool m_isExportMode = false;
    bool m_isWizardMode = false;
    bool m_isUIHidden = false;
    float m_autoSaveTimer = 0.0f;

    Toolbar m_toolbar;
    PreviewViewport m_viewport;
    std::unique_ptr<AnimationPanel> m_animationPanel;
    WorkspaceEnvironment m_workspace;
    ExportPreviewPanel m_exportPreview;
    AnimationBuilderPanel m_animBuilderPanel;
};

} // namespace StudioUI