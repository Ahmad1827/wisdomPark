#include "TopBar.h"
#include "../UITheme.h"
#include "../UIIcons.h"
#include <algorithm>
#include <cmath>

namespace WisdomUI {

    static sf::FloatRect s_projBadgeBounds;

    TopBar::TopBar() = default;

    void TopBar::Initialize(const sf::Font& font,
        std::function<void()> onNew,
        std::function<void()> onOpen,
        std::function<void()> onSave,
        std::function<void()> onExport,
        std::function<void()> onUndo,
        std::function<void()> onRedo,
        std::function<void()> onToggleFullscreen,
        std::function<void()> onExit) {
        m_font = font;

        m_menus.clear();
        m_menus.push_back({ "File", {}, {
            {"New Project (Ctrl+N)", onNew, 0.0f, {}},
            {"Open Project... (Ctrl+O)", onOpen, 0.0f, {}},
            {"Save Project (Ctrl+S)", onSave, 0.0f, {}},
            {"Export PNG / Sheet...", onExport, 0.0f, {}},
            {"Exit to Menu", onExit, 0.0f, {}}
        }, 0.0f, false, 240.0f });

        m_menus.push_back({ "Edit", {}, {
            {"Undo (Ctrl+Z)", onUndo, 0.0f, {}},
            {"Redo (Ctrl+Y)", onRedo, 0.0f, {}}
        }, 0.0f, false, 200.0f });

        m_menus.push_back({ "View", {}, {
            {"Toggle Fullscreen (F11)", onToggleFullscreen, 0.0f, {}}
        }, 0.0f, false, 220.0f });

        m_quickBtns.clear();
        m_quickBtns.push_back({ "undo", "Undo (Ctrl+Z)", {}, onUndo, 0.0f, 1.0f });
        m_quickBtns.push_back({ "redo", "Redo (Ctrl+Y)", {}, onRedo, 0.0f, 1.0f });
        m_quickBtns.push_back({ "save", "Save Project (Ctrl+S)", {}, onSave, 0.0f, 1.0f });
        m_quickBtns.push_back({ "fullscreen", "Toggle Fullscreen", {}, onToggleFullscreen, 0.0f, 1.0f });
    }

    void TopBar::SetProjectName(const std::string& name, bool isDirty) {
        m_projectName = name;
        m_isDirty = isDirty;
        SetBounds(m_bounds);
    }

    sf::FloatRect TopBar::getDropdownPanelBounds(int menuIndex) const {
        if (menuIndex < 0 || menuIndex >= static_cast<int>(m_menus.size())) return {};
        const auto& menu = m_menus[menuIndex];
        float itemH = 33.0f;
        float totalH = menu.actions.size() * itemH + 12.0f;
        float currentH = totalH * menu.openProgress;
        return sf::FloatRect(std::floor(menu.bounds.left), std::floor(m_bounds.top + m_bounds.height + 3.0f), menu.dropdownWidth, currentH);
    }

    sf::FloatRect TopBar::getDropdownItemBounds(int menuIndex, int actionIndex) const {
        if (menuIndex < 0 || menuIndex >= static_cast<int>(m_menus.size())) return {};
        const auto& menu = m_menus[menuIndex];
        if (actionIndex < 0 || actionIndex >= static_cast<int>(menu.actions.size())) return {};

        float dropX = std::floor(menu.bounds.left);
        float dropY = std::floor(m_bounds.top + m_bounds.height + 3.0f);
        float itemH = 33.0f;
        float itemY = dropY + 6.0f + actionIndex * itemH;
        float itemW = menu.dropdownWidth - 12.0f;

        return sf::FloatRect(dropX + 6.0f, itemY, itemW, itemH);
    }

    void TopBar::SetBounds(const sf::FloatRect& bounds) {
        m_bounds = bounds;

        float barH = bounds.height;
        float btnH = 30.0f;
        float btnY = std::floor(bounds.top + (barH - btnH) * 0.5f);

        sf::Text logoMeasure("WISDOM PARK", m_font, 14);
        float logoW = logoMeasure.getLocalBounds().width;
        float badgeStartX = std::floor(bounds.left + 34.0f + logoW + 18.0f);

        sf::Text projMeasure(m_projectName + (m_isDirty ? " *" : ""), m_font, 13);
        float projW = projMeasure.getLocalBounds().width;
        float projBadgeW = std::clamp(projW + 28.0f, 100.0f, 360.0f);

        s_projBadgeBounds = sf::FloatRect(badgeStartX, btnY, projBadgeW, btnH);

        float curX = s_projBadgeBounds.left + s_projBadgeBounds.width + 14.0f;

        for (auto& menu : m_menus) {
            sf::Text t(menu.title, m_font, 13);
            float w = t.getLocalBounds().width + 24.0f;
            menu.bounds = sf::FloatRect(std::floor(curX), btnY, std::floor(w), btnH);
            curX += w + 6.0f;
        }

        curX += 6.0f;

        m_pushGitImgBtnBounds = sf::FloatRect(std::floor(curX), btnY, 92.0f, btnH);
        curX += 92.0f + 6.0f;

        m_pushSheetBtnBounds = sf::FloatRect(std::floor(curX), btnY, 92.0f, btnH);
        curX += 92.0f + 6.0f;

        m_pullBtnBounds = sf::FloatRect(std::floor(curX), btnY, 70.0f, btnH);
        curX += 70.0f + 8.0f;

        m_opaqueCheckboxBounds = sf::FloatRect(std::floor(curX), btnY, 86.0f, btnH);
        curX += 86.0f + 8.0f;

        m_trackerBtnBounds = sf::FloatRect(std::floor(curX), btnY, 86.0f, btnH);
        curX += 86.0f + 8.0f;

        float btnSize = 30.0f;
        float totalQuickW = m_quickBtns.size() * (btnSize + 5.0f) - 5.0f;
        float qX = bounds.left + bounds.width - totalQuickW - 14.0f;

        for (auto& qb : m_quickBtns) {
            qb.bounds = sf::FloatRect(std::floor(qX), btnY, btnSize, btnSize);
            qX += btnSize + 5.0f;
        }

        float rightAnchor = m_quickBtns.empty() ? (bounds.left + bounds.width - 14.0f) : (m_quickBtns.front().bounds.left - 12.0f);

    

        if (m_gridControlsVisible) {
            float totalGridW = 90.f + 5.f + 30.f + 5.f + 30.f + 5.f + 64.f + 5.f + 30.f;
            rightAnchor -= totalGridW + 6.0f;
            float startX = std::floor(rightAnchor);

            m_gridToggleBtnBounds = sf::FloatRect(startX, btnY, 90.0f, btnH);
            m_gridColorBtnBounds = sf::FloatRect(startX + 95.0f, btnY, 30.0f, btnH);
            m_gridMinusBtnBounds = sf::FloatRect(startX + 130.0f, btnY, 30.0f, btnH);
            m_gridSizeBox = sf::FloatRect(startX + 165.0f, btnY, 64.0f, btnH);
            m_gridPlusBtnBounds = sf::FloatRect(startX + 234.0f, btnY, 30.0f, btnH);
        }
        else {
            m_gridToggleBtnBounds = sf::FloatRect(0.f, 0.f, 0.f, 0.f);
            m_gridColorBtnBounds = sf::FloatRect(0.f, 0.f, 0.f, 0.f);
            m_gridMinusBtnBounds = sf::FloatRect(0.f, 0.f, 0.f, 0.f);
            m_gridSizeBox = sf::FloatRect(0.f, 0.f, 0.f, 0.f);
            m_gridPlusBtnBounds = sf::FloatRect(0.f, 0.f, 0.f, 0.f);
        }
    }

    void TopBar::Update(float deltaTime, const sf::Vector2f& mousePos) {
        m_globalTime += deltaTime;
        m_shimmerOffset += deltaTime * 120.0f;
        if (m_shimmerOffset > m_bounds.width + 400.0f) {
            m_shimmerOffset = -200.0f;
        }

        for (size_t i = 0; i < m_menus.size(); ++i) {
            auto& menu = m_menus[i];
            bool isTargetOpen = (m_openMenuIndex == static_cast<int>(i));
            float target = isTargetOpen ? 1.0f : 0.0f;
            menu.openProgress += (target - menu.openProgress) * 18.0f * deltaTime;
            menu.openProgress = std::clamp(menu.openProgress, 0.0f, 1.0f);

            if (isTargetOpen && menu.openProgress > 0.1f) {
                for (size_t j = 0; j < menu.actions.size(); ++j) {
                    sf::FloatRect actBounds = getDropdownItemBounds(static_cast<int>(i), static_cast<int>(j));
                    bool actHover = actBounds.contains(mousePos);
                    menu.actions[j].hoverAlpha += ((actHover ? 1.0f : 0.0f) - menu.actions[j].hoverAlpha) * 20.0f * deltaTime;
                }
            }
        }

        for (auto& qb : m_quickBtns) {
            bool hover = qb.bounds.contains(mousePos);
            qb.hoverAlpha += ((hover ? 1.0f : 0.0f) - qb.hoverAlpha) * 14.0f * deltaTime;
            qb.scale += ((hover ? 1.10f : 1.0f) - qb.scale) * 16.0f * deltaTime;
        }

        float pullTarget = m_isPullOpen ? 1.0f : 0.0f;
        m_pullOpenProgress += (pullTarget - m_pullOpenProgress) * 18.0f * deltaTime;
        m_pullOpenProgress = std::clamp(m_pullOpenProgress, 0.0f, 1.0f);
    }

    void TopBar::renderPullDropdown(sf::RenderWindow& window, sf::Vector2f mousePos) {
        if (m_pullOpenProgress < 0.02f) return;

        float dropW = 350.0f;
        float totalH = 380.0f;
        float currentH = totalH * m_pullOpenProgress;
        float dropX = std::floor(m_pullBtnBounds.left);
        float dropY = std::floor(m_bounds.top + m_bounds.height + 3.0f);

        m_pullDropdownBounds = sf::FloatRect(dropX, dropY, dropW, currentH);

        Theme::DrawSunsetPanel(window, m_pullDropdownBounds, m_pullOpenProgress);

        if (m_pullOpenProgress < 0.4f) return;

        float headerH = 28.0f;
        sf::RectangleShape header(sf::Vector2f(dropW - 12.0f, headerH));
        header.setPosition(dropX + 6.0f, dropY + 6.0f);
        header.setFillColor(Theme::SunsetDeepDark);
        window.draw(header);

        Theme::DrawCrispText(window, m_font, "VERSION HISTORY", 12, dropX + 14.0f, dropY + 14.0f, Theme::SunsetAmber, sf::Color::Transparent, false, true);

        float listY = dropY + headerH + 8.0f;
        float listH = currentH - headerH - 16.0f;
        if (listH <= 20.0f) return;

        float rowH = 48.0f;
        float totalContentH = static_cast<float>(m_commits.size()) * rowH;
        m_pullMaxScroll = std::max(0.0f, totalContentH - listH);
        m_pullScrollOffset = std::clamp(m_pullScrollOffset, 0.0f, m_pullMaxScroll);

        m_commitRowBounds.clear();

        if (m_commits.empty()) {
            Theme::DrawCrispText(window, m_font, "No commits found in repository.", 13, dropX + dropW / 2.0f, dropY + currentH / 2.0f, Theme::SunsetPlum, sf::Color::Transparent, true, true);
            return;
        }

        sf::View savedView = window.getView();
        sf::FloatRect letterboxVp = savedView.getViewport();

        float normX = (dropX + 6.0f) / 1920.0f;
        float normY = listY / 1080.0f;
        float normW = (dropW - 12.0f) / 1920.0f;
        float normH = listH / 1080.0f;

        sf::FloatRect subVp(
            letterboxVp.left + normX * letterboxVp.width,
            letterboxVp.top + normY * letterboxVp.height,
            normW * letterboxVp.width,
            normH * letterboxVp.height
        );

        sf::View clipView(sf::FloatRect(dropX + 6.0f, listY, dropW - 12.0f, listH));
        clipView.setViewport(subVp);
        window.setView(clipView);

        float curY = listY - m_pullScrollOffset;
        for (size_t i = 0; i < m_commits.size(); ++i) {
            sf::FloatRect rowRect(dropX + 6.0f, curY, dropW - 18.0f, rowH - 4.0f);

            if (curY + rowH >= listY && curY <= listY + listH) {
                bool isHov = rowRect.contains(mousePos) && m_pullDropdownBounds.contains(mousePos);

                sf::RectangleShape rowBg(sf::Vector2f(rowRect.width, rowRect.height));
                rowBg.setPosition(rowRect.left, rowRect.top);
                rowBg.setFillColor(isHov ? sf::Color(46, 30, 58, 230) : sf::Color(26, 18, 34, 180));
                rowBg.setOutlineThickness(1.0f);
                rowBg.setOutlineColor(isHov ? Theme::SunsetAmber : sf::Color(50, 36, 64));
                window.draw(rowBg);

                Theme::DrawCrispText(window, m_font, m_commits[i].shortHash, 12, rowRect.left + 8.0f, rowRect.top + 6.0f, Theme::SunsetGold);
                Theme::DrawCrispText(window, m_font, m_commits[i].dateFormatted, 12, rowRect.left + 76.0f, rowRect.top + 6.0f, Theme::SunsetPeach);

                std::string msg = m_commits[i].message;
                if (msg.length() > 34) {
                    msg = msg.substr(0, 32) + "..";
                }
                Theme::DrawCrispText(window, m_font, msg, 13, rowRect.left + 8.0f, rowRect.top + 24.0f, sf::Color::White);

                m_commitRowBounds.push_back({ rowRect, m_commits[i].hash });
            }

            curY += rowH;
        }

        window.setView(savedView);

        if (m_pullMaxScroll > 0.0f) {
            float trackX = dropX + dropW - 8.0f;
            sf::RectangleShape track(sf::Vector2f(3.0f, listH));
            track.setPosition(trackX, listY);
            track.setFillColor(sf::Color(10, 6, 14));
            window.draw(track);

            float thumbH = std::max(20.0f, (listH / totalContentH) * listH);
            float thumbY = listY + (m_pullScrollOffset / m_pullMaxScroll) * (listH - thumbH);
            sf::RectangleShape thumb(sf::Vector2f(3.0f, thumbH));
            thumb.setPosition(trackX, thumbY);
            thumb.setFillColor(Theme::SunsetGold);
            window.draw(thumb);
        }
    }

    bool TopBar::HandleEvent(const sf::Event& event, const sf::RenderWindow& window) {
        sf::Vector2f mousePos = window.mapPixelToCoords({ event.mouseButton.x, event.mouseButton.y });

        if (m_isPullOpen) {
            if (event.type == sf::Event::MouseWheelScrolled) {
                sf::Vector2f scrollPos = window.mapPixelToCoords({ event.mouseWheelScroll.x, event.mouseWheelScroll.y });
                if (m_pullDropdownBounds.contains(scrollPos)) {
                    m_pullScrollOffset = std::clamp(m_pullScrollOffset - event.mouseWheelScroll.delta * 32.0f, 0.0f, m_pullMaxScroll);
                    return true;
                }
            }

            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                if (m_pullDropdownBounds.contains(mousePos)) {
                    for (const auto& item : m_commitRowBounds) {
                        if (item.first.contains(mousePos)) {
                            std::string targetHash = item.second;
                            m_isPullOpen = false;
                            if (m_onCheckoutCommit) {
                                m_onCheckoutCommit(targetHash);
                            }
                            return true;
                        }
                    }
                    return true;
                }
                else if (!m_pullBtnBounds.contains(mousePos)) {
                    m_isPullOpen = false;
                }
            }
        }

        if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
            if (m_pullBtnBounds.contains(mousePos)) {
                m_isPullOpen = !m_isPullOpen;
                if (m_isPullOpen) {
                    m_commits = GitImgClient::getCommitHistory();
                    m_pullScrollOffset = 0.0f;
                    m_openMenuIndex = -1;
                }
                return true;
            }

            
            if (m_opaqueCheckboxBounds.contains(mousePos)) {
                m_opaqueBg = !m_opaqueBg;
                return true;
            }

            if (m_pushGitImgBtnBounds.contains(mousePos)) {
                if (m_onPushGitImg) m_onPushGitImg(m_opaqueBg);
                return true;
            }
            if (m_pushSheetBtnBounds.contains(mousePos)) {
                if (m_onPushSpriteSheet) m_onPushSpriteSheet(m_opaqueBg);
                return true;
            }
            if (m_trackerBtnBounds.contains(mousePos)) {
                m_trackerActive = !m_trackerActive;
                if (m_onToggleTracker) m_onToggleTracker(m_trackerActive);
                return true;
            }
            if (m_gridControlsVisible) {
                if (m_gridColorBtnBounds.contains(mousePos)) {
                    if (m_onPickGridColor) m_onPickGridColor();
                    return true;
                }

                if (m_gridToggleBtnBounds.contains(mousePos)) {
                    if (m_onToggleGrid) m_onToggleGrid(!m_gridActive);
                    return true;
                }
                if (m_gridMinusBtnBounds.contains(mousePos)) {
                    int nextSize = std::max(1, m_gridSize - 1);
                    if (m_onChangeGridSize) m_onChangeGridSize(nextSize);
                    return true;
                }
                if (m_gridPlusBtnBounds.contains(mousePos)) {
                    int nextSize = std::min(1000, m_gridSize + 1);
                    if (m_onChangeGridSize) m_onChangeGridSize(nextSize);
                    return true;
                }
                if (m_gridSizeBox.contains(mousePos)) {
                    m_isEditingGridSize = true;
                    m_gridSizeInput = std::to_string(m_gridSize);
                    return true;
                }
                else if (m_isEditingGridSize) {
                    if (!m_gridSizeInput.empty()) {
                        try {
                            int val = std::clamp(std::stoi(m_gridSizeInput), 1, 1000);
                            if (m_onChangeGridSize) m_onChangeGridSize(val);
                        }
                        catch (...) {}
                    }
                    m_isEditingGridSize = false;
                }
            }

            if (m_openMenuIndex != -1) {
                auto& openMenu = m_menus[m_openMenuIndex];
                for (size_t j = 0; j < openMenu.actions.size(); ++j) {
                    sf::FloatRect itemBounds = getDropdownItemBounds(m_openMenuIndex, static_cast<int>(j));
                    if (itemBounds.contains(mousePos)) {
                        auto cb = openMenu.actions[j].callback;
                        m_openMenuIndex = -1;
                        if (cb) cb();
                        return true;
                    }
                }
            }

            for (size_t i = 0; i < m_menus.size(); ++i) {
                if (m_menus[i].bounds.contains(mousePos)) {
                    m_openMenuIndex = (m_openMenuIndex == static_cast<int>(i)) ? -1 : static_cast<int>(i);
                    if (m_openMenuIndex != -1) m_isPullOpen = false;
                    return true;
                }
            }

            for (auto& qb : m_quickBtns) {
                if (qb.bounds.contains(mousePos)) {
                    auto cb = qb.onClick;
                    m_openMenuIndex = -1;
                    if (cb) cb();
                    return true;
                }
            }

            if (m_bounds.contains(mousePos)) {
                m_openMenuIndex = -1;
                return true;
            }

            m_openMenuIndex = -1;
        }

        if (m_gridControlsVisible && m_isEditingGridSize) {
            if (event.type == sf::Event::TextEntered) {
                if (event.text.unicode == '\b') {
                    if (!m_gridSizeInput.empty()) m_gridSizeInput.pop_back();
                    return true;
                }
                else if (event.text.unicode == '\r' || event.text.unicode == '\n') {
                    if (!m_gridSizeInput.empty()) {
                        try {
                            int val = std::clamp(std::stoi(m_gridSizeInput), 1, 1000);
                            if (m_onChangeGridSize) m_onChangeGridSize(val);
                        }
                        catch (...) {}
                    }
                    m_isEditingGridSize = false;
                    return true;
                }
                else if (event.text.unicode >= '0' && event.text.unicode <= '9') {
                    if (m_gridSizeInput.length() < 4) {
                        m_gridSizeInput += static_cast<char>(event.text.unicode);
                    }
                    return true;
                }
            }
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) {
                    m_isEditingGridSize = false;
                    return true;
                }
                if (event.key.code == sf::Keyboard::Enter) {
                    if (!m_gridSizeInput.empty()) {
                        try {
                            int val = std::clamp(std::stoi(m_gridSizeInput), 1, 1000);
                            if (m_onChangeGridSize) m_onChangeGridSize(val);
                        }
                        catch (...) {}
                    }
                    m_isEditingGridSize = false;
                    return true;
                }
            }
        }
        return false;
    }

    void TopBar::Render(sf::RenderWindow& window) {
        SetBounds(m_bounds);

        Theme::DrawSunsetPanel(window, m_bounds, 1.0f);

        float barH = m_bounds.height;
        float centerY = std::floor(m_bounds.top + barH * 0.5f);

        sf::RectangleShape crest(sf::Vector2f(11.0f, 11.0f));
        crest.setOrigin(5.5f, 5.5f);
        crest.setPosition(m_bounds.left + 18.0f, centerY);
        crest.setRotation(45.0f);
        crest.setFillColor(Theme::SunsetAmber);
        crest.setOutlineThickness(1.0f);
        crest.setOutlineColor(Theme::SunsetCoralDark);
        window.draw(crest);

        Theme::DrawCrispText(window, m_font, "WISDOM PARK", 14, m_bounds.left + 32.0f, centerY, Theme::SunsetAmber, sf::Color(14, 6, 20), false, true);

        sf::RectangleShape badge(sf::Vector2f(s_projBadgeBounds.width, s_projBadgeBounds.height));
        badge.setPosition(s_projBadgeBounds.left, s_projBadgeBounds.top);
        badge.setFillColor(Theme::SunsetDeepDark);
        badge.setOutlineThickness(1.0f);
        badge.setOutlineColor(m_isDirty ? Theme::SunsetCoral : Theme::SunsetPlum);
        window.draw(badge);

        Theme::DrawCrispText(window, m_font, m_projectName + (m_isDirty ? " *" : ""), 13, s_projBadgeBounds.left + s_projBadgeBounds.width / 2.0f, s_projBadgeBounds.top + s_projBadgeBounds.height / 2.0f, m_isDirty ? Theme::SunsetGold : Theme::TextSecondary, sf::Color::Transparent, true, true);

        sf::Vector2i mPosI = sf::Mouse::getPosition(window);
        sf::Vector2f mPos = window.mapPixelToCoords(mPosI);

        for (size_t i = 0; i < m_menus.size(); ++i) {
            const auto& menu = m_menus[i];
            bool isOpen = (m_openMenuIndex == static_cast<int>(i));
            bool hovered = menu.bounds.contains(mPos);

            Theme::DrawSunsetButton(window, menu.bounds, menu.title, m_font, 13, isOpen, hovered, false, 1.0f);
        }

        bool hovPush = m_pushGitImgBtnBounds.contains(mPos);
        Theme::DrawSunsetButton(window, m_pushGitImgBtnBounds, "Push Frame", m_font, 12, false, hovPush, false, 1.0f);

        bool hovSheet = m_pushSheetBtnBounds.contains(mPos);
        Theme::DrawSunsetButton(window, m_pushSheetBtnBounds, "Push Anim", m_font, 12, false, hovSheet, false, 1.0f);

        bool hovPull = m_pullBtnBounds.contains(mPos);
        Theme::DrawSunsetButton(window, m_pullBtnBounds, "Pull", m_font, 12, m_isPullOpen, hovPull, m_isPullOpen, 1.0f);

        float chkBoxSize = 15.0f;
        float chkBoxY = std::floor(m_opaqueCheckboxBounds.top + (m_opaqueCheckboxBounds.height - chkBoxSize) * 0.5f);
        sf::RectangleShape box(sf::Vector2f(chkBoxSize, chkBoxSize));
        box.setPosition(m_opaqueCheckboxBounds.left + 4.0f, chkBoxY);
        box.setFillColor(m_opaqueBg ? sf::Color(70, 130, 180) : sf::Color(30, 30, 35));
        box.setOutlineColor(sf::Color(120, 120, 130));
        box.setOutlineThickness(1.0f);
        window.draw(box);

        if (m_opaqueBg) {
            sf::RectangleShape check(sf::Vector2f(7.0f, 7.0f));
            check.setPosition(m_opaqueCheckboxBounds.left + 8.0f, chkBoxY + 4.0f);
            check.setFillColor(sf::Color::White);
            window.draw(check);
        }

        Theme::DrawCrispText(window, m_font, "Opaque", 12, m_opaqueCheckboxBounds.left + 24.0f, centerY, sf::Color(200, 200, 200), sf::Color::Transparent, false, true);

        bool hovTrk = m_trackerBtnBounds.contains(mPos);
        Theme::DrawSunsetButton(window, m_trackerBtnBounds, m_trackerActive ? "Hand ON" : "Hand OFF", m_font, 12, m_trackerActive, hovTrk, false, 1.0f);

        if (m_gridControlsVisible) {
            bool hovToggle = m_gridToggleBtnBounds.contains(mPos);
            bool hovColor = m_gridColorBtnBounds.contains(mPos);
            bool hovMinus = m_gridMinusBtnBounds.contains(mPos);
            bool hovBox = m_gridSizeBox.contains(mPos);
            bool hovPlus = m_gridPlusBtnBounds.contains(mPos);

            Theme::DrawSunsetButton(window, m_gridToggleBtnBounds, m_gridActive ? "Grid: ON" : "Grid: OFF", m_font, 12, m_gridActive, hovToggle, m_gridActive, 1.0f);

            sf::RectangleShape colorBtn(sf::Vector2f(m_gridColorBtnBounds.width, m_gridColorBtnBounds.height));
            colorBtn.setPosition(m_gridColorBtnBounds.left, m_gridColorBtnBounds.top);
            colorBtn.setFillColor(sf::Color(m_gridColor.r, m_gridColor.g, m_gridColor.b, 255));
            colorBtn.setOutlineThickness(hovColor ? 2.0f : 1.0f);
            colorBtn.setOutlineColor(hovColor ? Theme::SunsetGold : Theme::SunsetPlum);
            window.draw(colorBtn);

            Theme::DrawSunsetButton(window, m_gridMinusBtnBounds, "-", m_font, 15, false, hovMinus, false, 1.0f);

            sf::RectangleShape valBg(sf::Vector2f(m_gridSizeBox.width, m_gridSizeBox.height));
            valBg.setPosition(m_gridSizeBox.left, m_gridSizeBox.top);
            valBg.setFillColor(sf::Color(14, 6, 20));
            valBg.setOutlineThickness(m_isEditingGridSize ? 2.0f : 1.0f);
            valBg.setOutlineColor(m_isEditingGridSize ? Theme::SunsetGold : (hovBox ? Theme::SunsetAmber : Theme::SunsetPlum));
            window.draw(valBg);

            std::string displayTxt = m_isEditingGridSize ? (m_gridSizeInput + "_") : (std::to_string(m_gridSize) + "px");
            Theme::DrawCrispText(window, m_font, displayTxt, 12,
                m_gridSizeBox.left + m_gridSizeBox.width * 0.5f,
                m_gridSizeBox.top + m_gridSizeBox.height * 0.5f,
                m_isEditingGridSize ? Theme::SunsetGold : Theme::SunsetPeach, sf::Color::Transparent, true, true);

            Theme::DrawSunsetButton(window, m_gridPlusBtnBounds, "+", m_font, 15, false, hovPlus, false, 1.0f);
        }

        

        for (const auto& qb : m_quickBtns) {
            Theme::DrawSunsetButton(window, qb.bounds, "", m_font, 12, false, qb.hoverAlpha > 0.5f, false, qb.scale);

            sf::Vector2f iconPos(qb.bounds.left + 5.0f, qb.bounds.top + 5.0f);
            Icons::Draw(window, qb.id, iconPos, 20.0f, qb.hoverAlpha > 0.5f ? Theme::SunsetAmber : Theme::TextSecondary);
        }

        for (size_t i = 0; i < m_menus.size(); ++i) {
            const auto& menu = m_menus[i];
            if (menu.openProgress > 0.02f) {
                sf::FloatRect dropPanel = getDropdownPanelBounds(static_cast<int>(i));
                Theme::DrawSunsetPanel(window, dropPanel, menu.openProgress);

                if (menu.openProgress > 0.35f) {
                    for (size_t j = 0; j < menu.actions.size(); ++j) {
                        const auto& act = menu.actions[j];
                        sf::FloatRect itemRect = getDropdownItemBounds(static_cast<int>(i), static_cast<int>(j));

                        if (act.hoverAlpha > 0.01f) {
                            sf::RectangleShape hovBg(sf::Vector2f(itemRect.width, itemRect.height));
                            hovBg.setPosition(itemRect.left, itemRect.top);
                            sf::Color hCol = Theme::SunsetPlum;
                            hCol.a = static_cast<sf::Uint8>(240 * act.hoverAlpha * menu.openProgress);
                            hovBg.setFillColor(hCol);
                            hovBg.setOutlineThickness(1.0f);
                            sf::Color hBorder = Theme::SunsetCoral;
                            hBorder.a = static_cast<sf::Uint8>(255 * act.hoverAlpha * menu.openProgress);
                            hovBg.setOutlineColor(hBorder);
                            window.draw(hovBg);
                        }

                        sf::Color txtColor = (act.hoverAlpha > 0.5f) ? Theme::SunsetGold : Theme::TextPrimary;
                        Theme::DrawCrispText(window, m_font, act.label, 13, itemRect.left + 10.0f, itemRect.top + 7.0f, txtColor, sf::Color(14, 6, 20));
                    }
                }
            }
        }

        renderPullDropdown(window, mPos);
    }

}