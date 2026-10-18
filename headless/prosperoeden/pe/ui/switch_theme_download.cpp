// ProsperoEden - Themezer Quick ID import for the PS5 launcher.
// SPDX-License-Identifier: GPL-3.0-or-later
#include "pe/ui/switch_theme_download.hpp"
#include "pe/ui/switch_bootstrap.hpp"
#include "remote/http.h"
#include <miniz.h>

// Decoder already used by ProsperoEden metadata. Keep a private copy of its
// functions so the launcher does not affect the emulator's global image state.
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG 1
#define STBI_ONLY_PNG 1
#define STBI_NO_STDIO
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#include "stb/stb_image.h"
#pragma GCC diagnostic pop

#include <algorithm>
#include <array>
#include <cerrno>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>

namespace pe::ui::switch_theme
{
namespace
{
constexpr std::size_t kMaxZip = 64u * 1024u * 1024u;
constexpr std::size_t kMaxThemeZip = 24u * 1024u * 1024u;
constexpr mz_uint64 kMaxImage = 28u * 1024u * 1024u;
constexpr int kWidth = 2048, kHeight = 1152;
constexpr int kBlurWidth = 512, kBlurHeight = 288;

ImportResult failed(const char* error) { return {false, {}, error}; }

bool directory(const std::string& path)
{
    if (mkdir(path.c_str(), 0755) == 0) return true;
    struct stat s{};
    return stat(path.c_str(), &s) == 0 && S_ISDIR(s.st_mode);
}

// Never replace an already-installed theme. On native PS5 filesystems,
// lstat() may not identify an existing directory although stat() does.
// Keep the non-following check first so dangling links are recognized where
// supported, then fall back to stat() for ordinary existing files/folders.
bool occupied(const std::string& path)
{
    struct stat s{};
    return lstat(path.c_str(), &s) == 0 || stat(path.c_str(), &s) == 0;
}

// This is always a private staging directory with a validated Quick ID.
void cleanup(const std::string& path)
{
    std::remove((path + "/art/backdrop.tga").c_str());
    std::remove((path + "/art/backdrop-blur.tga").c_str());
    std::remove((path + "/palette.ini").c_str());
    rmdir((path + "/art").c_str());
    rmdir(path.c_str());
}

struct Sink
{
    std::vector<unsigned char> bytes;
    const std::atomic<bool>* stop = nullptr;
    std::size_t limit = kMaxZip;
};
int receive(void* ptr, const void* data, std::size_t count)
{
    auto& sink = *static_cast<Sink*>(ptr);
    if (sink.stop->load() || count > sink.limit - sink.bytes.size()) return 0;
    const auto* in = static_cast<const unsigned char*>(data);
    sink.bytes.insert(sink.bytes.end(), in, in + count);
    return 1;
}
int cancelled(void* ptr)
{
    return static_cast<Sink*>(ptr)->stop->load() ? 1 : 0;
}
int start_body(void* ptr, int status, std::uint64_t size)
{
    const auto& sink = *static_cast<Sink*>(ptr);
    return !sink.stop->load() && status == 200 && size <= sink.limit;
}

std::string lower(std::string v)
{
    for (char& c : v) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return v;
}
int priority(const std::string& name)
{
    const auto n = lower(name);
    const auto pos = n.find_last_of('/');
    const auto base = n.substr(pos == std::string::npos ? 0 : pos + 1);
    if (n.find("preview") != std::string::npos || n.find("screenshot") != std::string::npos)
        return 999;
    if (base == "image.jpg" || base == "image.jpeg") return 0;
    if (base == "background.jpg" || base == "background.jpeg") return 1;
    if (base == "background.png") return 2;
    if (base == "bg.jpg" || base == "bg.png") return 3;
    if (base == "image.png") return 4;
    if (base.ends_with(".jpg") || base.ends_with(".jpeg") || base.ends_with(".png")) return 50;
    return 9999;
}

// New ZIP-based NXThemes include an info.json manifest (ThemeName/Author).
// We only need the bounded, user-visible title; never trust it as a path.
// A tiny quoted-string reader avoids a new library dependency in the PS5 build.
std::string field_value(const std::string& data, const char* key)
{
    const std::string needle = std::string("\"") + key + "\"";
    std::size_t start = data.find(needle);
    if (start == std::string::npos) return {};
    start = data.find(':', start + needle.size());
    if (start == std::string::npos) return {};
    ++start;
    while (start < data.size() && std::isspace(static_cast<unsigned char>(data[start]))) ++start;
    if (start >= data.size() || data[start++] != '"') return {};
    std::string result;
    for (; start < data.size() && result.size() <= 60; ++start)
    {
        char c = data[start];
        if (c == '"') return result.empty() ? std::string{} : result;
        if (c == '\\')
        {
            if (++start >= data.size()) return {};
            c = data[start];
            // We accept standard escapes for quotes and backslashes. Discard
            // names needing other JSON escapes rather than corrupting UTF-8.
            if (c != '"' && c != '\\' && c != '/') return {};
        }
        if (static_cast<unsigned char>(c) < 32 || c == '=' || c == '\n' || c == '\r') return {};
        result.push_back(c);
    }
    return {};
}

std::string archive_title(mz_zip_archive& zip)
{
    const int index = mz_zip_reader_locate_file(&zip, "info.json", nullptr, 0);
    if (index < 0) return {};
    mz_zip_archive_file_stat info{};
    if (!mz_zip_reader_file_stat(&zip, static_cast<mz_uint>(index), &info) ||
        info.m_uncomp_size == 0 || info.m_uncomp_size > 16384) return {};
    std::size_t bytes = 0;
    void* raw = mz_zip_reader_extract_to_heap(&zip, static_cast<mz_uint>(index), &bytes, 0);
    if (!raw || bytes > 16384) { mz_free(raw); return {}; }
    const std::string json(static_cast<const char*>(raw), bytes);
    mz_free(raw);
    for (const char* key : {"ThemeName", "Name", "name", "title"})
    {
        const auto name = field_value(json, key);
        if (!name.empty()) return name;
    }
    return {};
}

// Themezer's official Switch client resolves P Quick IDs using the public
// GraphQL lookup. Ask for only the name so a failed metadata request never
// prevents the (separately downloaded) artwork from installing.
std::string encode_url(std::string_view input)
{
    constexpr char hex[] = "0123456789ABCDEF";
    std::string result;
    result.reserve(input.size() * 3);
    for (unsigned char ch : input)
    {
        if (std::isalnum(ch) || ch == '-' || ch == '_' || ch == '.' || ch == '~')
            result.push_back(static_cast<char>(ch));
        else
        {
            result.push_back('%');
            result.push_back(hex[ch >> 4]);
            result.push_back(hex[ch & 15]);
        }
    }
    return result;
}

std::string lookup_pack_name(const std::string& id, const std::atomic<bool>& stop)
{
    if (id.empty() || id.front() != 'P' || stop.load()) return {};
    constexpr std::string_view query =
        "query LookupByQuickId($quickId:String!){switch{lookupByQuickId(quickId:$quickId)"
        "{__typename ... on SwitchPack{name}}}}";
    const std::string variables = "{\"quickId\":\"" + id + "\"}";
    const std::string url = "https://api.themezer.net/graphql?query=" +
        encode_url(query) + "&variables=" + encode_url(variables);
    Sink sink{};
    sink.stop = &stop;
    sink.limit = 12u * 1024u;
    remote_http_request req{};
    req.url = url.c_str();
    req.https_only = 1;
    req.timeout_ms = 12000; // Metadata is optional; don't hang the importer.
    req.raw = 1;
    req.sink = receive;
    req.begin = start_body;
    req.stop = cancelled;
    req.user = &sink;
    remote_http_result result{};
    if (remote_http_get(&req, &result) != 0 || result.status != 200 || stop.load())
        return {};
    const std::string json(sink.bytes.begin(), sink.bytes.end());
    if (field_value(json, "__typename") != "SwitchPack") return {};
    return field_value(json, "name"); // Bounded, printable, never used as a path.
}

// Update only the display name of an already-installed P pack. Retain its
// artwork, accent, pages.ini overrides, and active selection. Reject links,
// nonstandard palettes and unexpected on-disk state.
bool refresh_installed_pack_name(const std::string& id, const std::string& name)
{
    namespace fs = std::filesystem;
    if (name.empty() || id.empty() || id.front() != 'P') return false;
    const fs::path directory = fs::path(switch_bootstrap::root()) / "installed" / id;
    const fs::path palette = directory / "palette.ini";
    const fs::path staged = directory / "palette.ini.tmp";
    if (!switch_bootstrap::usable(directory) ||
        !switch_bootstrap::regular_without_links(palette)) return false;
    std::error_code ec;
    if (fs::exists(staged, ec) || ec) return false;
    std::ifstream input(palette, std::ios::binary);
    if (!input) return false;
    std::string line, content;
    bool replaced = false;
    while (std::getline(input, line))
    {
        if (content.size() + line.size() + 1 > 8192) return false;
        if (line.rfind("name=", 0) == 0)
        {
            if (replaced) return false;
            line = "name=" + name;
            replaced = true;
        }
        content += line + "\n";
    }
    if (input.bad() || !replaced) return false;
    {
        std::ofstream output(staged, std::ios::binary | std::ios::trunc);
        if (!output) return false;
        output << content;
        if (!output.good()) { output.close(); fs::remove(staged, ec); return false; }
    }
    fs::rename(staged, palette, ec);
    if (ec) fs::remove(staged, ec);
    return !ec;
}

// Center crop and bilinear resizing; preserves the original wallpaper's framing.
std::vector<unsigned char> resize(const unsigned char* input, int sw, int sh, int dw, int dh)
{
    std::vector<unsigned char> out(static_cast<std::size_t>(dw) * dh * 4);
    const double aspect = static_cast<double>(dw) / dh;
    const double cropw = std::min(static_cast<double>(sw), sh * aspect);
    const double croph = std::min(static_cast<double>(sh), sw / aspect);
    const double left = (sw - cropw) / 2.0, top = (sh - croph) / 2.0;
    for (int y = 0; y < dh; ++y)
    {
        const double sy = std::clamp(top + (y + 0.5) * croph / dh - 0.5, 0.0, static_cast<double>(sh-1));
        const int y0 = static_cast<int>(sy), y1 = std::min(y0+1, sh-1);
        const double fy = sy-y0;
        for (int x = 0; x < dw; ++x)
        {
            const double sx = std::clamp(left + (x+0.5) * cropw / dw - 0.5, 0.0, static_cast<double>(sw-1));
            const int x0 = static_cast<int>(sx), x1 = std::min(x0+1, sw-1);
            const double fx = sx-x0;
            const std::size_t at = (static_cast<std::size_t>(y)*dw+x)*4;
            for (int channel=0; channel<4; ++channel)
            {
                const double a = input[(static_cast<std::size_t>(y0)*sw+x0)*4+channel]*(1-fx) +
                                 input[(static_cast<std::size_t>(y0)*sw+x1)*4+channel]*fx;
                const double b = input[(static_cast<std::size_t>(y1)*sw+x0)*4+channel]*(1-fx) +
                                 input[(static_cast<std::size_t>(y1)*sw+x1)*4+channel]*fx;
                out[at+channel] = static_cast<unsigned char>(std::clamp(std::lround(a*(1-fy)+b*fy), 0L, 255L));
            }
        }
    }
    return out;
}

// Small blurred representation for translucent glass panels. No dynamic image decoding on display.
void blur(std::vector<unsigned char>& pixels, int w, int h)
{
    std::vector<unsigned char> tmp(pixels.size());
    for (int pass=0;pass<2;++pass)
    {
        // Separable box filter. Equivalent to a soft background blur at dock scale.
        for (int y=0;y<h;++y) for (int x=0;x<w;++x) for (int c=0;c<3;++c)
        {
            int sum=0, samples=0;
            for (int dx=-8;dx<=8;++dx)
            {
                const int xx=std::clamp(x+dx,0,w-1);
                sum+=pixels[(static_cast<std::size_t>(y)*w+xx)*4+c]; ++samples;
            }
            tmp[(static_cast<std::size_t>(y)*w+x)*4+c]=static_cast<unsigned char>(sum/samples);
            tmp[(static_cast<std::size_t>(y)*w+x)*4+3]=255;
        }
        for (int y=0;y<h;++y) for (int x=0;x<w;++x) for (int c=0;c<3;++c)
        {
            int sum=0, samples=0;
            for (int dy=-8;dy<=8;++dy)
            {
                const int yy=std::clamp(y+dy,0,h-1);
                sum+=tmp[(static_cast<std::size_t>(yy)*w+x)*4+c]; ++samples;
            }
            pixels[(static_cast<std::size_t>(y)*w+x)*4+c]=static_cast<unsigned char>(sum/samples);
            pixels[(static_cast<std::size_t>(y)*w+x)*4+3]=255;
        }
    }
}

bool write_tga(const std::string& filename, const std::vector<unsigned char>& rgba, int w, int h)
{
    std::FILE* out = std::fopen(filename.c_str(), "wb");
    if (!out) return false;
    unsigned char header[18]{};
    header[2]=2; header[12]=static_cast<unsigned char>(w); header[13]=static_cast<unsigned char>(w>>8);
    header[14]=static_cast<unsigned char>(h); header[15]=static_cast<unsigned char>(h>>8);
    header[16]=32; header[17]=0x28; // top origin, 8 alpha bits
    bool ok = std::fwrite(header,1,18,out)==18;
    std::vector<unsigned char> row(static_cast<std::size_t>(w)*4);
    for (int y=0;ok && y<h;++y)
    {
        for (int x=0;x<w;++x)
        {
            const auto src=(static_cast<std::size_t>(y)*w+x)*4;
            for (int c=0;c<3;++c) row[static_cast<std::size_t>(x)*4+c]=
                static_cast<unsigned char>((static_cast<unsigned>(rgba[src+2-c])*85u)/100u);
            row[static_cast<std::size_t>(x)*4+3]=255;
        }
        ok = std::fwrite(row.data(),1,row.size(),out)==row.size();
    }
    ok = std::fclose(out)==0 && ok;
    if (!ok) std::remove(filename.c_str());
    return ok;
}

ImportResult unpack(const std::string& id, const std::vector<unsigned char>& bytes,
                    const std::atomic<bool>& stop)
{
    mz_zip_archive zip{};
    if (!mz_zip_reader_init_mem(&zip,bytes.data(),bytes.size(),0)) return failed("Unsupported NXTheme archive (not a ZIP).");
    const auto entries = mz_zip_reader_get_num_files(&zip);
    int selected=-1, best=9999;
    if (entries <= 128)
    {
        for (mz_uint i=0;i<entries;++i)
        {
            if (stop.load()) break;
            mz_zip_archive_file_stat info{};
            if (!mz_zip_reader_file_stat(&zip,i,&info) || info.m_is_directory ||
                info.m_uncomp_size > kMaxImage) continue;
            const int rank = priority(info.m_filename);
            if (rank < best) { best=rank; selected=static_cast<int>(i); }
        }
    }
    if (selected < 0 || best >= 999)
    {
        mz_zip_reader_end(&zip);
        return failed("No supported JPEG/PNG wallpaper in NXTheme.");
    }
    // Preserve the human-readable Themezer name before closing the ZIP.
    std::string theme_title = archive_title(zip);
    if (theme_title.empty()) theme_title = "Themezer " + id;
    std::size_t length=0;
    void* raw = mz_zip_reader_extract_to_heap(&zip,static_cast<mz_uint>(selected),&length,0);
    mz_zip_reader_end(&zip);
    if (!raw || length > kMaxImage || length > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    {
        mz_free(raw);
        return failed("Cannot extract NXTheme wallpaper.");
    }
    int w=0,h=0,components=0;
    const auto* data=static_cast<const stbi_uc*>(raw);
    bool valid = stbi_info_from_memory(data,static_cast<int>(length),&w,&h,&components)!=0 &&
                 w>=500 && h>=280 && w<=4096 && h<=4096 &&
                 static_cast<double>(w)/h>=1.2 && static_cast<double>(w)/h<=2.8;
    stbi_uc* pixels = valid ? stbi_load_from_memory(data,static_cast<int>(length),&w,&h,&components,4) : nullptr;
    mz_free(raw);
    if (!pixels) return failed("Wallpaper decoding failed or image has unsupported dimensions.");
    if (stop.load()) { stbi_image_free(pixels); return failed("Download cancelled."); }
    auto full=resize(pixels,w,h,kWidth,kHeight);
    auto small=resize(pixels,w,h,kBlurWidth,kBlurHeight);
    stbi_image_free(pixels);
    blur(small,kBlurWidth,kBlurHeight);
    if (stop.load()) return failed("Download cancelled.");

    const std::string root=switch_bootstrap::root();
    const std::string installed=root+"/installed";
    const std::string stage=installed+"/.pending-"+id;
    const std::string final=installed+"/"+id;
    if (occupied(final)) return failed("This Quick ID is already installed. Existing themes are never overwritten.");
    if (!directory(root) || !directory(installed)) return failed("Theme storage folder cannot be created.");
    // Never reuse abandoned temporary contents. Only the names listed by cleanup are removed.
    cleanup(stage);
    if (!directory(stage) || !directory(stage+"/art")) return failed("Cannot prepare temporary theme folder.");
    bool ok=write_tga(stage+"/art/backdrop.tga",full,kWidth,kHeight) &&
            write_tga(stage+"/art/backdrop-blur.tga",small,kBlurWidth,kBlurHeight);
    std::FILE* ini=ok?std::fopen((stage+"/palette.ini").c_str(),"wb"):nullptr;
    if (ini)
    {
        const std::string text="name="+theme_title+"\naccent=#62d8e6\ndock_panel=off\n";
        ok=std::fwrite(text.data(),1,text.size(),ini)==text.size() && ok;
        ok=std::fclose(ini)==0 && ok;
    }
    else ok=false;
    if (stop.load()) ok=false;
    if (ok) ok=rename(stage.c_str(),final.c_str())==0;
    if (!ok)
    {
        cleanup(stage);
        return failed("Could not save theme artwork. Existing themes are untouched.");
    }
    return {true,id,"Installed "+theme_title+" ("+id+"). Select it and restart to apply."};
}

// Read a single nested NXTheme from a Themezer pack. Only its wallpaper is
// consumed; Switch layout patches and archive paths are never executed.
struct PackImage
{
    std::vector<unsigned char> full;
    std::vector<unsigned char> small;
};

std::string role_from_text(std::string value)
{
    value = lower(value);
    value.erase(std::remove_if(value.begin(), value.end(), [](unsigned char c) {
        return !std::isalnum(c);
    }), value.end());
    const auto has = [&](const char* word) { return value.find(word) != std::string::npos; };
    if (has("lockscreen") || has("entrance")) return "lockscreen";
    if (has("allapps") || has("allapp") || has("flaunch")) return "all_apps";
    if (has("settings") || has("setmenu")) return "settings";
    if (has("playerselection") || has("playerselect")) return "player_select";
    if (has("userpage") || has("mypage")) return "user";
    if (has("news")) return "news";
    if (has("homemenu") || has("residentmenu") || has("home")) return "home";
    return {};
}

bool nested_artwork(const void* bytes, std::size_t length, const std::atomic<bool>& stop,
                    PackImage& artwork, std::string& role)
{
    if (length < 22 || length > kMaxThemeZip || stop.load()) return false;
    mz_zip_archive inner{};
    if (!mz_zip_reader_init_mem(&inner,bytes,length,0)) return false;
    const mz_uint count = mz_zip_reader_get_num_files(&inner);
    if (count > 128) { mz_zip_reader_end(&inner); return false; }
    int selected = -1, best = 9999;
    const int info_idx = mz_zip_reader_locate_file(&inner,"info.json",nullptr,0);
    if (info_idx >= 0)
    {
        mz_zip_archive_file_stat st{};
        if (mz_zip_reader_file_stat(&inner,static_cast<mz_uint>(info_idx),&st) &&
            st.m_uncomp_size <= 16384)
        {
            std::size_t n = 0;
            void* mem = mz_zip_reader_extract_to_heap(&inner,static_cast<mz_uint>(info_idx),&n,0);
            if (mem && n <= 16384)
            {
                const std::string json(static_cast<const char*>(mem),n);
                for (const char* key : {"Target","target","Menu","menu","SzsName","szsName"})
                    if (auto r = role_from_text(field_value(json,key)); !r.empty()) {role=r;break;}
            }
            mz_free(mem);
        }
    }
    for (mz_uint i=0;i<count;++i)
    {
        mz_zip_archive_file_stat st{};
        if (!mz_zip_reader_file_stat(&inner,i,&st) || st.m_is_directory ||
            st.m_uncomp_size > kMaxImage) continue;
        if (int rank = priority(st.m_filename); rank < best)
        {
            best=rank;selected=static_cast<int>(i);
        }
    }
    if (selected < 0 || best >= 999) { mz_zip_reader_end(&inner); return false; }
    std::size_t n = 0;
    void* mem = mz_zip_reader_extract_to_heap(&inner,static_cast<mz_uint>(selected),&n,0);
    mz_zip_reader_end(&inner);
    if (!mem || n == 0 || n > kMaxImage ||
        n > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    { mz_free(mem); return false; }
    int w=0,h=0,comp=0;
    const auto* data=static_cast<const stbi_uc*>(mem);
    const bool valid = stbi_info_from_memory(data,static_cast<int>(n),&w,&h,&comp) != 0 &&
                       w>=500 && h>=280 && w<=4096 && h<=4096 &&
                       static_cast<double>(w)/h>=1.2 && static_cast<double>(w)/h<=2.8;
    stbi_uc* pixels = valid ? stbi_load_from_memory(data,static_cast<int>(n),&w,&h,&comp,4) : nullptr;
    mz_free(mem);
    if (!pixels) return false;
    if (!stop.load())
    {
        artwork.full=resize(pixels,w,h,kWidth,kHeight);
        artwork.small=resize(pixels,w,h,kBlurWidth,kBlurHeight);
        blur(artwork.small,kBlurWidth,kBlurHeight);
    }
    stbi_image_free(pixels);
    return !stop.load();
}

bool copy_exact(const std::string& src, const std::string& dst)
{
    std::error_code ec;
    return std::filesystem::copy_file(src,dst,std::filesystem::copy_options::none,ec) && !ec;
}

// Atomic install into our own private directory. If any page is unsupported,
// we skip it; HOME is mandatory. Existing installations are never overwritten.
ImportResult unpack_pack(const std::string& id, const std::vector<unsigned char>& bytes,
                         const std::atomic<bool>& stop, const std::string& remote_title)
{
    mz_zip_archive pack{};
    if (!mz_zip_reader_init_mem(&pack,bytes.data(),bytes.size(),0))
        return failed("Themezer pack is not a supported ZIP archive.");
    const std::string embedded_title = archive_title(pack);
    const mz_uint entries=mz_zip_reader_get_num_files(&pack);
    if (entries == 0 || entries > 96)
    { mz_zip_reader_end(&pack); return failed("Themezer pack has too many files."); }
    const std::string root=switch_bootstrap::root();
    const std::string installed=root+"/installed";
    const std::string final=installed+"/"+id;
    const std::string stage=installed+"/.pending-"+id;
    if (occupied(final) || occupied(stage))
    { mz_zip_reader_end(&pack); return failed("Pack already installed or previous staging directory exists."); }
    if (!switch_bootstrap::directory_or_create(root))
    { mz_zip_reader_end(&pack); return failed("Theme library root is inaccessible (themes/)."); }
    if (!switch_bootstrap::directory_or_create(installed))
    { mz_zip_reader_end(&pack); return failed("Theme install folder is inaccessible (themes/installed/)."); }
    // Require fresh exclusive creation: never reuse another install's stage.
    if (mkdir(stage.c_str(), 0755) != 0)
    { mz_zip_reader_end(&pack); return failed("Cannot create private pack staging directory."); }
    if (mkdir((stage+"/art").c_str(), 0755) != 0 ||
        mkdir((stage+"/art/pages").c_str(), 0755) != 0)
    {
        // Only the directories just created by this operation exist so far.
        // POSIX cleanup works even if C++ filesystem status fails on PS5.
        rmdir((stage+"/art/pages").c_str());
        rmdir((stage+"/art").c_str());
        rmdir(stage.c_str());
        mz_zip_reader_end(&pack);
        return failed("Cannot prepare private pack artwork directory.");
    }
    // The private staging directory has been created by this operation; its
    // filenames are generated from a fixed allowlist, never from ZIP entries.
    const auto abandon = [&] {
        std::error_code ec;
        std::filesystem::remove_all(stage,ec);
    };
    bool home=false, error=false;
    std::vector<std::string> roles;
    for (mz_uint i=0; i<entries && !stop.load(); ++i)
    {
        mz_zip_archive_file_stat info{};
        if (!mz_zip_reader_file_stat(&pack,i,&info)) {error=true;break;}
        if (info.m_is_directory || info.m_uncomp_size==0) continue;
        const std::string filename=lower(info.m_filename);
        if (!filename.ends_with(".nxtheme")) continue;
        if (info.m_uncomp_size > kMaxThemeZip) {error=true;break;}
        std::size_t count=0;
        void* nested=mz_zip_reader_extract_to_heap(&pack,i,&count,0);
        if (!nested || count == 0 || count > kMaxThemeZip)
        { mz_free(nested); error=true; break; }
        std::string role;
        PackImage art;
        const bool decoded=nested_artwork(nested,count,stop,art,role);
        mz_free(nested);
        if (role.empty()) role=role_from_text(filename);
        if (!decoded || role.empty() ||
            std::find(roles.begin(),roles.end(),role)!=roles.end()) continue;
        const std::string prefix=stage+"/art/pages/"+role;
        if (!write_tga(prefix+".tga",art.full,kWidth,kHeight) ||
            !write_tga(prefix+"-blur.tga",art.small,kBlurWidth,kBlurHeight))
        {error=true;break;}
        roles.push_back(role);
        if (role=="home") home=true;
    }
    mz_zip_reader_end(&pack);
    if (stop.load() || error || !home)
    {
        abandon();
        return failed(stop.load() ? "Download cancelled." :
                      "Pack import failed or no compatible Home Menu artwork exists.");
    }
    const std::string prefix=stage+"/art/pages/home";
    bool ok=copy_exact(prefix+".tga",stage+"/art/backdrop.tga") &&
            copy_exact(prefix+"-blur.tga",stage+"/art/backdrop-blur.tga");
    const auto write=[&](const std::string& name,const std::string& content) {
        std::ofstream out(stage+"/"+name,std::ios::binary);
        if (!out) return false;
        out << content;
        return out.good();
    };
    const std::string title = !remote_title.empty() ? remote_title :
        (!embedded_title.empty() ? embedded_title : "Themezer pack " + id);
    ok=ok && write("palette.ini","name="+title+"\naccent=#60d5db\n") &&
       write("pages.ini",
             "home=home\nlibrary=all_apps\nsettings=settings\nthemes=settings\n"
             "profiles=user\nmultiplayer=home\nrooms=home\n"
             "game_details=all_apps\nabout=settings\nstartup=lockscreen\n");
    if (ok && !stop.load()) ok=rename(stage.c_str(),final.c_str())==0;
    if (!ok || stop.load())
    {
        abandon();
        return failed(stop.load() ? "Download cancelled." : "Could not install pack. Existing themes are untouched.");
    }
    return {true,id,"Installed "+title+" with "+std::to_string(roles.size())+
            " menu backgrounds. Select it to apply."};
}

} // namespace

std::string normalize_quick_id(const std::string& input)
{
    std::string code;
    for (unsigned char c : input)
    {
        if (c==' ' || c=='\n' || c=='\r') continue;
        code += static_cast<char>(std::toupper(c));
    }
    char type='T';
    if (!code.empty() && (code.front()=='T' || code.front()=='P'))
    {
        type=code.front();
        code.erase(code.begin());
    }
    if (code.size()<3 || code.size()>8 ||
        (type=='T' && code.size()<4)) return {};
    for (unsigned char c : code) if (!std::isxdigit(c)) return {};
    return std::string(1,type)+code;
}

ImportResult install_quick_id(std::string input, const std::atomic<bool>& stop)
{
    const std::string id=normalize_quick_id(input);
    if (id.empty()) return failed("Enter a Themezer T theme or P pack Quick ID.");
    if (stop.load()) return failed("Download cancelled.");
    const bool pack=id.front()=='P';
    if (switch_bootstrap::usable(std::filesystem::path(switch_bootstrap::root()) / "installed" / id))
    {
        if (!pack) return failed("This theme is already installed.");
        const std::string current = switch_bootstrap::name_from_ini(
            std::filesystem::path(switch_bootstrap::root()) / "installed" / id / "palette.ini", id);
        const std::string placeholder = "Themezer pack " + id;
        if (current != placeholder && current != id)
            return {false, id, "Pack already installed as " + current + "."};
        const std::string title = lookup_pack_name(id, stop);
        if (title.empty()) return failed("Could not retrieve the pack title. No files changed.");
        if (!refresh_installed_pack_name(id, title))
            return failed("Could not update the pack title. No artwork changed.");
        return {true, id, "Updated pack name to " + title + "."};
    }
    const std::string url="https://api.themezer.net/switch/" +
            std::string(pack ? "packs/" : "themes/") + id.substr(1) +
            "/download?src=webapp";
    Sink sink{}; sink.stop=&stop; sink.limit=pack ? kMaxZip : 32u*1024u*1024u;
    remote_http_request req{};
    req.url=url.c_str(); req.https_only=1; req.timeout_ms=60000;
    req.raw=1; req.sink=receive; req.begin=start_body; req.stop=cancelled; req.user=&sink;
    remote_http_result result{};
    const int rc=remote_http_get(&req,&result);
    if (stop.load()) return failed("Download cancelled.");
    if (rc!=0 || result.status!=200)
    {
        if (result.status==403) return failed("Themezer denied the download (403). Try later or import the pack via Linux.");
        return failed("Themezer download failed. Check PS5 internet connectivity.");
    }
    if (sink.bytes.size()<22 || sink.bytes.size()>sink.limit)
        return failed("Downloaded theme or pack is empty or oversized.");
    const std::string pack_title = pack ? lookup_pack_name(id, stop) : std::string{};
    if (stop.load()) return failed("Download cancelled.");
    return pack ? unpack_pack(id,sink.bytes,stop,pack_title) : unpack(id,sink.bytes,stop);
}
} // namespace pe::ui::switch_theme
