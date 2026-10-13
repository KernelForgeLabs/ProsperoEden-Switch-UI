// ProsperoEden - Launcher sound cues and the loaded sound bank.
// Copyright (C) 2026 BlackBearReloaded
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pe/audio/sounds.hpp"

#include "pe/audio/wav.hpp"
#include "pe/core/file.hpp"

#include <algorithm>
#include <cstdio>

namespace pe::audio
{

namespace
{

struct CueInfo
{
    const char *name;
    Wave wave; // the synthesized stand-in
    float start_hz;
    float end_hz;
    float seconds;
    float gain;
};

constexpr CueInfo kCues[kCueCount] = {
    {"focus", Wave::sine, 660, 700, 0.035f, 0.22f},
    {"select", Wave::triangle, 740, 880, 0.08f, 0.36f},
    {"back", Wave::triangle, 520, 420, 0.08f, 0.32f},
    {"page", Wave::sine, 560, 640, 0.06f, 0.28f},
    {"toggle", Wave::triangle, 700, 700, 0.05f, 0.28f},
    {"slider", Wave::sine, 900, 900, 0.03f, 0.2f},
    {"open", Wave::sine, 420, 660, 0.16f, 0.3f},
    {"modal_open", Wave::triangle, 440, 560, 0.12f, 0.28f},
    {"modal_close", Wave::triangle, 560, 440, 0.12f, 0.28f},
    {"error", Wave::soft_square, 200, 160, 0.14f, 0.28f},
    {"saved", Wave::sine, 660, 990, 0.2f, 0.32f},
    {"launch", Wave::sine, 300, 900, 0.5f, 0.34f},
    {"welcome", Wave::sine, 392, 784, 0.6f, 0.3f},
    {"resume", Wave::sine, 523, 660, 0.3f, 0.28f},
    {"notify", Wave::sine, 660, 990, 0.3f, 0.32f},
};

} // namespace

const char *cue_name(Cue cue)
{
    return cue < Cue::count ? kCues[static_cast<std::size_t>(cue)].name : "";
}

int SoundBank::load(const std::string &directory, std::vector<std::string> *errors)
{
    int files = 0;
    for (std::size_t cue = 0; cue < kCueCount; ++cue)
    {
        sets_[cue].clear();
        next_[cue] = 0;
        for (int variation = 1; variation <= 8; ++variation)
        {
            char name[64];
            std::snprintf(name, sizeof(name), "%s_%02d.wav", kCues[cue].name, variation);
            std::string data;
            if (!read_file(directory + "/" + name, &data, 16u << 20))
                break;
            DecodedWav wav = decode_wav(data);
            if (!wav.ok())
            {
                if (errors != nullptr)
                    errors->push_back(std::string(name) + ": " + wav.error);
                continue;
            }
            auto sound = std::make_unique<Sound>();
            sound->samples = std::move(wav.samples);
            sound->clip.samples = sound->samples.data();
            sound->clip.frames = wav.frames;
            sets_[cue].push_back(std::move(sound));
            ++files;
        }
    }
    return files;
}

void SoundBank::play(Mixer &mixer, Cue cue)
{
    if (cue >= Cue::count)
        return;
    const std::size_t index = static_cast<std::size_t>(cue);
    PlayParams params;
    params.bus = Bus::ui;
    auto &set = sets_[index];
    const bool soft_cue = soft_navigation_ &&
        (cue == Cue::focus || cue == Cue::select || cue == Cue::back || cue == Cue::page);
    if (!set.empty() && !soft_cue)
    {
        // +-3 % pitch keeps repeated sounds from feeling mechanical.
        jitter_state_ = jitter_state_ * 1664525u + 1013904223u;
        params.pitch = 0.97f + 0.06f * static_cast<float>(jitter_state_ >> 8) / 16777216.0f;
        const Sound &sound = *set[next_[index] % set.size()];
        next_[index] = (next_[index] + 1) % set.size();
        mixer.play_clip(&sound.clip, params);
        return;
    }
    const CueInfo &info = kCues[index];
    Tone tone;
    tone.wave = soft_cue ? Wave::sine : info.wave;
    tone.start_hz = soft_cue ? (cue == Cue::focus ? 880.0f : cue == Cue::back ? 490.0f : 740.0f) : info.start_hz;
    tone.end_hz = soft_cue ? (cue == Cue::focus ? 950.0f : cue == Cue::back ? 420.0f : 880.0f) : info.end_hz;
    tone.seconds = soft_cue ? (cue == Cue::focus ? 0.026f : 0.060f) : info.seconds;
    tone.attack = std::min(0.006f, tone.seconds * 0.2f);
    tone.release = std::min(0.12f, tone.seconds * 0.5f);
    params.gain = soft_cue ? 0.16f : info.gain;
    mixer.play_tone(tone, params);
}

} // namespace pe::audio
