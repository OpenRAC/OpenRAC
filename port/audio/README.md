# audio

The sound engine the games' sound library drives: library `openrac_audio`,
namespace `openrac::audio`. All four games use the same library, 989snd,
whose work ran on the console's I/O processor and sound processor; on the
PC this engine does that work, and `port/game/common/lib/snd.c` (the
library replaced at its API) calls it. OpenGOAL did the same for Jak and
Daxter's build of the library, and this engine follows its behaviour
(https://github.com/open-goal/jak-project, `game/sound/989snd`; ISC License,
Copyright (c) 2020-2026 OpenGOAL Team), with
[ReRAC](https://github.com/re-rac/rerac)'s notes on RAC1's banks and voices
(ISC License, Copyright (c) 2026 ReRAC contributors). It is not an
emulation of the program or of the console: nothing runs the library's code
or models the processors' memory, registers or voices. The tones are mixed by
a native float mixer.

| File | |
|---|---|
| `sound_player.h` | banks loaded, sounds played: each sound's grain script run at 240 Hz (tones, loops, registers, random choices, pitch bends, LFOs), tones allocated onto the library's 48 voices by priority (its policy, not a chip's channels), instance limits, master volume groups, pausing; the volume and pan curves computed, not copied |
| `mixer.h` | the native mixer: each tone's sample decoded once to float at bank load, with its loop; its envelope read once from the bank's ADSR fields into rates and levels; voices played at a speed through a cubic Hermite resampler, at a left and right gain, summed in float with the reverb |
| `limiter.h` | the output's one limiter: the float mix (tones, reverb, streams, the mods' and the menu's sounds) kept under full scale, then 16 bits for the device |
| `pitch.h` | notes to speeds, pitch bend |
| `reverb.h` | a native stereo reverb with the character of each of the library's effect types (measured results, not the library's tables) |
| `ui_sounds.h` | the port's own menu sounds (not the game's): rounded pops (falling sines, a little filtered air) with a soft start, through a low-pass and a small room, mixed over the game's sound by the output, while the game is paused too |
| `sound_replacements.h` | the sounds packs replace (`audio/<key>.wav` of the mods folder, [extractor/AUDIO_MODDING.md](../../extractor/AUDIO_MODDING.md)): each sound's key, a hash of what it is, and its new sample |

The games drive it through `port/game/common/lib/sound_engine.cpp`: `snd.c`
replaces the library at its API (rac1's calls, and Going Commando's and Up Your
Arsenal's client entry points, whose answers go to their callbacks at the next
`snd_Pump` as the library hands them on), loads banks, plays sounds, sets
volumes and the reverb; rac1's command flush passes the sounds the game
batched; the game window's audio output pulls the float mix
(`openrac_sound_render_float`), adds the menu's sounds, and limits it once.
Music and speech (VAG streams) are played there too: each track read from the
disc and decoded on its own thread, a loop queued behind its intro, mixed after
the sounds. `OPENRAC_AUDIO_STATS=1` logs the mix's levels every 10 s. Not yet:
reverb on streams, movie sound through this engine.

## Tests

`tests/audio/`: the mixer (samples decoded with their loops, envelopes as
rates, speeds, the limiter); banks built by the test: volume and pan, a tone
keyed on a voice and released, register and skipping grains, instance limits, pausing,
group volumes, unloading; sounds packs replace, and the keys, with the same
answers as `extractor/test_audio.py`; the menu sounds: soft starts, quiet peaks, short, and a held
arrow's repeats no louder than one.
