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
#include <stb_image.h>
#pragma GCC diagnostic pop

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <filesystem>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <unistd.h>

namespace pe::ui::switch_theme
{
namespace
{
constexpr std::size_t kMaxZip = 32u * 1024u * 1024u;
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
};
int receive(void* ptr, const void* data, std::size_t count)
{
    auto& sink = *static_cast<Sink*>(ptr);
    if (sink.stop->load() || count > kMaxZip - sink.bytes.size()) return 0;
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
    return !sink.stop->load() && status == 200 && size <= kMaxZip;
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
    struct stat existing{};
    if (lstat(final.c_str(),&existing)==0) return failed("This Quick ID is already installed. Existing themes are never overwritten.");
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
} // namespace

std::string normalize_quick_id(const std::string& input)
{
    std::string code;
    for (unsigned char c : input)
    {
        if (c==' ' || c=='\n' || c=='\r') continue;
        code += static_cast<char>(std::toupper(c));
    }
    if (!code.empty() && code.front()=='T') code.erase(code.begin());
    if (code.size()<4 || code.size()>8) return {};
    for (unsigned char c : code)
        if (!std::isxdigit(c)) return {};
    return "T"+code;
}

ImportResult install_quick_id(std::string input, const std::atomic<bool>& stop)
{
    const std::string id=normalize_quick_id(input);
    if (id.empty()) return failed("Enter a Home Menu ID such as TF7CA (pack IDs P... are unsupported).");
    if (stop.load()) return failed("Download cancelled.");
    if (switch_bootstrap::usable(std::filesystem::path(switch_bootstrap::root()) / "installed" / id))
        return failed("This theme is already installed.");
    // Fixed HTTPS endpoint, no user-provided URL or arbitrary filesystem path.
    const std::string url="https://api.themezer.net/switch/themes/"+id.substr(1)+"/download?src=webapp";
    Sink sink{}; sink.stop=&stop;
    remote_http_request req{};
    req.url=url.c_str(); req.https_only=1; req.timeout_ms=45000;
    req.raw=1; req.sink=receive; req.begin=start_body; req.stop=cancelled; req.user=&sink;
    remote_http_result result{};
    const int rc=remote_http_get(&req,&result);
    if (stop.load()) return failed("Download cancelled.");
    if (rc!=0 || result.status!=200)
    {
        if (result.status==403) return failed("Themezer denied this download (403). Try later or use the Linux bridge.");
        return failed("Themezer download failed. Check PS5 internet connectivity.");
    }
    if (sink.bytes.size()<22 || sink.bytes.size()>kMaxZip) return failed("Downloaded theme is empty or oversized.");
    return unpack(id,sink.bytes,stop);
}
} // namespace pe::ui::switch_theme
