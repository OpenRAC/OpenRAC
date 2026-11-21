// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The mods' interface, area "sound": the api_<member> functions of the openrac_api members
// sdk/api/members.tsv puts in this area, and what only they use.

#include "mod_api/internal.h"

using namespace openrac;

namespace openrac::mods::detail {

// The most sounds the mods load together (OPENRAC_SOUNDS_MAX each).
constexpr std::size_t kSoundsMax = 4 * OPENRAC_SOUNDS_MAX;

int api_hero_sound_play(openrac_mod* mod, int sound, int loop) {
    std::int32_t level = -1;
    if (mod == nullptr || g_play == nullptr || g_play->hero_sound_play == nullptr || sound < 0
        || !in_level(phase_now(&level))) {
        return -1;
    }
    const int voice = g_play->hero_sound_play(sound, loop != 0 ? 1 : 0);
    const std::uint32_t loads = loads_now();
    std::erase_if(g_hero_voices, [&](const HeroVoice& v) {
        return v.loads != loads || v.voice == voice;
    });
    if (voice >= 0 && loop != 0) {
        g_hero_voices.push_back({mod, loads, voice});
    }
    return voice;
}

void api_hero_sound_stop(openrac_mod* mod, int voice) {
    if (mod == nullptr || g_play == nullptr || g_play->hero_sound_stop == nullptr || voice < 0) {
        return;
    }
    std::erase_if(g_hero_voices, [&](const HeroVoice& v) { return v.voice == voice; });
    g_play->hero_sound_stop(voice);
}

int api_sound_load(openrac_mod* mod, const char* path) {
    if (mod == nullptr || g_sound_load == nullptr || g_sounds.size() >= kSoundsMax) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const auto own = std::count_if(g_sounds.begin(), g_sounds.end(), [&](const Sound& s) {
        return s.mod == mod;
    });
    const std::filesystem::path file = mod_file(mod, path);
    const std::filesystem::path ext = file.extension();
    if (own >= OPENRAC_SOUNDS_MAX || file.empty() || (ext != ".wav" && ext != ".ogg")) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    const int host = g_sound_load(file);
    if (host < 0) {
        log::warn("mods: {} could not read the sound {}", mod_id(mod), path);
        return OPENRAC_HOOK_UNKNOWN;
    }
    g_sounds.push_back({mod, host});
    return static_cast<int>(g_sounds.size() - 1);
}

// Voices that ended (the host's sound_set answers no more) are forgotten as new ones are asked for.
void forget_ended_voices() {
    std::erase_if(g_voices, [](const auto& v) {
        return g_sound_set == nullptr || g_sound_set(v.second, -1.0f, -1.0f) == 0;
    });
}

std::uint32_t api_sound_play(openrac_mod* mod, int sound, float volume, float pitch, int loop) {
    if (mod == nullptr || g_sound_play == nullptr || sound < 0
        || sound >= static_cast<int>(g_sounds.size())
        || g_sounds[static_cast<std::size_t>(sound)].mod != mod || !std::isfinite(volume)
        || !std::isfinite(pitch)) {
        return 0;
    }
    forget_ended_voices();
    if (g_voices.size() >= OPENRAC_SOUND_VOICES_MAX) {
        return 0;
    }
    const std::uint32_t voice = g_sound_play(
        g_sounds[static_cast<std::size_t>(sound)].host,
        std::max(volume, 0.0f),
        std::max(pitch, 0.0f),
        loop != 0
    );
    if (voice != 0) {
        g_voices.emplace_back(mod, voice);
    }
    return voice;
}

// Whether `voice` is one the mod played.
bool own_voice(const openrac_mod* mod, std::uint32_t voice) {
    return mod != nullptr && voice != 0
           && std::find(g_voices.begin(), g_voices.end(), std::make_pair(mod, voice))
                  != g_voices.end();
}

int api_sound_set(openrac_mod* mod, std::uint32_t voice, float volume, float pitch) {
    if (g_sound_set == nullptr || !own_voice(mod, voice) || !std::isfinite(volume)
        || !std::isfinite(pitch)) {
        return OPENRAC_HOOK_UNKNOWN;
    }
    return g_sound_set(voice, std::max(volume, 0.0f), std::max(pitch, 0.0f)) != 0
               ? OPENRAC_HOOK_OK
               : OPENRAC_HOOK_UNKNOWN;
}

void api_sound_stop(openrac_mod* mod, std::uint32_t voice) {
    if (g_sound_stop != nullptr && own_voice(mod, voice)) {
        g_sound_stop(voice);
        std::erase(g_voices, std::make_pair(static_cast<const openrac_mod*>(mod), voice));
    }
}

}  // namespace openrac::mods::detail
