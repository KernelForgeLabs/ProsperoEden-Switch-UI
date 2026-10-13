// Switch HOME - native Quick ID text entry, download lifecycle.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "pe/ui/launcher.hpp"
#include "pe/ui/switch_bootstrap.hpp"
#include <chrono>
#include <exception>

namespace pe::ui {
void Launcher::begin_switch_theme_import() {
    if (theme_keyboard_.valid() || theme_download_.valid()) return;
    theme_stop_ = false;
    try {
        theme_keyboard_ = std::async(std::launch::async, [this] {
            return services_.theme_id_text(theme_stop_);
        });
    } catch (const std::exception&) {
        say(tr("The system keyboard could not be opened."), true);
        cue(audio::Cue::error);
    }
}

void Launcher::update_switch_theme_import() {
    using namespace std::chrono_literals;
    if (theme_keyboard_.valid() && theme_keyboard_.wait_for(0s) == std::future_status::ready) {
        std::optional<std::string> code;
        try { code = theme_keyboard_.get(); }
        catch (const std::exception&) {
            say(tr("The system keyboard is unavailable."), true);
            cue(audio::Cue::error);
        }
        if (code && !theme_stop_ && modal_ == Modal::theme) {
            theme_stop_ = false;
            say(tr("Downloading Themezer wallpaper..."));
            try {
                theme_download_ = std::async(std::launch::async, [this, code = *code] {
                    return switch_theme::install_quick_id(code, theme_stop_);
                });
            } catch (const std::exception&) {
                say(tr("Could not start Themezer download."), true);
                cue(audio::Cue::error);
            }
        }
    }
    if (theme_download_.valid() && theme_download_.wait_for(0s) == std::future_status::ready) {
        switch_theme::ImportResult result;
        try { result = theme_download_.get(); }
        catch (const std::exception&) { result.message = "Themezer importer failed."; }
        if (result.success) {
            refresh_switch_themes();
            say(result.message);
            cue(audio::Cue::saved);
        } else {
            say(result.message, true);
            cue(audio::Cue::error);
        }
    }
}
} // namespace pe::ui
