// ProsperoEden - The launcher's themes, and which one is in use.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pe/ui/theme.hpp"

#include "pe/core/strings.hpp"
#include "pe/ui/switch_bootstrap.hpp"

#include <array>
#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <filesystem>
#include <fstream>

namespace pe::ui
{

namespace
{

Color rgb(std::uint32_t hex, float alpha = 1.0f)
{
    return Color::rgb(hex, alpha);
}

// Switch Dark is the independent built-in colour and artwork baseline for
// the Switch UI edition. Themezer themes inherit from this, never from Canopy.
// It works offline when the persistent theme library is empty or unwritable.
Theme switch_dark()
{
    Theme t;
    (void)theme_from_kit("acrylic", &t);
    t.id = "switch-dark";
    t.name = "Switch Dark";
    t.summary = "Dark minimalist HOME theme, included with the app.";

    // Explicitly define all visible colour roles; do not inherit the former
    // ProsperoEden Canopy palette or its green/tropical fallback values.
    t.base        = rgb(0x0b1019);
    t.scrim       = rgb(0x060910);
    t.glass       = rgb(0x141c28);
    t.panel       = rgb(0x141a24);
    t.panel_edge  = rgb(0x526070);
    t.row         = rgb(0x202a38);
    t.row_edge    = rgb(0x4c5b70);
    t.row_focus   = rgb(0x31465b);

    t.accent      = rgb(0x66cdfb);
    t.accent_deep = rgb(0x315c83);
    t.accent_pale = rgb(0xc9edfd);
    t.on_accent   = rgb(0x07121c);
    t.highlight   = rgb(0x639dc7);

    t.text        = rgb(0xf5f7fb);
    t.title       = rgb(0xf8fafc);
    t.value       = rgb(0xeaf0f8);
    t.body        = rgb(0xe5edf5);
    t.copy        = rgb(0xc4d1df);
    t.meta        = rgb(0xb9c6d4);
    t.label       = rgb(0xafc0d0);
    t.muted       = rgb(0x9bacc0);
    t.faint       = rgb(0x8b9aad);
    t.warning     = rgb(0xe8b39a);
    t.rule        = rgb(0x425061);

    t.radius_row = 18.0f;
    t.radius_card = 22.0f;
    t.radius_panel = 30.0f;
    t.pill_chips = true;
    t.focus_width = 2.0f;
    t.focus_glow = 30.0f;
    t.glass_panels = true;
    t.focus_spring = 20.0f;
    t.cursor_spring = 26.0f;
    t.scroll_spring = 16.0f;

    t.backdrop = "switch-dark-backdrop.tga";
    t.backdrop_blur = "switch-dark-backdrop-blur.tga";
    t.brand = "brand.tga";
    t.motes = 0;
    t.drift = 0.0f;
    return t;
}

struct ExternalNames {
    std::string id, name, summary, backdrop, blur;
};

struct Store
{
    std::vector<Theme> all;
    std::vector<std::unique_ptr<ExternalNames>> names;
    int index = 0;
    Theme active;
    Store() : all{switch_dark()}, active(all[0]) {}
};

Store &store()
{
    static Store value;
    return value;
}

bool &ready()
{
    static bool started = false;
    return started;
}

} // namespace

namespace {
// Themezer pack palettes can optionally control focus accent in our HOME dock.
// A malformed palette leaves the bundled Switch Dark colours intact.
void apply_palette(Theme &theme, const std::string &folder)
{
    const auto file = std::filesystem::path(folder) / "palette.ini";
    if (!switch_bootstrap::regular_without_links(file)) return;
    std::error_code ec;
    if (std::filesystem::file_size(file, ec) > 4096 || ec) return;
    std::ifstream in(file);
    std::string line;
    while (std::getline(in, line)) {
        if (line.rfind("accent=#", 0) != 0 || line.size() != 14) continue;
        std::uint32_t value = 0;
        bool good = true;
        for (std::size_t i = 8; i < 14; ++i) {
            const char ch = line[i];
            const int nibble = (ch >= '0' && ch <= '9') ? ch - '0' :
                               (ch >= 'a' && ch <= 'f') ? ch - 'a' + 10 :
                               (ch >= 'A' && ch <= 'F') ? ch - 'A' + 10 : -1;
            if (nibble < 0) { good = false; break; }
            value = (value << 4) | static_cast<unsigned>(nibble);
        }
        if (good) theme.accent = Color::rgb(value);
        break;
    }
}

void append_installed(Store &s) {
    for (const auto &item : switch_bootstrap::installed(switch_bootstrap::root())) {
        auto name = std::make_unique<ExternalNames>();
        name->id = item.id;
        name->name = item.name;
        name->summary = "Installed Themezer wallpaper (from persistent storage)";
        name->backdrop = item.art + "/backdrop.tga";
        name->blur = item.art + "/backdrop-blur.tga";
        Theme theme = switch_dark();
        apply_palette(theme, (std::filesystem::path(switch_bootstrap::root()) /
                              "installed" / item.id).string());
        theme.id = name->id.c_str();
        theme.name = name->name.c_str();
        theme.summary = name->summary.c_str();
        theme.backdrop = name->backdrop.c_str();
        theme.backdrop_blur = name->blur.c_str();
        s.names.push_back(std::move(name));
        s.all.push_back(theme);
    }
}
} // namespace

void initialize_switch_themes() {
    if (ready()) return;
    ready() = true;
    Store &s = store();
    (void)switch_bootstrap::ensure(switch_bootstrap::root());
    append_installed(s);
}

// Refreshes the menu list after an import/deletion. The active Theme's string
// fields point into ExternalNames, so preserve its identity before rebuilding.
void refresh_switch_themes() {
    initialize_switch_themes();
    Store &s = store();
    const std::string active = s.active.id;
    s.active = switch_dark();
    s.index = 0;
    s.all.clear();
    s.all.push_back(switch_dark());
    s.names.clear();
    append_installed(s);
    (void)use_theme(active);
}

// Old v4 selection takes precedence during migration; otherwise use the
// official preference. An unknown/removed theme falls back to Switch Dark.
std::string startup_switch_theme(std::string_view preference)
{
    initialize_switch_themes();
    const std::string stored = switch_bootstrap::selected(switch_bootstrap::root());
    // Migrate a previously selected upstream-only theme without touching
    // installed Themezer artwork or other persistent application data.
    if (stored == "canopy") {
        (void)switch_bootstrap::set_selected(switch_bootstrap::root(), "switch-dark");
        return "switch-dark";
    }
    const auto available = [](std::string_view id) {
        for (const Theme &t : themes()) if (id == t.id) return true;
        return false;
    };
    if (!stored.empty() && stored != "default" && available(stored)) return stored;
    if (available(preference)) return std::string(preference);
    return "switch-dark";
}

void remember_switch_theme(std::string_view id)
{
    (void)switch_bootstrap::set_selected(switch_bootstrap::root(), std::string(id));
}

const Theme &th()
{
    return store().active;
}

std::span<const Theme> themes()
{
    initialize_switch_themes();
    return store().all;
}

int theme_index()
{
    return store().index;
}

bool use_theme(std::string_view id)
{
    initialize_switch_themes();
    Store &all = store();
    for (std::size_t i = 0; i < all.all.size(); ++i)
    {
        if (id == all.all[i].id)
        {
            all.index = static_cast<int>(i);
            all.active = all.all[i];
            return true;
        }
    }
    return false;
}

} // namespace pe::ui
