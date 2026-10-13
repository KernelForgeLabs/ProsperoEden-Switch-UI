// ProsperoEden Switch UI - persistent, offline theme activation.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "pe/core/file.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <utility>
#include <vector>

namespace pe::ui::switch_theme
{
// Theme assets are independent of /data/homebrew/PPSA99008 and app updates.
inline constexpr const char *kRoot = "/data/prosperoeden/themes";

inline bool valid_id(const std::string &id)
{
    if (id.empty() || id.size() > 32) return false;
    for (unsigned char c : id)
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-'))
            return false;
    return true;
}

inline std::string trim_id(std::string id)
{
    while (!id.empty() && (id.back() == '\r' || id.back() == '\n' || id.back() == ' '))
        id.pop_back();
    while (!id.empty() && id.front() == ' ') id.erase(id.begin());
    return id;
}

// Lightweight validation rather than allocating a full 1920x1080 image just
// to list themes. The existing gfx::load_tga() does the full decode at startup.
inline bool valid_tga(const std::string &path)
{
    std::FILE *f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    unsigned char header[18]{};
    bool ok = std::fread(header, 1, sizeof(header), f) == sizeof(header);
    const unsigned w = header[12] | (static_cast<unsigned>(header[13]) << 8);
    const unsigned h = header[14] | (static_cast<unsigned>(header[15]) << 8);
    const unsigned bpp = header[16] / 8;
    ok = ok && header[1] == 0 && header[2] == 2 &&
         (bpp == 3 || bpp == 4) && w > 0 && w <= 8192 && h > 0 && h <= 8192;
    if (ok && std::fseek(f, 0, SEEK_END) == 0)
    {
        const long length = std::ftell(f);
        const unsigned long long required = 18ULL + header[0] +
            static_cast<unsigned long long>(w) * h * bpp;
        ok = length >= 0 && static_cast<unsigned long long>(length) >= required &&
             static_cast<unsigned long long>(length) <= (64ULL << 20);
    }
    else ok = false;
    std::fclose(f);
    return ok;
}

class Store
{
public:
    explicit Store(std::string root) : root_(std::move(root)) {}
    std::string path_for(const std::string &id) const
    {
        if (id == "legacy") return root_ + "/active";
        if (id == "default" || !valid_id(id)) return {};
        return root_ + "/installed/" + id;
    }
    bool usable(const std::string &id) const
    {
        const auto dir = path_for(id);
        return !dir.empty() && valid_tga(dir + "/art/backdrop.tga") &&
               valid_tga(dir + "/art/backdrop-blur.tga");
    }
    std::vector<std::string> installed() const
    {
        std::vector<std::string> ids;
        const std::string folder = root_ + "/installed";
        DIR *dir = opendir(folder.c_str());
        if (!dir) return ids;
        while (dirent *entry = readdir(dir))
        {
            const std::string id = entry->d_name;
            if (valid_id(id) && id != "default" && id != "legacy" && usable(id))
                ids.push_back(id);
            if (ids.size() >= 128) break;
        }
        closedir(dir);
        std::sort(ids.begin(), ids.end());
        return ids;
    }
    std::string current_id() const
    {
        std::string data;
        if (pe::read_file(root_ + "/current.txt", &data, 64))
        {
            const std::string id = trim_id(data);
            if (id == "default") return id;
            if (id == "legacy" && usable(id)) return id;
            if (valid_id(id) && id != "legacy" && id != "default" && usable(id)) return id;
            return "default"; // Invalid / removed pack: fall back safely.
        }
        return usable("legacy") ? "legacy" : "default"; // v3 compatibility.
    }
    std::string asset_root() const { return path_for(current_id()); }
    std::string display_name(const std::string &id) const
    {
        const std::string folder = path_for(id);
        if (folder.empty()) return "Original";
        std::string data;
        if (pe::read_file(folder + "/palette.ini", &data, 4096))
        {
            const auto start = data.find("name=");
            if (start != std::string::npos && (start == 0 || data[start-1] == '\n'))
            {
                const auto end = data.find('\n', start);
                auto value = trim_id(data.substr(start+5, end == std::string::npos ?
                    std::string::npos : end - start - 5));
                if (!value.empty() && value.size() <= 60 &&
                    std::all_of(value.begin(), value.end(), [](unsigned char c){return c>=32;}))
                    return value;
            }
        }
        return id == "legacy" ? "Legacy active theme" : id;
    }
    bool activate(const std::string &id, std::string *error) const
    {
        if (id != "default" && !usable(id))
        {
            if (error) *error = "Theme artwork is missing or invalid.";
            return false;
        }
        const auto dir = root_;
        // Existing directory is fine. Never create or modify application files.
        if (mkdir(dir.c_str(), 0755) != 0)
        {
            struct stat info{};
            if (stat(dir.c_str(), &info) != 0 || !S_ISDIR(info.st_mode))
            {
                if (error) *error = "Cannot access theme storage.";
                return false;
            }
        }
        const std::string tmp = root_ + "/current.txt.tmp";
        std::FILE *f = std::fopen(tmp.c_str(), "wb");
        if (!f)
        {
            if (error) *error = "Cannot save theme selection.";
            return false;
        }
        const std::string line = id + "\n";
        bool ok = std::fwrite(line.data(), 1, line.size(), f) == line.size();
        ok = std::fclose(f) == 0 && ok;
        if (ok) ok = std::rename(tmp.c_str(), (root_ + "/current.txt").c_str()) == 0;
        if (!ok)
        {
            std::remove(tmp.c_str());
            if (error) *error = "Theme selection could not be saved.";
        }
        return ok;
    }
private:
    std::string root_;
};

inline Store &store()
{
    static Store instance{kRoot};
    return instance;
}
inline std::vector<std::string> installed() { return store().installed(); }
inline std::string current_id() { return store().current_id(); }
inline std::string asset_root() { return store().asset_root(); }
inline std::string display_name(const std::string &id) { return store().display_name(id); }
inline bool activate(const std::string &id, std::string *error) { return store().activate(id, error); }
} // namespace pe::ui::switch_theme
