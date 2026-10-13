// Native Themezer Home Menu wallpaper importer for the Switch-style PS5 launcher.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <atomic>
#include <string>

namespace pe::ui::switch_theme
{
struct ImportResult
{
    bool success = false;
    std::string id;
    std::string message;
};
// A complete HTTPS -> NXTheme ZIP -> JPG/PNG -> TGA transaction.
// No installed theme is modified on failure. Does not activate until successful.
ImportResult install_quick_id(std::string id, const std::atomic<bool>& stop);
std::string normalize_quick_id(const std::string& input);
}
