#!/usr/bin/env python3
"""The game's sound as files: every effect, piece of music and line of speech on a disc as a WAV file
to edit, and the edits back as a pack, as assets.py does for models and textures.

  audio.py export DISC OUT --what effects|music|speech|all [--level N] [--timestamp] [--unique]
                                 a new folder of WAV files; the disc is never changed
  audio.py import EXPORT --into OVERRIDES [--name NAME]
                                 what changed in that folder becomes a pack (overrides.py)

An export is a folder of WAV files, by kind and then by what plays them, and export.json, which says
which sound each file is and what it held. Delete what you do not need. A file that differs at the
import replaces its sound: any common WAV file, kept as 16-bit PCM, the form the port reads.

A pack holds audio/<key>.wav for each sound it replaces. A sound is known by its key, the start of a
SHA-256 of what it is, not by where it is: the same effect is in many levels under other numbers, and
the same music on both discs. So one file reaches a sound everywhere, and a pack works on either disc
wherever the sound is the same. The port works out the same keys when it loads a sound
(port/audio/sound_replacements.h):

- a stream (music, speech: a VAG file on the disc): the file, up to its first megabyte (recordings of
  one line in several languages can start alike for many sectors);
- an effect (one sound of a 989snd bank): for each of its grains, its type and delay, then for a tone
  its settings and the bytes of its sample, for any other grain its parameters.

editor/AUDIO_MODDING.md is the modder's guide.
"""

import argparse
from array import array
from concurrent.futures import ProcessPoolExecutor, as_completed
from dataclasses import dataclass, field
from datetime import datetime
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import signal
import struct
import sys
import tempfile

sys.path.insert(1, str(next(p for p in Path(__file__).resolve().parents if (p / "tools/openrac_common").is_dir()) / "tools"))
from openrac_common import files as file_ops  # noqa: E402

from disc import SECTOR, Disc
from errors import say as say_error
from formats import FormatError, span, unpack
from games import LEVEL_NAMES
from level import SECTION_NAMES, decoded
from mobys import moby_class_names
from progress_line import say as say_progress

SUMMARY = "export.json"
STREAM_KEY_BYTES = 1 << 20
TOOL = "openrac-audio"
EXPORT_FORMAT = 1
WHAT = ("effects", "music", "speech", "all")
KINDS = {"effects": ("effect",), "music": ("music",), "speech": ("speech",), "all": ("effect", "music", "speech")}
# The largest stream a disc holds, as the port reads them.
MOST_VAG = 64 << 20
# A replacement a player's computer can hold in memory: about an hour of stereo at 44.1 kHz.
MOST_WAV_BYTES = 512 << 20
LOWEST_RATE, HIGHEST_RATE = 500, 192000  # the discs' slowest effect is about 1030 Hz

# 989snd grains and sounds (port/assets/sound/sound_bank.h).
SOUND_RECORD = 12
GRAIN_RECORD = 0x28
TONES = (1, 9)
FLAG_END = 1
OUTPUT_RATE = 48000

# Which language each of a table's six slots holds. The tables the game indexes with game_language:
# 0 English, 2 French, 3 German, 4 Spanish, 5 Italian, and 1 no console setting reaches (the game's
# InitOnce). The tables of one line after another, whose slot the game works out as game_language less
# one, or 0 (vendor_menu.c, countdown_sequence.c): the same five in a row, and a sixth it never asks
# for, Japanese where PAL names it (speech12_J), which goes in a folder of its own.
LANGUAGES = ("en", "en", "fr", "de", "es", "it")
LINE_LANGUAGES = ("en", "fr", "de", "es", "it", None)
NOT_PLAYED = "Not played"

# The songs of each level, in the order of its music table: each is an intro the game plays once and a
# loop it then repeats (`music_start_track_body` queues track + 1 behind the one playing). What is left
# in a table after them is a short transition between songs. There is no official list that could be
# found: these are the names of the fan-made list at archive.org/details/ratchet-clank-2002-ost, because
# they agree with the disc's own names (L01_Cave, L01_Waterworks) and the game's code (Ride the Robot
# Train: metropolis_train_locomotive.c asks for Kerwan's third song). Which song is which comes from
# their lengths on the disc against that list, and that code where two are the same length: Jet Fighter
# Dogfighting is jet_fighter.c's, Giant Clank's giant_clank_pad.c's. None: not on the list.
SONGS = {
    0: ("Kyzil Plateau",),
    1: ("Tobruk Crater", "Reservoir Caves", "The Waterworks"),
    2: ("Outpost X11", "Sandshark Hunting", "Traversing the Factory"),
    3: ("Metropolis", "The Fitness Course", "Ride the Robot Train"),
    4: ("Logging Site", "Swing the Canyon"),
    5: ("Blackwater City", "Hovercon Hoverboard Race", "Sewer Escape"),
    6: ("Blarg Tactical Research Station", "Outside the Station", "Decommissioned Blarg Warship"),
    7: ("Qwark's HQ", "The Blargian Snagglebeast"),
    8: ("Fort Krontos", "Rail Grinding"),
    9: ("Blarg Depot",),
    10: ("Ratchet Explores Kogor", "Kogor Refinery"),
    11: ("Jowai Resort", "Jet Fighter Dogfighting", "The Scenic Sewers"),
    12: ("Bomb Factory", "The Rising Reservoir"),
    13: ("Gemlik Base", "Duel with Captain Qwark"),
    14: ("Gorda City Ruins", "Upper City Chills"),
    15: ("Robot Plant", "Giant Clank vs. the Ultra-Mechs"),
    16: ("Gadgetron Site", "Hovercon Hoverboard Race", None),
    17: ("Drek's Fleet", "Destroying the Cannons"),
    18: ("Veldin in Peril", "Ultimate Supreme Executive Chairman Drek"),
}
# A song of one track, with nothing to repeat: the countdown on the warship (countdown_sequence.c).
SINGLE_TRACKS = {(6, 6): "Evacuate the Warship"}
LANGUAGE_NAMES = {"en": "English", "fr": "French", "de": "German", "es": "Spanish", "it": "Italian"}

README = """Edit or replace the WAV files here, keeping each one's name and place, then choose this folder
under Import your edits on the Mods page's Audio tab. Files you did not change, or deleted, stay the
game's own. Any common WAV file works, at any length. See editor/AUDIO_MODDING.md in OpenRAC.

These sounds come from your disc: do not share them.
"""


def key_of(*parts: bytes) -> str:
    """The first 8 bytes of a SHA-256, in hex: port/audio/sound_replacements.cpp's make_key."""
    digest = hashlib.sha256()
    for part in parts:
        digest.update(part)
    return digest.hexdigest()[:16]


def stream_key(start: bytes) -> str:
    """The key of the VAG file START begins with, given at least its first STREAM_KEY_BYTES (or all of
    it): its header says how long it is, and what follows it is not part of it."""
    size, = struct.unpack_from(">I", start, 12)
    return key_of(start[:min(0x30 + size, STREAM_KEY_BYTES)])


# ---- banks -----------------------------------------------------------------------------------


@dataclass
class Grain:
    kind: int
    delay: int
    data: bytes

    @property
    def is_tone(self) -> bool:
        return self.kind in TONES

    @property
    def center(self) -> tuple[int, int]:
        return tuple(struct.unpack_from("<bb", self.data, 2))

    @property
    def sample_offset(self) -> int:
        return struct.unpack_from("<I", self.data, 16)[0]


@dataclass
class Sound:
    number: int
    flags: int
    grains: list[Grain] = field(default_factory=list)


@dataclass
class Bank:
    sounds: list[Sound]
    samples: bytes


def parse_bank(file: bytes) -> Bank:
    """A bank file, as port/assets/sound/sound_bank.cpp reads it."""
    file_type, chunks = unpack("<II", file)
    if file_type != 3 or chunks != 2:
        raise FormatError(f"sound bank: type {file_type}, {chunks} chunks (a 989snd bank is type 3 with 2)")
    block_at, block_size, samples_at, samples_size = unpack("<IIII", file, 8)
    block = span(file, block_at, block_size)
    samples = span(file, samples_at, samples_size)
    if block[:4] != b"SBlk":
        raise FormatError("sound bank: chunk 0 is not an SBlk block")
    version, = unpack("<I", block, 4)
    if version != 1:
        raise FormatError(f"SBlk version {version}: only version 1 is known")
    count, = unpack("<h", block, 0x16)
    if count < 0:
        raise FormatError("SBlk: negative sound count")
    first_sound, first_grain = unpack("<II", block, 0x1c)
    sounds = []
    for number in range(count):
        at = first_sound + SOUND_RECORD * number
        grains, _, flags, offset = unpack("<BbHI", block, at + 4)
        sound = Sound(number, flags)
        for k in range(grains):
            where = first_grain + offset + GRAIN_RECORD * k
            kind, delay = unpack("<Ii", block, where)
            sound.grains.append(Grain(kind, delay, span(block, where + 8, 32)))
        sounds.append(sound)
    return Bank(sounds, samples)


def sample_bytes(samples: bytes, offset: int) -> bytes:
    """A sample's frames, up to and including the one with the end flag (port/assets/sound/adpcm.cpp)."""
    if offset % 16:
        raise FormatError(f"sample offset {offset:#x} is not frame aligned")
    at = offset
    while True:
        if at + 16 > len(samples):
            raise FormatError(f"sample at {offset:#x} runs past the data without an end flag")
        if samples[at + 1] & FLAG_END:
            return samples[offset:at + 16]
        at += 16


def sound_key(bank: Bank, sound: Sound) -> str:
    parts = []
    for grain in sound.grains:
        parts.append(struct.pack("<Ii", grain.kind, grain.delay))
        if grain.is_tone:
            parts += [grain.data[:16], sample_bytes(bank.samples, grain.sample_offset)]
        else:
            parts.append(grain.data)
    return key_of(*parts)


def tone_rate(center_note: int, center_fine: int) -> float:
    """The rate a tone's sample plays at when the game asks for nothing else (port/audio/pitch.h,
    tone_pitch): note 60 against its centre, which its fine step raises. A negative centre is a sample
    at the console's rate, any other one a 44.1 kHz sample of the first PlayStation's kind."""
    note = abs(center_note) * 128 - center_fine
    ratio = 2.0 ** ((60 * 128 - note) / (12 * 128))
    return OUTPUT_RATE * ratio * (1.0 if center_note < 0 else 44100 / 48000)


# ---- what plays each sound ---------------------------------------------------------------------


def level_owners(index: bytes, number: int, classes: dict[int, str]) -> dict[int, str]:
    """Who plays each bank sound of a level, as the sound's name: the moby class the level's remap
    gives it to (CLASSES names them), or the level itself (port/assets/sound/level_sounds_rac1.h).
    {bank sound: name}."""
    base, = unpack("<i", index, 0x70)
    if base <= 0:
        return {}
    count, table = unpack("<II", index, 0x18)
    names: dict[int, str] = {}
    _, _, map_offset, map_count = unpack("<hhhh", index, base)
    for k in range(map_count):
        sound, = unpack("<H", index, base + map_offset + 4 * k)
        if sound != 0xffff:
            names.setdefault(sound, f"{level_name(number)} sound {k + 1}")
    for c in range(count):
        o_class, = unpack("<I", index, table + c * 32 + 4)
        offset, sounds = unpack("<hh", index, base + 8 + 4 * c)
        for k in range(max(sounds, 0)):
            sound, = unpack("<H", index, base + offset + 4 * k)
            if sound != 0xffff:
                owner = pretty(classes[o_class]) if o_class in classes else f"Class {o_class}"
                names.setdefault(sound, f"{owner} {k + 1}")
    return names


# ---- names -------------------------------------------------------------------------------------


def level_name(number: int) -> str:
    return LEVEL_NAMES["rac1"].get(number, f"Level {number}")


def pretty(name: str) -> str:
    """A name from the disc or the class table in a player's words: "L01_Enemy_Start" is
    "Enemy Start", "blarg_trooper" is "Blarg trooper"."""
    words = re.sub(r"_[fgsiFGSI]$", "", re.sub(r"^[Ll]\d+_", "", name)).replace("_", " ").strip()
    return words[:1].upper() + words[1:]


def natural(name: str) -> list:
    """A name's sort order: "Ratchet 3" before "Ratchet 26"."""
    return [int(part) if part.isdigit() else part.lower() for part in re.split(r"(\d+)", name)]


def vag_name(header: bytes) -> str:
    """The name in a VAG header, or "" for the folder most of them start with: only 16 characters
    fit, so nothing of the name itself is left."""
    name = header[0x20:0x30].split(b"\0")[0].decode("latin-1").strip()
    return "" if ":" in name or "\\" in name or name == "something" else name


def music_title(level: int, index: int) -> str:
    """A music table's entry by what it is: "Tobruk Crater (intro)", "Tobruk Crater (loop)", "Transition 1"."""
    songs = SONGS.get(level, ())
    if index < len(songs) * 2:
        song = songs[index // 2] or "Untitled"
        return f"{song} ({'loop' if index % 2 else 'intro'})"
    if (level, index) in SINGLE_TRACKS:
        return SINGLE_TRACKS[level, index]
    singles = sum(1 for at in SINGLE_TRACKS if at[0] == level)
    return f"Transition {index - len(songs) * 2 - singles + 1}"


# ---- what a disc holds -------------------------------------------------------------------------


@dataclass
class AudioFile:
    """One different sound of a disc: what it is called, and where its samples are."""
    key: str
    kind: str                       # "effect", "music" or "speech"
    group: str
    name: str
    levels: list[int] = field(default_factory=list)
    language: str | None = None
    lba: int | None = None          # a stream's sector
    sample: bytes | None = None     # an effect's first tone's sample,
    rate: int = 0                   # at the rate the tone plays it
    titles: dict[int, str] = field(default_factory=dict)  # a shared sound's name in each level's own table


class Catalogue:
    """What a disc holds, one entry per different sound."""

    def __init__(self, disc: Disc):
        self.disc = disc
        self.files: dict[str, AudioFile] = {}

    def stream(self, lba: int, kind: str, group: str, name: str, level: int | None = None,
               language: str | None = None) -> None:
        """A VAG file at LBA; anything else is left out (a table's slot may hold other data)."""
        if self.disc.read(lba * SECTOR, 4) != b"VAGp":
            return
        key = stream_key(self.disc.read(lba * SECTOR, min(STREAM_KEY_BYTES, self.disc.size - lba * SECTOR)))
        entry = self.files.setdefault(key, AudioFile(key, kind, group, name, language=language, lba=lba))
        if level is not None and level not in entry.levels:
            entry.levels.append(level)
            entry.titles[level] = name

    def bank(self, bank: Bank, owners: dict[int, str], level: int | None, fallback: str) -> None:
        for sound in bank.sounds:
            tones = [g for g in sound.grains if g.is_tone]
            if not tones:
                continue
            key = sound_key(bank, sound)
            entry = self.files.get(key)
            if entry is None:
                entry = self.files[key] = AudioFile(
                    key, "effect", "Effects", owners.get(sound.number) or f"{fallback} {sound.number + 1}",
                    sample=sample_bytes(bank.samples, tones[0].sample_offset), rate=round(tone_rate(*tones[0].center)))
            if level is not None and level not in entry.levels:
                entry.levels.append(level)


def flight_bank(lump: bytes) -> Bank:
    """The bank in the flight's lump: header word 1 is its data base, word 0x19 the bank from there
    (the game's space.c, header->sound)."""
    base, = unpack("<i", lump, 1 * 4)
    at, = unpack("<i", lump, 0x19 * 4)
    return parse_bank(lump[base + at:])


def level_sections(disc: Disc, info: dict) -> dict[str, bytes]:
    """A level's sound bank and core index, without reading the rest of its data."""
    start = info["ranges"]["data"]["lba"] * SECTOR
    pairs = disc.read(start, len(SECTION_NAMES) * 8)
    out = {}
    for name in ("sound_bank", "core_index"):
        offset, size = unpack("<ii", pairs, SECTION_NAMES.index(name) * 8)
        if size:
            out[name] = decoded(disc.read(start + offset, size))
    return out


def scene_line(name: str) -> str:
    """A scene's speech by its number: "speech12_F" is "Scene line 12"."""
    match = re.match(r"^speech(\d+)", name)
    return f"Scene line {match.group(1)}" if match else ""


def read_streams(cat: Catalogue, survey: dict) -> None:
    disc = cat.disc
    refs: dict[str, list[dict]] = {}
    for ref in survey["global_references"]:
        refs.setdefault(ref["group"], []).append(ref)

    def header(lba: int) -> bytes:
        return disc.read(lba * SECTOR, 0x30)

    def is_vag(lba: int) -> bool:
        return header(lba)[:4] == b"VAGp"

    def line_table(ref: dict, group: str, title: str, level: int | None = None) -> None:
        """A stream of a table of one line after another, six slots a line (LINE_LANGUAGES)."""
        language = LINE_LANGUAGES[ref["index"] % 6]
        cat.stream(ref["lba"], "speech", group if language else f"{NOT_PLAYED}/{group}", title, level, language)

    # The speech of the Help desk: 150 lines for each of six languages, named on the English ones.
    help_lines = {r["index"] % 150: vag_name(header(r["lba"])) for r in refs.get("help_audio", [])
                  if r["index"] < 150 and is_vag(r["lba"])}
    for ref in refs.get("help_audio", []):
        line = ref["index"] % 150
        name = help_lines.get(line) or vag_name(header(ref["lba"]))
        match = re.match(r"^[Ll](\d+)_", name)
        cat.stream(ref["lba"], "speech", "Help", pretty(name) or f"Help line {line + 1}",
                   int(match.group(1)) if match and int(match.group(1)) < 19 else None, LANGUAGES[ref["index"] // 150])
    for ref in refs.get("qwark_boss_audio", []):
        name = vag_name(header(ref["lba"]))
        title = pretty(name) if name else f"Scene lines {ref['index'] // 6 + 1}"
        cat.stream(ref["lba"], "speech", "Scene lines", title, None, LANGUAGES[ref["index"] % 6])
    for ref in refs.get("vendor_audio", []):
        name = vag_name(header(ref["lba"]))
        if ref["index"] >= 36:  # the seventh line, the one left over: a fanfare, which is no speech
            cat.stream(ref["lba"], "effect", "Vendor", pretty(name) or "Vendor fanfare")
        else:
            line_table(ref, "Vendor", f"Vendor {ref['index'] // 6 + 1}")
    # The ship's flights between planets: no speech (the disc keeps them in sound\\space), eighteen sounds
    # with their two sides apart, left then right (the last two are named batman_takeoff_L and _R).
    for ref in refs.get("space_audio", []):
        name = pretty(re.sub(r"_[LR]$", "", vag_name(header(ref["lba"])))) or f"Flight {ref['index'] % 18 + 1}"
        side = "left" if ref["index"] < 18 else "right"
        cat.stream(ref["lba"], "effect", "Ship flights", f"{name} ({side})")
    for ref in refs.get("post_credits_audio", []):
        name = vag_name(header(ref["lba"]))
        line_table(ref, "Credits", scene_line(name) or f"Credits line {ref['index'] // 6 + 1}")
    # An entry of the table the game's code treats as padding: another encoding of Novalis's first loop.
    for ref in refs.get("music", []):
        cat.stream(ref["lba"], "music", NOT_PLAYED, pretty(vag_name(header(ref["lba"]))) or "Music")
    for info in survey["levels"]:
        level = info["id"]
        for ref in info["references"]:
            if ref["group"] == "music":
                cat.stream(ref["lba"], "music", "Music", music_title(level, ref["index"]), level)
            elif ref["group"] == "scene_audio":
                # A scene takes two of the header's records (the game's scene_sounds, by 0x250); the
                # second holds the scene's data, not its speech.
                if is_vag(ref["lba"]):
                    cat.stream(ref["lba"], "speech", "Cutscenes", f"Cutscene {ref['scene'] // 2 + 1}", level,
                               LANGUAGES[ref["index"]])
            elif ref["group"] == "bindata" and is_vag(ref["lba"]):
                name = vag_name(header(ref["lba"]))
                line_table(ref, "Scene lines", scene_line(name) or f"Scene line {ref['index'] // 6 + 1}", level)


def read_sounds(iso: Path) -> tuple[str, list[AudioFile]]:
    """The disc's version, and every different sound on it."""
    with Disc(iso) as disc:
        survey = disc.survey()
        cat = Catalogue(disc)
        for ref in survey["global_references"]:
            if ref["group"] == "sound_bank":
                # The menus' and the game's own sounds: its bank is on the disc as it is.
                cat.bank(parse_bank(decoded(disc.read(ref["lba"] * SECTOR, ref["bytes"]))), {}, None, "Menu sound")
            elif ref["group"] == "transition":
                # The flight between planets has a bank of its own in its lump (level.py, load_flight).
                cat.bank(flight_bank(decoded(disc.read(ref["lba"] * SECTOR, ref["bytes"]))), {}, None, "Flight sound")
        classes = moby_class_names()
        for info in survey["levels"]:
            sections = level_sections(disc, info)
            if "sound_bank" in sections:
                owners = level_owners(sections["core_index"], info["id"], classes) if "core_index" in sections else {}
                cat.bank(parse_bank(sections["sound_bank"]), owners, info["id"], f"{level_name(info['id'])} sound")
        read_streams(cat, survey)
    return survey["version"], list(cat.files.values())


# ---- WAV files and the console's ADPCM ----------------------------------------------------------


def decode_adpcm(adpcm: bytes) -> bytes:
    """The console's sound ADPCM as 16-bit samples, from zero history, the flags not read: as
    port/assets/sound/adpcm.cpp decodes it. 16-byte frames: byte 0 is `shift | filter << 4`, byte 1
    the flags, then 28 four-bit samples, low nibble first."""
    positive, negative = (0, 60, 115, 98, 122), (0, 0, -52, -55, -60)
    signed = tuple((n - 16 if n >= 8 else n) * 4096 for n in range(16))
    frames = len(adpcm) // 16
    out = array("h", bytes(frames * 28 * 2))
    h1 = h2 = at = 0
    for start in range(0, frames * 16, 16):
        shift = adpcm[start] & 0x0f
        filter_ = min(adpcm[start] >> 4, 4)
        k0, k1 = positive[filter_], negative[filter_]
        for byte in adpcm[start + 2:start + 16]:
            for nibble in (byte & 0x0f, byte >> 4):
                sample = (signed[nibble] >> shift) + ((k0 * h1 + k1 * h2 + 32) >> 6)
                if sample > 32767:
                    sample = 32767
                elif sample < -32768:
                    sample = -32768
                h2, h1 = h1, sample
                out[at] = sample
                at += 1
    if sys.byteorder == "big":
        out.byteswap()
    return out.tobytes()


def wav_file(channels: int, rate: int, pcm: bytes) -> bytes:
    """A 16-bit PCM WAV file of these samples (interleaved, little-endian)."""
    return (b"RIFF" + struct.pack("<I", 36 + len(pcm)) + b"WAVEfmt "
            + struct.pack("<IHHIIHH", 16, 1, channels, rate, rate * channels * 2, channels * 2, 16)
            + b"data" + struct.pack("<I", len(pcm)) + pcm)


def wav_format(data: bytes) -> tuple[int, int, int, int, bytes]:
    """(kind, channels, rate, bits, samples) of a WAV file, in a player's words when it is none. Kind:
    1 PCM, 3 float; the data is cut to the file when its header says more."""
    not_wav = "not a WAV file: save it as a WAV first, any audio editor can"
    if len(data) < 12 or data[:4] != b"RIFF" or data[8:12] != b"WAVE":
        raise ValueError(not_wav)
    at, shape = 12, None
    while at + 8 <= len(data):
        tag, size = data[at:at + 4], struct.unpack_from("<I", data, at + 4)[0]
        body = at + 8
        if tag == b"fmt ":
            chunk = data[body:body + size]
            if len(chunk) < 16:
                raise ValueError(not_wav)
            kind, channels, rate = struct.unpack_from("<HHI", chunk)
            bits, = struct.unpack_from("<H", chunk, 14)
            if kind == 0xfffe and len(chunk) >= 26:  # extensible: its subformat's first two bytes
                kind, = struct.unpack_from("<H", chunk, 24)
            shape = (kind, channels, rate, bits)
        elif tag == b"data":
            if shape is None:
                raise ValueError(not_wav)
            return (*shape, data[body:body + size])
        at = body + size + (size & 1)
    raise ValueError(not_wav)


def check_shape(channels: int, rate: int, samples: bytes) -> None:
    if channels not in (1, 2):
        raise ValueError(f"{channels} channels; the game plays mono or stereo")
    if not LOWEST_RATE <= rate <= HIGHEST_RATE:
        raise ValueError(f"{rate} Hz; the game plays {LOWEST_RATE} to {HIGHEST_RATE} Hz")
    if not samples:
        raise ValueError("no sound in it")


def check_wav(data: bytes) -> None:
    """A file in a pack, or ValueError: 16-bit PCM, mono or stereo (port/assets/sound/wav.h)."""
    if len(data) > MOST_WAV_BYTES:
        raise ValueError(f"larger than {MOST_WAV_BYTES >> 20} MB")
    kind, channels, rate, bits, samples = wav_format(data)
    if kind != 1 or bits != 16:
        raise ValueError(f"{bits}-bit audio of format {kind}; a pack's WAV files must be 16-bit PCM")
    check_shape(channels, rate, samples)


def to_16_bit(samples: bytes, kind: int, bits: int) -> bytes:
    """PCM of 8, 16, 24 or 32 bits, or 32- or 64-bit floats, as 16-bit samples."""
    if (kind, bits) == (1, 16):
        return samples[:len(samples) // 2 * 2]
    if (kind, bits) == (1, 8):
        out = bytearray(len(samples) * 2)
        out[1::2] = samples.translate(bytes((b + 128) & 0xff for b in range(256)))  # unsigned to signed
        return bytes(out)
    if kind == 1 and bits in (24, 32):
        width = bits // 8
        count = len(samples) // width
        out = bytearray(count * 2)
        out[0::2] = samples[width - 2:count * width:width]
        out[1::2] = samples[width - 1:count * width:width]
        return bytes(out)
    if kind == 3 and bits in (32, 64):
        values = array("f" if bits == 32 else "d")
        values.frombytes(samples[:len(samples) // (bits // 8) * (bits // 8)])
        return array("h", (round(max(-1.0, min(1.0, v)) * 32767) for v in values)).tobytes()
    raise ValueError(f"format {kind} at {bits} bits cannot be read: save it as 16-bit PCM")


def to_game_wav(data: bytes) -> bytes:
    """An edited file as a pack holds it, or ValueError in a player's words."""
    kind, channels, rate, bits, samples = wav_format(data)
    check_shape(channels, rate, samples)
    pcm = to_16_bit(samples, kind, bits)
    pcm = pcm[:len(pcm) // (2 * channels) * (2 * channels)]
    if not pcm:
        raise ValueError("no sound in it")
    if len(pcm) > MOST_WAV_BYTES:
        raise ValueError(f"too long: the game holds at most {MOST_WAV_BYTES >> 20} MB of sound for one")
    return wav_file(channels, rate, pcm)


# ---- export -------------------------------------------------------------------------------------


def file_name(name: str) -> str:
    """A sound's name as a file's, with nothing a file system refuses."""
    return re.sub(r"[^\w ()'.-]+", "", name).strip(" .") or "sound"


def place_of(entry: AudioFile, level: int | None = None) -> str:
    """Where a sound's file goes in an export, the name last: by kind, then language and group, then a
    level. A sound that several levels share has one file, in the level exported or else the first
    that has it, which is the only way one edit can be the sound's in all of them."""
    parts = {"effect": ["effects"], "music": ["music"], "speech": ["speech"]}[entry.kind]
    if entry.kind == "speech" and entry.language:
        parts.append(LANGUAGE_NAMES[entry.language])
    if entry.group not in ("Effects", "Music"):
        parts.append(entry.group)
    where = level if level is not None else (entry.levels[0] if entry.levels else None)
    if entry.kind != "effect" and where is not None:
        parts.append(file_name(level_name(where)))
    return "/".join(parts + [file_name(entry.titles.get(level, entry.name))])


def chosen(files: list[AudioFile], what: str, level: int | None) -> list[AudioFile]:
    """The sounds an export takes, in a steady order. A level has the sounds that play in it; what
    the whole game shares comes with all the levels."""
    picked = [f for f in files if f.kind in KINDS[what] and (level is None or level in f.levels)]
    return sorted(picked, key=lambda f: (f.kind, natural(place_of(f, level)), f.key))


def stamped(path: Path, when: datetime | None = None) -> Path:
    """PATH with the date and time after its name, local time to the minute: audio-all-20261101T0900."""
    return path.with_name(f"{path.name}-{when or datetime.now():%Y%m%dT%H%M}")


def write_stream(iso: str, lba: int, path: str) -> str:
    """The VAG file at a disc's sector as a WAV file; returns the file's hash."""
    with open(iso, "rb") as disc:
        disc.seek(lba * SECTOR)
        header = disc.read(0x30)
        size, rate = struct.unpack_from(">II", header, 12)
        if header[:4] != b"VAGp" or size > MOST_VAG or rate == 0:
            raise FormatError(f"no VAG file at sector {lba}")
        adpcm = disc.read(size)
    data = wav_file(1, rate, decode_adpcm(adpcm))
    Path(path).write_bytes(data)
    return hashlib.sha256(data).hexdigest()


def write_effect(sample: bytes, rate: int, path: str) -> str:
    """An effect's sample as a WAV file; returns the file's hash."""
    data = wav_file(1, rate, decode_adpcm(sample))
    Path(path).write_bytes(data)
    return hashlib.sha256(data).hexdigest()


def export(iso: Path, out: Path, what: str, level: int | None = None, workers: int | None = None) -> dict:
    """Write the export (see the module's docstring) into the new folder OUT. Decoding is plain
    Python, so each file is a job of its own for the machine's processors."""
    from assets import AssetError, output_dir  # the same rules for where an export may go

    if what not in WHAT:
        raise AssetError(f"--what is one of {', '.join(WHAT)}")
    out = output_dir(out)
    print("Reading the sounds on the disc...", flush=True)
    version, files = read_sounds(iso)
    picked = chosen(files, what, level)
    if not picked:
        raise AssetError(f"nothing to export: {level_name(level) if level is not None else 'the disc'} has no {what}")
    out.parent.mkdir(parents=True, exist_ok=True)
    temporary = Path(tempfile.mkdtemp(prefix=f".{out.name}-", dir=out.parent))
    try:
        places: dict[str, AudioFile] = {}
        for entry in picked:
            # Two sounds of one name: the one after it is told apart.
            base, place, n = place_of(entry, level), place_of(entry, level), 2
            while place in places:
                place, n = f"{base} ({n})", n + 1
            places[place] = entry
            (temporary / place).parent.mkdir(parents=True, exist_ok=True)
        hashes: dict[str, str] = {}
        total, shown = len(places), -1

        def finished(place: str, digest: str) -> None:
            nonlocal shown
            hashes[place] = digest
            # Each file for the job's Details and log; the progress lines are only the bar's.
            print(f"{place}.wav", flush=True)
            # A line for each hundredth at most: an export can have thousands of files.
            if len(hashes) * 100 // total != shown:
                shown = len(hashes) * 100 // total
                say_progress("audio", len(hashes), total)

        def job(place: str, entry: AudioFile) -> tuple:
            path = str(temporary / f"{place}.wav")
            return (write_stream, str(iso), entry.lba, path) if entry.lba is not None else \
                (write_effect, entry.sample, entry.rate, path)

        if workers == 1:
            for place, entry in places.items():
                work, *args = job(place, entry)
                finished(place, work(*args))
        else:
            with ProcessPoolExecutor(max_workers=workers) as pool:
                running = {}
                for place, entry in places.items():
                    work, *args = job(place, entry)
                    running[pool.submit(work, *args)] = place
                for done in as_completed(running):
                    finished(running[done], done.result())
        summary = {"format": EXPORT_FORMAT, "tool": TOOL, "game": "rac1", "version": version, "what": what,
                   "level": level, "id": os.urandom(8).hex(),
                   "sounds": {f"{place}.wav": {"key": places[place].key, "sha256": hashes[place]} for place in places}}
        (temporary / SUMMARY).write_text(json.dumps(summary, indent=1, ensure_ascii=False) + "\n", encoding="utf-8")
        (temporary / "README.txt").write_text(README)
        file_ops.rename(temporary, out)
    finally:
        if temporary.exists():
            shutil.rmtree(temporary)
    return summary


# ---- import -------------------------------------------------------------------------------------


def read_edits(edited: Path) -> tuple[dict, dict[str, bytes], list[str]]:
    """An edited export: its summary, the files a pack takes from it (audio/<key>.wav: what changed,
    as the game reads it) and what was refused, each in a sentence."""
    from assets import AssetError

    try:
        summary = json.loads((edited / SUMMARY).read_text(encoding="utf-8"))
    except (OSError, ValueError):
        raise AssetError(f"{edited.name} is not an audio export: it has no {SUMMARY}") from None
    import overrides

    sounds = summary.get("sounds") if isinstance(summary, dict) else None
    # The folder can come from anyone, and the pack's file names are made of its keys.
    if (not isinstance(sounds, dict) or summary.get("tool") != TOOL or summary.get("format") != EXPORT_FORMAT
            or summary.get("game") != "rac1" or summary.get("version") not in overrides.VERSIONS
            or not isinstance(summary.get("id"), str)
            or not all(isinstance(e, dict) and isinstance(e.get("sha256"), str)
                       and re.fullmatch("[0-9a-f]{16}", str(e.get("key"))) for e in sounds.values())):
        raise AssetError(f"{edited.name} is not an audio export this OpenRAC reads")
    changed, refused = {}, []
    for path in sorted(p for p in edited.rglob("*") if p.is_file() and p.suffix.lower() == ".wav"):
        place = path.relative_to(edited).as_posix()
        entry = sounds.get(place)
        if entry is None:
            refused.append(f"{place} is not one of this export's sounds: keep each file's name and place")
            continue
        data = path.read_bytes()
        if hashlib.sha256(data).hexdigest() == entry["sha256"]:
            continue
        try:
            changed[f"audio/{entry['key']}.wav"] = to_game_wav(data)
        except ValueError as exc:
            refused.append(f"{place}: {exc}")
    return summary, changed, refused


def import_edits(edited: Path, into: Path, name: str | None = None) -> tuple[str | None, int, list[str]]:
    """What changed in the export at EDITED as a pack in the overrides folder INTO; importing it again
    updates that pack, or removes it when nothing differs any more. Returns the pack's id (None: nothing changed), how many sounds it replaces and
    what was refused."""
    import overrides

    summary, changed, refused = read_edits(edited)
    if not changed:
        # Every edit taken back: the pack made of them goes too. A refused file may still be one.
        pack = overrides.export_pack(into, summary["id"]) if not refused else None
        if pack:
            overrides.remove(into, pack)
        return None, 0, refused
    manifest = {"format": overrides.PACK_FORMAT, "game": summary["game"], "version": summary["version"],
                "name": name or edited.name, "author": "", "description": "", "export": summary["id"]}
    with tempfile.TemporaryDirectory(prefix=".import-", dir=overrides.ensure(into)) as scratch:
        staged = Path(scratch) / "pack"
        (staged / "audio").mkdir(parents=True)
        for rel, data in changed.items():
            (staged / rel).write_bytes(data)
        (staged / overrides.MANIFEST).write_text(json.dumps(manifest, indent=1) + "\n")
        return overrides.install(staged, into, replace_export=summary["id"]), len(changed), refused


def main(argv: list[str] | None = None) -> int:
    from assets import AssetError, unique

    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    out = commands.add_parser("export", help="the game's sounds as WAV files")
    out.add_argument("disc", type=Path, help="the disc image")
    out.add_argument("out", type=Path, help="a new folder for the export")
    out.add_argument("--what", choices=WHAT, default="effects", help="what to export (default: effects)")
    out.add_argument("--level", type=int, help="only the sounds that play in this level (default: the whole game)")
    out.add_argument("--timestamp", action="store_true",
                     help="put the date and time after OUT's name: audio-all-20261101T0900")
    out.add_argument("--unique", action="store_true",
                     help="when OUT exists, write OUT-2 (OUT-3...) instead: an earlier export is never touched")
    back = commands.add_parser("import", help="read an edited export back as a pack")
    back.add_argument("edited", type=Path, help="the export folder, edited")
    back.add_argument("--into", type=Path, required=True, help="an overrides folder (overrides.py): the pack goes in it")
    back.add_argument("--name", help="the pack's name (default: the export folder's)")
    args = parser.parse_args(argv)
    # How the launcher cancels a job (launcher/core/src/jobs.rs): this way the staging folder goes too.
    signal.signal(signal.SIGTERM, lambda *_: sys.exit(143))
    try:
        if args.command == "export":
            target = stamped(args.out) if args.timestamp else args.out
            target = unique(target.resolve()) if args.unique else target
            summary = export(args.disc, target, args.what, args.level)
            print(f"Exported {len(summary['sounds'])} sounds: {target.resolve()}")
            print(f"openrac-output: {target.resolve()}")  # the folder, for the launcher (jobs.rs)
            return 0
        edited = args.edited.resolve()
        pack, count, refused = import_edits(edited, args.into, args.name)
        for line in refused:
            print(f"refused: {line}", flush=True)
        if pack is None:
            print("Nothing to import: no file in the folder differs from what was exported."
                  + (f" ({len(refused)} refused, above.)" if refused else " So no pack is left from it."))
            return 1
        tail = f"; {len(refused)} refused (see above)" if refused else ""
        print(f"Imported {count} sound(s) into {args.into.resolve()} as {pack}{tail}")
        print(f"openrac-output: {args.into.resolve() / 'packs' / pack}")  # the folder, for the launcher (jobs.rs)
        return 0
    except (AssetError, FormatError, OSError) as exc:
        say_error(exc, code=None if isinstance(exc, (OSError, FormatError)) else "asset-invalid")
        print(f"audio: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
