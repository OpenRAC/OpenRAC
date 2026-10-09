# media

The games' movies: library `openrac_media`, namespace `openrac::media`.
An MPEG-2 video decoder of OpenRAC's own, from the public standard
(ISO/IEC 13818-2), converted from [ReRAC](https://github.com/re-rac/rerac)'s
`rc-video` (ISC License, Copyright (c) 2026 ReRAC contributors). Nothing
here comes from Sony's movie library or its sample players.

| File | |
|---|---|
| `bit_reader.h` | big-endian bit reading and start codes |
| `vlc.h` | the standard's variable-length code tables (Annex B), as direct lookups |
| `idct.h` | the 8x8 inverse DCT (Annex A), meeting IEEE 1180-1990 |
| `mpeg2.h` | the decoder, scoped to what the games' movies use (Main Profile at Main Level, 4:2:0, frame pictures; I, P and B); anything else is an error, never a guess |
| `movie.h` | a movie opened for playback: PSS split ([assets/sound/pss.h](../assets/sound/pss.h)), one audio channel to 48 kHz stereo, frames as RGBA, the frame due at a time |

RAC1's movies are surveyed (all 84 decode within this scope); the other
games' have not been checked yet.

## Tests

`tests/media/`: the bit reader, the code tables, the IDCT against IEEE 1180's
generator and limits, and small MPEG-2 streams written by the test itself.
