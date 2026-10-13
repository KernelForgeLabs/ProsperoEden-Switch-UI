// ProsperoEden Switch HOME frontend for v1.001.000.
// Adapted from the v1.001.000 launcher; retains its library and launch paths.
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pe/ui/launcher.hpp"
#include "pe/ui/switch_home_model.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace pe::ui
{

using audio::Cue;

namespace
{
// Four complete square covers and about half of a fifth at 1920x1080.
constexpr float kFirstX = switch_home::kCarouselLeft;
constexpr float kClipRight = switch_home::kCarouselRight;
constexpr float kCoverTop = 332.0f;
constexpr float kCoverSize = switch_home::kCoverWidth;
constexpr float kCoverStep = switch_home::kCoverStep;
constexpr float kDockY = 827.0f;
constexpr float kDockRadius = 48.0f;

struct DockAction
{
    Icon icon;
    const char *label;
};
constexpr DockAction kDock[] = {
    {Icon::library, TR("All Games")},
    {Icon::multiplayer, TR("Multiplayer")},
    {Icon::settings, TR("Settings")},
    {Icon::about, TR("About")},
    {Icon::gallery, TR("Themes")},
};

Rect cover_rect(int index, float scroll)
{
    return {kFirstX + static_cast<float>(index) * kCoverStep - scroll,
            kCoverTop, kCoverSize, kCoverSize};
}

float dock_x(int index)
{
    return 535.0f + 215.0f * static_cast<float>(index);
}

} // namespace

// Preserve upstream controller connection tracking and sound notifications.
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
            controller_lit_[index].snap(controller_lit_[index].target);
        else if (on != ((controllers_ >> player & 1u) != 0))
        {
            (on ? joined : left) = true;
            if (on)
                controller_pop_[index] = 1.0f;
        }
        controller_lit_[index].update(dt, 10.0f);
        controller_pop_[index] = std::max(0.0f, controller_pop_[index] - dt / 0.5f);
    }
    if ((joined || left) && selected_game_.empty())
        cue(joined ? Cue::saved : Cue::modal_close);
    controllers_ = now;
    controllers_known_ = true;
}

// One visible entry per local game file: Continue Playing, recent history,
// then the rest of the installed Library. The Library owns the launch paths.
std::vector<Recent> Launcher::switch_home_games() const
{
    std::vector<Recent> items;
    const auto add = [&](const std::string &file, const std::string &title, const std::string &image)
    {
        switch_home::append_unique(items, Recent{file, title, image});
    };
    if (home_.setup_ready && home_.last_exists)
        add(home_.last_file, home_.last_title, home_.last_cover);
    if (home_.setup_ready)
        for (const Recent &recent : home_.recents)
            add(recent.file, recent.title, recent.cover);
    for (const Game &game : games_)
        if (!game.remote)
            add(game.file, game.name, game.cover);
    return items;
}

void Launcher::open_game_settings_from_home(const std::string &file)
{
    // The full Library still owns this game's settings and actions.
    open(Screen::library, true);
    enter_library();
    for (int i = 0; i < static_cast<int>(games_.size()); ++i)
    {
        if (games_[static_cast<std::size_t>(i)].file != file)
            continue;
        library_.reset(library_.count, i);
        refresh_selected_game();
        sync_grid(true);
        press_library(Key::triangle);
        game_from_home_ = modal_ == Modal::game;
        return;
    }
}

void Launcher::press_home(Key key)
{
    const std::vector<Recent> items = switch_home_games();
    const int count = static_cast<int>(items.size());
    const int before_index = switch_home_index_;
    const bool before_dock = switch_home_dock_;
    const int before_action = switch_home_action_;

    // Library scans happen asynchronously. Clamp when games are added or removed.
    if (count > 0)
        switch_home_index_ = std::clamp(switch_home_index_, 0, count - 1);
    else
        switch_home_index_ = 0;

    switch (key)
    {
    case Key::left:
        if (switch_home_dock_)
        {
            if (switch_home_action_ > 0)
                --switch_home_action_;
            // Stop at the first action rather than focusing a hidden rail.
        }
        else if (switch_home_index_ > 0)
            --switch_home_index_;
        // The HOME menu has no visible side rail; the left edge is the start of the row.
        break;
    case Key::right:
        if (switch_home_dock_)
            switch_home_action_ = std::min(4, switch_home_action_ + 1);
        else if (switch_home_index_ + 1 < count)
            ++switch_home_index_;
        break;
    case Key::down:
        switch_home_dock_ = true;
        break;
    case Key::up:
        if (count > 0)
            switch_home_dock_ = false;
        break;
    case Key::triangle:
        if (count > 0)
            open_game_settings_from_home(items[static_cast<std::size_t>(switch_home_index_)].file);
        else
            cue(Cue::error);
        return;
    case Key::cross:
        press_ = 1.0f;
        if (!switch_home_dock_ && count > 0)
        {
            const Recent &entry = items[static_cast<std::size_t>(switch_home_index_)];
            const Rect position = cover_rect(switch_home_index_, switch_home_scroll_.value);
            launch_from_ = position;
            launch(entry.file, entry.title, entry.cover);
        }
        else if (switch_home_dock_)
        {
            switch (switch_home_action_)
            {
            case 0:
                go_section(1); // All Games
                break;
            case 1:
                go_section(2); // Multiplayer
                break;
            case 2:
                go_section(3); // Settings
                break;
            case 3:
                go_section(4); // About
                break;
            case 4:
                // Open the existing Themezer picker directly, not the Settings grid.
                // Triangle remains the shortcut to the selected game's details.
                open_modal(Modal::theme);
                option_ = theme_index() + 1;
                option_cursor_.snap(dialog_row_top(Modal::theme, option_));
                break;
            default:
                break;
            }
        }
        else
            go_section(home_.setup_ready ? 1 : 3);
        return;
    default:
        return;
    }
    if (before_index != switch_home_index_ || before_dock != switch_home_dock_ ||
        before_action != switch_home_action_)
        cue(Cue::focus);
}

// The new HOME screen uses the official theme palette, clock, artwork and
// controller glyphs. The old controller status pane is not needed here.
void Launcher::draw_home(Canvas &c)
{
    const Theme &t = th();
    gfx::DrawList &list = c.list;
    const std::vector<Recent> items = switch_home_games();
    const int count = static_cast<int>(items.size());
    const int selected = count > 0 ? std::clamp(switch_home_index_, 0, count - 1) : 0;
    const float scroll = switch_home_scroll_.value;

    // Only the highlighted game's title is shown, above the covers.
    if (count > 0)
    {
        const Recent &active = items[static_cast<std::size_t>(selected)];
        const Heavy heavy(c, 0.75f);
        text_shrink(c, active.title, kFirstX, baseline(232.0f, 65.0f, t.size_heading),
                    t.size_heading, t.title, 1420.0f);
    }
    else
    {
        text(c, tr("Choose a game from your library."), kFirstX,
             baseline(236.0f, 60.0f, t.size_heading), t.size_heading, t.title);
    }

    list.push_clip({kFirstX - 8.0f, kCoverTop - 18.0f, kClipRight - kFirstX + 8.0f,
                    kCoverSize + 43.0f});
    for (int i = 0; i < count; ++i)
    {
        const Rect r = cover_rect(i, scroll);
        // Only draw tiles within or moving toward the visible window.
        if (r.x + r.w < kFirstX - 20.0f || r.x > kClipRight + 20.0f)
            continue;
        const bool focused = !switch_home_dock_ && !rail_focus_ && selected == i;
        const float focus = focused ? 1.0f : 0.0f;
        const float scale = 1.0f + 0.012f * focus * motion();
        list.push_transform(scale, r.x + r.w * 0.5f, r.y + r.h * 0.5f, 0.0f, 0.0f);
        cover(c, items[static_cast<std::size_t>(i)].cover, r, 15.0f, 0.9f);
        if (focused)
            // Clean Switch-style focus: a thin outline, without upstream's animated halo.
            list.bordered_rect({r.x - 3.0f, r.y - 3.0f, r.w + 6.0f, r.h + 6.0f},
                               18.0f, Color{0.0f, 0.0f, 0.0f, 0.0f}, 3.0f, t.accent_pale);
        list.pop_transform();
    }
    list.pop_clip();

    // No additional dock plate: leave the wallpaper visible beneath the icons.
    for (int i = 0; i < 5; ++i)
    {
        const bool focused = switch_home_dock_ && !rail_focus_ && switch_home_action_ == i;
        const float x = dock_x(i);
        const Color face = focused ? t.accent : t.panel.with_alpha(0.79f);
        const Color ink = focused ? t.on_accent : t.title;
        list.circle(x, kDockY, kDockRadius, face);
        if (focused)
            list.bordered_rect({x - kDockRadius - 4.0f, kDockY - kDockRadius - 4.0f,
                                2.0f * kDockRadius + 8.0f, 2.0f * kDockRadius + 8.0f},
                               kDockRadius + 4.0f, Color{0.0f, 0.0f, 0.0f, 0.0f},
                               2.4f, t.title);
        icon(c, kDock[i].icon, x, kDockY, 45.0f, ink);
        text_shrink(c, tr(kDock[i].label), x, baseline(kDockY + 64.0f, 33.0f, t.size_small),
                    t.size_small, t.title, 186.0f, Align::center);
    }

    // The official glyph painter keeps the hints consistent across screens.
    static constexpr Hint hints[] = {
        {Pad::cross, TR("Select")},
        {Pad::leftright, TR("Browse")},
        {Pad::updown, TR("Games / Menu")},
        {Pad::triangle, TR("Game settings")},
    };
    // Avoid a permanent "N games installed / Firmware ready" label. Only report
    // an actionable missing-setup warning, never a success/status counter.
    draw_footer(c, hints, 4,
                home_.setup_ready ? std::string_view{} : std::string_view{home_.system_status});
}

} // namespace pe::ui
