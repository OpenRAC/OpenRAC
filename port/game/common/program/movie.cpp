// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors
//
// The game's movies in the window (program::play_movie, openrac_game_play_movie's body in
// program/game_host.cpp): a PSS movie read by sector from the disc's files, its picture shown and
// its sound played, blocking as the game's own player does. Without a window or without the movie
// support (OPENRAC_MOVIES) there is none, and the game goes on as if it had played.
// The player follows ReRAC's movie mode (ISC License): the comment above play_movie credits it.

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "common/breadcrumbs.h"
#include "common/log.h"
#include "common/precise_wait.h"
#include "openrac/game_host.h"
#include "program/program.h"

#if defined(OPENRAC_FRONTEND) && defined(OPENRAC_MOVIES)
#include "frontend.h"
#include "game/common/frontend/touch_controls.h"
#include "media/movie.h"
#include "platform/audio.h"
#include "settings.h"
#endif

namespace openrac::program {

/*
 * The movie player, after ReRAC's movie mode (crates/rc-game/src/movie_player.rs,
 * crates/rc-engine/src/movie_render.rs; ISC License, Copyright (c) 2026 ReRAC contributors):
 * the audio is the clock. Each vertical blank hands the device one blank's worth of the movie's
 * 48 kHz audio, and the picture shown is the one whose time holds the sample being heard (what has
 * been handed over less what the device still holds), so picture and sound cannot drift apart.
 * When the video ends (or is skipped) the last picture stays up for the console's FadeToBlack(4).
 */
int play_movie(
    uint32_t lsn,
    uint32_t bytes,
    int channel,
    int start_skips,
    void (*overlay)(const std::uint8_t* rgba, int width, int height)
) {
    if (openrac_game_map != nullptr && openrac_game_map->skip_movies) {
        log::info("map {}: disc movie at sector {} omitted", openrac_game_map->name, lsn);
        return 1;
    }

    if (g_options.skip_movies) {
        log::info("movie at sector {}: skipped by --skip-movies (development probe)", lsn);
        return 1;  // the same result as the movie player's Start-skip path
    }
#if defined(OPENRAC_FRONTEND) && defined(OPENRAC_MOVIES)
    if (!g_window || !openrac_disc_available() || bytes == 0) {
        return 0;
    }

    // The PSS file, by sector from the disc's files (its lump, through the sector map).
    std::vector<std::uint8_t> file(bytes);

    if (!openrac_disc_read(static_cast<std::uint64_t>(lsn) * 2048, file.data(), file.size())) {
        log::warn("movie at sector {}: the disc's files end before it does", lsn);
        return 0;
    }

    std::optional<media::MoviePlayer> player;

    try {
        player.emplace(media::Movie::open(file, static_cast<std::uint8_t>(channel)));
    } catch (const std::exception& e) {
        log::warn("movie at sector {}: {}", lsn, e.what());
        return 0;
    }

    const media::VideoSequence& sequence = player->movie().sequence();
    log::info(
        "movie at sector {}: {}x{}, {}/{} fps, audio channel {}",
        lsn,
        sequence.width,
        sequence.height,
        sequence.fps_num,
        sequence.fps_den,
        player->movie().audio_channel() ? int(*player->movie().audio_channel()) : -1
    );
    std::string error;
    auto output = platform::AudioOutput::open(error);

    if (!output) {
        log::warn("movie: no sound ({})", error);
    }

    const int rate = tick_rate();
    const std::size_t per_blank = static_cast<std::size_t>(platform::kAudioRate / rate);
    std::vector<std::int16_t> samples(per_blank * 2);
    std::vector<std::uint8_t> rgba;
    std::shared_ptr<const media::VideoFrame> shown;
    std::uint16_t previous = 0xFFFF;
    int skipped = 0;
    auto pace = [&] {
        using namespace std::chrono;
        const auto now = steady_clock::now();

        if (g_next_frame.time_since_epoch().count() == 0
            || now > g_next_frame + milliseconds(100)) {
            g_next_frame = now;
        }

        g_next_frame += microseconds(1000000 / rate);
        precise_wait::until(g_next_frame);
        g_frame++;
    };

    // The touch pad shows its skip button while Start skips the movie.
    struct TouchSkip {
        explicit TouchSkip(bool on) { frontend::touch_controls::set_movie(on); }

        ~TouchSkip() { frontend::touch_controls::set_movie(false); }
    } touch_skip(start_skips != 0);

    // A movie the game draws itself fills the window (frontend::set_movie_playing).
    struct MoviePlaying {
        explicit MoviePlaying(bool on) : on(on) { frontend::set_movie_playing(on); }

        ~MoviePlaying() {
            if (on) {
                frontend::set_movie_playing(false);
            }
        }

        bool on;
    } movie_playing(overlay != nullptr);

    for (std::uint64_t blank = 0;; ++blank) {
        if (output) {
            output->set_gain(settings::output_gain());
            const std::size_t got = player->read_audio(samples);

            if (got > 0) {
                output->queue(std::span<const std::int16_t>(samples.data(), got * 2));
            }
        }

        const std::uint64_t handed = (blank + 1) * per_blank;
        const std::uint64_t held = output ? static_cast<std::uint64_t>(output->queued_frames()) : 0;
        const double heard =
            static_cast<double>(handed > held ? handed - held : 0) / platform::kAudioRate;
        auto frame = player->frame_at(heard);

        if (frame) {
            shown = frame;
        }

        if (player->video_finished()) {
            break;
        }

        if (shown) {
            media::frame_to_rgba(*shown, rgba);

            if (overlay != nullptr) {
                overlay(rgba.data(), int(shown->width), int(shown->height));
            } else if (!frontend::show_picture(
                           rgba.data(), int(shown->width), int(shown->height), 0.0f
                       )) {
                leave();
            }
        }

        // The list the game's player sends each blank (the picture and what it draws over it), as
        // a tick of the game: vsync draws it, reads the window and paces the blank.
        if (overlay != nullptr && shown) {
            vsync();
        } else if (overlay != nullptr) {
            pace();
        }

        // Start skips when pressed during the movie: what is held as it starts is not a press.
        std::uint16_t buttons = 0xFFFF;
        std::uint8_t analog[4];
        openrac_game_pad_through_mods(0, &buttons, analog);  // a mod's key bound to Start skips too

        if (blank == 0) {
            previous = buttons;
        }

        const bool start_now = (buttons & 0x0008) == 0;
        const bool start_before = (previous & 0x0008) == 0;
        previous = buttons;

        if (start_skips && start_now && !start_before) {
            crash::breadcrumb(crash::BreadcrumbThread::Main, "movie skip requested");
            log::info("movie at sector {}: Start pressed (pad {:#06x})", lsn, buttons);
            skipped = 1;
            break;
        }

        if (overlay == nullptr) {
            pace();  // with an overlay, vsync (or the pace above) paced the blank
        }
    }

    // FadeToBlack(4) over the last picture, then its closing black blank. A game that draws the
    // picture itself is left on its last frame, which the game's own transition then follows.
    if (shown) {
        crash::breadcrumb(crash::BreadcrumbThread::Main, "movie fade-out started");
    }
    for (int step = 0; step <= 4 && shown && overlay == nullptr; ++step) {
        frontend::show_picture(
            rgba.data(),
            int(shown->width),
            int(shown->height),
            step >= 4 ? 1.0f : float(step + 1) / 4.0f
        );
        pace();
    }

    if (output) {
        output->pause(true);
    }

    crash::breadcrumb(crash::BreadcrumbThread::Main, "movie returning to game");
    log::info("movie at sector {}: {}", lsn, skipped ? "skipped" : "played");
    return skipped;
#else
    (void)lsn;
    (void)bytes;
    (void)channel;
    (void)start_skips;
    (void)overlay;
    return 0;
#endif
}

}  // namespace openrac::program
