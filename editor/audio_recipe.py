"""Share audio adjustments, not recordings. The player's disc supplies every sample.

  audio_recipe.py create DISC OUT --settings '{"what":"music","speed":1.04}'
      [--timestamp] [--unique] [--into OVERRIDES]

OUT and OUT.zip contain only a format-5 manifest and a README. Stacking an enabled
recipe generates a private WAV cache; that cache never becomes part of the pack.
Speed changes pitch and duration together. Independent pitch/tempo is not implemented.
"""

import argparse
from array import array
import hashlib
import json
import math
from pathlib import Path
import re
import shutil
import signal
import sys
import tempfile
import uuid
import zipfile

import audio
from openrac_common import files as file_ops

PACK_FORMAT = 5
CACHE_FORMAT = 1
KEY = re.compile(r"[0-9a-f]{16}")
ID = re.compile(r"[a-z0-9]+(?:-[a-z0-9]+)*")
VERSION = re.compile(r"\d+\.\d+\.\d+(?:-[0-9A-Za-z.-]+)?")
MANIFEST = "openrac-mod.json"
CACHE_NOTE = ".audio-recipe-cache.json"
MAX_KEYS = 4096


def adjustments(value: dict) -> tuple[float, float]:
    def number(key, default, low, high):
        n = value.get(key, default)
        if type(n) not in (int, float) or not low <= n <= high or not math.isfinite(n):
            raise ValueError(f'audio recipe "{key}" must be a number from {low} to {high}')
        return float(n)
    return number("speed", 1, .5, 2), number("gain_db", 0, -24, 12)


def settings(manifest: dict) -> dict:
    """A closed, declarative schema: no paths, scripts, commands or external inputs."""
    value = manifest.get("audio_recipe")
    if (type(manifest.get("format")) is not int or manifest["format"] != PACK_FORMAT
            or manifest.get("game") != "rac1" or manifest.get("type") != "data"):
        raise ValueError("audio recipes require a format-5 Ratchet & Clank pack")
    if (not isinstance(manifest.get("id"), str) or len(manifest["id"]) > 64 or not ID.fullmatch(manifest["id"])
            or not isinstance(manifest.get("version"), str) or not VERSION.fullmatch(manifest["version"])):
        raise ValueError("audio recipe needs a mod id and a MAJOR.MINOR.PATCH version")
    supported = manifest.get("games")
    if (not isinstance(supported, dict) or set(supported) != {"rac1"}
            or not isinstance(supported["rac1"], list) or not supported["rac1"]
            or any(v not in ("pal", "ntsc", "pal1", "jp") for v in supported["rac1"])):
        raise ValueError("audio recipe must declare its supported Ratchet & Clank discs")
    if not isinstance(value, dict) or set(value) - {"format", "keys", "speed", "gain_db"}:
        raise ValueError("audio recipe contains unknown settings")
    if type(value.get("format")) is not int or value["format"] != 1:
        raise ValueError("this OpenRAC reads audio recipe format 1")
    keys = value.get("keys")
    if (not isinstance(keys, list) or not 1 <= len(keys) <= MAX_KEYS
            or any(not isinstance(k, str) or not KEY.fullmatch(k) for k in keys)
            or len(set(keys)) != len(keys)):
        raise ValueError("audio recipe needs distinct 16-digit sound keys")
    speed, gain = adjustments(value)
    return {"format": 1, "keys": sorted(keys), "speed": speed, "gain_db": gain}


def transform(data: bytes, recipe: dict) -> bytes:
    """Keep the decoded samples and loop boundaries; playback rate supplies varispeed."""
    audio.check_wav(data)
    _, channels, rate, _, samples = audio.wav_format(data)
    speed, gain_db = adjustments(recipe)
    changed_rate = round(rate * speed)
    if not audio.LOWEST_RATE <= changed_rate <= audio.HIGHEST_RATE:
        raise ValueError("audio recipe would put a sound outside the supported playback rates")
    if gain_db:
        pcm = array("h", samples)
        if sys.byteorder != "little":
            pcm.byteswap()
        gain = 10 ** (gain_db / 20)
        for i, sample in enumerate(pcm):
            pcm[i] = max(-32768, min(32767, round(sample * gain)))
        if sys.byteorder != "little":
            pcm.byteswap()
        samples = pcm.tobytes()
    return audio.wav_file(channels, changed_rate, samples)


def create(iso: Path, out: Path, options: dict, into: Path | None = None) -> dict:
    from assets import output_dir
    import overrides
    if not isinstance(options, dict) or set(options) - {"name", "what", "level", "speed", "gain_db"}:
        raise ValueError("unknown audio recipe creation options")
    speed, gain = adjustments(options)
    what, level = options.get("what", "music"), options.get("level")
    if what not in audio.WHAT or (level is not None and (type(level) is not int or not 0 <= level < 19)):
        raise ValueError("choose a sound kind and a level from 0 to 18, or the whole game")
    version, catalogue = audio.read_sounds(iso)
    picked = audio.chosen(catalogue, what, level)
    name = options.get("name", "Audio adjustments")
    if not isinstance(name, str) or not name.strip() or len(name) > 120:
        raise ValueError("audio recipe needs a name of up to 120 characters")
    # Two exports with the same display name are still independent mods. Published updates keep this id.
    ident = overrides.pack_id(name, ()) + "-" + uuid.uuid4().hex[:8]
    manifest = {"format": PACK_FORMAT, "type": "data", "game": "rac1", "disc_version": version,
                "id": ident, "version": "1.0.0", "games": {"rac1": [version]},
                "name": name, "author": "", "description": "",
                "export": "audio-recipe:" + ident,
                "audio_recipe": {"format": 1, "keys": sorted({s.key for s in picked}),
                                 "speed": speed, "gain_db": gain}}
    overrides.read_manifest(json.dumps(manifest).encode(), str(out))
    out = output_dir(out)
    archive = out.with_name(out.name + ".zip")
    if archive.exists():
        raise ValueError(f"{archive} already exists; choose a new output folder")
    out.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".recipe-", dir=out.parent) as scratch:
        stage = Path(scratch) / "pack"
        stage.mkdir()
        (stage / MANIFEST).write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
        (stage / "README.txt").write_text(
            "Add this folder or its ZIP in OpenRAC's Mods. The player's disc supplies the audio.\n"
            "Speed changes pitch and duration together. Generated audio stays in the private cache.\n"
            "Share this recipe, not that cache. Requires an OpenRAC that reads format-5 packs.\n",
            encoding="utf-8")
        # Exclusive creation never replaces a previous release, including one made during validation.
        with zipfile.ZipFile(archive, "x", zipfile.ZIP_DEFLATED) as z:
            for path in sorted(stage.iterdir()):
                z.write(path, path.name)
        shutil.copytree(stage, out)
    if into is not None:
        overrides.add(out, into)
    return manifest


def valid_cache(folder: Path, token: str, recipe: dict, expected_version: str | None) -> str | None:
    try:
        note = json.loads((folder / CACHE_NOTE).read_text())
        if (note.get("token") != token or set(note["files"]) != set(recipe["keys"])
                or (expected_version is not None and note["version"] != expected_version)):
            return None
        for key, digest in note["files"].items():
            path = folder / "audio" / (key + ".wav")
            if path.is_symlink() or hashlib.sha256(path.read_bytes()).hexdigest() != digest:
                return None
        return note["version"]
    except (OSError, ValueError, KeyError, TypeError):
        return None


def materialize(manifest: dict, iso: Path, cache_root: Path,
                expected_version: str | None = None) -> tuple[Path, str]:
    recipe = settings(manifest)
    iso = Path(iso).resolve()
    stat = iso.stat()
    identity = {"format": CACHE_FORMAT, "recipe": recipe, "disc": str(iso),
                "size": stat.st_size, "mtime_ns": stat.st_mtime_ns}
    token = hashlib.sha256(json.dumps(identity, sort_keys=True).encode()).hexdigest()
    cache_root = Path(cache_root).resolve()
    cache_root.mkdir(parents=True, exist_ok=True)
    folder = cache_root / token
    if folder.is_symlink():
        raise ValueError("audio recipe cache must not be a symbolic link")
    version = valid_cache(folder, token, recipe, expected_version)
    if version is not None:
        if version not in manifest.get("games", {"rac1": [version]}).get("rac1", []):
            raise ValueError("audio recipe does not support this disc version")
        return folder, version
    version, catalogue = audio.read_sounds(iso)
    if expected_version is not None and version != expected_version:
        raise ValueError("audio recipe needs the disc used to extract these levels")
    if version not in manifest.get("games", {"rac1": [version]}).get("rac1", []):
        raise ValueError("audio recipe does not support this disc version")
    sources = {sound.key: sound for sound in catalogue}
    missing = set(recipe["keys"]) - sources.keys()
    if missing:
        raise ValueError(f"audio recipe: {len(missing)} sound(s) not found on this disc")
    print(f"Preparing {len(recipe['keys'])} audio replacement(s) from your disc...", flush=True)
    with tempfile.TemporaryDirectory(prefix=".generate-", dir=cache_root) as scratch:
        stage = Path(scratch) / "pack"
        (stage / "audio").mkdir(parents=True)
        hashes = {}
        audio.say_progress("audio", 0, len(recipe["keys"]))
        for done, key in enumerate(recipe["keys"], 1):
            sound, path = sources[key], stage / "audio" / (key + ".wav")
            if sound.lba is not None:
                audio.write_stream(str(iso), sound.lba, str(path))
            else:
                audio.write_effect(sound.sample, sound.rate, str(path))
            path.write_bytes(transform(path.read_bytes(), recipe))
            hashes[key] = hashlib.sha256(path.read_bytes()).hexdigest()
            audio.say_progress("audio", done, len(recipe["keys"]))
        (stage / CACHE_NOTE).write_text(json.dumps({"token": token, "version": version, "files": hashes}))
        if folder.exists():
            ready = valid_cache(folder, token, recipe, expected_version)
            if ready is not None:
                return folder, ready
            # Preserve an invalid cache for diagnosis; never purge another pack's generated data.
            old = cache_root / (token + ".invalid-" + uuid.uuid4().hex)
            assert folder.resolve().parent == old.parent == cache_root
            file_ops.rename(folder, old)
        try:
            file_ops.rename(stage, folder)
        except OSError:
            # Another stack may have finished the same immutable input while we were generating.
            if valid_cache(folder, token, recipe, expected_version) is None:
                raise
    return folder, version


def main(argv=None) -> int:
    from assets import unique
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("command", choices=("create",))
    parser.add_argument("disc", type=Path)
    parser.add_argument("out", type=Path)
    parser.add_argument("--settings", default='{"what":"music","speed":1.04}')
    parser.add_argument("--timestamp", action="store_true")
    parser.add_argument("--unique", action="store_true")
    parser.add_argument("--into", type=Path)
    args = parser.parse_args(argv)
    signal.signal(signal.SIGTERM, lambda *_: sys.exit(143))
    try:
        out = audio.stamped(args.out) if args.timestamp else args.out
        out = unique(out.resolve()) if args.unique else out
        manifest = create(args.disc, out, json.loads(args.settings), args.into)
        print(f"Created recipe for {len(manifest['audio_recipe']['keys'])} sounds; no recordings included.")
        print(f"openrac-output: {out.resolve()}")
        print(f"openrac-output: {out.resolve()}.zip")
        return 0
    except (ValueError, OSError) as exc:
        from errors import say
        say(exc, code="asset-invalid")
        print(f"audio recipe: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
