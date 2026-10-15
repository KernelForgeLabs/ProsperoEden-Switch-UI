// ProsperoEden - Launcher settings: the category list and its dialogs.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pe/ui/launcher.hpp"
#include "pe/ui/switch_bootstrap.hpp"
#include "pe/ui/switch_pages.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace pe::ui
{

using audio::Cue;

namespace
{

constexpr Rect kDialog{550.0f, 180.0f, 820.0f, 720.0f};
// The categories: a tile each, four to a row; under them, what the focused one is about.
enum Category
{
    kProfiles,
    kVideo,
    kPerformance,
    kAudio,
    kControls,
    kAccessibility,
    kDiagnostics,
    kFiles,
    kDownloads,
    kSaveSync,
    kLanguage,
    kTheme,
    kCategoryCount,
};
constexpr const char *kCategories[kCategoryCount] = {
    TR("Profiles"), TR("Video"), TR("Performance"), TR("Audio"), TR("Controls"), TR("Accessibility"),
    TR("Diagnostics"), TR("Game files"), TR("Downloads"), TR("Save sync"), TR("Language"), TR("Theme")};
constexpr Icon kIcons[kCategoryCount] = {
    Icon::profile, Icon::video, Icon::performance, Icon::audio, Icon::controls, Icon::accessibility,
    Icon::diagnostics, Icon::files, Icon::downloads, Icon::sync, Icon::language, Icon::theme};
// v5.8: Switch-like continuous settings navigation and a flat details pane.
// Preserve category indices and row order: press_settings() owns all actions.
constexpr int kTileColumns = 1;
constexpr int kTileRows = kCategoryCount;
constexpr float kTileGap = 0.0f;
constexpr float kTileWidth = 602.0f;
constexpr float kTileHeight = 64.0f;
constexpr float kTilesTop = 146.0f;
constexpr Rect kStrip{836.0f, 152.0f, 954.0f, 770.0f};
Rect tile_rect(int index)
{
    return {layout::kLeft, kTilesTop + (kTileHeight + kTileGap) * static_cast<float>(index),
            kTileWidth, kTileHeight};
}

// The Video dialog's rows, and the window that shows five of them (placed as a game's settings
// are).
enum VideoRow : int
{
    video_renderer,
    video_output,
    video_resolution,
    video_filter,
    video_astc,
    video_refresh,
    video_overlay,
    // Frame generation: not shown in a build without it (Services::frame_gen_state).
    video_frame_gen,
    video_frame_gen_target,
    video_frame_gen_multiplier,
    kVideoRows,
};
constexpr int kFrameGenTargets = 6;     // Auto, 60, 90, 120, 144, 240 Hz (settings_store.h)
constexpr int kFrameGenMultipliers = 3; // 2x, 3x, 4x
constexpr float kVideoRowsTop = 334.0f;
constexpr float kVideoRowPitch = 96.0f;
constexpr float kVideoRowHeight = 94.0f;
constexpr int kVideoRowsShown = 5;
constexpr Rect kVideoWindow{592.0f, kVideoRowsTop, 736.0f,
                            kVideoRowPitch * (kVideoRowsShown - 1) + kVideoRowHeight};
// The Performance dialog's switches, and the window that shows four of them over what the
// highlighted one does.
enum PerformanceRow : int
{
    speed_block_list,
    speed_async_shaders,
    speed_fast_gpu,
    speed_unsafe_cpu,
    speed_unsafe_dma,
    speed_reactive_flushing,
    speed_skip_invalidation,
    kPerformanceRows,
};
constexpr int kPerformanceRowsShown = 4;
constexpr Rect kPerformanceWindow{592.0f, kVideoRowsTop, 736.0f,
                                  kVideoRowPitch * (kPerformanceRowsShown - 1) + kVideoRowHeight};
// The sizes of Preferences::output, as every language writes them.
constexpr const char *kOutputs[] = {"1080p", "1440p", "2160p"};

constexpr const char *kFrameGenTargetNames[] = {"", "60", "90", "120", "144", "240"};
constexpr const char *kFrameGenMultiplierNames[] = {"2x", "3x", "4x"};

const char *on_off(bool value)
{
    return value ? tr("On") : tr("Off");
}

std::string percent(int value)
{
    return fill(tr("{0}%"), {std::to_string(value)});
}

std::string pick(const std::vector<std::string> &values, int index)
{
    return index >= 0 && index < static_cast<int>(values.size()) ?
               values[static_cast<std::size_t>(index)] : std::string{"-"};
}

// A path that fits a label: the end is what tells folders apart.
std::string short_path(const std::string &path, std::size_t limit)
{
    return path.size() <= limit ? path : "..." + path.substr(path.size() - (limit - 3));
}

} // namespace

void Launcher::press_settings(Key key)
{
    int &at = settings_.selected;
    const int before = at;
    switch (key)
    {
    case Key::circle:
        go_section(0);
        return;
    case Key::up:
        if (at >= kTileColumns)
            at -= kTileColumns;
        break;
    case Key::down:
        if (at < kTileColumns * (kTileRows - 1))
            at += kTileColumns;
        break;
    case Key::left:
    case Key::right:
        // Switch Settings moves vertically; L1/R1 changes launcher sections.
        break;
    case Key::cross:
        press_ = 1.0f;
        switch (at)
        {
        case kFiles:
            open(Screen::files, true);
            enter_files();
            break;
        case kLanguage:
            open(Screen::language, true);
            enter_language();
            break;
        case kProfiles:
            open_profiles();
            break;
        case kDownloads:
            open_sources();
            break;
        case kSaveSync:
            open_sync_setup();
            break;
        case kVideo:
            open_modal(Modal::video);
            video_rows_.visible = kVideoRowsShown;
            video_rows_.pitch = kVideoRowPitch;
            video_rows_.reset(dialog_rows(Modal::video), 0);
            break;
        case kPerformance:
            open_modal(Modal::performance);
            performance_rows_.visible = kPerformanceRowsShown;
            performance_rows_.pitch = kVideoRowPitch;
            performance_rows_.reset(kPerformanceRows, 0);
            break;
        case kAudio:
            open_modal(Modal::audio);
            break;
        case kControls:
            open_modal(Modal::controls);
            break;
        case kAccessibility:
            open_modal(Modal::accessibility);
            break;
        case kTheme:
            open_modal(Modal::theme);
            option_ = theme_index() + 1;
            option_cursor_.snap(dialog_row_top(Modal::theme, option_));
            break;
        default:
            open_modal(Modal::diagnostics);
            break;
        }
        return;
    default:
        return;
    }
    if (at != before)
    {
        // What the tile is about comes in again under the tiles.
        section_.value = 0.0f;
        section_.velocity = 0.0f;
        message_.clear();
        cue(Cue::focus);
    }
}

void Launcher::draw_settings(Canvas &c)
{
    gfx::DrawList &list = c.list;
    const Theme &t = th();
    draw_frame(c, tr("Settings"), tr("Fine-tune your experience"));

    // What each category is set to, in a few words.
    const int switches = prefs_.block_list + prefs_.async_shaders + prefs_.fast_gpu + prefs_.unsafe_cpu +
                         prefs_.unsafe_dma + prefs_.reactive_flushing + prefs_.skip_invalidation;
    const std::string summaries[kCategoryCount] = {
        playing_,
        pick(services_.resolution_keys(), prefs_.resolution) + "  /  " + pick(services_.filter_labels(), prefs_.filter) +
            "  /  " + output_name(prefs_.output) + "  /  " + hertz(prefs_.refresh),
        fill(tr("{0} of {1} on"), {std::to_string(switches), std::to_string(static_cast<int>(kPerformanceRows))}),
        prefs_.mute ? tr("Muted") : percent(prefs_.volume),
        prefs_.vibration ? tr("Vibration on") : tr("Vibration off"),
        prefs_.large_text || prefs_.high_contrast || prefs_.reduce_motion ? tr("On") : tr("Off"),
        prefs_.detailed_logging ? tr("Detailed logs on") : home_.setup_ready ? tr("Ready") : tr("Needs attention"),
        short_path(services_.files_folder(), 28),
        !sources_.configured ? std::string{tr("Not set up")} :
        sources_.list.size() == 1 ? sources_.list.front().name :
                                    fill(tr("{0} sources"), {std::to_string(sources_.list.size())}),
        [&] {
            int linked = 0;
            for (const SaveSyncProfile &profile : sync_setup_.profiles)
                linked += profile.linked ? 1 : 0;
            return linked == 0 ? std::string{tr("Off")} : fill(tr("{0} of {1} profiles"), {std::to_string(linked),
                                                                                           std::to_string(sync_setup_.profiles.size())});
        }(),
        pick(services_.language_labels(), prefs_.language),
        t.name,
    };
    // A single continuous navigation surface, not a grid of translucent cards.
    // The art remains visible between the category list and the details pane.
    const Rect navigation{layout::kLeft, kTilesTop, kTileWidth,
                          kTileHeight * static_cast<float>(kCategoryCount)};
    list.rounded_rect(navigation, 3.0f,
                      look().high_contrast ? t.panel : t.panel.with_alpha(0.37f));
    for (int i = 0; i < kCategoryCount; ++i)
    {
        const Rect r = tile_rect(i);
        const bool selected = i == settings_.selected && !rail_focus_;
        // Quiet section separators keep the left side readable over vivid wallpapers.
        if (i != kCategoryCount - 1)
            list.rounded_rect({r.x + 26.0f, r.y + r.h - 1.0f,
                               r.w - 52.0f, 1.0f}, 0.0f,
                              t.rule.with_alpha(look().high_contrast ? 0.65f : 0.27f));
        if (selected)
        {
            const Rect focus{r.x + 3.0f, r.y + 2.0f, r.w - 6.0f, r.h - 4.0f};
            list.bordered_rect(focus, 3.0f,
                               look().high_contrast ? t.row_focus :
                                                      t.row_focus.with_alpha(0.76f),
                               2.5f, t.accent_pale);
            list.rounded_rect({r.x + 5.0f, r.y + 11.0f, 4.0f, r.h - 22.0f},
                              1.0f, t.accent_pale);
        }
        icon(c, kIcons[i], r.x + 40.0f, r.y + r.h * 0.5f, 23.0f,
             selected ? t.accent_pale : t.copy);
        text_shrink(c, tr(kCategories[i]), r.x + 79.0f,
                    baseline(r.y, r.h, 26.0f), 26.0f,
                    selected ? t.title : t.body, r.w - 112.0f);
    }

    // A quiet, unframed content surface echoes Switch Settings. Keep a stronger
    // surface in high-contrast mode rather than letting text disappear into art.
    if (look().high_contrast)
        list.rounded_rect(kStrip, 2.0f, t.panel);
    else
        list.rounded_rect(kStrip, 2.0f, t.panel.with_alpha(0.23f));
    list.rounded_rect({kStrip.x - 19.0f, kStrip.y + 8.0f, 1.0f,
                       kStrip.h - 16.0f}, 0.0f,
                      t.rule.with_alpha(0.58f));
    const int selected = std::clamp(settings_.selected, 0, kCategoryCount - 1);
    icon(c, kIcons[selected], kStrip.x + 66.0f, kStrip.y + 70.0f, 44.0f,
         t.accent_pale);
    {
        const Heavy heavy(c);
        text_shrink(c, tr(kCategories[selected]), kStrip.x + 112.0f,
                    baseline(kStrip.y + 38.0f, 66.0f, t.size_lead), t.size_lead,
                    t.title, kStrip.w - 160.0f);
    }
    list.rounded_rect({kStrip.x + 40.0f, kStrip.y + 133.0f,
                       kStrip.w - 80.0f, 1.0f}, 0.0f,
                      t.rule.with_alpha(0.52f));
    text_shrink(c, tr("CURRENT SETTING"), kStrip.x + 40.0f,
                baseline(kStrip.y + 158.0f, 30.0f, t.size_small),
                t.size_small, t.meta, kStrip.w - 80.0f);
    text_shrink(c, summaries[selected], kStrip.x + 40.0f,
                baseline(kStrip.y + 204.0f, 48.0f, t.size_heading),
                t.size_heading, t.title, kStrip.w - 80.0f);
    list.rounded_rect({kStrip.x + 40.0f, kStrip.y + 295.0f,
                       kStrip.w - 80.0f, 1.0f}, 0.0f,
                      t.rule.with_alpha(0.38f));
    const char *about = "";
    switch (settings_.selected)
    {
    case kProfiles:
        about = tr("Who is playing. Each profile keeps its own save data and settings.");
        break;
    case kVideo:
        about = tr("Graphics backend and how games are scaled to your TV.");
        break;
    case kPerformance:
        about = tr("Faster games, at some cost in accuracy.");
        break;
    case kAudio:
        about = tr("Game volume, and the sounds of this menu.");
        break;
    case kControls:
        about = tr("Vibration, button mapping and the shortcuts during a game.");
        break;
    case kAccessibility:
        about = tr("Make the menu easier to see and follow.");
        break;
    case kDiagnostics:
        about = tr("Setup status and detailed logs.");
        break;
    case kFiles:
        about = tr("The folder that holds your keys, firmware and games.");
        break;
    case kDownloads:
        about = tr("Games on your network, downloaded when you play them.");
        break;
    case kSaveSync:
        about = tr("Each profile's save data on a server of its own, synced before and after a game.");
        break;
    case kLanguage:
        about = tr("The language games use when they offer it.");
        break;
    case kTheme:
        about = tr(t.summary);
        break;
    default:
        break;
    }
    text_block(c, about, kStrip.x + 40.0f,
               baseline(kStrip.y + 326.0f, 38.0f, t.size_text),
               t.size_text, 42.0f, t.copy, kStrip.w - 80.0f, 5, kShrink);
    text_shrink(c, version_, kStrip.x + 40.0f,
                baseline(kStrip.y + kStrip.h - 52.0f, 30.0f, t.size_small),
                t.size_small, t.faint, kStrip.w - 80.0f);

    static constexpr Hint kHints[] = {
        {Pad::cross, TR("Select")}, {Pad::circle, TR("Back")}, {Pad::l1, TR("Sections"), Pad::r1}};
    draw_footer(c, kHints, 3, home_.system_status);
}

const char *Launcher::performance_about(int row)
{
    static constexpr const char *kAbout[kPerformanceRows] = {
        TR("Compiles the code a game used before as it starts, so new areas stutter less."),
        TR("Draws an effect once its shader is ready instead of pausing. Things can be missing "
           "for a moment."),
        TR("Lowest GPU accuracy: faster in demanding games, and graphics can be wrong."),
        TR("Less exact floating-point math: faster, and a few games misbehave."),
        TR("Less exact memory transfers to the GPU: faster, and a few games show wrong graphics."),
        TR("Keeps what a game reads back from the GPU exact. Off is faster, and some effects "
           "break."),
        TR("Skips some checks when a game changes memory the GPU uses: faster, and textures "
           "can be stale.")};
    return kAbout[std::clamp(row, 0, kPerformanceRows - 1)];
}

// ---------------------------------------------------------------- dialogs

int Launcher::dialog_rows(Modal modal) const
{
    switch (modal)
    {
    case Modal::video:
        return services_.frame_gen_state() == 0 ? static_cast<int>(video_frame_gen) : static_cast<int>(kVideoRows);
    case Modal::performance:
        return kPerformanceRows;
    case Modal::audio:
    case Modal::accessibility:
        return 3;
    case Modal::theme:
        return 1 + static_cast<int>(themes().size());
    case Modal::game:
        // Console mode, video, performance, audio, controls, system, language, mods; save data in
        // builds that move saves.
        // A game a download source has can be deleted from the console (and downloaded again).
        return (services_.save_transfer_available() ? 9 : 8) +
               (library_.selected < static_cast<int>(games_.size()) &&
                        !games_[static_cast<std::size_t>(library_.selected)].sources.empty() &&
                        !games_[static_cast<std::size_t>(library_.selected)].remote ? 1 : 0);
    case Modal::controls:     // vibration, the button mapping
    case Modal::diagnostics:  // detailed logging, logs written at once
        return 2;
    default:
        return 1;
    }
}

float Launcher::dialog_row_top(Modal modal, int row) const
{
    switch (modal)
    {
    case Modal::audio:
    case Modal::accessibility:
    case Modal::theme:
        return 370.0f + 102.0f * static_cast<float>(row);
    case Modal::performance: // four of its rows show; the list scrolls to the others
        return kVideoRowsTop + kVideoRowPitch * static_cast<float>(row) - performance_rows_.scroll();
    case Modal::video: // five of its rows show; the list scrolls to the others
        return kVideoRowsTop + kVideoRowPitch * static_cast<float>(row) - video_rows_.scroll();
    case Modal::game:
        return 334.0f + 96.0f * static_cast<float>(row);
    case Modal::controls: // under the shortcuts
        return 560.0f + 96.0f * static_cast<float>(row);
    case Modal::diagnostics: // under the setup's state
        return 500.0f + 102.0f * static_cast<float>(row);
    default:
        return 670.0f;
    }
}

void Launcher::press_dialog(Key key)
{
    const int rows = dialog_rows(modal_);
    const bool adjust = key == Key::left || key == Key::right;
    const bool activate = key == Key::cross;
    const int step = key == Key::left ? -1 : 1;
    if (modal_ == Modal::theme && (theme_keyboard_.valid() || theme_download_.valid())) {
        if (key == Key::circle) {
            theme_stop_ = true;
            say(tr("Cancelling Themezer import..."));
        }
        return;
    }
    // v5.6.1: Handle the page editor before the regular Themes dialog keys.
    // The v5.6 artwork/drawing changes alone did not wire up Triangle or
    // dispatch the editor's directional, reset and back controls.
    if (modal_ == Modal::theme && theme_mapping_open_)
    {
        if (key == Key::circle)
        {
            theme_mapping_open_ = false;
            theme_mapping_id_.clear();
            textures_.invalidate_page_art();
            message_.clear();
            cue(Cue::back);
            return;
        }
        if (key == Key::up || key == Key::down)
        {
            const int count = static_cast<int>(switch_pages::kEditableRoles.size());
            theme_mapping_row_ = (theme_mapping_row_ + (key == Key::down ? 1 : count - 1)) % count;
            textures_.invalidate_page_art();
            message_.clear();
            cue(Cue::focus);
            return;
        }
        if (key != Key::left && key != Key::right && key != Key::cross && key != Key::square)
            return;

        const std::string root = switch_bootstrap::root();
        const std::string_view role =
            switch_pages::kEditableRoles[static_cast<std::size_t>(theme_mapping_row_)];
        std::string next = "auto";
        if (key != Key::square)
        {
            std::vector<std::string> choices{"auto"};
            const auto installed = switch_pages::available_assets(root, theme_mapping_id_);
            choices.insert(choices.end(), installed.begin(), installed.end());
            const std::string current = switch_pages::override_for(root, theme_mapping_id_, role);
            const auto found = std::find(choices.begin(), choices.end(), current);
            const int position = found == choices.end() ? 0 : static_cast<int>(found - choices.begin());
            const int count = static_cast<int>(choices.size());
            const int direction = key == Key::left ? -1 : 1; // Cross advances, too.
            next = choices[static_cast<std::size_t>((position + count + direction) % count)];
        }
        if (switch_pages::set_override(root, theme_mapping_id_, role, next))
        {
            textures_.invalidate_page_art(); // Reread edited page even when its role is unchanged.
            message_.clear();
            cue(Cue::toggle);
        }
        else
        {
            say(tr("Could not save that screen background."), true);
            cue(Cue::error);
        }
        return;
    }
    if (modal_ == Modal::theme && key == Key::triangle)
    {
        const auto all = themes();
        const int index = option_ - 1; // First dialog entry installs new themes.
        if (index < 0 || index >= static_cast<int>(all.size()))
        {
            say(tr("Highlight an installed theme pack to customize it."), true);
            cue(Cue::error);
            return;
        }
        const std::string id = all[static_cast<std::size_t>(index)].id;
        if (id != th().id)
        {
            say(tr("Apply this pack first, then press Triangle to customize it."), true);
            cue(Cue::error);
            return;
        }
        const std::string root = switch_bootstrap::root();
        if (!switch_pages::pack_directory(root, id) ||
            switch_pages::available_assets(root, id).empty())
        {
            say(tr("Only installed full Themezer packs can be customized."), true);
            cue(Cue::error);
            return;
        }
        theme_mapping_id_ = id;
        theme_mapping_row_ = 0;
        theme_mapping_open_ = true;
        theme_delete_confirm_.clear();
        textures_.invalidate_page_art();
        message_.clear();
        cue(Cue::modal_open);
        return;
    }
    if (key == Key::circle)
    {
        theme_delete_confirm_.clear();
        close_modal();
        return;
    }
    if ((key == Key::up || key == Key::down) && (modal_ == Modal::video || modal_ == Modal::performance))
    {
        ListView &rows_view = modal_ == Modal::video ? video_rows_ : performance_rows_;
        if (rows_view.move(key == Key::down ? 1 : -1))
        {
            option_ = rows_view.selected;
            message_.clear();
            cue(Cue::focus);
        }
        return;
    }
    if (modal_ == Modal::theme && key == Key::square) {
        // Row zero is the Themezer Quick ID installer, not a theme.
        // Only Switch Dark is bundled. The old <= 2 guard was left over
        // from the two-theme (Canopy + Switch Dark) selection list and
        // incorrectly protected the first downloaded Themezer theme.
        if (option_ <= 0) return;
        const auto all = themes();
        const int index = option_ - 1;
        if (index < 0 || index >= static_cast<int>(all.size())) return;
        const std::string id = all[static_cast<std::size_t>(index)].id;
        if (id == "switch-dark" || id == "legacy") {
            say(tr("Bundled themes cannot be deleted."), true);
            return;
        }
        if (id == th().id || id == switch_bootstrap::selected(switch_bootstrap::root())) {
            say(tr("Select another theme before deleting this one."), true);
            return;
        }
        if (theme_delete_confirm_ != id) {
            theme_delete_confirm_ = id;
            say(tr("Press Square again to delete this inactive theme."));
        } else {
            theme_delete_confirm_.clear();
            const bool deleted = switch_bootstrap::remove_installed(switch_bootstrap::root(), id);
            if (deleted) {
                refresh_switch_themes();
                textures_.clear_theme_previews();
                option_ = std::clamp(option_, 0, dialog_rows(Modal::theme) - 1);
            }
            say(deleted ? tr("Theme deleted.") : tr("Could not delete theme. It may contain other files."), !deleted);
            cue(deleted ? Cue::saved : Cue::error);
        }
        return;
    }
    if ((key == Key::up || key == Key::down) && rows > 1)
    {
        option_ = (option_ + (key == Key::down ? 1 : rows - 1)) % rows;
        theme_delete_confirm_.clear();
        message_.clear();
        cue(Cue::focus);
        return;
    }
    if (!adjust && !activate)
        return;
    if (modal_ == Modal::theme)
    {
        // Row zero is an actual network installer, not a placeholder.
        if (!activate) return;
        if (option_ == 0) return begin_switch_theme_import();
        const Theme &chosen = themes()[static_cast<std::size_t>(std::clamp(option_ - 1, 0, rows - 2))];
        const std::string had = prefs_.theme;
        prefs_.theme = chosen.id;
        const bool saved = save_preferences(true);
        if (saved)
        {
            use_theme(chosen.id);
            remember_switch_theme(chosen.id);
            theme_reload_pending_ = true;
        }
        else
            prefs_.theme = had;
        cue(saved ? Cue::saved : Cue::error);
        return;
    }

    const Preferences before = prefs_;
    Cue sound = Cue::toggle;
    switch (modal_)
    {
    case Modal::video:
        if (option_ == video_renderer)
            prefs_.renderer = prefs_.renderer != 0 ? 0 : 1;
        else if (option_ == video_output)
        {
            // The menu follows at once (the frontend opens its display again at this size).
            const int count = static_cast<int>(std::size(kOutputs));
            prefs_.output = (std::clamp(prefs_.output, 0, count - 1) + step + count) % count;
        }
        else if (option_ == video_resolution)
        {
            const int count = static_cast<int>(services_.resolution_labels().size());
            prefs_.resolution = (prefs_.resolution + step + count) % count;
        }
        else if (option_ == video_filter)
        {
            const int count = static_cast<int>(services_.filter_labels().size());
            prefs_.filter = (prefs_.filter + step + count) % count;
        }
        else if (option_ == video_astc)
            prefs_.astc = prefs_.astc != 0 ? 0 : 1;
        else if (option_ == video_refresh)
            prefs_.refresh = prefs_.refresh != 0 ? 0 : 1;
        else if (option_ >= video_frame_gen && services_.frame_gen_state() != 2)
        {
            say(tr("Frame generation needs Lossless.dll, from Lossless Scaling, in the lossless folder."), true);
            cue(Cue::error);
            return;
        }
        else if (option_ == video_frame_gen)
            prefs_.frame_gen = !prefs_.frame_gen;
        else if (option_ == video_frame_gen_target)
            prefs_.frame_gen_target =
                (std::clamp(prefs_.frame_gen_target, 0, kFrameGenTargets - 1) + step + kFrameGenTargets) % kFrameGenTargets;
        else if (option_ == video_frame_gen_multiplier)
            prefs_.frame_gen_multiplier = (std::clamp(prefs_.frame_gen_multiplier, 0, kFrameGenMultipliers - 1) + step +
                                           kFrameGenMultipliers) % kFrameGenMultipliers;
        else
            prefs_.hud = !prefs_.hud;
        break;
    case Modal::audio:
        if (option_ == 0)
        {
            if (!adjust)
                return;
            prefs_.volume = std::clamp(prefs_.volume + 10 * step, 0, 100);
            sound = Cue::slider;
        }
        else if (option_ == 1)
            prefs_.mute = !prefs_.mute;
        else
        {
            if (!adjust)
                return;
            prefs_.menu_volume = std::clamp(prefs_.menu_volume + 10 * step, 0, 100);
            sound = Cue::slider;
        }
        break;
    case Modal::performance:
    {
        bool *const switches[kPerformanceRows] = {
            &prefs_.block_list, &prefs_.async_shaders,     &prefs_.fast_gpu, &prefs_.unsafe_cpu,
            &prefs_.unsafe_dma, &prefs_.reactive_flushing, &prefs_.skip_invalidation};
        bool &value = *switches[std::clamp(option_, 0, kPerformanceRows - 1)];
        value = !value;
        break;
    }
    case Modal::controls:
        if (option_ == 1)
        {
            // The button mapping has its own list (game_options.cpp).
            if (activate)
                open_mapping(false);
            return;
        }
        prefs_.vibration = !prefs_.vibration;
        break;
    case Modal::accessibility:
        if (option_ == 0)
            prefs_.large_text = !prefs_.large_text;
        else if (option_ == 1)
            prefs_.high_contrast = !prefs_.high_contrast;
        else
            prefs_.reduce_motion = !prefs_.reduce_motion;
        break;
    case Modal::diagnostics:
        if (option_ == 0)
            prefs_.detailed_logging = !prefs_.detailed_logging;
        else
            prefs_.immediate_logs = !prefs_.immediate_logs;
        break;
    default:
        return;
    }
    // The launcher's look changes on the spot: that is its own confirmation.
    if (!save_preferences(modal_ == Modal::accessibility))
    {
        prefs_ = before;
        sound = Cue::error;
    }
    else if (modal_ == Modal::video && option_ == video_refresh && prefs_.refresh == 1)
    {
        // 120 Hz is a request: the display has the last word.
        say(tr("Saved. A display that cannot show 120 Hz stays at 60 Hz."));
    }
    else if (modal_ == Modal::video && option_ == video_astc && prefs_.astc == 1)
    {
        // Held decoded, these textures take four times the memory they take compressed.
        say(tr("Saved. Full quality uses four times the graphics memory for these textures and can run out "
               "above 1x."));
    }
    apply_look();
    cue(sound);
}

void Launcher::draw_dialog(Canvas &c, Modal modal, float open)
{
    gfx::DrawList &list = c.list;
    list.push_opacity(open);
    list.push_transform(1.0f - 0.03f * (1.0f - open) * motion(), 960.0f, 540.0f, 0.0f,
                        (1.0f - open) * 26.0f * motion());
    // v5.9: Flat settings sheet.  Only the active row gets a focus outline;
    // high-contrast mode retains a fully opaque backdrop for readability.
    // Preserve all modal and row coordinates so controller navigation is unchanged.
    const Rect frame = modal == Modal::theme ?
        Rect{550.0f, 180.0f, 1210.0f, 720.0f} : kDialog;
    list.rounded_rect(frame, 3.0f, look().high_contrast ? th().panel :
                      th().panel.with_alpha(modal == Modal::theme ? 0.80f : 0.73f));
    list.rounded_rect({frame.x + 40.0f, 319.0f, frame.w - 80.0f, 1.0f},
                      0.0f, th().rule.with_alpha(0.42f));

    const char *title = "";
    const char *copy = "";
    switch (modal)
    {
    case Modal::video:
        title = tr("Video");
        copy = tr("How games are drawn and scaled to your TV.");
        break;
    case Modal::performance:
        title = tr("Performance");
        copy = tr("Faster games, at some cost in accuracy.");
        break;
    case Modal::audio:
        title = tr("Audio");
        copy = tr("Game audio; PS5 system-menu music is unchanged.");
        break;
    case Modal::controls:
        title = tr("Controls");
        copy = tr("Controller shortcuts and supported features.");
        break;
    case Modal::accessibility:
        title = tr("Accessibility");
        copy = tr("Make the menu easier to see and follow.");
        break;
    case Modal::theme:
        title = theme_mapping_open_ ? tr("Screen backgrounds") : tr("Themes");
        copy = theme_mapping_open_ ?
            tr("Pick a screen, then change its wallpaper. Background updates are previewed.") :
            tr("Cross: apply   Triangle: customize active pack   Square: delete inactive theme");
        break;
    default:
        title = tr("Diagnostics");
        copy = tr("Detailed logs apply to the next game launch.");
        break;
    }
    text_shrink(c, title, 592.0f, baseline(218.0f, 62.0f, th().size_display), th().size_display,
                th().title, 736.0f);
    text_shrink(c, copy, 592.0f, baseline(291.0f, 32.0f, th().size_small), th().size_small,
                th().copy, 736.0f);

    const int rows = modal == Modal::theme && theme_mapping_open_ ?
        static_cast<int>(switch_pages::kEditableRoles.size()) : dialog_rows(modal);
    // Video's rows scroll in a window of five and Performance's in one of four; the other dialogs
    // show all of theirs.
    const bool scrolls = modal == Modal::video || modal == Modal::performance;
    const ListView &rows_view = modal == Modal::performance ? performance_rows_ : video_rows_;
    const Rect &window = modal == Modal::performance ? kPerformanceWindow : kVideoWindow;
    const bool theme_scroll = modal == Modal::theme;
    const int theme_selected = theme_mapping_open_ && modal == Modal::theme ? theme_mapping_row_ : option_;
    const int theme_first = theme_scroll ? std::clamp(theme_selected - 2, 0, std::max(0, rows - 4)) : 0;
    const int first = scrolls ? rows_view.first_row() : theme_first;
    const int last = scrolls ? rows_view.last_row() : theme_scroll ? std::min(rows - 1, theme_first + 3) : rows - 1;
    if (scrolls)
        list.push_clip({window.x - 24.0f, window.y - 6.0f, window.w + 48.0f, window.h + 12.0f});
    for (int row = first; row <= last; ++row)
    {
        list.push_opacity(scrolls ? rows_view.row_alpha(row, kVideoRowHeight) : 1.0f);
        const Rect r{592.0f, dialog_row_top(modal, theme_scroll ? row - theme_first : row), 736.0f, 94.0f};
        // No individual frosted cards: a quiet rule separates neighboring rows.
        if (row < last)
            list.rounded_rect({r.x + 24.0f, r.y + r.h + 2.0f,
                               r.w - 48.0f, 1.0f}, 0.0f,
                              th().rule.with_alpha(look().high_contrast ? 0.62f : 0.35f));
        list.pop_opacity();
    }
    const Rect focus_row{592.0f,
                         scrolls ? kVideoRowsTop + rows_view.cursor() - rows_view.scroll() :
                         theme_scroll ? dialog_row_top(modal, theme_selected - theme_first) : option_cursor_.value,
                         736.0f, 94.0f};
    // One crisp Switch-style selection rectangle for every settings subpage.
    list.bordered_rect(focus_row, 3.0f,
                       look().high_contrast ? th().row_focus :
                                                th().row_focus.with_alpha(0.56f),
                       2.0f, th().accent_pale);

    // A row's name takes what its control (`taken` wide, at the right) leaves of the row.
    const auto label = [&](int row, const char *value, float taken)
    {
        text_shrink(c, value, 628.0f, baseline(dialog_row_top(modal, theme_scroll ? row - theme_first : row), 94.0f, th().size_text),
                    th().size_text, th().value, 664.0f - taken - 28.0f);
    };
    const auto choice = [&](int row, const std::string &value)
    {
        return chooser(c, value, 1296.0f,
                       baseline(dialog_row_top(modal, theme_scroll ? row - theme_first : row), 94.0f, th().size_text),
                       row == option_ ? 1.0f : 0.0f, th().accent_pale);
    };
    constexpr float kToggle = 64.0f;
    const auto row_centre = [&](int row) { return dialog_row_top(modal, row) + 47.0f; };
    const float knob = tween::clamp01(switches_[0].value);

    switch (modal)
    {
    case Modal::video:
    {
        const std::string values[] = {
            prefs_.renderer != 0 ? tr("Vulkan (recommended)") : "OpenGL",
            output_name(prefs_.output),
            pick(services_.resolution_labels(), prefs_.resolution),
            pick(services_.filter_labels(), prefs_.filter),
            pick(services_.astc_labels(), prefs_.astc),
            hertz(prefs_.refresh),
            std::string{}, // FPS overlay: a switch
            on_off(prefs_.frame_gen),
            prefs_.frame_gen_target <= 0 ? std::string{tr("Auto")} :
                fill(tr("{0} Hz"), {kFrameGenTargetNames[std::clamp(prefs_.frame_gen_target, 1, kFrameGenTargets - 1)]}),
            kFrameGenMultiplierNames[std::clamp(prefs_.frame_gen_multiplier, 0, kFrameGenMultipliers - 1)],
        };
        const bool frame_gen_locked = services_.frame_gen_state() != 2;
        static constexpr const char *kNames[kVideoRows] = {
            TR("Renderer"),         TR("Output resolution"), TR("Resolution"),
            TR("Upscaling filter"), TR("ASTC textures"),     TR("Refresh rate"),
            TR("FPS overlay"),      TR("Frame generation"),  TR("Frame gen target"),
            TR("Frame gen multiplier")};
        for (int row = first; row <= last; ++row)
        {
            list.push_opacity(video_rows_.row_alpha(row, kVideoRowHeight));
            if (row == video_overlay)
            {
                label(row, tr(kNames[row]), kToggle);
                toggle(c, 1292.0f, row_centre(row), knob);
            }
            else if (row >= video_frame_gen && frame_gen_locked)
            {
                // Greyed out: the user's Lossless.dll is not there.
                list.push_opacity(0.38f);
                label(row, tr(kNames[row]), choice(row, tr("Unavailable")));
                list.pop_opacity();
            }
            else
            {
                label(row, tr(kNames[row]), choice(row, values[row]));
            }
            list.pop_opacity();
        }
        break;
    }
    case Modal::audio:
    {
        // A level row: its value at the right, the bar ending 20 before it (or where "100%"
        // would leave it), and the name in what remains.
        const auto level_row = [&](int row, const char *name, const std::string &value, int level)
        {
            const float shown =
                text(c, value, 1292.0f, baseline(dialog_row_top(modal, theme_scroll ? row - theme_first : row), 94.0f, th().size_text),
                     th().size_text, th().accent_pale, Align::right);
            const float gap = std::max(96.0f, shown + 20.0f);
            level_bar(c, 1292.0f - gap, row_centre(row), 260.0f, static_cast<float>(level) / 100.0f,
                      option_ == row ? 1.0f : 0.0f);
            label(row, name, 260.0f + gap);
        };
        level_row(0, tr("Game volume"), percent(prefs_.volume), prefs_.volume);
        label(1, tr("Mute"), kToggle);
        toggle(c, 1292.0f, row_centre(1), knob);
        level_row(2, tr("Menu sounds"), prefs_.menu_volume > 0 ? percent(prefs_.menu_volume) : tr("Off"),
                  prefs_.menu_volume);
        break;
    }
    case Modal::controls:
    {
        // Each shortcut: the two buttons held together, drawn as the controller has them, then
        // what they do.
        struct Shortcut
        {
            Pad with;
            const char *action;
        };
        static constexpr Shortcut kShortcuts[] = {
            {Pad::l1, TR("End the game and return to this menu")},
            {Pad::r1, TR("Show or hide the FPS overlay")}};
        for (int i = 0; i < 2; ++i)
        {
            const float top = 366.0f + 78.0f * static_cast<float>(i);
            const Rect cap{592.0f, top, 186.0f, 54.0f};
            list.rounded_rect(cap, 3.0f, th().row.with_alpha(0.42f));
            constexpr float kButton = 38.0f;
            constexpr float kPlus = 28.0f;
            const float cy = cap.y + cap.h * 0.5f;
            float x = cap.x + (cap.w - pad_width(Pad::touchpad, kButton) - kPlus - pad_width(kShortcuts[i].with, kButton)) * 0.5f;
            draw_pad(c, Pad::touchpad, x, cy, kButton);
            x += pad_width(Pad::touchpad, kButton);
            list.line(x + kPlus * 0.5f - 5.0f, cy, x + kPlus * 0.5f + 5.0f, cy, 2.0f, th().label);
            list.line(x + kPlus * 0.5f, cy - 5.0f, x + kPlus * 0.5f, cy + 5.0f, 2.0f, th().label);
            draw_pad(c, kShortcuts[i].with, x + kPlus, cy, kButton);
            text_shrink(c, tr(kShortcuts[i].action), 802.0f, baseline(top, 54.0f, 22.0f), 22.0f,
                        th().body, 526.0f);
        }
        label(0, tr("Vibration"), kToggle);
        toggle(c, 1292.0f, row_centre(0), knob);
        // The button mapping: as usual or changed, opened with Cross.
        const float shown = text_shrink(
            c, prefs_.mapping == kDefaultMapping ? tr("As usual") : tr("Changed"), 1292.0f,
            baseline(dialog_row_top(modal, 1), 94.0f, th().size_small), th().size_small,
            prefs_.mapping == kDefaultMapping ? th().meta : th().accent_pale, 320.0f, Align::right);
        label(1, tr("Button mapping"), shown);
        break;
    }
    case Modal::performance:
    {
        static constexpr const char *kNames[kPerformanceRows] = {
            TR("Compile ahead"),        TR("Asynchronous shaders"), TR("Faster GPU emulation"),
            TR("Faster CPU emulation"), TR("Faster DMA"),           TR("Reactive flushing"),
            TR("Skip CPU invalidation")};
        for (int row = first; row <= last; ++row)
        {
            list.push_opacity(rows_view.row_alpha(row, kVideoRowHeight));
            // Asynchronous shaders act with Vulkan only: the OpenGL renderer compiles in its own
            // context here (graphics.cpp), and Eden then leaves them off.
            const std::string name =
                std::string(tr(kNames[row])) + (row == speed_async_shaders ? " (Vulkan)" : "");
            label(row, name.c_str(), kToggle);
            toggle(c, 1292.0f, row_centre(row),
                   tween::clamp01(switches_[static_cast<std::size_t>(row)].value));
            list.pop_opacity();
        }
        break;
    }
    case Modal::accessibility:
    {
        static constexpr const char *kNames[] = {TR("Larger text"), TR("High contrast"),
                                                 TR("Reduce motion")};
        static constexpr const char *kAbout[] = {
            TR("Draws the menu's small text larger."),
            TR("Solid panels, brighter text and an outlined highlight."),
            TR("Stops the background drifting and the screens sliding, here and on the loading "
               "screen.")};
        for (int row = 0; row < 3; ++row)
        {
            label(row, tr(kNames[row]), kToggle);
            toggle(c, 1292.0f, row_centre(row),
                   tween::clamp01(switches_[static_cast<std::size_t>(row)].value));
        }
        // What the highlighted switch does.
        text_block(c, tr(kAbout[std::clamp(option_, 0, 2)]), 592.0f,
                   baseline(700.0f, 30.0f, th().size_small), th().size_small, 30.0f, th().meta, 736.0f,
                   2, kShrink);
        break;
    }
    case Modal::theme:
    {
        if (theme_mapping_open_) {
            static constexpr std::array<const char*,8> labels = {
                "HOME", "Library", "Settings", "Themes", "Profiles", "Multiplayer", "Public Rooms", "About"};
            for (int row = first; row <= last; ++row) {
                const auto role=switch_pages::kEditableRoles[static_cast<std::size_t>(row)];
                const std::string asset=switch_pages::override_for(
                    switch_bootstrap::root(),theme_mapping_id_,role);
                label(row,labels[static_cast<std::size_t>(row)],292.0f);
                const std::string label_name = asset == "auto" ? "Auto (pack default)" :
                    asset == "all_apps" ? "All Apps" :
                    asset == "player_select" ? "Player Selection" :
                    asset == "lockscreen" ? "Lock Screen" :
                    asset == "home" ? "Home Menu" :
                    asset == "user" ? "User Page" :
                    asset == "news" ? "News" : "Settings";
                text_shrink(c,label_name,
                    1292.0f,baseline(dialog_row_top(modal,row-theme_first),94.0f,th().size_text),
                    th().size_text,th().accent_pale,270.0f,Align::right);
            }
            text_block(c, "The highlighted screen is previewed behind this dialog. Auto uses the pack's default mapping; missing artwork falls back to HOME.",
                592.0f,baseline(820.0f,30.0f,th().size_small),th().size_small,30.0f,
                th().meta,736.0f,2,kShrink);
        } else {
            const auto all = themes();
            for (int row = first; row <= last; ++row) {
                if (row == 0) {
                    icon(c, Icon::downloads, 656.0f,
                         dialog_row_top(modal, row - theme_first) + 47.0f,
                         31.0f, th().accent_pale);
                    text_shrink(c, tr("Install from Themezer Quick ID"), 720.0f,
                        baseline(dialog_row_top(modal, row - theme_first), 94.0f, th().size_text),
                        th().size_text, th().value, 530.0f);
                } else {
                    const Theme &entry = all[static_cast<std::size_t>(row - 1)];
                    const auto preview = c.textures.theme_preview(entry.id, entry.backdrop,
                                                                  std::string_view(entry.id) == "switch-dark");
                    const Rect thumb{610.0f,
                                     dialog_row_top(modal, row - theme_first) + 12.0f,
                                     120.0f, 68.0f};
                    if (preview != 0)
                        list.rounded_image(preview, thumb, {0.0f,0.0f,1.0f,1.0f}, 5.0f,
                                           gfx::Color{1.0f,1.0f,1.0f,1.0f});
                    else
                        list.rounded_rect(thumb, 5.0f, th().row_focus);
                    text_shrink(c, entry.name, 750.0f,
                        baseline(dialog_row_top(modal, row - theme_first), 94.0f, th().size_text),
                        th().size_text, th().value, 468.0f);
                }
                if (row != theme_index() + 1) continue;
                const float y = row_centre(row - theme_first);
                list.circle(1268.0f, y, 17.0f, th().accent);
                icon(c, Icon::check, 1268.0f, y, 22.0f, th().on_accent);
            }
            const std::string about = option_ == 0
                ? "Enter a T or P Quick ID. Re-enter an installed P code to refresh its pack name; your artwork stays intact."
                : all[static_cast<std::size_t>(std::clamp(option_ - 1, 0, static_cast<int>(all.size()) - 1))].summary;
            // The selected theme's summary is already shown in the preview column.
            // Keeping a second description at the bottom made it overlap the hints.
            (void)about;
        }
        // Thumbnail gallery detail pane. Full pack previews come from the
        // installed artwork, not the currently active wallpaper.
        const Rect large{1374.0f, 375.0f, 352.0f, 198.0f};
        const auto all = themes();
        const bool editor = theme_mapping_open_;
        const int selected_index = editor ? theme_index() : option_ - 1;
        text(c, editor ? tr("SCREEN PREVIEW") : tr("THEME PREVIEW"),
             1374.0f, baseline(328.0f, 32.0f, th().size_small),
             th().size_small, th().accent_pale);
        std::uint32_t preview = 0;
        if (editor)
            preview = c.textures.backdrop();
        else if (selected_index >= 0 && selected_index < static_cast<int>(all.size()))
        {
            const Theme &entry = all[static_cast<std::size_t>(selected_index)];
            preview = c.textures.theme_preview(entry.id, entry.backdrop,
                                               std::string_view(entry.id) == "switch-dark");
        }
        if (preview != 0)
            list.rounded_image(preview, large, {0.0f,0.0f,1.0f,1.0f}, 7.0f,
                               gfx::Color{1.0f,1.0f,1.0f,1.0f});
        else
            list.rounded_rect(large, 7.0f, th().row.with_alpha(0.9f));
        if (!editor && selected_index < 0)
        {
            text_shrink(c, tr("Install from Themezer Quick ID"), 1374.0f,
                        baseline(596.0f, 44.0f, th().size_heading),
                        th().size_heading, th().title, 352.0f);
            text_block(c,
                "Use a T code for one theme or a P code for a full pack. "
                "Re-enter an installed P code to refresh its name.",
                1374.0f, baseline(654.0f, 30.0f, th().size_small),
                th().size_small, 32.0f, th().copy, 352.0f, 4, kShrink);
        }
        if (!editor && selected_index >= 0 && selected_index < static_cast<int>(all.size()))
        {
            const Theme &entry = all[static_cast<std::size_t>(selected_index)];
            text_shrink(c, entry.name, 1374.0f,
                baseline(596.0f, 44.0f, th().size_heading),
                th().size_heading, th().title, 352.0f);
            text_shrink(c, entry.id, 1374.0f,
                baseline(648.0f, 34.0f, th().size_small),
                th().size_small, th().meta, 352.0f);
            text_block(c, entry.summary, 1374.0f,
                baseline(701.0f, 30.0f, th().size_small), th().size_small,
                32.0f, th().copy, 352.0f, 3, kShrink);
        }
        else if (editor)
        {
            const auto role = switch_pages::kEditableRoles[static_cast<std::size_t>(theme_mapping_row_)];
            text_shrink(c, std::string(role), 1374.0f,
                baseline(596.0f, 44.0f, th().size_heading), th().size_heading,
                th().title, 352.0f);
            text_block(c, tr("Change artwork with Left/Right; Square restores Auto."),
                1374.0f, baseline(665.0f, 30.0f, th().size_small),
                th().size_small, 32.0f, th().copy, 352.0f, 3, kShrink);
        }
        break;
    }
    default:
    {
        text_block(c, services_.setup_details(), 592.0f, baseline(364.0f, 40.0f, th().size_text),
                   th().size_text, 40.0f, th().body, 736.0f, 3, kShrink);
        static constexpr const char *kNames[] = {TR("Detailed logging"), TR("Write logs at once")};
        static constexpr const char *kAbout[] = {
            TR("Eden's debug messages in the logs of a game."),
            TR("Keeps the last line before a crash, but games stutter. From the next start.")};
        for (int row = 0; row < 2; ++row)
        {
            label(row, tr(kNames[row]), kToggle);
            toggle(c, 1292.0f, row_centre(row),
                   tween::clamp01(switches_[static_cast<std::size_t>(row)].value));
        }
        // What the highlighted switch does.
        text_block(c, tr(kAbout[std::clamp(option_, 0, 1)]), 592.0f,
                   baseline(716.0f, 30.0f, th().size_small), th().size_small, 30.0f, th().meta, 736.0f,
                   2, kShrink);
        break;
    }
    }

    if (scrolls)
    {
        list.pop_clip();
        scrollbar(c, rows_view, 1340.0f, window.y, window.h);
    }
    if (modal == Modal::performance)
    {
        // What the highlighted switch does, under the window.
        text_block(c, tr(performance_about(option_)), 592.0f,
                   baseline(window.y + window.h + 22.0f, 30.0f, th().size_small), th().size_small, 30.0f,
                   th().meta, 736.0f, 2, kShrink);
    }

    // Under the rows: Video's five, and Performance's four with their line of text, end lower
    // than the other dialogs' three.
    // Themes uses the lower inside edge for hints, separate from the preview
    // and the fourth installed-theme row (which ends near y=770).
    const float foot = modal == Modal::theme ? 842.0f : (scrolls ? 848.0f : 811.0f);
    if (!message_.empty() && scrolls)
    {
        // Two lines, placed as a game's own settings place theirs: what a Video choice costs
        // takes more than one.
        notice_block(c, message_, 592.0f, foot - 6.0f, th().size_small, 26.0f,
                     message_warning_ ? th().warning : th().accent_pale, 736.0f, 2, message_warning_);
    }
    else if (!message_.empty())
    {
        notice(c, message_, 592.0f, foot + 7.0f, th().size_small,
               message_warning_ ? th().warning : th().accent_pale, 736.0f, message_warning_);
    }
    else if (modal == Modal::theme && theme_mapping_open_)
    {
        static constexpr Hint kMapHints[] = {{Pad::updown, TR("Screen")}, {Pad::leftright, TR("Artwork")},
                                              {Pad::square, TR("Auto")}, {Pad::circle, TR("Back")}};
        draw_hints(c, kMapHints, 4, 592.0f, foot, th().copy, 736.0f);
    }
    else if (modal == Modal::theme)
    {
        static constexpr Hint kThemeHints[] = {{Pad::cross, TR("Apply")}, {Pad::triangle, TR("Customize")},
                                                {Pad::square, TR("Delete")}, {Pad::circle, TR("Back")}};
        draw_hints(c, kThemeHints, 4, 592.0f, foot, th().copy, 736.0f);
    }
    else if (modal == Modal::controls && option_ == 1)
    {
        static constexpr Hint kOpen[] = {
            {Pad::updown, TR("Select")}, {Pad::cross, TR("Open")}, {Pad::circle, TR("Back")}};
        draw_hints(c, kOpen, 3, 592.0f, foot, th().copy, 736.0f);
    }
    else if (rows > 1)
    {
        static constexpr Hint kHints[] = {
            {Pad::updown, TR("Select")}, {Pad::leftright, TR("Change")}, {Pad::circle, TR("Back")}};
        draw_hints(c, kHints, 3, 592.0f, foot, th().copy, 736.0f);
    }
    else
    {
        static constexpr Hint kHints[] = {{Pad::cross, TR("Change")}, {Pad::circle, TR("Back")}};
        draw_hints(c, kHints, 2, 592.0f, foot, th().copy, 736.0f);
    }
    list.pop_transform();
    list.pop_opacity();
}

} // namespace pe::ui
