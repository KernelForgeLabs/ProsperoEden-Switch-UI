// ProsperoEden Switch UI - first-run theme library and v4 migration.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace pe::ui::switch_bootstrap {
namespace fs = std::filesystem;

inline constexpr const char *kDefaultRoot = "/data/prosperoeden/themes";

inline std::string root() {
    // Allows host tests and developers to use an isolated theme library.
    if (const char *custom = std::getenv("PE_SWITCH_THEME_ROOT"); custom && *custom)
        return custom;
    return kDefaultRoot;
}

inline bool valid_id(const std::string &id) {
    if (id.empty() || id.size() > 32) return false;
    for (unsigned char c : id)
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') || c == '-' || c == '_')) return false;
    return true;
}

inline std::string trim(std::string value) {
    const auto start = value.find_first_not_of(" \r\n\t");
    if (start == std::string::npos) return {};
    const auto end = value.find_last_not_of(" \r\n\t");
    return value.substr(start, end - start + 1);
}

inline bool regular_without_links(const fs::path &path) {
    std::error_code ec;
    return !fs::is_symlink(fs::symlink_status(path, ec)) && !ec &&
           fs::is_regular_file(path, ec) && !ec;
}

inline bool directory_without_links(const fs::path &path) {
    std::error_code ec;
    return !fs::is_symlink(fs::symlink_status(path, ec)) && !ec &&
           fs::is_directory(path, ec) && !ec;
}

// Fast structural validation. The texture loader still fully decodes TGA files.
inline bool valid_tga(const fs::path &path) {
    if (!regular_without_links(path)) return false;
    std::error_code ec;
    const auto length = fs::file_size(path, ec);
    if (ec || length < 18 || length > 64ULL * 1024 * 1024) return false;
    std::ifstream in(path, std::ios::binary);
    unsigned char h[18]{};
    if (!in.read(reinterpret_cast<char *>(h), sizeof h)) return false;
    const unsigned w = h[12] | (static_cast<unsigned>(h[13]) << 8);
    const unsigned height = h[14] | (static_cast<unsigned>(h[15]) << 8);
    const unsigned bytes = h[16] / 8;
    const unsigned long long minimum = 18ULL + h[0] +
        static_cast<unsigned long long>(w) * height * bytes;
    return h[1] == 0 && h[2] == 2 && (bytes == 3 || bytes == 4) &&
        w > 0 && w <= 8192 && height > 0 && height <= 8192 && length >= minimum;
}

struct Installed {
    std::string id;
    std::string name;
    std::string art;
};

inline std::string name_from_ini(const fs::path &path, const std::string &fallback) {
    if (!regular_without_links(path)) return fallback;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        if (line.rfind("name=", 0) != 0) continue;
        std::string name = trim(line.substr(5));
        if (name.size() > 60 || name.empty()) break;
        const bool printable = std::all_of(name.begin(), name.end(), [](unsigned char ch) {
            return ch >= 32 && ch != 127;
        });
        if (printable) return name;
        break;
    }
    return fallback;
}

inline bool usable(const fs::path &path) {
    return directory_without_links(path) && directory_without_links(path / "art") &&
           valid_tga(path / "art" / "backdrop.tga") &&
           valid_tga(path / "art" / "backdrop-blur.tga");
}

inline std::vector<Installed> installed(const std::string &root_path) {
    std::vector<Installed> out;
    const fs::path root_dir(root_path);
    const fs::path folder = root_dir / "installed";
    if (directory_without_links(folder)) {
        std::error_code ec;
        for (fs::directory_iterator i(folder, fs::directory_options::skip_permission_denied, ec), end;
             !ec && i != end && out.size() < 128; i.increment(ec)) {
            const std::string id = i->path().filename().string();
            if (!valid_id(id) || id == "default" || id == "legacy" || id == "switch-dark" ||
                id == "canopy" || !usable(i->path())) continue;
            out.push_back({id, name_from_ini(i->path() / "palette.ini", id),
                           (i->path() / "art").string()});
        }
    }
    // v3 used /active before the installed/ library existed.
    if (usable(root_dir / "active"))
        out.push_back({"legacy", "Previous active theme", (root_dir / "active" / "art").string()});
    std::sort(out.begin(), out.end(), [](const Installed &a, const Installed &b) {return a.id < b.id;});
    return out;
}

inline std::string selected(const std::string &root_path) {
    std::ifstream file(fs::path(root_path) / "current.txt");
    if (!file) return {};
    std::string id;
    std::getline(file, id);
    id = trim(id);
    return valid_id(id) ? id : std::string{};
}

// Idempotent, never overwrites settings or existing themes. Failure is nonfatal:
// the built-in dark theme still works without writable storage.
inline bool ensure(const std::string &root_path) {
    std::error_code ec;
    const fs::path root_dir(root_path);
    fs::create_directories(root_dir / "installed", ec);
    if (ec || !directory_without_links(root_dir) || !directory_without_links(root_dir / "installed"))
        return false;
    ec.clear();
    fs::create_directories(root_dir / "active", ec);
    if (ec) return false;
    const fs::path current = root_dir / "current.txt";
    if (fs::exists(current, ec)) return !ec && regular_without_links(current);
    // Preserve the v3 active directory when upgrading with no current.txt.
    const std::string default_id = usable(root_dir / "active") ? "legacy" : "switch-dark";
    std::ofstream output(current, std::ios::binary | std::ios::trunc);
    output << default_id << '\n';
    return output.good();
}

inline bool set_selected(const std::string &root_path, const std::string &id) {
    if (!valid_id(id)) return false;
    const fs::path root_dir(root_path);
    if (!directory_without_links(root_dir)) return false;
    const fs::path temp = root_dir / "current.txt.tmp";
    if (fs::exists(temp) && !regular_without_links(temp)) return false;
    {
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        if (!out) return false;
        out << id << '\n';
        if (!out.good()) return false;
    }
    std::error_code ec;
    fs::rename(temp, root_dir / "current.txt", ec);
    if (ec) fs::remove(temp);
    return !ec;
}

} // namespace pe::ui::switch_bootstrap
