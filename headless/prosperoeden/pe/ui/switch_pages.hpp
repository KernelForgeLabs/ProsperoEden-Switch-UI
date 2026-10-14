// ProsperoEden Switch UI: sandboxed, manifest-driven page wallpaper lookup.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "pe/ui/switch_bootstrap.hpp"
#include <array>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <vector>
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
// Per-screen overrides are a small user-owned manifest. Only installed, valid
// pack assets may be selected. "auto" restores the default mapping for a role.
inline constexpr std::array<std::string_view, 8> kEditableRoles = {
    "home", "library", "settings", "themes", "profiles", "multiplayer", "rooms", "about"};
inline constexpr std::array<std::string_view, 7> kAssets = {
    "home", "all_apps", "settings", "user", "player_select", "news", "lockscreen"};

inline bool pack_directory(const std::string &root, const std::string &id) {
    if (!switch_bootstrap::valid_id(id)) return false;
    const fs::path base = fs::path(root) / "installed" / id;
    return switch_bootstrap::directory_without_links(base) &&
           switch_bootstrap::directory_without_links(base / "art") &&
           switch_bootstrap::directory_without_links(base / "art" / "pages") &&
           switch_bootstrap::regular_without_links(base / "pages.ini");
}
inline std::vector<std::string> available_assets(const std::string &root, const std::string &id) {
    std::vector<std::string> result;
    if (!pack_directory(root,id)) return result;
    const fs::path path = fs::path(root) / "installed" / id / "art" / "pages";
    for (std::string_view name : kAssets) {
        const fs::path main = path / (std::string(name) + ".tga");
        const fs::path blur = path / (std::string(name) + "-blur.tga");
        if (switch_bootstrap::valid_tga(main) && switch_bootstrap::valid_tga(blur))
            result.emplace_back(name);
    }
    return result;
}
inline bool read_overrides(const std::string &root, const std::string &id,
                           std::map<std::string,std::string> &values) {
    values.clear();
    if (!pack_directory(root,id)) return false;
    const fs::path file = fs::path(root) / "installed" / id / "pages.ini";
    std::error_code ec;
    if (fs::file_size(file,ec) > 4096 || ec) return false;
    std::ifstream input(file, std::ios::binary);
    if (!input) return false;
    std::string line;
    while (std::getline(input,line)) {
        if (line.empty() || line[0]=='#') continue;
        const auto sep = line.find('=');
        if (sep == std::string::npos || line.size() > 128) return false;
        const std::string key=line.substr(0,sep), value=line.substr(sep+1);
        if (!role(key) || !known(value)) return false;
        values[key]=value;
    }
    return !input.bad();
}
inline std::string override_for(const std::string &root, const std::string &id,
                                std::string_view page) {
    std::map<std::string,std::string> entries;
    if (!role(page) || !read_overrides(root,id,entries)) return "auto";
    const auto it=entries.find(std::string(page));
    return it == entries.end() ? "auto" : it->second;
}
inline bool set_override(const std::string &root, const std::string &id,
                         std::string_view page, std::string_view artwork) {
    if (!role(page) || (artwork != "auto" && !known(artwork))) return false;
    std::map<std::string,std::string> entries;
    if (!read_overrides(root,id,entries)) return false;
    if (artwork != "auto") {
        const auto available=available_assets(root,id);
        if (std::find(available.begin(),available.end(),std::string(artwork))==available.end()) return false;
        entries[std::string(page)]=std::string(artwork);
    } else entries.erase(std::string(page));
    const fs::path base=fs::path(root) / "installed" / id;
    const fs::path temporary=base / "pages.ini.tmp";
    const fs::path manifest=base / "pages.ini";
    std::error_code ec;
    if (fs::exists(temporary,ec) || ec) return false;
    {
        std::ofstream out(temporary,std::ios::binary|std::ios::trunc);
        if (!out) return false;
        for (const auto &[key,value]:entries) out << key << '=' << value << '\n';
        out.flush();
        if (!out.good()) {out.close();fs::remove(temporary,ec);return false;}
    }
    ec.clear();
    fs::rename(temporary,manifest,ec);
    if (ec) {std::error_code ignored;fs::remove(temporary,ignored);return false;}
    return true;
}

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
