// ProsperoEden Switch UI: sandboxed, manifest-driven page wallpaper lookup.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "pe/ui/switch_bootstrap.hpp"
#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <utility>

namespace pe::ui::switch_pages {
namespace fs = std::filesystem;

inline bool known(std::string_view name) {
    for (std::string_view value : {"home", "all_apps", "settings", "user", "player_select",
                                   "news", "lockscreen"})
        if (name == value) return true;
    return false;
}
inline bool role(std::string_view name) {
    for (std::string_view value : {"home", "library", "settings", "themes", "profiles",
                                   "multiplayer", "rooms", "game_details", "about", "startup"})
        if (name == value) return true;
    return false;
}
// This map deliberately doesn't associate Multiplayer with Switch player selection:
// game-controller pairing artwork may be incongruous with multiplayer rooms.
inline std::string default_asset(std::string_view page) {
    if (page == "library" || page == "game_details") return "all_apps";
    if (page == "settings" || page == "themes" || page == "about") return "settings";
    if (page == "profiles") return "user";
    if (page == "startup") return "lockscreen";
    return "home";
}
struct Art {
    std::string main;
    std::string blur;
};
// Only known names within the validated theme directory are used as paths.
inline Art lookup(const std::string &root, const std::string &id, std::string_view page) {
    if (!switch_bootstrap::valid_id(id) || !role(page)) return {};
    const fs::path base = fs::path(root) / "installed" / id;
    if (!switch_bootstrap::directory_without_links(base) ||
        !switch_bootstrap::directory_without_links(base / "art") ||
        !switch_bootstrap::directory_without_links(base / "art" / "pages")) return {};
    std::string target = default_asset(page);
    const fs::path manifest = base / "pages.ini";
    if (switch_bootstrap::regular_without_links(manifest)) {
        std::error_code ec;
        if (fs::file_size(manifest, ec) <= 4096 && !ec) {
            std::ifstream in(manifest);
            std::string line;
            while (std::getline(in, line)) {
                if (line.size() > 128) continue;
                auto n = line.find('=');
                if (n == std::string::npos) continue;
                if (line.substr(0, n) == page && known(line.substr(n + 1))) {
                    target = line.substr(n + 1); break;
                }
            }
        }
    }
    const auto find = [&](const std::string &name) -> Art {
        const fs::path p = base / "art" / "pages" / (name + ".tga");
        const fs::path b = base / "art" / "pages" / (name + "-blur.tga");
        if (switch_bootstrap::valid_tga(p) && switch_bootstrap::valid_tga(b))
            return {p.string(), b.string()};
        return {};
    };
    if (auto a = find(target); !a.main.empty()) return a;
    // The active pack's Home image is preferred to the app's dark-mode fallback.
    return find("home");
}
} // namespace pe::ui::switch_pages
