#pragma once
#include <SFML/Graphics.hpp>

// Geometry for the start-menu sub-screens (settings, tutorials, credits).
// Drawing and click handling both read from here so they can never drift apart.
namespace MenuLayout {

    inline constexpr float kContentLeft = 110.0f;
    inline constexpr float kContentWidth = 1700.0f;
    inline constexpr float kContentTop = 268.0f;
    inline constexpr float kContentBottom = 992.0f;

    inline sf::FloatRect BackButton() { return sf::FloatRect(kContentLeft, 54.0f, 140.0f, 46.0f); }

    // ---- Settings ----------------------------------------------------------------------

    enum class SettingId {
        Fullscreen, VSync, FpsLimit, Resolution, Theme,
        AutoBackup, Autosave, VaultDir, ExportFormat,
        HwAccel, PreviewRate, UndoHistory, GridContrast,
        AiProvider, ApiKey
    };

    // Info rows show a value that cannot be changed from this screen.
    enum class SettingKind { Toggle, Stepper, Dropdown, Info, TextField };

    struct SettingRow {
        SettingId id;
        SettingKind kind;
        const char* label;
        int section;
        int row;
    };

    inline constexpr int kSettingSectionCount = 4;
    inline constexpr const char* kSettingSections[kSettingSectionCount] = {
        "DISPLAY & GRAPHICS", "BACKUPS & STORAGE", "PERFORMANCE & TIMELINE", "STUDIO ASSISTANT"
    };

    inline constexpr SettingRow kSettingRows[] = {
        { SettingId::Fullscreen,   SettingKind::Toggle,    "Fullscreen Mode",            0, 0 },
        { SettingId::VSync,        SettingKind::Toggle,    "Vertical Sync (VSync)",      0, 1 },
        { SettingId::FpsLimit,     SettingKind::Stepper,   "FPS Target Limit",           0, 2 },
        { SettingId::Resolution,   SettingKind::Dropdown,  "Display Resolution",         0, 3 },
        { SettingId::Theme,        SettingKind::Info,      "UI Palette Theme",           0, 4 },

        { SettingId::AutoBackup,   SettingKind::Toggle,    "Auto-Backup Vault",          1, 0 },
        { SettingId::Autosave,     SettingKind::Info,      "Autosave Frequency",         1, 1 },
        { SettingId::VaultDir,     SettingKind::Info,      "Vault Directory",            1, 2 },
        { SettingId::ExportFormat, SettingKind::Info,      "Export Format",              1, 3 },

        { SettingId::HwAccel,      SettingKind::Toggle,    "Hardware GPU Acceleration",  2, 0 },
        { SettingId::PreviewRate,  SettingKind::Stepper,   "Preview Rate",               2, 1 },
        { SettingId::UndoHistory,  SettingKind::Stepper,   "Undo Stack History",         2, 2 },
        { SettingId::GridContrast, SettingKind::Info,      "Pixel Grid Contrast",        2, 3 },

        { SettingId::AiProvider,   SettingKind::Stepper,   "Assistant Engine",         3, 0 },
        { SettingId::ApiKey,       SettingKind::TextField, "Access Key",           3, 1 }
    };

    inline constexpr int kResolutionOptionCount = 3;
    inline constexpr int kResolutionOptions[kResolutionOptionCount][2] = { { 1280, 720 }, { 1600, 900 }, { 1920, 1080 } };

    inline sf::FloatRect SettingsSection(int section) {
        const float gap = 24.0f;
        const float w = (kContentWidth - gap) * 0.5f;
        const float h = (kContentBottom - kContentTop - 20.0f) * 0.5f;
        float col = static_cast<float>(section % 2);
        float row = static_cast<float>(section / 2);
        return sf::FloatRect(kContentLeft + col * (w + gap), kContentTop + row * (h + 20.0f), w, h);
    }

    inline sf::FloatRect SettingsRowBounds(const SettingRow& row) {
        sf::FloatRect s = SettingsSection(row.section);
        return sf::FloatRect(s.left + 16.0f, s.top + 58.0f + static_cast<float>(row.row) * 56.0f, s.width - 32.0f, 50.0f);
    }

    inline sf::FloatRect ToggleSwitch(const sf::FloatRect& row) { return sf::FloatRect(row.left + row.width - 80.0f, row.top + 11.0f, 64.0f, 28.0f); }
    inline sf::FloatRect StepperLeft(const sf::FloatRect& row) { return sf::FloatRect(row.left + row.width - 284.0f, row.top + 6.0f, 40.0f, 38.0f); }
    inline sf::FloatRect StepperValue(const sf::FloatRect& row) { return sf::FloatRect(row.left + row.width - 238.0f, row.top + 6.0f, 176.0f, 38.0f); }
    inline sf::FloatRect StepperRight(const sf::FloatRect& row) { return sf::FloatRect(row.left + row.width - 56.0f, row.top + 6.0f, 40.0f, 38.0f); }
    inline sf::FloatRect DropdownButton(const sf::FloatRect& row) { return sf::FloatRect(row.left + row.width - 284.0f, row.top + 6.0f, 268.0f, 38.0f); }
    inline sf::FloatRect DropdownOption(const sf::FloatRect& button, int index) {
        return sf::FloatRect(button.left, button.top + button.height + 4.0f + static_cast<float>(index) * 40.0f, button.width, 40.0f);
    }
    inline sf::FloatRect TextField(const sf::FloatRect& row) { return sf::FloatRect(row.left + row.width - 344.0f, row.top + 6.0f, 328.0f, 38.0f); }

    // ---- Tutorials ---------------------------------------------------------------------

    inline constexpr int kTutorialCount = 9;

    inline sf::FloatRect TutorialCard(int index) {
        const float gap = 20.0f;
        const float w = (kContentWidth - gap * 2.0f) / 3.0f;
        const float h = (kContentBottom - kContentTop - gap * 2.0f) / 3.0f;
        float col = static_cast<float>(index % 3);
        float row = static_cast<float>(index / 3);
        return sf::FloatRect(kContentLeft + col * (w + gap), kContentTop + row * (h + gap), w, h);
    }

    // Chapter list shown beside an open guide.
    inline sf::FloatRect TutorialNavItem(int index) {
        return sf::FloatRect(kContentLeft, kContentTop + static_cast<float>(index) * 54.0f, 380.0f, 48.0f);
    }

    inline sf::FloatRect TutorialContent() {
        const float left = kContentLeft + 404.0f;
        return sf::FloatRect(left, kContentTop, kContentLeft + kContentWidth - left, kContentBottom - kContentTop);
    }

    inline sf::FloatRect TutorialReturnButton() {
        sf::FloatRect c = TutorialContent();
        return sf::FloatRect(c.left + c.width - 244.0f, c.top + c.height - 70.0f, 212.0f, 46.0f);
    }

    // ---- Credits -----------------------------------------------------------------------

    inline sf::FloatRect CreditsAuthor() { return sf::FloatRect(kContentLeft, kContentTop, 620.0f, 590.0f); }
    inline sf::FloatRect CreditsBlock(int index) {
        const float left = kContentLeft + 644.0f;
        const float fullW = kContentLeft + kContentWidth - left;
        const float halfW = (fullW - 24.0f) * 0.5f;
        if (index == 0) return sf::FloatRect(left, kContentTop, halfW, 283.0f);
        if (index == 1) return sf::FloatRect(left + halfW + 24.0f, kContentTop, halfW, 283.0f);
        return sf::FloatRect(left, kContentTop + 307.0f, fullW, 283.0f);
    }
    inline sf::FloatRect CreditsEgg() { return sf::FloatRect(kContentLeft, kContentTop + 614.0f, kContentWidth, 110.0f); }

}
