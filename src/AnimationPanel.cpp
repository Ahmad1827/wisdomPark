#include "Panels/AnimationPanel.h"
#include "Panels/PreviewViewport.h"
#include "StudioEngineFacade.h"
#include "DataModels/Project.h"
#include "DataModels/AnimationGroup.h"
#include "DataModels/SpriteDefinition.h"
#include "DataModels/SourceTexture.h"
#include "Systems/PlaybackEngine.h"
#include "Theme.h"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace StudioUI {

AnimationPanel::AnimationPanel() {
    m_bgShape.setFillColor(sf::Color(40, 40, 40, 255));
    m_bgShape.setOutlineColor(sf::Color(100, 100, 100));
    m_bgShape.setOutlineThickness(-1.0f);
}

bool AnimationPanel::InitializeFont(const std::string& customPath) {
    std::vector<std::string> fontPaths = {
        customPath,
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf"
    };
    for (const auto& path : fontPaths) {
        if (m_font.loadFromFile(path)) {
            m_hasFont = true;
            return true;
        }
    }
    std::cerr << "[AnimationPanel] Failed to load font!" << std::endl;
    return false;
}

void AnimationPanel::SetBounds(const sf::FloatRect& bounds) {
    m_bounds = bounds;
}

void AnimationPanel::HandleEvent(const sf::Event& event, const sf::RenderWindow& window, StudioCore::StudioEngineFacade& engine, PreviewViewport& viewport) {
    if (!engine.IsProjectActive()) return;

    // Synchronize bounding logic dynamically so hits match the visual renders
    float timelineHeight = 160.0f;
    float rightPanelWidth = 300.0f;
    float topOffset = StudioUI::Theme::ToolbarHeight; 
    
    m_timelineArea = sf::FloatRect(m_bounds.left, m_bounds.top + m_bounds.height - timelineHeight, m_bounds.width, timelineHeight);
    m_listArea = sf::FloatRect(m_bounds.left + m_bounds.width - rightPanelWidth, m_bounds.top + topOffset, rightPanelWidth, (m_bounds.height - topOffset - timelineHeight) / 2.0f);
    m_previewArea = sf::FloatRect(m_bounds.left + m_bounds.width - rightPanelWidth, m_listArea.top + m_listArea.height, rightPanelWidth, m_listArea.height);

    const auto& selectedSprites = viewport.GetSelectedSpriteIds();

    if (event.type == sf::Event::KeyPressed) {
        if (event.key.code == sf::Keyboard::N && event.key.shift) {
            engine.CreateAnimation("New Animation");
        } else if (event.key.code == sf::Keyboard::Space) {
            if (engine.GetPlaybackEngine().IsPlaying()) {
                engine.GetPlaybackEngine().Pause();
            } else {
                engine.GetPlaybackEngine().Play();
            }
        } 
        else if (event.key.code == sf::Keyboard::A) {
            engine.ToggleAutoAlign();
        }
        else if ((event.key.code == sf::Keyboard::Delete || event.key.code == sf::Keyboard::BackSpace) && !m_selectedAnimation.empty()) {
            // Clears animation frame timeline
            engine.ModifyAnimationFrames(m_selectedAnimation, {});
        }
    } 
    else if (event.type == sf::Event::MouseButtonPressed) {
        sf::Vector2f mousePos(event.mouseButton.x, event.mouseButton.y);

        if (event.mouseButton.button == sf::Mouse::Left) {
            // --- PLAYBACK BUTTON CLICKS (Inside timeline area) ---
            if (m_playBtnArea.contains(mousePos)) {
                engine.GetPlaybackEngine().Play();
                return;
            } else if (m_pauseBtnArea.contains(mousePos)) {
                engine.GetPlaybackEngine().Pause();
                return;
            } else if (m_stopBtnArea.contains(mousePos)) {
                engine.GetPlaybackEngine().Stop();
                return;
            } 
            // --- FPS CONTROL CLICKS ---
            else if (!m_selectedAnimation.empty()) {
                auto anim = engine.GetCurrentProject()->GetAnimationById(m_selectedAnimation);
                if (anim) {
                    if (m_fpsMinusBtnArea.contains(mousePos)) {
                        float newFps = std::max(1.0f, anim->GetFPS() - 1.0f);
                        engine.EditAnimationSettings(m_selectedAnimation, anim->GetName(), newFps, anim->IsLooping());
                        return;
                    } else if (m_fpsPlusBtnArea.contains(mousePos)) {
                        float newFps = std::min(60.0f, anim->GetFPS() + 1.0f);
                        engine.EditAnimationSettings(m_selectedAnimation, anim->GetName(), newFps, anim->IsLooping());
                        return;
                    }
                }
            }

            // --- ANIMATION LIST SELECTION ---
            if (m_listArea.contains(mousePos)) {
                float yOffset = StudioUI::Theme::HeaderHeight + 10.f;
                auto project = engine.GetCurrentProject();
                for (const auto& anim : project->GetAnimationGroups()) {
                    sf::FloatRect itemRect(m_listArea.left + 10.f, m_listArea.top + yOffset, m_listArea.width - 20.f, 25.f);
                    if (itemRect.contains(mousePos)) {
                        m_selectedAnimation = anim->GetId();
                        engine.GetPlaybackEngine().SetActiveAnimation(m_selectedAnimation);
                        break;
                    }
                    yOffset += 30.f;
                }
            } 
            // --- TIMELINE APPEND / DRAG ---
            else if (m_timelineArea.contains(mousePos) && !m_selectedAnimation.empty()) {
                auto anim = engine.GetCurrentProject()->GetAnimationById(m_selectedAnimation);
                if (anim) {
                    if (!selectedSprites.empty()) {
                        // Append sprites from workspace directly to timeline
                        auto frames = anim->GetFrames();
                        frames.insert(frames.end(), selectedSprites.begin(), selectedSprites.end());
                        engine.ModifyAnimationFrames(m_selectedAnimation, frames);
                        viewport.ClearSelection();
                    } else {
                        // Start drag operations
                        float timelineStartX = m_timelineArea.left + 270.0f; // 250px panel + 20px pad
                        float frameWidth = 32.0f;
                        int index = 0;
                        for (const auto& frame : anim->GetFrames()) {
                            sf::FloatRect frameRect(timelineStartX + (index * frameWidth), m_timelineArea.top + StudioUI::Theme::HeaderHeight + 30.f, frameWidth, 40.f);
                            if (frameRect.contains(mousePos)) {
                                m_draggedFrameIndex = index;
                                m_dropTargetIndex = index;
                                break;
                            }
                            index++;
                        }
                    }
                }
            }
        } 
        // Right Click on Frame -> Delete Frame
        else if (event.mouseButton.button == sf::Mouse::Right && m_timelineArea.contains(mousePos) && !m_selectedAnimation.empty()) {
            auto anim = engine.GetCurrentProject()->GetAnimationById(m_selectedAnimation);
            if (anim) {
                float timelineStartX = m_timelineArea.left + 270.0f;
                float frameWidth = 32.0f;
                int index = 0;
                auto frames = anim->GetFrames();
                for (auto it = frames.begin(); it != frames.end(); ++it) {
                    sf::FloatRect frameRect(timelineStartX + (index * frameWidth), m_timelineArea.top + StudioUI::Theme::HeaderHeight + 30.f, frameWidth, 40.f);
                    if (frameRect.contains(mousePos)) {
                        frames.erase(it);
                        engine.ModifyAnimationFrames(m_selectedAnimation, frames);
                        break;
                    }
                    index++;
                }
            }
        }
    } 
    else if (event.type == sf::Event::MouseMoved && m_draggedFrameIndex != -1) {
        sf::Vector2f mousePos(event.mouseMove.x, event.mouseMove.y);
        if (m_timelineArea.contains(mousePos)) {
            float timelineStartX = m_timelineArea.left + 270.0f;
            float relX = mousePos.x - timelineStartX;
            m_dropTargetIndex = std::max(0, static_cast<int>(std::round(relX / 32.f)));
        }
    }
    else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
        if (m_draggedFrameIndex != -1 && !m_selectedAnimation.empty()) {
            auto anim = engine.GetCurrentProject()->GetAnimationById(m_selectedAnimation);
            if (anim) {
                auto frames = anim->GetFrames();
                int maxIdx = static_cast<int>(frames.size()) - 1;
                int target = std::max(0, std::min(m_dropTargetIndex, maxIdx));

                if (target != m_draggedFrameIndex && target >= 0 && target <= maxIdx) {
                    std::string id = frames[m_draggedFrameIndex];
                    frames.erase(frames.begin() + m_draggedFrameIndex);
                    frames.insert(frames.begin() + target, id);
                    engine.ModifyAnimationFrames(m_selectedAnimation, frames);
                }
            }
        }
        m_draggedFrameIndex = -1;
        m_dropTargetIndex = -1;
    }
}

void AnimationPanel::Render(sf::RenderWindow& window, const StudioCore::StudioEngineFacade& engine) {
    if (!engine.IsProjectActive() || !m_hasFont) return;
    
    window.setView(window.getDefaultView());

    float timelineHeight = 160.0f;
    float rightPanelWidth = 300.0f;
    float topOffset = StudioUI::Theme::ToolbarHeight;
    
    m_timelineArea = sf::FloatRect(m_bounds.left, m_bounds.top + m_bounds.height - timelineHeight, m_bounds.width, timelineHeight);
    m_listArea = sf::FloatRect(m_bounds.left + m_bounds.width - rightPanelWidth, m_bounds.top + topOffset, rightPanelWidth, (m_bounds.height - topOffset - timelineHeight) / 2.0f);
    m_previewArea = sf::FloatRect(m_bounds.left + m_bounds.width - rightPanelWidth, m_listArea.top + m_listArea.height, rightPanelWidth, m_listArea.height);

    RenderList(window, engine);
    RenderPreview(window, engine);
    RenderTimeline(window, engine);
}

void AnimationPanel::RenderList(sf::RenderWindow& window, const StudioCore::StudioEngineFacade& engine) {
    sf::RectangleShape bg(sf::Vector2f(m_listArea.width, m_listArea.height));
    bg.setPosition(m_listArea.left, m_listArea.top);
    bg.setFillColor(StudioUI::Theme::PanelBackground);
    bg.setOutlineThickness(StudioUI::Theme::BorderThickness);
    bg.setOutlineColor(StudioUI::Theme::BorderColor);
    window.draw(bg);

    sf::RectangleShape header(sf::Vector2f(m_listArea.width, StudioUI::Theme::HeaderHeight));
    header.setPosition(m_listArea.left, m_listArea.top);
    header.setFillColor(StudioUI::Theme::InspectorBackground);
    window.draw(header);

    sf::Text title("ANIMATIONS (Shift+N: New)", m_font, 11);
    title.setPosition(m_listArea.left + 12.f, m_listArea.top + 6.f);
    title.setFillColor(StudioUI::Theme::TextSecondary);
    window.draw(title);

    float yOffset = StudioUI::Theme::HeaderHeight + 10.f;
    auto project = engine.GetCurrentProject();
    int animCount = 0;
    
    for (const auto& anim : project->GetAnimationGroups()) {
        sf::RectangleShape item(sf::Vector2f(m_listArea.width - 20.f, 25.f));
        item.setPosition(m_listArea.left + 10.f, m_listArea.top + yOffset);
        item.setFillColor(anim->GetId() == m_selectedAnimation ? StudioUI::Theme::AccentColor : StudioUI::Theme::HoverColor);
        window.draw(item);

        sf::Text name(anim->GetName(), m_font, 12);
        name.setPosition(item.getPosition().x + 8.f, item.getPosition().y + 5.f);
        name.setFillColor(StudioUI::Theme::TextPrimary);
        window.draw(name);
        
        yOffset += 30.f;
        animCount++;
    }

    sf::Text countText("Total Animations: " + std::to_string(animCount), m_font, 11);
    countText.setPosition(m_listArea.left + 10.f, m_listArea.top + m_listArea.height - 20.f);
    countText.setFillColor(StudioUI::Theme::TextMuted);
    window.draw(countText);
}

void AnimationPanel::RenderPreview(sf::RenderWindow& window, const StudioCore::StudioEngineFacade& engine) {
    sf::RectangleShape bg(sf::Vector2f(m_previewArea.width, m_previewArea.height));
    bg.setPosition(m_previewArea.left, m_previewArea.top);
    bg.setFillColor(StudioUI::Theme::PanelBackground);
    bg.setOutlineThickness(StudioUI::Theme::BorderThickness);
    bg.setOutlineColor(StudioUI::Theme::BorderColor);
    window.draw(bg);

    sf::RectangleShape header(sf::Vector2f(m_previewArea.width, StudioUI::Theme::HeaderHeight));
    header.setPosition(m_previewArea.left, m_previewArea.top);
    header.setFillColor(StudioUI::Theme::InspectorBackground);
    window.draw(header);

    bool autoAlign = engine.IsAutoAlignEnabled();
    std::string titleStr = "PREVIEW [Align: " + std::string(autoAlign ? "ON" : "OFF") + "] (Key: A)";
    sf::Text title(titleStr, m_font, 11);
    title.setPosition(m_previewArea.left + 12.f, m_previewArea.top + 6.f);
    title.setFillColor(autoAlign ? StudioUI::Theme::AccentColor : StudioUI::Theme::TextSecondary);
    window.draw(title);

    // --- SPRITE RENDER & GIZMOS ---
    auto spriteDef = engine.GetPlaybackEngine().GetCurrentSprite(engine.GetCurrentProject().get());
    auto texture = engine.GetCurrentTexture();
    
    if (spriteDef && texture) {
        sf::Texture gpuTex;
        gpuTex.create(texture->GetWidth(), texture->GetHeight());
        gpuTex.update(texture->GetPixels().data());
        
        sf::Sprite renderSprite;
        renderSprite.setTexture(gpuTex);
        const auto& r = spriteDef->GetSourceRect();
        renderSprite.setTextureRect(sf::IntRect(r.x, r.y, r.width, r.height));
        
        sf::Vector2f activeAnchor;
        float activeBaseline = 0.0f;
        
        if (autoAlign) {
            auto alignment = engine.GetPlaybackEngine().GetCurrentAlignment(engine.GetCurrentProject().get());
            activeAnchor.x = r.width * alignment.alignedPivot.x;
            activeAnchor.y = r.height * alignment.alignedPivot.y;
            activeBaseline = alignment.alignedBaseline;
        } else {
            activeAnchor.x = r.width * spriteDef->GetPivot().x;
            activeAnchor.y = r.height * spriteDef->GetPivot().y;
            activeBaseline = spriteDef->GetBaseline();
        }

        renderSprite.setOrigin(activeAnchor.x, activeAnchor.y);
        float scale = std::min((m_previewArea.width - 40.f) / r.width, (m_previewArea.height - 100.f) / r.height);
        renderSprite.setScale(scale, scale);
        
        sf::Vector2f previewCenter(m_previewArea.left + m_previewArea.width / 2.f, m_previewArea.top + 30.f + m_previewArea.height / 2.f);
        renderSprite.setPosition(previewCenter.x, previewCenter.y + (activeBaseline * scale));
        
        window.draw(renderSprite);

        // Gizmo Visualization
        sf::Vector2f activePos = renderSprite.getPosition();
        sf::Vector2f activeOrigin = renderSprite.getOrigin();

        sf::Vector2f tl;
        tl.x = activePos.x - (activeOrigin.x * scale);
        tl.y = activePos.y - (activeOrigin.y * scale);

        sf::RectangleShape activeBaselineLine(sf::Vector2f(m_previewArea.width, 1.f));
        activeBaselineLine.setPosition(m_previewArea.left, tl.y + (r.height - activeBaseline) * scale);
        activeBaselineLine.setFillColor(autoAlign ? sf::Color(0, 255, 255, 200) : sf::Color(255, 165, 0, 200));
        window.draw(activeBaselineLine);

        sf::CircleShape activePivotDot(4.f);
        activePivotDot.setOrigin(4.f, 4.f);
        activePivotDot.setPosition(activePos);
        activePivotDot.setFillColor(autoAlign ? sf::Color::Magenta : sf::Color::Green);
        window.draw(activePivotDot);
    }
}

void AnimationPanel::RenderTimeline(sf::RenderWindow& window, const StudioCore::StudioEngineFacade& engine) {
    float posY = m_timelineArea.top;
    float panelHeight = m_timelineArea.height;
    
    // 1. Background
    sf::RectangleShape bg(sf::Vector2f(m_timelineArea.width, panelHeight));
    bg.setPosition(m_timelineArea.left, posY);
    bg.setFillColor(StudioUI::Theme::PanelBackground);
    bg.setOutlineThickness(StudioUI::Theme::BorderThickness);
    bg.setOutlineColor(StudioUI::Theme::BorderColor);
    window.draw(bg);

    // 2. Header Bar
    sf::RectangleShape header(sf::Vector2f(m_timelineArea.width, StudioUI::Theme::HeaderHeight));
    header.setPosition(m_timelineArea.left, posY);
    header.setFillColor(StudioUI::Theme::InspectorBackground);
    window.draw(header);

    sf::Text headerText("ANIMATION TIMELINE (Drag to reorder, Right-Click frame to delete, Del/Backspace to clear all)", m_font, 11);
    headerText.setFillColor(StudioUI::Theme::TextSecondary);
    headerText.setPosition(m_timelineArea.left + 12.0f, posY + 6.0f);
    window.draw(headerText);

    // 3. Left Controls Panel Divider
    float leftPanelWidth = 250.0f;
    sf::RectangleShape leftBorder(sf::Vector2f(StudioUI::Theme::BorderThickness, panelHeight - StudioUI::Theme::HeaderHeight));
    leftBorder.setPosition(m_timelineArea.left + leftPanelWidth, posY + StudioUI::Theme::HeaderHeight);
    leftBorder.setFillColor(StudioUI::Theme::BorderColor);
    window.draw(leftBorder);

    // Data Binding for Playback
    bool isPlaying = engine.GetPlaybackEngine().IsPlaying();
    float currentFps = 12.0f;
    int frameCount = 0;
    
    if (!m_selectedAnimation.empty()) {
        auto anim = engine.GetCurrentProject()->GetAnimationById(m_selectedAnimation);
        if (anim) {
            currentFps = anim->GetFPS();
            frameCount = anim->GetFrames().size();
        }
    }
    int currentFrame = engine.GetPlaybackEngine().GetCurrentFrameIndex();
    float ctrlY = posY + StudioUI::Theme::HeaderHeight + 20.0f;

    // Interactive Text Controls mapping to hitboxes
    m_playBtnArea = sf::FloatRect(m_timelineArea.left + 20.f, ctrlY, 30.f, 20.f);
    sf::Text playTxt("Play", m_font, 12);
    playTxt.setPosition(m_playBtnArea.left, m_playBtnArea.top);
    playTxt.setFillColor(isPlaying ? StudioUI::Theme::AccentColor : StudioUI::Theme::TextPrimary);
    window.draw(playTxt);

    m_pauseBtnArea = sf::FloatRect(m_timelineArea.left + 65.f, ctrlY, 40.f, 20.f);
    sf::Text pauseTxt("Pause", m_font, 12);
    pauseTxt.setPosition(m_pauseBtnArea.left, m_pauseBtnArea.top);
    pauseTxt.setFillColor(!isPlaying ? StudioUI::Theme::AccentHoverColor : StudioUI::Theme::TextPrimary);
    window.draw(pauseTxt);

    m_stopBtnArea = sf::FloatRect(m_timelineArea.left + 120.f, ctrlY, 35.f, 20.f);
    sf::Text stopTxt("Stop", m_font, 12);
    stopTxt.setPosition(m_stopBtnArea.left, m_stopBtnArea.top);
    stopTxt.setFillColor(sf::Color(235, 87, 87));
    window.draw(stopTxt);

    // FPS Toggles
    m_fpsMinusBtnArea = sf::FloatRect(m_timelineArea.left + 20.f, ctrlY + 30.f, 15.f, 20.f);
    sf::Text minusTxt("-", m_font, 14);
    minusTxt.setPosition(m_fpsMinusBtnArea.left, m_fpsMinusBtnArea.top);
    window.draw(minusTxt);

    sf::Text fpsTxt("FPS: " + std::to_string(static_cast<int>(currentFps)), m_font, 12);
    fpsTxt.setPosition(m_timelineArea.left + 40.f, ctrlY + 32.f);
    fpsTxt.setFillColor(StudioUI::Theme::TextPrimary);
    window.draw(fpsTxt);

    m_fpsPlusBtnArea = sf::FloatRect(m_timelineArea.left + 100.f, ctrlY + 30.f, 15.f, 20.f);
    sf::Text plusTxt("+", m_font, 14);
    plusTxt.setPosition(m_fpsPlusBtnArea.left, m_fpsPlusBtnArea.top);
    window.draw(plusTxt);

    sf::Text frameInfoTxt("Frame: " + std::to_string(currentFrame + 1) + " / " + std::to_string(std::max(1, frameCount)), m_font, 12);
    frameInfoTxt.setFillColor(StudioUI::Theme::TextPrimary);
    frameInfoTxt.setPosition(m_timelineArea.left + 20.f, ctrlY + 60.f);
    window.draw(frameInfoTxt);

    // 4. Structured Frame Tracks mapping directly to anim logic
    if (m_selectedAnimation.empty()) return;
    auto anim = engine.GetCurrentProject()->GetAnimationById(m_selectedAnimation);
    if (!anim) return;

    float timelineStartX = m_timelineArea.left + leftPanelWidth + 20.0f;
    float timelineY = posY + StudioUI::Theme::HeaderHeight + 30.0f;
    float frameWidth = 32.0f;
    float frameHeight = 40.0f;
    
    float xOffset = timelineStartX;
    int index = 0;

    for (const auto& frameId : anim->GetFrames()) {
        sf::RectangleShape frameBox(sf::Vector2f(frameWidth - 2.0f, frameHeight));
        frameBox.setPosition(xOffset, timelineY);
        frameBox.setFillColor(index % 2 == 0 ? sf::Color(45, 48, 54) : sf::Color(40, 42, 48));
        
        if (index == currentFrame) {
            frameBox.setOutlineThickness(1.5f);
            frameBox.setOutlineColor(StudioUI::Theme::AccentColor);
            frameBox.setFillColor(StudioUI::Theme::HoverColor);
        } else if (index == m_draggedFrameIndex) {
            frameBox.setOutlineThickness(2.f);
            frameBox.setOutlineColor(sf::Color::Yellow);
        }
        window.draw(frameBox);

        sf::Text tickText(std::to_string(index), m_font, 10);
        tickText.setFillColor(StudioUI::Theme::TextMuted);
        tickText.setPosition(xOffset + 2.0f, timelineY - 18.0f);
        window.draw(tickText);

        if (m_draggedFrameIndex != -1 && index == m_dropTargetIndex) {
            sf::RectangleShape dropMarker(sf::Vector2f(4.f, frameHeight + 20.f));
            dropMarker.setPosition(xOffset - 2.f, timelineY - 10.f);
            dropMarker.setFillColor(sf::Color::Cyan);
            window.draw(dropMarker);
        }

        xOffset += frameWidth;
        index++;
    }

    // 5. Playhead
    if (frameCount > 0 && currentFrame >= 0 && currentFrame < frameCount) {
        float playheadX = timelineStartX + (currentFrame * frameWidth) + (frameWidth / 2.0f);
        sf::RectangleShape playheadLine(sf::Vector2f(2.0f, frameHeight + 25.0f));
        playheadLine.setPosition(playheadX, timelineY - 15.0f);
        playheadLine.setFillColor(sf::Color(235, 87, 87));
        window.draw(playheadLine);
    }
}

}