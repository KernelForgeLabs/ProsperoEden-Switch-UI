// ProsperoEden - Launcher home: continue playing, recent games, navigation.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pe/ui/launcher.hpp"
#include "pe/ui/switch_style.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <string_view>

namespace pe::ui
{

using audio::Cue;

namespace
{

const Color kWhite{1.0f, 1.0f, 1.0f, 1.0f};

// Experimental Home Menu. Set to false to use the untouched original renderer and navigation.
constexpr bool kSwitchHomePrototype = true;

constexpr Rect kHero{120.0f, 200.0f, 1680.0f, 480.0f};
// Header tabs, hero buttons and "view all" are as wide as their words need in the language shown;
// these are their sizes in English and the edges they keep.
constexpr float kNavRight = 1480.0f;
constexpr float kNavWidth = 136.0f;
constexpr float kButtonWidth = 272.0f;
constexpr float kButtonWidest = 390.0f;
constexpr float kViewAllWidth = 304.0f;

Rect tile_rect(int index)
{
    return {120.0f + 424.0f * static_cast<float>(index), 760.0f, 400.0f, 144.0f};
}

// The four controllers, at the right of the hero beside its buttons.
constexpr float kPadsRight = 1752.0f;
Rect pad_rect(int player)
{
    return {kPadsRight - 354.0f + 94.0f * static_cast<float>(player), 506.0f, 72.0f, 50.0f};
}

} // namespace

void Launcher::open_library_at_file(const std::string &file)
{
    open(Screen::library, true);
    enter_library();
    for (int i = 0; i < static_cast<int>(games_.size()); ++i)
    {
        if (games_[static_cast<std::size_t>(i)].file == file)
        {
            library_.reset(library_.count, i);
            refresh_selected_game();
            break;
        }
    }
}

void Launcher::open_library_at_last()
{
    open_library_at_file(home_.last_file);
}

// The home row starts with Continue Playing, then recents, then all locally
// installed games. The actual library scan feeds this vector, so unlike the
// v2 recent-only row it can scroll beyond five titles.
std::vector<Launcher::SwitchHomeTile> Launcher::switch_home_tiles() const
{
    std::vector<SwitchHomeTile> tiles;
    if (!home_.setup_ready) return tiles;
    const auto add = [&](const std::string &file, const std::string &title,
                         const std::string &cover)
    {
        if (file.empty()) return;
        for (const SwitchHomeTile &tile : tiles)
            if (tile.file == file) return;
        tiles.push_back({file, title, cover});
    };
    if (home_.last_exists)
        add(home_.last_file, home_.last_title, home_.last_cover);
    for (const Recent &recent : home_.recents)
        add(recent.file, recent.title, recent.cover);
    for (const Game &game : games_)
        if (!game.remote)
            add(game.file, game.name, game.cover);
    return tiles;
}

void Launcher::press_home(Key key)
{
    const bool ready = home_.setup_ready;
    const bool continue_ready = ready && home_.last_exists;
    // The status panel takes the place of the recent games.
    const bool recents_shown = home_.status.empty();
    const int recent_count = recents_shown ? static_cast<int>(home_.recents.size()) : 0;
    const int before = home_focus_;
    int &focus = home_focus_;

    if (kSwitchHomePrototype)
    {
        const auto tiles = switch_home_tiles();
        const int count = static_cast<int>(tiles.size());
        if (count > 0)
            switch_home_index_ = std::clamp(switch_home_index_, 0, count - 1);
        else
            switch_home_index_ = 0;
        const auto details = [&]
        {
            if (count > 0)
                open_library_at_file(tiles[static_cast<std::size_t>(switch_home_index_)].file);
            else
                cue(Cue::error);
        };
        switch (key)
        {
        case Key::left:
        case Key::right:
            if (focus == 0 && count > 0)
                switch_home_index_ = std::clamp(switch_home_index_ + (key == Key::right ? 1 : -1),
                                                0, count - 1);
            else if (focus >= 1 && focus <= 4)
                focus = std::clamp(focus + (key == Key::right ? 1 : -1), 1, 4);
            cue(Cue::focus);
            return;
        case Key::down:
            if (focus == 0) { focus = 1; cue(Cue::focus); }
            return;
        case Key::up:
            if (focus >= 1 && focus <= 4 && count > 0) { focus = 0; cue(Cue::focus); }
            return;
        case Key::triangle:
            details();
            return;
        case Key::cross:
            press_ = 1.0f;
            if (focus == 0 && count > 0)
            {
                const auto &tile = tiles[static_cast<std::size_t>(switch_home_index_)];
                launch(tile.file, tile.title, tile.cover);
            }
            else if (focus == 0 || focus == 1)
            {
                if (ready) { open(Screen::library, true); enter_library(); }
                else cue(Cue::error);
            }
            else if (focus == 2)
            {
                open(Screen::settings, true);
                prefs_ = services_.preferences();
                section_.snap(1.0f);
            }
            else if (focus == 3) open(Screen::about, true);
            else if (focus == 4) details();
            return;
        default:
            return;
        }
    }

    switch (key)
    {
    case Key::up:
        if (focus == 0 || focus == 4)
            focus = ready ? 1 : 2;
        else if (focus >= 5)
            focus = 0;
        break;
    case Key::down:
        if (focus >= 1 && focus <= 3)
        {
            if (ready)
                focus = 0;
        }
        else if ((focus == 0 || focus == 4) && recents_shown)
        {
            focus = recent_count > 0 ? 5 : 9;
        }
        break;
    case Key::left:
    case Key::right:
    {
        const int delta = key == Key::right ? 1 : -1;
        if (focus >= 1 && focus <= 3)
        {
            focus = 1 + (focus - 1 + delta + 3) % 3;
            if (focus == 1 && !ready)
                focus = delta > 0 ? 2 : 3;
        }
        else if (focus == 0 || focus == 4)
        {
            if (continue_ready)
                focus = focus == 0 ? 4 : 0;
        }
        else
        {
            const int length = recent_count + 1;
            int position = focus == 9 ? recent_count : focus - 5;
            position = (position + delta + length) % length;
            focus = position == recent_count ? 9 : position + 5;
        }
        break;
    }
    case Key::triangle:
        if ((focus == 0 || focus == 4) && continue_ready)
            open_library_at_last();
        return;
    case Key::cross:
        press_ = 1.0f;
        if (focus == 0 && continue_ready)
        {
            launch(home_.last_file, home_.last_title, home_.last_cover);
        }
        else if ((focus == 0 || focus == 1 || focus == 9) && ready)
        {
            open(Screen::library, true);
            enter_library();
        }
        else if (focus == 2)
        {
            open(Screen::settings, true);
            prefs_ = services_.preferences();
            section_.snap(1.0f);
        }
        else if (focus == 3)
        {
            open(Screen::about, true);
        }
        else if (focus == 4 && continue_ready)
        {
            open_library_at_last();
        }
        else if (focus >= 5 && focus < 5 + recent_count && ready)
        {
            const Recent &recent = home_.recents[static_cast<std::size_t>(focus - 5)];
            launch(recent.file, recent.title, recent.cover);
        }
        else
        {
            cue(Cue::error);
        }
        return;
    default:
        return;
    }
    if (focus != before)
        cue(Cue::focus);
}

void Launcher::update_controllers(float dt)
{
    const unsigned now = services_.controllers() & 0xfu;
    bool joined = false;
    bool left = false;
    for (int player = 0; player < 4; ++player)
    {
        const std::size_t index = static_cast<std::size_t>(player);
        const bool on = (now >> player & 1u) != 0;
        controller_lit_[index].target = on ? 1.0f : 0.0f;
        if (!controllers_known_)
        {
            controller_lit_[index].snap(controller_lit_[index].target);
        }
        else if (on != ((controllers_ >> player & 1u) != 0))
        {
            (on ? joined : left) = true;
            if (on)
                controller_pop_[index] = 1.0f;
        }
        controller_lit_[index].update(dt, 10.0f);
        controller_pop_[index] = std::max(0.0f, controller_pop_[index] - dt / 0.5f);
    }
    // One sound however many changed at once; none while a game is starting.
    if ((joined || left) && selected_game_.empty())
        cue(joined ? Cue::saved : Cue::modal_close);
    controllers_ = now;
    controllers_known_ = true;
}

void Launcher::draw_controllers(Canvas &c)
{
    if (textures_.controller() == 0)
        return;
    text(c, tr("CONTROLLERS"), kPadsRight, baseline(462.0f, 30.0f, theme::kSmall), theme::kSmall,
         Color::rgb(0xc6d2c7), Align::right, 3.0f);
    for (int player = 0; player < 4; ++player)
    {
        const std::size_t index = static_cast<std::size_t>(player);
        const Rect r = pad_rect(player);
        const float lit = tween::clamp01(controller_lit_[index].value);
        // A controller that joins lands with a small bounce.
        const float pop = controller_pop_[index];
        const float bounce = std::sin(3.14159265f * (1.0f - pop)) * pop * motion();
        c.list.push_transform(1.0f + 0.34f * bounce, r.x + r.w * 0.5f, r.y + r.h * 0.5f, 0.0f,
                              -5.0f * bounce);
        controller_icon(c, r, lit);
        c.list.pop_transform();
        const char number[] = {static_cast<char>('1' + player), 0};
        text(c, number, r.x + r.w * 0.5f, baseline(564.0f, 26.0f, theme::kSmall), theme::kSmall,
             gfx::mix(theme::kFaint.with_alpha(0.6f), theme::kLime, lit), Align::center);
    }
}

void Launcher::draw_home(Canvas &c)
{
    gfx::DrawList &list = c.list;
    const bool ready = home_.setup_ready;
    const bool continue_ready = ready && home_.last_exists;
    const auto focus = [&](int index) { return home_springs_[static_cast<std::size_t>(index)].value; };
    const auto measure = [&](std::string_view value, float size, float tracking = 0.0f)
    { return text_width(c, value, size, tracking); };
    // BEGIN SWITCH HOME PROTOTYPE v3
    if (kSwitchHomePrototype)
    {
        const auto &style = switch_ui::palette();
        const auto tiles = switch_home_tiles();
        const int count = static_cast<int>(tiles.size());
        const int selected = count > 0 ? std::clamp(switch_home_index_, 0, count - 1) : 0;
        const float row_x = 136.0f;
        const float row_y = 340.0f;
        const float step = 310.0f;
        const float size = 268.0f;

        // Switch Home v3.1: keep the wallpaper clean. No header title or
        // frame/plate is drawn over art; retain the useful clock alone.
        text(c, clock_, 1800.0f, baseline(124.0f, 48.0f, theme::kClock),
             theme::kClock, style.text, Align::right);
        // Switch Home v3.2: deliberately no games-row heading. The cover tiles
        // and bottom controls identify the navigation without overlaying art.

        // Clip before drawing covers: off-screen games are not allowed to
        // spill into the margins. Their positions move on a critically damped spring.
        list.push_clip({112.0f, 325.0f, 1696.0f, 364.0f});
        for (int i = 0; i < count; ++i)
        {
            const float x = row_x + step * static_cast<float>(i) - switch_home_scroll_.value;
            if (x + size < 104.0f || x > 1820.0f) continue;
            const bool highlighted = home_focus_ == 0 && i == selected;
            const float lift = highlighted ? 1.0f : 0.0f;
            const Rect r{x, row_y, size, size};
            list.push_transform(1.0f + 0.055f * lift * motion(),
                                r.x + r.w * 0.5f, r.y + r.h * 0.5f, 0.0f, -4.0f * lift * motion());
            list.rounded_rect({r.x - 5.0f, r.y - 5.0f, r.w + 10.0f, r.h + 10.0f},
                              24.0f, style.panel.with_alpha(0.70f));
            cover(c, tiles[static_cast<std::size_t>(i)].cover,
                  {r.x + 3.0f, r.y + 3.0f, r.w - 6.0f, r.h - 6.0f}, 16.0f, 1.0f);
            if (highlighted)
                list.bordered_rect({r.x - 5.0f, r.y - 5.0f, r.w + 10.0f, r.h + 10.0f},
                                   23.0f, Color::rgb(0x000000, 0.01f), 4.0f, style.accent);
            list.pop_transform();
            list.rounded_rect({x - 4.0f, row_y + size + 12.0f, size + 8.0f, 45.0f},
                              10.0f, style.panel.with_alpha(0.73f));
            text_shrink(c, tiles[static_cast<std::size_t>(i)].title, x + size * 0.5f,
                        baseline(row_y + size + 16.0f, 36.0f, theme::kText24),
                        theme::kText24, style.text, size - 12.0f, Align::center);
        }
        list.pop_clip();
        if (count == 0)
            text(c, "No installed games yet - select All Games", 136.0f,
                 baseline(410.0f, 42.0f, theme::kText24), theme::kText24, style.text);
        if (!home_.status.empty())
            notice(c, home_.status, 136.0f,
                   baseline(714.0f, 34.0f, theme::kText24),
                   theme::kText24, theme::kWarning, 1490.0f, home_.launch_failed);
        // Subtle page position hint only when there are more than five covers.
        if (count > 5)
            text(c, std::to_string(selected + 1) + " / " + std::to_string(count),
                 1770.0f, baseline(700.0f, 30.0f, theme::kSmall),
                 theme::kSmall, style.muted, Align::right);

        // Switch-style system dock, preserving existing ProsperoEden actions.
        // Themes with their own built-in bottom bar should not have a second
        // opaque oval painted on top. Other wallpapers may opt into a dock
        // panel with `dock_panel=on` in the persistent theme palette.ini.
        if (style.dock_panel)
            list.rounded_rect({490.0f, 760.0f, 940.0f, 188.0f}, 80.0f,
                              style.panel.with_alpha(look().high_contrast ? 0.96f : 0.78f));
        static constexpr const char *labels[] = {"All Games", "Settings", "About", "Details"};
        for (int i = 0; i < 4; ++i)
        {
            const float cx = 660.0f + static_cast<float>(i) * 200.0f;
            const float cy = 831.0f;
            const bool highlighted = home_focus_ == i + 1;
            list.circle(cx, cy, 48.0f, highlighted ? style.accent :
                         Color::rgb(0xdce4ec, look().high_contrast ? 0.29f : 0.20f));
            if (highlighted) list.ring(cx, cy, 53.0f, 3.0f, style.text);
            const Color ink = highlighted ? style.panel : style.text;
            if (i == 0) // library: four squares
            {
                for (int dx = -1; dx <= 1; dx += 2)
                    for (int dy = -1; dy <= 1; dy += 2)
                        list.rounded_rect({cx + dx * 11.0f - 8.0f, cy + dy * 11.0f - 8.0f,
                                           16.0f, 16.0f}, 3.0f, ink);
            }
            else if (i == 1) // settings: cog-like spokes and central hole
            {
                for (int n = 0; n < 8; ++n)
                {
                    const float angle = static_cast<float>(n) * 3.14159265f / 4.0f;
                    list.line(cx + 18.0f * std::cos(angle), cy + 18.0f * std::sin(angle),
                              cx + 27.0f * std::cos(angle), cy + 27.0f * std::sin(angle), 4.0f, ink);
                }
                list.ring(cx, cy, 15.0f, 6.0f, ink);
            }
            else if (i == 2) // about
            {
                list.ring(cx, cy, 26.0f, 3.0f, ink);
                list.circle(cx, cy - 13.0f, 3.6f, ink);
                list.rounded_rect({cx - 2.2f, cy - 4.0f, 4.4f, 23.0f}, 2.0f, ink);
            }
            else // details
            {
                for (int n = 0; n < 3; ++n)
                    list.line(cx - 22.0f, cy - 15.0f + n * 15.0f,
                              cx + 23.0f, cy - 15.0f + n * 15.0f, 4.0f, ink);
            }
            text(c, labels[i], cx, baseline(892.0f, 35.0f, theme::kSmall),
                 theme::kSmall, style.text, Align::center);
        }
        text(c, "Cross: Select      Left/Right: Browse      Down: Menu      Triangle: Details",
             130.0f, baseline(998.0f, 30.0f, theme::kSmall), theme::kSmall,
             style.text);
        return;
    }
    // END SWITCH HOME PROTOTYPE
    // Each band of the screen arrives a moment after the one above it.
    const auto arrive = [&](int band)
    { return tween::cubic_out((intro_ - 0.12f - 0.08f * static_cast<float>(band)) / 0.55f); };
    const auto begin_band = [&](int band, float rise)
    {
        const float e = arrive(band);
        list.push_opacity(e);
        list.push_transform(1.0f, 0.0f, 0.0f, 0.0f, (1.0f - e) * rise * motion());
    };
    const auto end_band = [&]
    {
        list.pop_transform();
        list.pop_opacity();
    };
    // A focused surface grows a little and dips when pressed.
    const auto begin_lift = [&](const Rect &r, float amount, float grow)
    {
        const float scale = 1.0f + (grow * amount - 0.035f * press_ * amount) * motion();
        list.push_transform(scale, r.x + r.w * 0.5f, r.y + r.h * 0.5f, 0.0f,
                            -3.0f * amount * motion());
    };

    // ---- header ----
    begin_band(0, -18.0f);
    if (textures_.brand() != 0)
        list.rounded_image(textures_.brand(), {120.0f, 72.0f, 72.0f, 72.0f},
                           {0.0f, 0.0f, 1.0f, 1.0f}, 16.0f, kWhite);
    text(c, "PROSPEROEDEN", 216.0f, baseline(72.0f, 44.0f, theme::kBrand), theme::kBrand,
         theme::kText, Align::left, 3.0f);
    // Who is playing is named once there is more than one profile to be.
    const float edition = text(c, fill(tr("PS5 EDITION  /  {0}"), {version_}), 216.0f,
                               baseline(120.0f, 28.0f, theme::kSmall), theme::kSmall, Color::rgb(0x9fac9e),
                               Align::left, 1.0f);
    if (profiles_.size() > 1 && !playing_.empty())
        text_shrink(c, "/  " + playing_, 216.0f + edition + 16.0f, baseline(120.0f, 28.0f, theme::kSmall),
                    theme::kSmall, theme::kLimePale, 300.0f);
    static constexpr const char *kNav[] = {TR("Library"), TR("Settings"), TR("About")};
    Rect nav[3];
    float nav_right = kNavRight;
    for (int i = 2; i >= 0; --i)
    {
        const float width = std::max(kNavWidth, measure(tr(kNav[i]), theme::kText24) + 48.0f);
        nav[i] = {nav_right - width, 80.0f, width, 64.0f};
        nav_right -= width + 8.0f;
    }
    for (int i = 0; i < 3; ++i)
    {
        const Rect r = nav[i];
        const float f = focus(1 + i);
        list.push_opacity(i == 0 && !ready ? 0.4f : 1.0f);
        plate(c, kNavPlate, r, f);
        text(c, tr(kNav[i]), r.x + r.w * 0.5f, baseline(r.y, 60.0f, theme::kText24), theme::kText24,
             gfx::mix(theme::kMuted, theme::kText, f), Align::center);
        list.pop_opacity();
    }
    text(c, clock_, 1800.0f, baseline(92.0f, 40.0f, theme::kClock), theme::kClock,
         Color::rgb(0xc1cdc2), Align::right);
    list.rounded_rect({120.0f, 160.0f, 1680.0f, 1.0f}, 0.0f, theme::kText.with_alpha(0.12f));
    end_band();

    // ---- continue playing ----
    begin_band(1, 28.0f);
    glass(c, kHero, 14.0f, theme::kGlass.with_alpha(0.60f), kWhite.with_alpha(0.125f));
    cover(c, home_.last_cover, {168.0f, 272.0f, 336.0f, 336.0f}, 16.0f, 1.0f);
    text(c, tr("CONTINUE PLAYING"), 560.0f, baseline(264.0f, 30.0f, theme::kSmall), theme::kSmall,
         theme::kLime, Align::left, 3.0f);
    text_block(c, home_.last_file.empty() ? tr("Your next adventure") : home_.last_title, 560.0f,
               baseline(312.0f, 56.0f, theme::kDisplay), theme::kDisplay, 56.0f, theme::kText,
               1120.0f, 2);
    notice(c, home_.last_file.empty() ? tr("Choose a game from your library.") : home_.last_caption,
           560.0f, baseline(424.0f, 36.0f, theme::kText24), theme::kText24,
           home_.last_caption_warning ? theme::kWarning : Color::rgb(0xabb8ae), 1040.0f,
           home_.last_caption_warning);
    // The line under it ends before the controllers' label.
    const float info_width =
        kPadsRight - measure(tr("CONTROLLERS"), theme::kSmall, 3.0f) - 48.0f - 560.0f;
    if (!home_.last_language.empty())
        text_shrink(c,
                    fill(tr("Add-ons: {0}  /  Language: {1}"),
                         {addons_line(home_.last_addons, home_.last_mods, home_.last_mods_on),
                          home_.last_language}),
                    560.0f, baseline(462.0f, 30.0f, theme::kSmall), theme::kSmall,
                    Color::rgb(0xabb8ae), info_width);
    const char *first = continue_ready ? tr("Launch game") : tr("Open library");
    const char *second = tr("Game details");
    const float button = std::clamp(
        std::max(measure(first, theme::kText24), measure(second, theme::kText24)) + 64.0f,
        kButtonWidth, kButtonWidest);
    const Rect continue_rect{560.0f, 504.0f, button, 72.0f};
    const Rect details_rect{560.0f + button + 16.0f, 504.0f, button, 72.0f};
    const auto hero_button = [&](const Rect &r, const char *label, float f, bool enabled)
    {
        list.push_opacity(enabled ? 1.0f : 0.4f);
        begin_lift(r, f, 0.035f);
        plate(c, kButtonPlate, r, f);
        text_shrink(c, label, r.x + r.w * 0.5f, baseline(r.y, 70.0f, theme::kText24),
                    theme::kText24, theme::kText, r.w - 32.0f, Align::center);
        list.pop_transform();
        list.pop_opacity();
    };
    hero_button(continue_rect, first, focus(0), ready);
    hero_button(details_rect, second, focus(4), continue_ready);
    text(c, tr("Eden emulator for PlayStation 5"), 560.0f, baseline(616.0f, 30.0f, theme::kSmall),
         theme::kSmall, theme::kFaint);
    draw_controllers(c);
    end_band();

    // ---- recently played, or what needs attention ----
    begin_band(2, 28.0f);
    if (!home_.status.empty())
    {
        const Rect panel{120.0f, 728.0f, 1680.0f, 152.0f};
        const Color accent = home_.launch_failed ? theme::kWarning : theme::kLime;
        glass(c, panel, 10.0f, Color::rgb(0x0a1611, 0.62f), kWhite.with_alpha(0.08f), 0.6f);
        list.rounded_rect({panel.x, panel.y + 10.0f, 4.0f, panel.h - 20.0f}, 2.0f, accent);
        notice_block(c, home_.status, 148.0f, baseline(752.0f, 36.0f, theme::kText24),
                     theme::kText24, 36.0f, theme::kText, 1624.0f, 3, home_.launch_failed);
    }
    else
    {
        text(c, tr("RECENTLY PLAYED"), 120.0f, baseline(712.0f, 30.0f, theme::kSmall), theme::kSmall,
             Color::rgb(0xc6d2c7), Align::left, 3.0f);
        {
            const float f = focus(9);
            const float width =
                std::max(kViewAllWidth, measure(tr("VIEW ALL GAMES"), theme::kSmall) + 48.0f);
            const Rect r{1800.0f - width, 708.0f, width, 40.0f};
            plate(c, kNavPlate, r, f);
            text(c, tr("VIEW ALL GAMES"), r.x + r.w - 24.0f, baseline(r.y, 40.0f, theme::kSmall),
                 theme::kSmall, gfx::mix(Color::rgb(0xafbbaf), theme::kLime, f), Align::right);
        }
        if (home_.recents.empty())
            text(c, tr("Games you launch will appear here."), 120.0f,
                 baseline(784.0f, 36.0f, theme::kText24), theme::kText24, theme::kMuted);
        for (int i = 0; i < static_cast<int>(home_.recents.size()) && i < 4; ++i)
        {
            const Recent &recent = home_.recents[static_cast<std::size_t>(i)];
            const Rect r = tile_rect(i);
            const float f = focus(5 + i);
            begin_lift(r, f, 0.03f);
            plate(c, kTilePlate, r, f);
            cover(c, recent.cover, {r.x + 24.0f, r.y + 24.0f, 96.0f, 96.0f}, 10.0f, 0.5f);
            text_block(c, recent.title, r.x + 144.0f, baseline(r.y + 40.0f, 32.0f, theme::kText24),
                       theme::kText24, 32.0f, gfx::mix(Color::rgb(0xd7e1d9), theme::kTitle, f),
                       232.0f, 2);
            list.pop_transform();
        }
    }
    end_band();

    // ---- footer ----
    begin_band(3, 0.0f);
    list.rounded_rect({120.0f, 952.0f, 1680.0f, 1.0f}, 0.0f, theme::kText.with_alpha(0.10f));
    static constexpr Hint kHints[] = {
        {Pad::cross, TR("Select")}, {Pad::triangle, TR("Details")}, {Pad::dpad, TR("Navigate")}};
    std::string status = home_.system_status;
    if (services_.room_status().phase == Eden::Multiplayer::Phase::Connected)
        status += std::string{"  /  "} + tr("Multiplayer") + ": " + tr("Connected");
    const float status_width = text_shrink(c, status, 1800.0f, 989.0f, theme::kSmall,
                                          Color::rgb(0x9eac9f), 1120.0f, Align::right);
    draw_hints(c, kHints, 3, 120.0f, 982.0f, theme::kMuted, 1680.0f - status_width - 48.0f);
    end_band();
}

} // namespace pe::ui
