// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

#include "platform/audio.h"

#include <format>
#include <utility>

#include <SDL3/SDL.h>

namespace openrac::platform {
namespace {

constexpr int kFrameBytes = kAudioChannels * static_cast<int>(sizeof(std::int16_t));

}  // namespace

std::unique_ptr<AudioOutput> AudioOutput::open(std::string& error, Callback callback) {
    std::unique_ptr<AudioOutput> out(new AudioOutput());
    out->m_callback = std::move(callback);
    const SDL_AudioSpec spec{SDL_AUDIO_S16, kAudioChannels, kAudioRate};
    // SDL converts to whatever the device really runs at; the game always
    // sees 48 kHz stereo.
    out->m_stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
        &spec,
        out->m_callback ? &AudioOutput::feed : nullptr,
        out->m_callback ? out.get() : nullptr
    );
    if (out->m_stream == nullptr) {
        error = std::format("could not open the audio device: {}", SDL_GetError());
        return nullptr;
    }
    // Opened paused; start it.
    SDL_ResumeAudioStreamDevice(out->m_stream);
    return out;
}

AudioOutput::~AudioOutput() {
    if (m_stream != nullptr) {
        SDL_DestroyAudioStream(m_stream);
    }
}

void AudioOutput::feed(void* self, SDL_AudioStream* stream, int additional, int /*total*/) {
    auto* out = static_cast<AudioOutput*>(self);
    if (additional <= 0) {
        return;
    }
    const int frames = (additional + kFrameBytes - 1) / kFrameBytes;
    out->m_scratch.assign(static_cast<std::size_t>(frames * kAudioChannels), 0);
    out->m_callback(out->m_scratch);
    SDL_PutAudioStreamData(stream, out->m_scratch.data(), frames * kFrameBytes);
}

bool AudioOutput::queue(std::span<const std::int16_t> samples) {
    if (samples.empty()) {
        return true;
    }
    return SDL_PutAudioStreamData(m_stream, samples.data(), static_cast<int>(samples.size_bytes()));
}

int AudioOutput::queued_frames() const {
    const int bytes = SDL_GetAudioStreamQueued(m_stream);
    return bytes > 0 ? bytes / kFrameBytes : 0;
}

void AudioOutput::pause(bool paused) {
    if (paused) {
        SDL_PauseAudioStreamDevice(m_stream);
    } else {
        SDL_ResumeAudioStreamDevice(m_stream);
    }
}

void AudioOutput::set_gain(float gain) {
    SDL_SetAudioStreamGain(m_stream, gain);
}

}  // namespace openrac::platform
