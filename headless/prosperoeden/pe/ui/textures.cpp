// ProsperoEden - The launcher's textures: its own art and lazily loaded covers.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pe/ui/textures.hpp"

#include "pe/ui/theme.hpp"
#include "pe/ui/switch_pages.hpp"

#include "pe/core/log.hpp"

#include <algorithm>
#include <filesystem>
#include <chrono>
#include <future>
#include <utility>

namespace pe::ui
{

namespace
{

constexpr std::size_t kMaxCovers = 160;

} // namespace

Textures::~Textures()
{
    release();
}

std::uint32_t Textures::create(const gfx::Image &image)
{
    return image.empty() ? 0 : batch_.create_texture(image.width, image.height, image.rgba.data());
}

bool Textures::load_art(const std::string &directory)
{
    // Stage the UI textures before touching the active ones. A corrupt or
    // incomplete imported theme must not blank the running HOME screen.
    std::uint32_t backdrop = 0, blur = 0, brand = 0, forge = 0, controller = 0;
    const auto load = [&](const std::string &name, std::uint32_t *texture)
    {
        gfx::Image image;
        const std::string path = !name.empty() && name.front() == '/' ? name : directory + "/" + name;
        if (!gfx::load_tga(path, &image))
        {
            sys::log("art %s could not be read", name.c_str());
            return false;
        }
        *texture = create(image);
        return *texture != 0;
    };
    const auto from_theme = [&](const std::string &value)
    {
        return !value.empty() && value.front() == '/' ? value : "art/" + value;
    };
    bool ok_backdrop = load(from_theme(th().backdrop), &backdrop);
    bool ok_blur = load(from_theme(th().backdrop_blur), &blur);
    if (!ok_backdrop) ok_backdrop = load("art/switch-dark-backdrop.tga", &backdrop);
    if (!ok_blur) ok_blur = load("art/switch-dark-backdrop-blur.tga", &blur);
    const bool ok_brand = load("art/" + std::string(th().brand), &brand);
    // Publisher art is kept separate from the actual PS5 app icon and does
    // not depend on the currently selected Themezer wallpaper.
    (void)load("art/kernel-forge-labs.tga", &forge);
    (void)load("art/controller.tga", &controller);
    if (!(ok_backdrop && ok_blur && ok_brand))
    {
        for (const auto texture : {backdrop, blur, brand, forge, controller})
            if (texture != 0) batch_.delete_texture(texture);
        return false;
    }
    for (const auto texture : {backdrop_, backdrop_blur_, brand_, forge_, controller_})
        if (texture != 0) batch_.delete_texture(texture);
    backdrop_ = backdrop;
    backdrop_blur_ = blur;
    brand_ = brand;
    forge_ = forge;
    controller_ = controller;
    art_directory_ = directory;
    loaded_page_ = "base";
    last_theme_page_.clear();
    return true;
}

bool Textures::select_page_art(std::string_view page)
{
    // Call only on the render thread, where the OpenGL context is current.
    const std::string key = std::string(th().id) + ":" + std::string(page);
    if (key == last_theme_page_) return true; // no filesystem work on unchanged frames
    const auto art = switch_pages::lookup(switch_bootstrap::root(), th().id, page);
    const std::string desired = art.main.empty() ? "base" : art.main;
    if (loaded_page_ == desired) {
        last_theme_page_ = key;
        return true;
    }
    if (art.main.empty()) {
        if (art_directory_.empty()) return false;
        if (!load_art(art_directory_)) return false;
        last_theme_page_ = key;
        return true;
    }
    gfx::Image main_image, blur_image;
    if (!gfx::load_tga(art.main, &main_image) || !gfx::load_tga(art.blur, &blur_image))
        return false;
    const std::uint32_t main = create(main_image);
    const std::uint32_t blur = create(blur_image);
    if (main == 0 || blur == 0)
    {
        if (main != 0) batch_.delete_texture(main);
        if (blur != 0) batch_.delete_texture(blur);
        return false; // never discard the old wallpaper if decoding fails
    }
    batch_.delete_texture(backdrop_);
    batch_.delete_texture(backdrop_blur_);
    backdrop_ = main;
    backdrop_blur_ = blur;
    loaded_page_ = desired;
    last_theme_page_ = key;
    return true;
}

std::uint32_t Textures::theme_preview(std::string_view id, std::string_view art, bool bundled)
{
    // A preview is read only from the trusted bundle or a validated installed
    // theme folder; external metadata is never used as a filesystem path.
    if (!bundled && !switch_bootstrap::valid_id(std::string(id))) return 0;
    const std::string key(id);
    if (const auto it = theme_previews_.find(key); it != theme_previews_.end())
        return it->second;
    // Cap GL memory and prevent stale previews if a removed theme is reinstalled.
    if (theme_previews_.size() >= 16) clear_theme_previews();
    std::filesystem::path path;
    if (bundled)
        path = std::filesystem::path(art_directory_) / "art" / std::string(art);
    else
        path = std::filesystem::path(switch_bootstrap::root()) /
               "installed" / key / "art" / "backdrop.tga";
    std::uint32_t texture = 0;
    if (!path.empty() && (bundled || switch_bootstrap::valid_tga(path)))
    {
        gfx::Image image;
        if (gfx::load_tga(path.string(), &image))
        {
            while (image.width > 384 && image.height > 128)
                image = gfx::halve(image);
            texture = create(image);
        }
    }
    theme_previews_.emplace(key, texture); // Cache missing art too.
    return texture;
}

void Textures::clear_theme_previews()
{
    for (const auto &[id, texture] : theme_previews_)
        if (texture != 0) batch_.delete_texture(texture);
    theme_previews_.clear();
}

bool Textures::load_splash(const std::string &path)
{
    drop_splash();
    gfx::Blocks blocks;
    if (!gfx::load_dds_bc7(path, &blocks))
    {
        sys::log("the launch picture %s could not be read", path.c_str());
        return false;
    }
    splash_ = batch_.create_bc7_texture(blocks.width, blocks.height, blocks.bc7.data(), blocks.bc7.size());
    if (splash_ == 0)
        sys::log("the launch picture was not taken as a texture");
    return splash_ != 0;
}

void Textures::drop_splash()
{
    batch_.delete_texture(splash_);
    splash_ = 0;
}

void Textures::load_game_backdrop(const std::string &cover_path)
{
    if (cover_path == game_backdrop_path_)
        return;
    clear_game_backdrop();
    game_backdrop_path_ = cover_path;
    if (cover_path.empty())
        return;
    gfx::Image image;
    if (!services_.load_image(cover_path, &image))
        return;
    for (std::size_t i = 0; i + 3 < image.rgba.size(); i += 4)
    {
        image.rgba[i + 0] = static_cast<std::uint8_t>(image.rgba[i + 0] * 0.35f);
        image.rgba[i + 1] = static_cast<std::uint8_t>(image.rgba[i + 1] * 0.35f);
        image.rgba[i + 2] = static_cast<std::uint8_t>(image.rgba[i + 2] * 0.35f);
    }
    gfx::blur(image, image.width / 32);
    game_backdrop_ = create(image);
    gfx::Image blurred = gfx::halve(image);
    game_backdrop_blur_ = create(blurred);
}

void Textures::clear_game_backdrop()
{
    batch_.delete_texture(game_backdrop_);
    batch_.delete_texture(game_backdrop_blur_);
    game_backdrop_ = 0;
    game_backdrop_blur_ = 0;
    game_backdrop_path_.clear();
}

void Textures::load_wallpaper(const std::string &path)
{
    if (path == wallpaper_path_)
        return;
    if (path.empty())
    {
        clear_wallpaper();
        return;
    }
    // A finished read: taken when it is the wanted one, and dropped when the choice moved on
    // while it was being prepared. The wait never blocks: it only asks whether it is done.
    if (wallpaper_task_.valid() &&
        wallpaper_task_.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
    {
        WallpaperImage done = wallpaper_task_.get();
        if (done.path == path)
        {
            // The textures of the wallpaper shown until now go first: each one is up to 16 MiB.
            clear_wallpaper();
            wallpaper_ = create(done.image);
            wallpaper_blur_ = create(done.frosted);
            wallpaper_w_ = done.image.width;
            wallpaper_h_ = done.image.height;
            wallpaper_path_ = std::move(done.path);
            return;
        }
    }
    // One read at a time: a running one is replaced only after it finished and turned out
    // stale, so flipping through the list never queues up decodes.
    if (!wallpaper_task_.valid())
        wallpaper_task_ = std::async(std::launch::async, [path]
        {
            WallpaperImage done;
            done.path = path;
            gfx::Image image;
            if (!gfx::load_image(path, &image))
            {
                sys::log("the wallpaper %s could not be read", path.c_str());
                return done;
            }
            // The screens show at most 2048 pixels a side of it; Backdrop resizes the whole of
            // it to the screen, so no part is cut.
            while (image.width > 2048 || image.height > 2048)
                image = gfx::halve(image);
            // The frosted panels' copy: softer and smaller, so it reads as glass over it.
            done.frosted = gfx::halve(image);
            gfx::blur(done.frosted, std::max(4, done.frosted.width / 12));
            done.image = std::move(image);
            return done;
        });
}

void Textures::clear_wallpaper()
{
    batch_.delete_texture(wallpaper_);
    batch_.delete_texture(wallpaper_blur_);
    wallpaper_ = 0;
    wallpaper_blur_ = 0;
    wallpaper_w_ = 0;
    wallpaper_h_ = 0;
    wallpaper_path_.clear();
}

void Textures::release()
{
    clear_theme_previews();
    drop_splash();
    clear_game_backdrop();
    clear_wallpaper();
    for (std::uint32_t *texture : {&backdrop_, &backdrop_blur_, &brand_, &forge_, &controller_})
    {
        batch_.delete_texture(*texture);
        *texture = 0;
    }
    for (auto &[key, entry] : covers_)
        batch_.delete_texture(entry.texture);
    covers_.clear();
    queue_.clear();
}

Cover Textures::cover(const std::string &path, float size)
{
    if (path.empty())
        return {0, 0.0f, true};
    // Covers are 256 pixels a side: draw a small one from a halved copy, which
    // a single-level texture cannot do for itself.
    const float pixels = size * output_scale_;
    const int level = pixels > 128.0f ? 0 : pixels > 64.0f ? 1 : 2;
    const std::string key = path + '#' + static_cast<char>('0' + level);
    auto it = covers_.find(key);
    if (it == covers_.end())
    {
        Entry entry;
        entry.path = path;
        entry.level = level;
        it = covers_.emplace(key, std::move(entry)).first;
        queue_.push_back(key);
    }
    it->second.used = frame_;
    return {it->second.texture, it->second.age, it->second.loaded && it->second.texture == 0};
}

void Textures::pump(float dt, int budget)
{
    ++frame_;
    for (auto &[key, entry] : covers_)
        if (entry.loaded)
            entry.age += dt;
    while (budget > 0 && !queue_.empty())
    {
        const std::string key = queue_.front();
        queue_.erase(queue_.begin());
        const auto it = covers_.find(key);
        if (it == covers_.end() || it->second.loaded)
            continue;
        Entry &entry = it->second;
        entry.loaded = true;
        gfx::Image image;
        if (!services_.load_image(entry.path, &image))
            continue; // stays without a texture: the placeholder is drawn
        for (int level = 0; level < entry.level && image.width > 64; ++level)
            image = gfx::halve(image);
        entry.texture = create(image);
        --budget;
    }
    if (covers_.size() > kMaxCovers)
    {
        // Drop the covers not drawn for the longest time.
        std::vector<std::pair<std::uint64_t, std::string>> order;
        for (const auto &[key, entry] : covers_)
            if (entry.loaded && entry.used + 2 < frame_)
                order.emplace_back(entry.used, key);
        std::sort(order.begin(), order.end());
        for (std::size_t index = 0; index < order.size() && covers_.size() > kMaxCovers * 3 / 4;
             ++index)
        {
            batch_.delete_texture(covers_[order[index].second].texture);
            covers_.erase(order[index].second);
        }
    }
}

} // namespace pe::ui
