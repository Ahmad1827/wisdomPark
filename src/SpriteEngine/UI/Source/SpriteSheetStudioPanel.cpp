#include "SpriteSheetStudioPanel.h"
#include "Utils/NativeFileDialog.h"
#include "DataModels/Project.h"

#ifdef LoadImage
#undef LoadImage
#endif

#if defined(_WIN32)
#include <windows.h>
#include <commdlg.h>

enum class DialogMode {
    ImageOnly,
    CombinedOpen,
    ProjectSave
};

static std::string openWindowsFileDialog(DialogMode mode) {
    char currentDir[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, currentDir);
    OPENFILENAMEA ofn;
    char szFile[MAX_PATH] = { 0 };
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    const char imageFilter[] = "Image Files (*.png;*.jpg;*.jpeg)\0*.png;*.jpg;*.jpeg\0All Files (*.*)\0*.*\0\0";
    const char combinedFilter[] = "All Supported Files (*.png;*.jpg;*.jpeg;*.sps)\0*.png;*.jpg;*.jpeg;*.sps\0Image Files (*.png;*.jpg;*.jpeg)\0*.png;*.jpg;*.jpeg\0Sprite Sheet Studio (*.sps)\0*.sps\0All Files (*.*)\0*.*\0\0";
    ofn.lpstrFilter = (mode == DialogMode::CombinedOpen) ? combinedFilter : imageFilter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    std::string result = "";
    if (GetOpenFileNameA(&ofn)) {
        result = std::string(ofn.lpstrFile);
    }
    SetCurrentDirectoryA(currentDir);
    return result;
}

static std::string saveWindowsFileDialog(const char* defaultName = "project.sps") {
    char currentDir[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, currentDir);
    OPENFILENAMEA ofn;
    char szFile[MAX_PATH] = { 0 };
    strcpy_s(szFile, defaultName);
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    const char saveFilter[] = "Sprite Sheet Studio (*.sps)\0*.sps\0All Files (*.*)\0*.*\0\0";
    ofn.lpstrFilter = saveFilter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
    std::string result = "";
    if (GetSaveFileNameA(&ofn)) {
        result = std::string(ofn.lpstrFile);
    }
    SetCurrentDirectoryA(currentDir);
    return result;
}
#endif

namespace StudioUI {

SpriteSheetStudioPanel::SpriteSheetStudioPanel() {
    m_animationPanel = std::make_unique<AnimationPanel>();
}

SpriteSheetStudioPanel::~SpriteSheetStudioPanel() = default;

void SpriteSheetStudioPanel::Initialize() {
    m_engine.Initialize();
    m_engine.CreateProject();
    m_viewport.Initialize();
    if (!m_engine.IsAutoAlignEnabled()) {
        m_engine.ToggleAutoAlign();
    }
    m_toolbar.Initialize("Resources/font.ttf",
        [this]() {
#if defined(_WIN32)
            std::string path = openWindowsFileDialog(DialogMode::ImageOnly);
#else
            std::string path = NativeFileDialog::OpenFileDialog();
#endif
            if (!path.empty()) LoadImage(path);
        },
        [this]() {
#if defined(_WIN32)
            std::string path = openWindowsFileDialog(DialogMode::CombinedOpen);
#else
            std::string path = NativeFileDialog::OpenFileDialog("Supported Files (*.png;*.jpg;*.jpeg;*.sps)");
#endif
            if (!path.empty()) {
                if (path.find(".sps") != std::string::npos) {
                    std::string err;
                    if (m_engine.LoadProject(path, err)) m_viewport.RefreshTexture(m_engine);
                } else {
                    LoadImage(path);
                }
            }
        },
        [this]() {
#if defined(_WIN32)
            std::string path = saveWindowsFileDialog("project.sps");
#else
            std::string path = NativeFileDialog::SaveFileDialog("project.sps");
#endif
            if (!path.empty()) m_engine.SaveProject(path);
        },
        [this]() { m_isExportMode = true; m_exportPreview.Activate(m_engine); },
        [this]() {
            m_isUIHidden = !m_isUIHidden;
            m_viewport.SetUIHidden(m_isUIHidden);
        },
        [this]() {
            if (!m_engine.IsProjectActive() || !m_engine.GetCurrentProject() || m_engine.GetCurrentProject()->GetSprites().empty()) return;
            m_isWizardMode = true;
            m_animBuilderPanel.Activate(m_engine);
        },
        [this]() {
            if (m_engine.IsProjectActive()) {
                StudioCore::DetectionConfig config;
                m_engine.RunAutoDetection(config);
                m_viewport.RefreshTexture(m_engine);
            }
        }
    );
    m_animationPanel->InitializeFont("Resources/font.ttf");
    m_exportPreview.InitializeFont("Resources/font.ttf");
    m_animBuilderPanel.InitializeFont("Resources/font.ttf");
    m_workspace.InitializeFont("Resources/font.ttf");
}

void SpriteSheetStudioPanel::LoadImage(const std::string& filePath) {
    std::string errorMsg;
    if (m_engine.ImportImage(filePath, errorMsg)) {
        m_viewport.RefreshTexture(m_engine);
    }
}

void SpriteSheetStudioPanel::HandleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    if (!m_isActive) return;

    sf::FloatRect currentBounds(0.f, 0.f, static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y));
    SetBounds(currentBounds);

    if (event.type == sf::Event::KeyPressed) {
        bool isShift = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift);
        bool isControl = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::RControl);

        if (isShift && event.key.code == sf::Keyboard::N) {
            if (m_engine.IsProjectActive()) {
                static int animCounter = 1;
                m_engine.CreateAnimation("New Animation " + std::to_string(animCounter++));
                m_viewport.RefreshTexture(m_engine);
                return;
            }
        }
        if (isControl) {
            if (event.key.code == sf::Keyboard::Z) { m_engine.Undo(); m_viewport.RefreshTexture(m_engine); return; }
            if (event.key.code == sf::Keyboard::Y) { m_engine.Redo(); m_viewport.RefreshTexture(m_engine); return; }
            if (event.key.code == sf::Keyboard::E) { m_isExportMode = true; m_exportPreview.Activate(m_engine); return; }
            if (event.key.code == sf::Keyboard::L) {
#if defined(_WIN32)
                std::string path = openWindowsFileDialog(DialogMode::CombinedOpen);
#else
                std::string path = NativeFileDialog::OpenFileDialog("Supported Files (*.png;*.jpg;*.jpeg;*.sps)");
#endif
                if (!path.empty()) {
                    if (path.find(".sps") != std::string::npos) {
                        std::string err;
                        if (m_engine.LoadProject(path, err)) m_viewport.RefreshTexture(m_engine);
                    } else {
                        LoadImage(path);
                    }
                }
                return;
            }
            if (event.key.code == sf::Keyboard::S) {
#if defined(_WIN32)
                std::string path = saveWindowsFileDialog("project.sps");
#else
                std::string path = NativeFileDialog::SaveFileDialog("project.sps");
#endif
                if (!path.empty()) m_engine.SaveProject(path);
                return;
            }
        }
        if (event.key.code == sf::Keyboard::A) {
            m_engine.ToggleAutoAlign();
            m_viewport.RefreshTexture(m_engine);
            return;
        }
    }

    if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Right) {
        sf::Vector2i pixelPos(event.mouseButton.x, event.mouseButton.y);
        sf::Vector2f worldPos = m_viewport.MapPixelToWorld(pixelPos, window);
        std::string targetSpriteId = "";
        if (m_engine.IsProjectActive() && m_engine.GetCurrentProject()) {
            auto sprites = m_engine.GetCurrentProject()->GetSprites();
            for (auto it = sprites.rbegin(); it != sprites.rend(); ++it) {
                auto sprite = *it;
                if (!sprite) continue;
                auto rect = sprite->GetSourceRect();
                sf::FloatRect bounds(rect.x, rect.y, rect.width, rect.height);
                if (bounds.contains(worldPos)) {
                    targetSpriteId = sprite->GetId();
                    break;
                }
            }
        }
        if (!targetSpriteId.empty()) {
            std::vector<StudioUI::ContextMenuItem> items = {
                {"Delete Sprite", [this, targetSpriteId]() {
                    m_engine.DeleteSpriteWithPixels(targetSpriteId);
                    m_viewport.RefreshTexture(m_engine);
                }},
                {"Reset Pivot", [this, targetSpriteId]() {
                    auto proj = m_engine.GetCurrentProject();
                    if (!proj) return;
                    auto sprite = proj->GetSpriteById(targetSpriteId);
                    if (sprite) {
                        auto rect = sprite->GetSourceRect();
                        sprite->SetPivot({rect.width / 2.0f, rect.height / 2.0f});
                        m_viewport.RefreshTexture(m_engine);
                    }
                }}
            };
            m_workspace.ShowContextMenu({static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y)}, items);
            return;
        }
    }

    if (m_workspace.HandleEvent(event, window)) return;
    if (m_isWizardMode) {
        bool exitWizard = false;
        m_animBuilderPanel.HandleEvent(event, window, m_engine, exitWizard);
        if (exitWizard) m_isWizardMode = false;
        return;
    }
    if (!m_isExportMode) {
        if (m_toolbar.HandleEvent(event, window, m_engine)) return;
    } else {
        if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
            m_exportPreview.Deactivate();
            m_isExportMode = false;
        } else {
            m_exportPreview.HandleEvent(event, window, m_engine);
            if (!m_exportPreview.IsActive()) {
                m_isExportMode = false;
            }
        }
        return;
    }
    if (m_animationPanel) {
        m_animationPanel->HandleEvent(event, window, m_engine, m_viewport);
    }
    m_viewport.HandleEvent(event, window, m_engine);
}

void SpriteSheetStudioPanel::Update(float deltaTime, const sf::RenderWindow& window) {
    if (!m_isActive) return;

    sf::FloatRect currentBounds(0.f, 0.f, static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y));
    SetBounds(currentBounds);

    if (m_isExportMode) {
        if (!m_exportPreview.IsActive()) {
            m_isExportMode = false;
        }
        return;
    }
    if (!m_isWizardMode) {
        m_engine.Update(deltaTime);
        m_viewport.Update(deltaTime);
        m_autoSaveTimer += deltaTime;
        if (m_autoSaveTimer >= 300.0f) {
            if (m_engine.IsProjectActive()) m_engine.SaveProject("autosave_backup.sps");
            m_autoSaveTimer = 0.0f;
        }
        sf::Vector2i pixelPos = sf::Mouse::getPosition(window);
        sf::Vector2f worldPos = window.mapPixelToCoords(pixelPos);
        int totalSprites = (m_engine.IsProjectActive() && m_engine.GetCurrentProject())
                            ? static_cast<int>(m_engine.GetCurrentProject()->GetSprites().size()) : 0;
        m_workspace.UpdateStatusBar(1.0f, worldPos, totalSprites, 0, "Ready");
    }
}

void SpriteSheetStudioPanel::SetBounds(const sf::FloatRect& bounds) {
    m_bounds = bounds;
    m_toolbar.SetBounds(bounds);
    m_viewport.SetBounds(bounds);
    m_workspace.SetBounds(bounds);
    
    if (m_animationPanel) {
        m_animationPanel->SetBounds(bounds);
    }
}

void SpriteSheetStudioPanel::Render(sf::RenderWindow& window) {
    if (!m_isActive) return;

    sf::FloatRect currentBounds(0.f, 0.f, static_cast<float>(window.getSize().x), static_cast<float>(window.getSize().y));
    SetBounds(currentBounds);

    sf::View uiView(currentBounds);
    window.setView(uiView);

    if (m_isExportMode && m_exportPreview.IsActive()) {
        m_exportPreview.Render(window);
    }
    else {
        m_isExportMode = false;
        m_viewport.Render(window, m_engine);
        window.setView(uiView);
        if (!m_isUIHidden && m_animationPanel) {
            m_animationPanel->Render(window, m_engine);
        }
        m_toolbar.Render(window);
        if (m_isWizardMode) {
            m_animBuilderPanel.Render(window);
        }
        m_workspace.Render(window);
    }
}
}