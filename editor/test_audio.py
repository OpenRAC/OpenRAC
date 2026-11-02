"""audio.py: the keys the port works out too (port/tests/audio/sound_replacements.cpp has the same bank
and the same answers), the sounds as files and back, and a pack's sound through overrides.py on either disc."""

from array import array
from datetime import datetime
import io
import json
import signal
import struct
import tempfile
import unittest
from unittest import mock
import wave
from pathlib import Path

import audio
import overrides

# A bank of one sound: a tone, then a stop. The port's test makes the same bytes.
SOUND_KEY = "3d5f9cc75b576b92"
STREAM_KEY = "e9b65507686eaed0"


def tone_data(sample_offset: int = 16) -> bytes:
    return (struct.pack("<bbbbh", 5, 100, -60, 0, 0) + bytes([0, 127, 2, 2])
            + struct.pack("<HHHII", 0x00ff, 0x1fc0, 0, sample_offset, 0) + bytes(8))


def sample_frames() -> bytes:
    """A frame that is not the sound's, then its one frame, with the end flag."""
    return bytes(16) + bytes([0, audio.FLAG_END]) + bytes(14)


def bank_file(grains=None, flags: int = 0, samples: bytes | None = None) -> bytes:
    grains = grains if grains is not None else [(1, 0, tone_data()), (24, 3, bytes(32))]
    samples = sample_frames() if samples is None else samples
    first_sound, first_grain = 0x3c, 0x48
    block = bytearray(first_sound)
    block[:4] = b"SBlk"
    struct.pack_into("<I", block, 4, 1)
    struct.pack_into("<hhh", block, 0x16, 1, len(grains), 1)
    struct.pack_into("<II", block, 0x1c, first_sound, first_grain)
    block += struct.pack("<bbhBbHI", 127, 0, 0, len(grains), 0, flags, 0)
    for kind, delay, data in grains:
        block += struct.pack("<Ii", kind, delay) + data
    head = struct.pack("<6I", 3, 2, 0x18, len(block), 0x18 + len(block), len(samples))
    return head + bytes(block) + samples


def stream_sector() -> bytes:
    header = bytearray(audio.SECTOR)
    header[:4] = b"VAGp"
    struct.pack_into(">IIII", header, 4, 0x20, 0, 160, 44100)
    header[0x20:0x20 + 9] = b"L01_Enemy"
    return bytes(header)


def wav_bytes(channels: int = 1, width: int = 2, rate: int = 44100, frames: int = 100) -> bytes:
    out = io.BytesIO()
    with wave.open(out, "wb") as file:
        file.setnchannels(channels)
        file.setsampwidth(width)
        file.setframerate(rate)
        file.writeframes(bytes(frames * channels * width))
    return out.getvalue()


def wav_of(kind: int, channels: int, rate: int, bits: int, samples: bytes) -> bytes:
    return (b"RIFF\0\0\0\0WAVEfmt " + struct.pack("<IHHIIHH", 16, kind, channels, rate, rate * channels * bits // 8,
                                                  channels * bits // 8, bits)
            + b"data" + struct.pack("<I", len(samples)) + samples)


def samples_in(wav: bytes) -> list[int]:
    return list(array("h", wav[44:]))


class Keys(unittest.TestCase):
    def test_the_same_answers_as_the_port(self):
        bank = audio.parse_bank(bank_file())
        self.assertEqual(audio.sound_key(bank, bank.sounds[0]), SOUND_KEY)
        self.assertEqual(audio.stream_key(stream_sector()), STREAM_KEY)

    def test_what_follows_a_stream_is_not_part_of_it(self):
        self.assertEqual(audio.stream_key(stream_sector()), audio.stream_key(stream_sector()[:0x30 + 160] + b"after"))

    def test_streams_that_start_alike_are_different_sounds(self):
        # Recordings of one line in two languages: the same header, silence for many sectors.
        long = bytearray(stream_sector()[:0x30] + bytes(40000))
        struct.pack_into(">I", long, 12, 40000)
        other = bytearray(long)
        other[30000] = 1
        self.assertNotEqual(audio.stream_key(bytes(long)), audio.stream_key(bytes(other)))

    def test_only_a_long_streams_first_megabyte_counts(self):
        long = bytearray(stream_sector()[:0x30] + bytes(audio.STREAM_KEY_BYTES + 100))
        struct.pack_into(">I", long, 12, audio.STREAM_KEY_BYTES + 100)
        later = bytearray(long)
        later[-1] = 1
        self.assertEqual(audio.stream_key(bytes(long)), audio.stream_key(bytes(later)))

    def test_a_sound_is_the_same_wherever_its_sample_is(self):
        moved = bytes(32) + bytes([0, audio.FLAG_END]) + bytes(14)
        other = audio.parse_bank(bank_file([(1, 0, tone_data(32)), (24, 3, bytes(32))], samples=moved))
        self.assertEqual(audio.sound_key(other, other.sounds[0]), SOUND_KEY)

    def test_another_sample_or_script_is_another_sound(self):
        louder = bytes(16) + bytes([0, audio.FLAG_END, 0x77]) + bytes(13)
        changed = audio.parse_bank(bank_file(samples=louder))
        self.assertNotEqual(audio.sound_key(changed, changed.sounds[0]), SOUND_KEY)
        slower = audio.parse_bank(bank_file([(1, 0, tone_data()), (24, 9, bytes(32))]))
        self.assertNotEqual(audio.sound_key(slower, slower.sounds[0]), SOUND_KEY)


class Names(unittest.TestCase):
    def test_names_in_a_players_words(self):
        self.assertEqual(audio.pretty("L01_Enemy_Start"), "Enemy Start")
        self.assertEqual(audio.pretty("blarg_trooper"), "Blarg trooper")
        self.assertEqual(audio.pretty("Practicing_F"), "Practicing")

    def test_the_header_keeps_no_name_of_a_long_path(self):
        header = bytearray(0x30)
        header[0x20:0x30] = b"Z:\\I5\\sound\\musi"[:16]
        self.assertEqual(audio.vag_name(bytes(header)), "")

    def test_a_tone_plays_at_the_rate_its_note_says(self):
        self.assertAlmostEqual(audio.tone_rate(-60, 0), 48000)
        self.assertAlmostEqual(audio.tone_rate(60, 0), 44100)
        self.assertAlmostEqual(audio.tone_rate(72, 0), 22050)
        self.assertAlmostEqual(audio.tone_rate(-80, 124), 15989, delta=1, msg="a centre's fine step raises it")

    def test_an_exports_timestamp(self):
        when = datetime(2026, 11, 1, 9, 5, 42)
        self.assertEqual(audio.stamped(Path("exports/audio-all"), when), Path("exports/audio-all-20261101T0905"))
        self.assertEqual(audio.stamped(Path("audio-level_01-music"), when).name, "audio-level_01-music-20261101T0905")

    def test_a_file_name_any_system_takes(self):
        self.assertEqual(audio.file_name("Veldin (the return) sound 3"), "Veldin (the return) sound 3")
        self.assertEqual(audio.file_name("../../evil?"), "evil")
        self.assertEqual(audio.file_name("Giant Clank vs. the Ultra-Mechs (intro)"), "Giant Clank vs. the Ultra-Mechs (intro)")
        self.assertEqual(audio.file_name("Qwark's HQ (loop)"), "Qwark's HQ (loop)")
        self.assertEqual(audio.file_name("???"), "sound")

    def test_where_a_sound_goes(self):
        def place(kind, name, levels=(), language=None, group="Help"):
            return audio.place_of(audio.AudioFile("k", kind, group, name, list(levels), language))
        self.assertEqual(place("effect", "Wrench 2", range(19), group="Effects"), "effects/Wrench 2")
        self.assertEqual(place("music", "Enemy Loop", [1], group="Music"), "music/Novalis/Enemy Loop")
        self.assertEqual(place("speech", "Bombglove", [1], "fr"), "speech/French/Help/Novalis/Bombglove")
        self.assertEqual(place("speech", "Vendor 3", group="Not played/Vendor"), "speech/Not played/Vendor/Vendor 3")
        self.assertEqual(place("effect", "Flight 1 (left)", group="Ship flights"), "effects/Ship flights/Flight 1 (left)")
        self.assertEqual(place("music", "Enemy Loop", group=audio.NOT_PLAYED), "music/Not played/Enemy Loop")

    def test_a_sound_of_several_levels_is_in_one_folder(self):
        shared = audio.AudioFile("k", "music", "Music", "Hovercon Hoverboard Race (intro)", [5, 16])
        self.assertEqual(audio.place_of(shared), "music/Rilgar/Hovercon Hoverboard Race (intro)")
        self.assertEqual(audio.place_of(shared, 16), "music/Kalebo III/Hovercon Hoverboard Race (intro)")
        clip = audio.AudioFile("k", "music", "Music", "Transition 2", [5, 16], titles={5: "Transition 2", 16: "Transition 1"})
        self.assertEqual(audio.place_of(clip), "music/Rilgar/Transition 2", "the whole game's: its first level's")
        self.assertEqual(audio.place_of(clip, 16), "music/Kalebo III/Transition 1", "a level's: as that level's table has it")
        wrench = audio.AudioFile("k", "effect", "Effects", "Wrench 1", list(range(19)))
        self.assertEqual(audio.place_of(wrench, 5), "effects/Wrench 1", "an effect is by what plays it, not where")

    def test_songs_are_named_with_their_two_parts(self):
        self.assertEqual(audio.music_title(1, 0), "Tobruk Crater (intro)")
        self.assertEqual(audio.music_title(1, 1), "Tobruk Crater (loop)")
        self.assertEqual(audio.music_title(1, 5), "The Waterworks (loop)")
        self.assertEqual(audio.music_title(1, 6), "Transition 1")
        self.assertEqual(audio.music_title(1, 8), "Transition 3")
        self.assertEqual(audio.music_title(6, 6), "Evacuate the Warship")
        self.assertEqual(audio.music_title(6, 7), "Transition 1")
        self.assertEqual(audio.music_title(16, 4), "Untitled (intro)")

    def test_every_song_on_the_list_has_its_place(self):
        titles = {song for songs in audio.SONGS.values() for song in songs if song} | set(audio.SINGLE_TRACKS.values())
        self.assertEqual(len(titles), 42)
        self.assertEqual(sorted(audio.SONGS), list(range(19)))


class Languages(unittest.TestCase):
    """Each table's slots hold what the game asks them for, on a disc of VAG files made here."""

    def streams(self, refs: list[dict], levels: list[dict] | None = None, names=None) -> list[audio.AudioFile]:
        names = names or {}

        class FakeDisc:
            size = 1 << 40

            def read(self, offset: int, size: int) -> bytes:
                lba = offset // audio.SECTOR
                data = bytearray(max(size, 0x30))
                data[:4] = b"VAGp"
                struct.pack_into(">I", data, 8, lba)  # each file its own bytes, so its own key
                data[0x20:0x30] = names.get(lba, "").encode().ljust(16, b"\0")
                return bytes(data[:size])

        cat = audio.Catalogue(FakeDisc())
        audio.read_streams(cat, {"global_references": refs, "levels": levels or []})
        return list(cat.files.values())

    def test_a_vendors_slots_run_one_after_another(self):
        files = self.streams([{"group": "vendor_audio", "index": i, "lba": 100 + i} for i in range(37)],
                             names={136: "Fanfare_Vendor", 117: "Thats_a_real_J"})
        third = [(f.language, f.group, f.name) for f in files if f.lba in range(112, 118)]
        self.assertEqual(third, [("en", "Vendor", "Vendor 3"), ("fr", "Vendor", "Vendor 3"), ("de", "Vendor", "Vendor 3"),
                                 ("es", "Vendor", "Vendor 3"), ("it", "Vendor", "Vendor 3"),
                                 (None, "Not played/Vendor", "Vendor 3")])
        fanfare = next(f for f in files if f.lba == 136)
        self.assertEqual((fanfare.kind, fanfare.language, fanfare.group, fanfare.name), ("effect", None, "Vendor", "Fanfare Vendor"))
        self.assertEqual(audio.place_of(fanfare), "effects/Vendor/Fanfare Vendor")
        places = [audio.place_of(f) for f in files]
        self.assertEqual(len(places), len(set(places)), "no two sounds of one name in one folder")
        self.assertEqual(sum(p.startswith("speech/English/Vendor/") for p in places), 6)

    def test_the_ships_flights_are_effects_with_their_two_sides(self):
        files = self.streams([{"group": "space_audio", "index": i, "lba": 100 + i} for i in range(36)],
                             names={117: "batman_takeoff_L", 135: "batman_takeoff_R"})
        places = {f.lba: audio.place_of(f) for f in files}
        self.assertTrue(all(f.kind == "effect" and f.language is None for f in files))
        self.assertEqual(places[100], "effects/Ship flights/Flight 1 (left)")
        self.assertEqual(places[118], "effects/Ship flights/Flight 1 (right)")
        self.assertEqual(places[117], "effects/Ship flights/Batman takeoff (left)")
        self.assertEqual(places[135], "effects/Ship flights/Batman takeoff (right)")
        self.assertEqual(len(set(places.values())), 36)

    def test_music_is_named_by_its_level_and_place_in_the_table(self):
        level = [{"group": "music", "index": i, "lba": 100 + i} for i in range(3)]
        files = self.streams([{"group": "music", "index": 0, "lba": 90}], [{"id": 9, "references": level}], {90: "L01_Enemy_Loop"})
        self.assertEqual([audio.place_of(f) for f in files],
                         ["music/Not played/Enemy Loop", "music/Gaspar/Blarg Depot (intro)", "music/Gaspar/Blarg Depot (loop)",
                          "music/Gaspar/Transition 1"])

    def test_the_credits_sixth_slot_is_not_taken_for_english(self):
        suffixes = ("", "_F", "_G", "_S", "_I", "_J")
        files = self.streams([{"group": "post_credits_audio", "index": i, "lba": 100 + i} for i in range(6)],
                             names={100 + i: f"speech12{suffixes[i]}" for i in range(6)})
        self.assertEqual([f.language for f in files], ["en", "fr", "de", "es", "it", None])
        self.assertEqual(audio.place_of(files[5]), "speech/Not played/Credits/Scene line 12")

    def test_the_tables_by_game_language_have_an_empty_second_slot(self):
        files = self.streams([{"group": "qwark_boss_audio", "index": i, "lba": 100 + i} for i in (0, 2, 3, 4, 5)])
        self.assertEqual([f.language for f in files], ["en", "fr", "de", "es", "it"])
        files = self.streams([{"group": "help_audio", "index": i * 150, "lba": 100 + i} for i in (0, 2, 3, 4, 5)])
        self.assertEqual([f.language for f in files], ["en", "fr", "de", "es", "it"])
        scene = [{"group": "scene_audio", "scene": 0, "index": i, "lba": 100 + i} for i in (0, 2, 3, 4, 5)]
        files = self.streams([], [{"id": 1, "references": scene}])
        self.assertEqual([f.language for f in files], ["en", "fr", "de", "es", "it"])


class Formats(unittest.TestCase):
    def test_adpcm_decodes_as_the_port_does(self):
        # Filter 0, shift 0: each nibble is the sample's top four bits, low nibble first.
        frame = bytearray(16)
        frame[2], frame[3] = 0x71, 0xf8
        self.assertEqual(list(array("h", audio.decode_adpcm(bytes(frame))))[:4], [0x1000, 0x7000, -0x8000, -0x1000])
        # Shift 12 is the nibble itself; filter 1 predicts 60/64 of the sample before.
        filtered = bytearray(16)
        filtered[0], filtered[2] = 0x1c, 0x07
        decoded = list(array("h", audio.decode_adpcm(bytes(filtered))))
        self.assertEqual(decoded[0], 7)
        self.assertEqual(decoded[1], (60 * 7 + 32) >> 6)
        self.assertEqual(len(decoded), 28)
        # A part of a frame is no frame.
        self.assertEqual(audio.decode_adpcm(bytes(15)), b"")

    def test_common_wavs_become_what_the_game_reads(self):
        def converted(kind, channels, bits, samples):
            return samples_in(audio.to_game_wav(wav_of(kind, channels, 44100, bits, samples)))
        self.assertEqual(converted(1, 2, 16, struct.pack("<hh", 1, -2)), [1, -2])
        self.assertEqual(converted(1, 1, 8, bytes([128, 255, 0])), [0, 127 << 8, -32768])
        self.assertEqual(converted(1, 1, 24, bytes([9, 0, 0x40, 9, 0, 0xc0])), [0x4000, -0x4000])
        self.assertEqual(converted(1, 1, 32, bytes([9, 9, 0, 0x40, 9, 9, 0, 0xc0])), [0x4000, -0x4000])
        self.assertEqual(converted(3, 1, 32, struct.pack("<ff", 0.5, -2.0)), [16384, -32767])
        self.assertEqual(converted(3, 1, 64, struct.pack("<d", 2.0)), [32767])
        # A stereo file cut in the middle of a frame keeps its whole frames.
        self.assertEqual(converted(1, 2, 16, struct.pack("<hhh", 1, 2, 3)), [1, 2])
        # A chunk before the format, of odd size, is stepped over.
        wav = wav_of(1, 1, 44100, 16, struct.pack("<h", 5))
        tagged = wav[:12] + b"LIST" + struct.pack("<I", 3) + b"abc\0" + wav[12:]
        self.assertEqual(samples_in(audio.to_game_wav(tagged)), [5])

    def test_what_is_refused_says_so_in_a_players_words(self):
        for data, says in ((b"ID3 an mp3 file really", "not a WAV"), (b"fLaC", "not a WAV"),
                           (wav_of(1, 6, 44100, 16, bytes(12)), "6 channels"), (wav_of(1, 1, 100, 16, bytes(2)), "100 Hz"),
                           (wav_of(1, 1, 44100, 16, b""), "no sound"), (wav_of(85, 1, 44100, 16, bytes(2)), "16-bit PCM")):
            with self.assertRaises(ValueError) as caught:
                audio.to_game_wav(data)
            self.assertIn(says, str(caught.exception))

    def test_the_slowest_effect_on_the_discs_can_come_back(self):
        audio.to_game_wav(audio.wav_file(1, round(audio.tone_rate(126, 124)), bytes(2)))

    def test_a_packs_wav_is_one_the_port_can_play(self):
        audio.check_wav(wav_bytes())
        audio.check_wav(wav_bytes(channels=2, rate=8000))
        for bad in (wav_bytes(width=1), wav_bytes(channels=3), wav_bytes(rate=400), wav_bytes(frames=0), b"not a wav"):
            with self.assertRaises(ValueError):
                audio.check_wav(bad)


def effect_frame(loud: int) -> bytes:
    frame = bytearray(16)
    frame[1] = audio.FLAG_END
    frame[2] = loud
    return bytes(frame)


class ExportAndImport(unittest.TestCase):
    """The sounds as files and back, on a disc image made here."""

    def setUp(self):
        self.scratch = tempfile.TemporaryDirectory()
        self.addCleanup(self.scratch.cleanup)
        self.root = Path(self.scratch.name)
        self.into = self.root / "data" / "overrides"
        # A stream at sector 2 of the image, one frame long at 44100 Hz.
        sector = bytearray(audio.SECTOR)
        sector[:4] = b"VAGp"
        struct.pack_into(">II", sector, 12, 16, 44100)
        sector[0x30:0x40] = effect_frame(0x07)
        self.iso = self.root / "disc.iso"
        self.iso.write_bytes(bytes(2 * audio.SECTOR) + bytes(sector))
        self.files = [
            audio.AudioFile("000000000000000a", "effect", "Effects", "Wrench 1", list(range(19)), sample=effect_frame(0x01), rate=22050),
            audio.AudioFile("000000000000000b", "effect", "Effects", "Blarg trooper 2", [13], sample=effect_frame(0x02), rate=16000),
            audio.AudioFile("000000000000000c", "effect", "Effects", "Menu sound 1", sample=effect_frame(0x03), rate=22050),
            audio.AudioFile("000000000000000d", "music", "Music", "Enemy Loop", [1], lba=2),
            audio.AudioFile("000000000000000e", "speech", "Help", "Bombglove", [1], "en", lba=2),
            audio.AudioFile("000000000000000f", "speech", "Help", "Bombglove", [1], "fr", lba=2),
        ]
        patcher = mock.patch.object(audio, "read_sounds", lambda _iso: ("ntsc", self.files))
        patcher.start()
        self.addCleanup(patcher.stop)

    def export(self, name: str, what: str, level=None) -> Path:
        out = self.root / name
        with mock.patch("sys.stdout", io.StringIO()):
            audio.export(self.iso, out, what, level, workers=1)
        return out

    def test_what_an_export_takes(self):
        def places(what, level=None):
            return sorted(json.loads((self.export(f"e-{what}-{level}", what, level) / audio.SUMMARY).read_text())["sounds"])
        self.assertEqual(places("effects"), ["effects/Blarg trooper 2.wav", "effects/Menu sound 1.wav", "effects/Wrench 1.wav"])
        self.assertEqual(places("effects", 13), ["effects/Blarg trooper 2.wav", "effects/Wrench 1.wav"])
        self.assertEqual(places("music", 1), ["music/Novalis/Enemy Loop.wav"])
        self.assertEqual(places("speech", 1), ["speech/English/Help/Novalis/Bombglove.wav",
                                               "speech/French/Help/Novalis/Bombglove.wav"])
        self.assertEqual(len(places("all")), 6)

    def test_each_file_is_logged_as_it_is_written(self):
        said = io.StringIO()
        with mock.patch("sys.stdout", said):
            audio.export(self.iso, self.root / "logged", "effects", workers=1)
        lines = [line for line in said.getvalue().splitlines() if not line.startswith("openrac-progress: ")]
        self.assertEqual(lines[0], "Reading the sounds on the disc...", "something to show from the start")
        self.assertEqual(sorted(lines[1:]), ["effects/Blarg trooper 2.wav", "effects/Menu sound 1.wav", "effects/Wrench 1.wav"])

    def test_the_command_line_stamps_the_folder_and_keeps_two_of_one_minute_apart(self):
        said = io.StringIO()
        with mock.patch("sys.stdout", said), mock.patch.object(audio, "datetime") as clock:
            clock.now.return_value = datetime(2026, 11, 1, 9, 0)
            for _ in range(2):
                self.assertEqual(audio.main(["export", str(self.iso), str(self.root / "audio-effects"), "--what", "effects",
                                             "--timestamp", "--unique"]), 0)
        self.assertTrue((self.root / "audio-effects-20261101T0900" / audio.SUMMARY).is_file())
        self.assertTrue((self.root / "audio-effects-20261101T0900-2" / audio.SUMMARY).is_file())
        self.assertFalse((self.root / "audio-effects").exists())
        self.assertIn(f"openrac-output: {(self.root / 'audio-effects-20261101T0900').resolve()}", said.getvalue())

    def test_a_cancelled_export_leaves_nothing_behind(self):
        self.addCleanup(signal.signal, signal.SIGTERM, signal.getsignal(signal.SIGTERM))
        with mock.patch("sys.stdout", io.StringIO()), self.assertRaises(SystemExit), \
                mock.patch.object(audio, "say_progress", lambda *_: signal.raise_signal(signal.SIGTERM)):
            audio.main(["export", str(self.iso), str(self.root / "stopped"), "--what", "effects"])
        self.assertEqual([p.name for p in self.root.iterdir()], ["disc.iso"])

    def test_nothing_to_export_says_so(self):
        from assets import AssetError
        with self.assertRaises(AssetError):
            self.export("none", "music", 5)
        self.assertFalse((self.root / "none").exists())

    def test_the_files_are_wavs_of_the_disc(self):
        out = self.export("music", "music", 1)
        with wave.open(str(out / "music/Novalis/Enemy Loop.wav")) as file:
            self.assertEqual((file.getnchannels(), file.getsampwidth(), file.getframerate(), file.getnframes()), (1, 2, 44100, 28))
            self.assertEqual(array("h", file.readframes(1))[0], 0x7000)
        with wave.open(str(self.export("fx", "effects", 13) / "effects/Blarg trooper 2.wav")) as file:
            self.assertEqual(file.getframerate(), 16000)

    def test_two_sounds_of_one_name_each_get_a_file(self):
        self.files.append(audio.AudioFile("0000000000000010", "effect", "Effects", "Wrench 1", [2], sample=effect_frame(0x04), rate=22050))
        sounds = json.loads((self.export("both", "effects", 2) / audio.SUMMARY).read_text())["sounds"]
        self.assertEqual(sorted(sounds), ["effects/Wrench 1 (2).wav", "effects/Wrench 1.wav"])
        self.assertNotEqual(sounds["effects/Wrench 1.wav"]["key"], sounds["effects/Wrench 1 (2).wav"]["key"])

    def test_nothing_changed_makes_no_pack(self):
        out = self.export("fx", "effects")
        self.assertEqual(audio.import_edits(out, self.into), (None, 0, []))
        self.assertFalse((self.into / "packs" / "fx").exists())

    def test_what_changed_becomes_a_pack(self):
        out = self.export("my sounds", "effects")
        # Edited in place, 24-bit from an audio editor; one deleted; one left alone.
        (out / "effects/Wrench 1.wav").write_bytes(wav_of(1, 2, 48000, 24, bytes([9, 0, 0x40]) * 2))
        (out / "effects/Blarg trooper 2.wav").unlink()
        pack, count, refused = audio.import_edits(out, self.into)
        self.assertEqual((pack, count, refused), ("my-sounds", 1, []))
        folder = self.into / "packs" / pack
        self.assertEqual(sorted(p.name for p in (folder / "audio").iterdir()), ["000000000000000a.wav"])
        kept = (folder / "audio/000000000000000a.wav").read_bytes()
        audio.check_wav(kept)
        self.assertEqual(samples_in(kept), [0x4000, 0x4000])
        manifest = json.loads((folder / overrides.MANIFEST).read_text())
        self.assertEqual((manifest["format"], manifest["game"], manifest["version"], manifest["name"]),
                         (1, "rac1", "ntsc", "my sounds"))
        self.assertEqual(overrides.check(folder)[3], [], "a pack overrides.py takes")
        self.assertTrue(overrides.load_state(self.into)["packs"][0]["enabled"])

    def test_importing_again_updates_that_pack(self):
        out = self.export("fx", "effects")
        (out / "effects/Wrench 1.wav").write_bytes(wav_bytes())
        first, _, _ = audio.import_edits(out, self.into)
        overrides.save_state(self.into, {"format": 1, "packs": [{"id": first, "enabled": False}]})
        (out / "effects/Menu sound 1.wav").write_bytes(wav_bytes())
        again, count, _ = audio.import_edits(out, self.into)
        self.assertEqual((again, count), (first, 2))
        self.assertEqual(overrides.load_state(self.into)["packs"], [{"id": first, "enabled": False}], "its place and state")
        self.assertEqual(len(list((self.into / "packs" / first / "audio").iterdir())), 2)

    def test_taking_back_every_edit_removes_that_pack(self):
        out = self.export("fx", "effects")
        original = (out / "effects/Wrench 1.wav").read_bytes()
        (out / "effects/Wrench 1.wav").write_bytes(wav_bytes())
        first, _, _ = audio.import_edits(out, self.into)
        (out / "effects/Wrench 1.wav").write_bytes(b"an mp3, renamed")
        self.assertEqual(audio.import_edits(out, self.into)[0], None)
        self.assertTrue((self.into / "packs" / first).is_dir(), "a file refused may still be an edit")
        (out / "effects/Wrench 1.wav").write_bytes(original)
        self.assertEqual(audio.import_edits(out, self.into), (None, 0, []))
        self.assertFalse((self.into / "packs" / first).exists())
        self.assertEqual(overrides.load_state(self.into)["packs"], [])

    def test_a_key_is_no_way_out_of_the_pack(self):
        from assets import AssetError
        out = self.export("fx", "effects")
        summary = json.loads((out / audio.SUMMARY).read_text())
        summary["sounds"]["effects/Wrench 1.wav"]["key"] = "../../../victim"
        (out / audio.SUMMARY).write_text(json.dumps(summary))
        (out / "effects/Wrench 1.wav").write_bytes(wav_bytes())
        with self.assertRaises(AssetError):
            audio.import_edits(out, self.into)
        self.assertEqual([p.name for p in self.into.rglob("victim*")], [])

    def test_what_is_refused_says_why_and_the_rest_goes_in(self):
        out = self.export("fx", "effects")
        (out / "effects/Wrench 1.wav").write_bytes(wav_bytes())
        (out / "effects/Menu sound 1.wav").write_bytes(b"an mp3, renamed")
        (out / "effects/My new sound.wav").write_bytes(wav_bytes())
        pack, count, refused = audio.import_edits(out, self.into)
        self.assertEqual((pack, count), ("fx", 1))
        self.assertEqual(len(refused), 2)
        self.assertTrue(any("Menu sound 1.wav" in line and "not a WAV" in line for line in refused), refused)
        self.assertTrue(any("My new sound.wav" in line and "name and place" in line for line in refused), refused)

    def test_a_folder_that_is_no_export_is_refused(self):
        from assets import AssetError
        (self.root / "folder").mkdir()
        with self.assertRaises(AssetError):
            audio.import_edits(self.root / "folder", self.into)
        (self.root / "folder" / audio.SUMMARY).write_text('{"tool": "openrac-assets"}')
        with self.assertRaises(AssetError):
            audio.import_edits(self.root / "folder", self.into)


class Packs(unittest.TestCase):
    """A pack of sound through overrides.py, made on one disc and stacked for the other."""

    def setUp(self):
        self.scratch = tempfile.TemporaryDirectory()
        self.addCleanup(self.scratch.cleanup)
        self.root = Path(self.scratch.name)
        self.into = self.root / "data" / "overrides"

    def pack(self, name: str, wavs=(), version: str = "pal") -> Path:
        folder = self.root / name
        (folder / "audio").mkdir(parents=True)
        (folder / overrides.MANIFEST).write_text(json.dumps({"format": 1, "game": "rac1", "version": version, "name": name}))
        for key in wavs:
            (folder / "audio" / f"{key}.wav").write_bytes(wav_bytes())
        return folder

    def stack_for(self, version: str, out: Path) -> dict:
        class Levels:
            game = "rac1"
        Levels.version = version
        with mock.patch.object(overrides, "Target", lambda _levels: Levels()):
            return overrides.stack(self.into, out, levels=self.root)

    def test_check_takes_it_and_says_what_is_wrong(self):
        key = "0123456789abcdef"
        _, used, _, problems = overrides.check(self.pack("good", [key]))
        self.assertEqual((used, problems), ([f"audio/{key}.wav"], []))
        wrong = self.pack("wrong", [key])
        (wrong / "audio" / f"{key}.wav").write_bytes(wav_bytes(width=1))
        (wrong / "level_00" / "textures").mkdir(parents=True)
        pixel = self.root / "unused"
        pixel.write_bytes(b"")
        with self.assertRaises(overrides.PackError):
            overrides.check(wrong)  # nothing it could use is left

    def test_a_name_that_is_no_key_is_not_taken(self):
        folder = self.pack("named", ["0123456789abcdef"])
        (folder / "audio" / "../../evil.wav".replace("../", "")).write_bytes(wav_bytes())
        (folder / "audio" / "ABCDEF0123456789.wav").write_bytes(wav_bytes())
        _, used, ignored, _ = overrides.check(folder)
        self.assertEqual(used, ["audio/0123456789abcdef.wav"])
        self.assertEqual(sorted(ignored), ["audio/ABCDEF0123456789.wav", "audio/evil.wav"])

    def test_stack_puts_the_packs_together_for_either_disc(self):
        first, second = "0123456789abcdef", "fedcba9876543210"
        overrides.add(self.pack("soundtrack", [first, second], version="pal"), self.into)
        later = self.pack("mine", [first], version="ntsc")
        (later / "audio" / f"{first}.wav").write_bytes(wav_bytes(rate=22050))
        overrides.add(later, self.into)
        for version in ("pal", "ntsc"):
            out = self.root / "data" / "mods" / version
            summary = self.stack_for(version, out)
            self.assertEqual(sorted(p.name for p in (out / "audio").iterdir()), [f"{first}.wav", f"{second}.wav"], version)
            with wave.open(str(out / "audio" / f"{first}.wav")) as file:
                self.assertEqual(file.getframerate(), 22050, "the later pack wins")
            self.assertEqual(summary["files"][f"audio/{first}.wav"], "mine")
            self.assertEqual(summary["skipped"], {}, version)

    def test_a_pack_that_only_replaces_sounds_is_a_pack(self):
        new, _, problems = overrides.add(self.pack("quiet", ["0123456789abcdef"]), self.into)
        self.assertEqual((new, problems), ("quiet", []))
        self.assertEqual(overrides.describe(["audio/0123456789abcdef.wav"]), {})


if __name__ == "__main__":
    unittest.main()
