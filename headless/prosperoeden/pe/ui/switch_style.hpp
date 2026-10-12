// ProsperoEden Switch UI - external theme contract and update guard.
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "pe/core/file.hpp"
#include "pe/ui/theme.hpp"
#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>

namespace pe::ui::switch_ui
{
// Fork safety: official app releases replace eboot.bin. Never call the updater
// from our modified build; merge upstream in Git and rebuild instead.
inline constexpr bool kOfficialSelfUpdateEnabled = false;
inline constexpr const char *kActiveThemeDir = "/data/prosperoeden/themes/active";

struct Palette
{
    std::string name = "Switch Home";
    Color accent = Color::rgb(0x62d8e6);
    Color panel = Color::rgb(0x111c28);
    Color text = Color::rgb(0xf8fbff);
    Color muted = Color::rgb(0xd6e0e9);
};

inline std::string_view trim(std::string_view v)
{
    const auto space = [](char c) { return c == ' ' || c == '\t' || c == '\r'; };
    while (!v.empty() && space(v.front())) v.remove_prefix(1);
    while (!v.empty() && space(v.back())) v.remove_suffix(1);
    return v;
}

inline bool hex_color(std::string_view value, Color *output)
{
    if (value.size() != 7 || value.front() != '#') return false;
    std::uint32_t value_hex = 0;
    for (std::size_t i = 1; i < 7; ++i)
    {
        const char c = value[i];
        const int digit = c >= '0' && c <= '9' ? c - '0' :
                          c >= 'a' && c <= 'f' ? c - 'a' + 10 :
                          c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
        if (digit < 0) return false;
        value_hex = (value_hex << 4) | static_cast<std::uint32_t>(digit);
    }
    *output = Color::rgb(value_hex);
    return true;
}

inline Palette read_palette()
{
    Palette out;
    std::string content;
    // One bounded, tiny file outside the application install. Missing or malformed
    // files do not prevent the launcher from starting.
    if (!pe::read_file(std::string(kActiveThemeDir) + "/palette.ini", &content, 4096))
        return out;
    std::size_t position = 0;
    while (position < content.size())
    {
        const std::size_t end = content.find('\n', position);
        const std::size_t length = (end == std::string::npos ? content.size() : end) - position;
        const std::string_view line = trim(std::string_view(content).substr(position, length));
        position = end == std::string::npos ? content.size() : end + 1;
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        const std::size_t equals = line.find('=');
        if (equals == std::string::npos) continue;
        const auto key = trim(line.substr(0, equals));
        const auto value = trim(line.substr(equals + 1));
        if (key == "name")
        {
            // Bound text length and ignore control characters.
            bool valid = true;
            for (char c : value) if (static_cast<unsigned char>(c) < 32) valid = false;
            if (valid && value.size() <= 60) out.name = std::string(value);
        }
        else if (key == "accent") hex_color(value, &out.accent);
        else if (key == "panel") hex_color(value, &out.panel);
        else if (key == "text") hex_color(value, &out.text);
        else if (key == "muted") hex_color(value, &out.muted);
    }
    return out;
}

// Read on first use; changing the active theme requires restarting the app in v3.
inline const Palette &palette()
{
    static const Palette theme = read_palette();
    return theme;
}
} // namespace pe::ui::switch_ui
