"""Declarative audio packs: distribution, real WAV semantics, private caches and stack isolation."""

import hashlib
import json
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch
import zipfile

import audio
import audio_recipe
import overrides

INTRO, LOOP = "0123456789abcdef", "fedcba9876543210"


def manifest(**changes):
    return {"format": 5, "type": "data", "game": "rac1", "disc_version": "pal",
            "id": "faster-music", "version": "1.0.0", "games": {"rac1": ["pal"]}, "name": "Faster music",
            "audio_recipe": {"format": 1, "keys": [INTRO, LOOP], "speed": 1.04, **changes}}


def recording(rate=44100):
    return audio.wav_file(1, rate, struct.pack("<4h", -12000, 12000, 20000, -20000) * 441)


class AudioRecipes(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.iso = self.root / "owned-disc.iso"
        self.iso.write_bytes(b"synthetic input")
        self.catalogue = [audio.AudioFile(key, "music", "World", name, [0], lba=i)
                          for i, (key, name) in enumerate(((INTRO, "Intro"), (LOOP, "Loop")))]
        self.catalogue_patch = patch.object(audio, "read_sounds", return_value=("pal", self.catalogue))
        self.read_sounds = self.catalogue_patch.start()
        self.addCleanup(self.catalogue_patch.stop)
        self.writer_patch = patch.object(audio, "write_stream", side_effect=lambda iso, lba, path:
                                        Path(path).write_bytes(recording()))
        self.writer = self.writer_patch.start()
        self.addCleanup(self.writer_patch.stop)

    def install(self, value=None):
        source = self.root / "source"
        source.mkdir(exist_ok=True)
        (source / overrides.MANIFEST).write_text(json.dumps(value or manifest()))
        packs = self.root / "overrides"
        ident, _, _ = overrides.add(source, packs)
        return packs, ident

    def test_create_zip_contains_no_recording_and_selects_scope(self):
        self.catalogue.append(audio.AudioFile("aaaaaaaaaaaaaaaa", "speech", "World", "Speech", [0], lba=2))
        out, packs = self.root / "release", self.root / "installed"
        created = audio_recipe.create(self.iso, out, {"name": "Music +4%", "what": "music", "level": 0,
                                                    "speed": 1.04}, packs)
        self.assertEqual(created["audio_recipe"]["keys"], [INTRO, LOOP])
        self.assertFalse(self.writer.called)
        with zipfile.ZipFile(out.with_suffix(".zip")) as z:
            self.assertEqual(set(z.namelist()), {"openrac-mod.json", "README.txt"})
        self.assertEqual(overrides.check(out)[1:], ([], [], []))
        self.assertEqual(len(overrides.load_state(packs)["packs"]), 1)
        self.assertEqual(overrides.load_state(packs)["packs"][0]["id"], created["id"])
        before = out.with_suffix(".zip").read_bytes()
        with self.assertRaises(ValueError):
            audio_recipe.create(self.iso, out, {"what": "music"})
        self.assertEqual(out.with_suffix(".zip").read_bytes(), before)

    def test_schema_rejects_code_paths_duplicates_and_nonfinite_controls(self):
        for field, values in {"keys": [[INTRO, INTRO], [], ["../escape"], ["0" * 16] * 4097],
                              "speed": [True, 0, 3, "1.04", float("nan")],
                              "gain_db": [False, -25, 13, float("inf")],
                              "command": ["execute something"]}.items():
            for value in values:
                with self.subTest(field=field, value=str(value)[:40]), self.assertRaises(ValueError):
                    overrides.read_manifest(json.dumps(manifest(**{field: value})).encode(), "test")
        legacy = manifest()
        legacy["format"] = 1
        legacy["version"] = "pal"
        with self.assertRaisesRegex(ValueError, "format.*5"):
            overrides.read_manifest(json.dumps(legacy).encode(), "test")

    def test_recipe_refuses_packaged_audio_instead_of_installing_it(self):
        packs, ident = self.install()
        source = self.root / "source/audio"
        source.mkdir()
        (source / (INTRO + ".wav")).write_bytes(recording())
        state_before = (packs / overrides.STATE).read_bytes()
        with self.assertRaisesRegex(ValueError, "metadata only"):
            overrides.add(source.parent, packs)
        self.assertEqual((packs / overrides.STATE).read_bytes(), state_before)
        self.assertFalse((packs / "packs" / ident / "audio").exists())

    def test_varispeed_preserves_samples_and_scales_duration(self):
        source = recording()
        result = audio_recipe.transform(source, {"speed": 1.04})
        _, channels, rate, bits, pcm = audio.wav_format(result)
        self.assertEqual((channels, rate, bits), (1, 45864, 16))
        self.assertEqual(pcm, audio.wav_format(source)[4])
        self.assertAlmostEqual((len(pcm) / 2 / rate) / (len(pcm) / 2 / 44100), 1 / 1.04)
        self.assertEqual(audio_recipe.transform(source, {}), source)
        with self.assertRaisesRegex(ValueError, "playback rates"):
            audio_recipe.transform(recording(192000), {"speed": 2})

    def test_gain_is_bounded_and_never_wraps_pcm(self):
        result = audio_recipe.transform(recording(), {"gain_db": 12})
        self.assertEqual(struct.unpack_from("<4h", audio.wav_format(result)[4]), (-32768, 32767, 32767, -32768))
        quiet = audio_recipe.transform(recording(), {"gain_db": -6.020599913279624})
        self.assertEqual(struct.unpack_from("<4h", audio.wav_format(quiet)[4]), (-6000, 6000, 10000, -10000))

    def test_cache_reuses_only_verified_data_and_invalidates_changed_inputs(self):
        cache = self.root / "private-cache"
        folder, version = audio_recipe.materialize(manifest(), self.iso, cache, "pal")
        self.assertEqual(version, "pal")
        self.assertEqual(self.writer.call_count, 2)
        self.assertEqual(audio_recipe.materialize(manifest(), self.iso, cache, "pal")[0], folder)
        self.assertEqual(self.writer.call_count, 2)
        (folder / "audio" / (INTRO + ".wav")).write_bytes(b"damaged")
        audio_recipe.materialize(manifest(), self.iso, cache, "pal")
        self.assertEqual(self.writer.call_count, 4)
        self.assertTrue(list(cache.glob("*.invalid-*")))
        self.iso.write_bytes(b"different synthetic input")
        changed, _ = audio_recipe.materialize(manifest(), self.iso, cache, "pal")
        self.assertNotEqual(changed, folder)
        self.assertEqual(self.writer.call_count, 6)
        self.assertNotEqual(audio_recipe.materialize(manifest(speed=.9), self.iso, cache, "pal")[0], changed)

    def test_stack_precedence_disable_and_missing_disc_preserve_previous_output(self):
        packs, ident = self.install()
        out = self.root / "mods"
        iso_hash = hashlib.sha256(self.iso.read_bytes()).hexdigest()
        prepared = overrides.stack(packs, out, iso=self.iso)
        self.assertEqual(prepared["layers"][0]["id"], "faster-music")
        self.assertEqual(prepared["layers"][0]["version"], "1.0.0")
        first = (out / "audio" / (INTRO + ".wav")).read_bytes()
        self.assertEqual(audio.wav_format(first)[2], 45864)
        with self.assertRaisesRegex(ValueError, "original disc"):
            overrides.stack(packs, out)
        self.assertEqual((out / "audio" / (INTRO + ".wav")).read_bytes(), first)
        ordinary = self.root / "ordinary"
        (ordinary / "audio").mkdir(parents=True)
        (ordinary / overrides.MANIFEST).write_text(json.dumps({"format": 1, "game": "rac1", "version": "pal",
                                                              "name": "Later custom recording"}))
        (ordinary / "audio" / (INTRO + ".wav")).write_bytes(recording())
        last, _, _ = overrides.add(ordinary, packs)
        overrides.stack(packs, out, iso=self.iso)
        self.assertEqual((out / "audio" / (INTRO + ".wav")).read_bytes(), recording())
        # Disabling the recipe leaves ordinary packs usable without the disc, as before.
        state = overrides.load_state(packs)
        next(e for e in state["packs"] if e["id"] == ident)["enabled"] = False
        overrides.save_state(packs, state)
        overrides.stack(packs, out)
        self.assertEqual((out / "audio" / (INTRO + ".wav")).read_bytes(), recording())
        next(e for e in state["packs"] if e["id"] == last)["enabled"] = False
        overrides.save_state(packs, state)
        overrides.stack(packs, out)
        self.assertFalse((out / "audio" / (INTRO + ".wav")).exists())
        self.assertEqual(hashlib.sha256(self.iso.read_bytes()).hexdigest(), iso_hash)
        self.assertFalse((packs / "packs" / ident / "audio").exists())

    def test_missing_keys_and_wrong_disc_are_failures_before_generation(self):
        with self.assertRaisesRegex(ValueError, "not found"):
            audio_recipe.materialize(manifest(keys=["aaaaaaaaaaaaaaaa"]), self.iso, self.root / "cache")
        with self.assertRaisesRegex(ValueError, "disc used"):
            audio_recipe.materialize(manifest(), self.iso, self.root / "cache", "ntsc")
        self.assertFalse(self.writer.called)

    def test_same_recipe_update_keeps_its_identity_position_and_enabled_state(self):
        packs, ident = self.install()
        state = overrides.load_state(packs)
        state["packs"][0]["enabled"] = False
        overrides.save_state(packs, state)
        value = manifest(speed=.9)
        value["version"] = "1.1.0"
        (self.root / "source" / overrides.MANIFEST).write_text(json.dumps(value))
        updated, _, _ = overrides.add(self.root / "source", packs)
        self.assertEqual(updated, ident)
        self.assertEqual(overrides.load_state(packs), state)
        self.assertEqual(json.loads((packs / "packs" / ident / overrides.MANIFEST).read_text())["version"], "1.1.0")

    def test_new_exports_with_the_same_name_do_not_replace_each_other(self):
        packs = self.root / "installed"
        first = audio_recipe.create(self.iso, self.root / "first", {"name": "Audio adjustments"}, packs)
        second = audio_recipe.create(self.iso, self.root / "second", {"name": "Audio adjustments", "speed": .9}, packs)
        self.assertNotEqual(first["id"], second["id"])
        self.assertEqual(len(overrides.load_state(packs)["packs"]), 2)
        self.assertEqual({p["id"] for p in overrides.load_state(packs)["packs"]}, {first["id"], second["id"]})

    def test_recipe_identity_never_replaces_an_unrelated_pack(self):
        ordinary = self.root / "ordinary"
        ordinary.mkdir()
        (ordinary / overrides.MANIFEST).write_text(json.dumps({
            "format": 1, "game": "rac1", "version": "pal", "name": "Faster music",
            "export": "audio-recipe:faster-music"}))
        packs = self.root / "overrides"
        ident, _, _ = overrides.add(ordinary, packs)
        before = (packs / "packs" / ident / overrides.MANIFEST).read_bytes()
        with self.assertRaisesRegex(ValueError, "different pack"):
            self.install()
        self.assertEqual((packs / "packs" / ident / overrides.MANIFEST).read_bytes(), before)

    def test_install_never_uses_an_unchecked_recipe_id_as_a_path(self):
        stage = self.root / "stage"
        stage.mkdir()
        value = manifest()
        value["id"] = "../outside"
        (stage / overrides.MANIFEST).write_text(json.dumps(value))
        with self.assertRaisesRegex(ValueError, "safe mod id"):
            overrides.install(stage, self.root / "packs")
        self.assertTrue((stage / overrides.MANIFEST).is_file())
        self.assertFalse((self.root / "packs/outside").exists())


if __name__ == "__main__":
    unittest.main()
