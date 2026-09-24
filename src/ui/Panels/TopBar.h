#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <functional>
#include "../../core/GitImgClient.h"

namespace WisdomUI {

    struct TopMenuAction {
        std::string label;
        std::function<void()> callback;
        float hoverAlpha = 0.0f;
        sf::FloatRect bounds;
    };

    struct TopMenu {
        std::string title;
        sf::FloatRect bounds;
        std::vector<TopMenuAction> actions;
        float openProgress = 0.0f;
        bool isHovered = false;
        float dropdownWidth = 230.0f;
    };

    struct TopQuickButton {
        std::string id;
        std::string tooltip;
        sf::FloatRect bounds;
        std::function<void()> onClick;
        float hoverAlpha = 0.0f;
        float scale = 1.0f;
    };

    class TopBar {
    private:
        sf::FloatRect m_bounds;
        sf::Font m_font;
        std::string m_projectName = "Untitled";
        bool m_isDirty = false;

        std::vector<TopMenu> m_menus;
        std::vector<TopQuickButton> m_quickBtns;
        int m_openMenuIndex = -1;

        float m_globalTime = 0.0f;
        float m_shimmerOffset = -200.0f;
        bool m_symmetryActive{ false };
        std::function<void()> m_onDisableSymmetry;
        sf::FloatRect m_symmetryBtnBounds;
        bool m_gridControlsVisible{ false };
        bool m_gridActive{ false };
        int m_gridSize{ 16 };
        bool m_isEditingGridSize{ false };
        std::string m_gridSizeInput;
        std::function<void(bool)> m_onToggleGrid;
        std::function<void(int)> m_onChangeGridSize;
        sf::Color m_gridColor{ sf::Color(70, 130, 210, 180) };
        std::function<void()> m_onPickGridColor;
        sf::FloatRect m_gridToggleBtnBounds;
        sf::FloatRect m_gridColorBtnBounds;
        sf::FloatRect m_gridMinusBtnBounds;
        sf::FloatRect m_gridSizeBox;
        sf::FloatRect m_gridPlusBtnBounds;
        std::function<void(bool)> m_onPushGitImg;
        std::function<void(bool)> m_onPushSpriteSheet;
        std::function<void(bool)> m_onToggleTracker;
        sf::FloatRect m_pushGitImgBtnBounds;
        sf::FloatRect m_pushSheetBtnBounds;
        sf::FloatRect m_opaqueCheckboxBounds;
        sf::FloatRect m_trackerBtnBounds;
        bool m_opaqueBg{ true };
        bool m_trackerActive{ false };

        sf::FloatRect m_pullBtnBounds;
        sf::FloatRect m_pullDropdownBounds;
        bool m_isPullOpen{ false };
        float m_pullOpenProgress{ 0.0f };
        float m_pullScrollOffset{ 0.0f };
        float m_pullMaxScroll{ 0.0f };
        std::vector<GitImgCommit> m_commits;
        std::vector<std::pair<sf::FloatRect, std::string>> m_commitRowBounds;
        std::unordered_map<std::string, sf::Texture> m_commitThumbnails;
        std::unordered_map<std::string, bool> m_thumbnailRequested;
        std::mutex m_thumbMutex;
        std::vector<std::pair<std::string, sf::Image>> m_pendingThumbnails;
        std::function<void(const std::string&)> m_onCheckoutCommit;

        sf::FloatRect getDropdownItemBounds(int menuIndex, int actionIndex) const;
        sf::FloatRect getDropdownPanelBounds(int menuIndex) const;
        void renderPullDropdown(sf::RenderWindow& window, sf::Vector2f mousePos);

    public:
        TopBar();

        void Initialize(const sf::Font& font,
            std::function<void()> onNew,
            std::function<void()> onOpen,
            std::function<void()> onSave,
            std::function<void()> onExport,
            std::function<void()> onUndo,
            std::function<void()> onRedo,
            std::function<void()> onToggleFullscreen,
            std::function<void()> onExit);

        void SetProjectName(const std::string& name, bool isDirty);
        void SetSymmetryState(bool active, std::function<void()> onDisable) {
            m_symmetryActive = active;
            m_onDisableSymmetry = onDisable;
        }
        void SetPushGitImgCallback(std::function<void(bool)> onPush) {
            m_onPushGitImg = onPush;
        }
        void SetPushSpriteSheetCallback(std::function<void(bool)> onPushSheet) {
            m_onPushSpriteSheet = onPushSheet;
        }
        void SetToggleTrackerCallback(std::function<void(bool)> onToggleTracker) {
            m_onToggleTracker = onToggleTracker;
        }
        void SetCheckoutCommitCallback(std::function<void(const std::string&)> onCheckout) {
            m_onCheckoutCommit = onCheckout;
        }
        void SetGridControls(bool visible, bool active, int size, sf::Color color,
            std::function<void(bool)> onToggle,
            std::function<void(int)> onChangeSize,
            std::function<void()> onPickColor) {
            m_gridControlsVisible = visible;
            m_gridActive = active;
            m_gridSize = size;
            m_gridColor = color;
            m_onToggleGrid = onToggle;
            m_onChangeGridSize = onChangeSize;
            m_onPickGridColor = onPickColor;
        }
        void SetBounds(const sf::FloatRect& bounds);
        void Update(float deltaTime, const sf::Vector2f& mousePos);
        bool HandleEvent(const sf::Event& event, const sf::RenderWindow& window);
        void Render(sf::RenderWindow& window);
    };

}