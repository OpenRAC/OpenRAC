# Sound banks, sound definitions and VAG audio

The sound data of RAC1: 989snd bank files (a sound block and raw SPU ADPCM
samples), the game's own sound definitions and the remap that ties them to
bank sounds, VAG streams for music and speech, and the ADPCM frame format.
ReRAC read them from the NTSC-U disc (`SCUS_971.99`) and decodes every bank of
the 19 levels plus the global bank (5,392 sounds, 10,590 grains, 4,806
samples, 56.6 M PCM samples). The runtime is in [AUDIO.md](../systems/AUDIO.md).

989snd is Sony's sound library; OpenGOAL reimplements the same library
(`game/sound/989snd`, ISC), which ReRAC used as the reference for the player.
Nothing here comes from Sony's source.

**Games.** RAC1. RAC2's boot carries 989snd identifiers too
([README.md, section 6](../README.md#6-other-subsystems)); the sequels' bank
versions are not measured.

## 1. Where the data is

| Data | Where |
|---|---|
| Level bank | level data +0x08 ([LEVEL.md](LEVEL.md)) |
| Global bank | table of contents +0x14E0 |
| Sound definitions and remap | core index +0x70 |
| Class sound definitions | moby class +0x28, count at +0x0D ([MOBY.md](MOBY.md)) |
| Music | level header `music[15]` ([DISC.md](DISC.md#5-the-level-header-0x2434-bytes)) |
| Cutscene speech | scene records, one VAG per language |
| Help, vendor, space, boss, credits audio | table of contents VAG ranges |
| IOP modules | table of contents `irx` (+0x12C0) |

**IOP module bundle**: decompressed, a table of 24 `{offset, size}` at 0.
Entries 14–22 are sio2man 2.05, mcman, mcserv, Dbc_Manager, sio2d, ds2u,
IOP_stash_daemon, libsd 3.03 and 989snd 2.09 ("NO MIDI VERSION": sound
blocks and VAG streams only). Entries 0–13 (except 3 and 9) are compressed
0xD0000-byte blobs of unknown purpose. `IOPRP243.IMG` holds only kernel
modules.

## 2. Bank file

`u32 type = 3`, `u32 chunk count = 2`, then two `{offset, size}`: chunk 0 the
sound block, chunk 1 raw SPU ADPCM without headers. Sample offsets in the
block are relative to chunk 1.

**`SBlk` header** (block-relative):

| Offset | Content |
|---|---|
| 0x00 | `SBlk` |
| 0x04 | version 1 (0x28-byte grains) |
| 0x08 | flags (4; names and user data absent) |
| 0x0C | bank id |
| 0x10 | bank number, pad |
| 0x16 | sound, grain and sample counts (s16) |
| 0x1C | first sound: the header length varies, so use this |
| 0x20 | first grain |
| 0x24 | samples in SPU RAM, sample data size, SPU allocation size, next block |

**Sound record** (12 bytes): s8 volume, s8 volume group, s16 pan, s8 grain
count, s8 instance limit, u16 flags (1 = loop), u32 first grain (offset from
the first grain).

**Grain** (0x28 bytes): u32 type, s32 delay in 240 Hz ticks, 32 bytes of data.
Types 1 and 9 are **tones**: priority, volume, centre note, centre fine, pan,
key range, pitch-bend range, ADSR words 1 and 2, flags (1 = to reverb), sample
offset. Negative volume or pan select registers. Other types are a small
script machine: 4 LFO, 20–43 loops, stops, random play, delays, pitch bend,
registers, markers, key-off. Level-bank tones go to reverb; global-bank tones
are dry.

## 3. Sound definitions (`SoundDef`, 0x20 bytes)

The game's record for a level or class sound:

| Offset | Content |
|---|---|
| 0x00, 0x04 | near and far distance (f32) |
| 0x08, 0x0C | volume at far and at near (0x400 = unity; up to 0x800) |
| 0x10, 0x14 | pitch-bend range, drawn at random per play |
| 0x18 | loop (must match the play's loop flag, or the play is refused) |
| 0x19 | bit 0 squared falloff; bit 1 no occlusion; bit 2 not halved above water; bit 3 no pitch drop underwater |
| 0x1A | index; after load, the bank sound id |
| 0x1C | bank handle, written at load |

**Remap** (core index +0x70): `s16 defs offset, defs count, map offset, map
count`, then one `{s16 offset, s16 count}` per moby class in core-index
class order, each pointing at `{u16 bank id, u16 0}` entries. The loader
copies the level definitions, replaces each index by the map's bank id, and
writes each loaded class's ids into its class definitions. Gadget classes
(no blob in the class table) get their ids parked until the gadget loads.
The core index's moby sound remap (+0xB4) is not read by RAC1.

**Level definition roles** (Novalis; the footstep base is per level): 0 help
box opening, 1 skill point jingle (the same on every level, 2-D, fixed
volume); 2–3 ambient; **4–19 footsteps**, index `base[level] + surface class
× 4 + foot × 2 + variant + 2` with `base` = 0, 2, 6, 1, 0, 2, 2, 0, 2, 2, 1, 7,
0, 0, 4, 0, 0, 0, 0 for levels 0–18; 20 underwater loop; 21 on, sound
instance emitters.

## 4. VAG

A 0x30-byte header, **big-endian**: `VAGp`, version 0x20, pad, data size,
sample rate (44,100 music, 44,056 speech), 12 zero bytes, a 16-byte name
(`L01_Enemy_Loop`). Body: mono SPU ADPCM; the last two frames have flags 1
(end) and 7. Size = 0x30 + data size.

## 5. SPU ADPCM

16-byte frames: byte 0 = shift | filter << 4, byte 1 = flags, 14 bytes = 28
four-bit samples, low nibble first.

    K0 = [0, 60, 115, 98, 122], K1 = [0, 0, −52, −55, −60]   (/64)
    s = ((s16)(n << 12)) >> shift
    s += (K0[f]·h1 + K1[f]·h2 + 32) >> 6
    s = clamp(s, −32768, 32767); h2 = h1; h1 = s

The rounded form is PCSX2's and DuckStation's; OpenGOAL floors each term
separately, which differs by up to 2 per prediction and drifts through the
history (41.6 M of the 56.6 M samples differ). Nothing on the disc decides
between them. Flags: bit 0 end, bit 1 repeat, bit 2 loop start; a voice
latches the loop address at a loop-start frame and at an end frame jumps there
if repeat is set, else stops. Disc invariants: shift ≤ 12, filter ≤ 4; every
sample starts with an all-zero frame; one-shots end `1, 7`; loops are
`0, 6, 2…, 3` or `0, 2…, 6, 2…, 3`.

**Pitch**: SPU pitch 0x1000 = 48 kHz. Tones start at note 60; the pitch
comes from libsd's note-to-pitch table (`trunc(0x8000 · 2^(k/12))` and
`trunc(0x8000 · 2^(k/1536))`, which ReRAC checked against the 140 values in the
libsd module), × 44,100 / 48,000 when the centre note is not negative. The
989snd pan table in the module equals `trunc(0x3FFF · (cos, sin)(k/2°))`
except one entry.

## Open

- The global bank's owner; entries 0–13 of the module bundle.
- RAC2–RAC4: not measured.

Sources: ReRAC (https://github.com/re-rac/rerac, ISC, Copyright (c) 2026
ReRAC contributors), `docs/plan/audio.md` sections 1–2 and "In the port",
`docs/formats/disc_layout.md` section 2.5.
