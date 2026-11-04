# assets/sound

The games' sound and movie containers: library `openrac_assets_sound`,
namespace `openrac::assets`. Converted from [ReRAC](https://github.com/re-rac/rerac)'s
`rc-formats` and its specs (ISC License, Copyright (c) 2026 ReRAC
contributors); each file names its source.

| File | |
|---|---|
| `adpcm.h` | the console's sound ADPCM: 16-byte frames, their prediction filters, where a sample ends and loops (the same in all four games), and an encoder for what a pack replaces a sound with |
| `vag.h` | VAG files: music and speech streams |
| `sound_bank.h` | 989snd banks: the SFX block (sounds and their grain scripts) and the sample chunk; grain types as OpenGOAL names them for Jak's build of the same library |
| `level_sounds_rac1.h` | what a RAC1 level says about its sounds beside the bank: the sound definitions and their remap, sound instances, environment sample points |
| `pss.h` | PSS movies split into the video stream and the audio channels (MPEG-2 program stream, a public standard) |
| `wav.h` | WAV files: what a pack replaces a sound with, 16-bit PCM only |

The movies are decoded by [media](../../media/README.md); the banks played by
[audio](../../audio/README.md).

## Tests

`tests/sound/`: synthetic ADPCM frames, VAG files, WAV files, banks, level sound
tables and PSS streams.
