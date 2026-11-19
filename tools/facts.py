#!/usr/bin/env python3
"""Decomp build facts: the few files of a decomp's finished build that OpenRAC's table tools read,
packed once by whoever builds, and downloaded by everyone else (docs/SOURCES.md, "Build facts").

Some port tables are made from a decomp's build (rac2's derived names and its PAL and v2.00
release tables read build/**/placed.json and build/<version>/carry.json). A bundle holds exactly
those files for one (decomp, commit, version), laid out as in the decomp, plus manifest.json
(repository, commit, version, this tool's version, the sha256 of every file). They hold names,
addresses, sizes and hashes only; pack refuses anything else, so no retail bytes go in.

  python3 tools/facts.py pack DECOMP VERSION [--source DIR] [--out ZIP]   pack a bundle from a full build
  python3 tools/facts.py publish DECOMP [VERSION...] [--source DIR]       pack and upload (needs a token)
  python3 tools/facts.py fetch DECOMP VERSION [--commit SHA]              download and check into the cache
  python3 tools/facts.py check DECOMP [VERSION...] [--commit SHA]         is every bundle published?

DECOMP is a folder under games/ (rac2/decomp). The commit is the source's HEAD (default: the pin).
Bundles are assets of the GitHub release DECOMP_FACTS_TAG of OpenRAC: <decomp>-<commit>-<version>.zip
(rac2-decomp-4ea93cea...-pal.zip). OPENRAC_FACTS_URL replaces the download address (a file:// URL
works); GH_TOKEN or GITHUB_TOKEN (else `gh auth token`) is used to upload and to list.

The table tools call root(): a local build of the commit wins; else the bundle, fetched into
build/facts/ and checked, is handed back as a folder that looks like the decomp (its build/ in it).
hostgen itself never reads bundles: it keeps reading the committed tables.
"""
from __future__ import annotations

import argparse
import hashlib
import io
import json
import os
import re
import shutil
import subprocess
import sys
import urllib.error
import urllib.request
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TOOL_VERSION = 1
FORMAT = 1
REPOSITORY = "OpenRAC/OpenRAC"
RELEASE_TAG = "decomp-facts"
CACHE = ROOT / "build" / "facts"
API = "https://api.github.com"

# What each decomp's bundles hold, by version: the version's build folder (relative to the decomp)
# and the globs of files under it the table tools read. A decomp no tool reads a build of has none.
#   rac2/decomp us:  port/tools/hostgen/derived.py (boot and level placed.json); release.py (v1.01's values)
#   rac2/decomp pal, us2: release.py build (*/placed.json), release_carry.py (carry.json, boot placed.json)
SPECS: dict[str, dict[str, dict]] = {
    "rac2/decomp": {
        "us": {"build": "build", "files": ["boot/placed.json", "[0-9][0-9]_*/placed.json"]},
        "pal": {"build": "build/pal", "files": ["boot/placed.json", "[0-9][0-9]_*/placed.json", "carry.json"]},
        "us2": {"build": "build/us2", "files": ["boot/placed.json", "[0-9][0-9]_*/placed.json", "carry.json"]},
    },
}

# Each decomp's primary version: the one its build/ (not build/<version>/) holds.
PRIMARY = {"rac2/decomp": "us"}

# What a fact may be: a name, an address or a number, a short word, a hash. Nothing that could hold
# a run of a program's bytes: hex longer than 16 digits only as an md5, sha1 or sha256.
KEY = re.compile(r"[A-Za-z0-9_.:/@$ -]{1,200}")
WORD = re.compile(r"[A-Za-z0-9_.$@][A-Za-z0-9_.:/@$ -]{0,199}")
LONG_HEX = re.compile(r"[0-9A-Fa-f]{17,}")
NUMBER = re.compile(r"-?(?:0[xX][0-9A-Fa-f]{1,16}|\d{1,20})")
HASH = re.compile(r"[0-9a-f]{32}|[0-9a-f]{40}|[0-9a-f]{64}")


class FactsError(Exception):
    """A bundle that is missing, or does not check."""


def slug(decomp: str) -> str:
    return decomp.replace("/", "-")


def asset_name(decomp: str, commit: str, version: str) -> str:
    return f"{slug(decomp)}-{commit}-{version}.zip"


def spec(decomp: str, version: str) -> dict:
    try:
        return SPECS[decomp][version]
    except KeyError:
        known = ", ".join(f"{d} {v}" for d in SPECS for v in SPECS[d]) or "none"
        raise FactsError(f"facts: no tool reads a build of {decomp} {version} (known: {known})") from None


def git(source: Path, *args: str) -> str:
    r = subprocess.run(["git", *args], cwd=source, capture_output=True, text=True)
    if r.returncode:
        raise FactsError(f"facts: git {' '.join(args)} in {source}: {r.stderr.strip()}")
    return r.stdout.strip()


def head(source: Path) -> str:
    return git(source, "rev-parse", "HEAD")


def pinned(decomp: str) -> str:
    """The commit OpenRAC's index pins games/DECOMP to."""
    parts = git(ROOT, "ls-files", "-s", "--", f"games/{decomp}").split()
    if not parts or parts[0] != "160000":
        raise FactsError(f"facts: games/{decomp} is not a submodule")
    return parts[1]


def check_value(value, where: str) -> None:
    """Refuse anything in a fact file but names, numbers, short words and hashes."""
    if isinstance(value, dict):
        for k, v in value.items():
            if not KEY.fullmatch(k) or LONG_HEX.search(k):
                raise FactsError(f"facts: {where}: key {k[:40]!r} is not a name")
            check_value(v, f"{where}/{k}")
    elif isinstance(value, list):
        for i, v in enumerate(value):
            check_value(v, f"{where}[{i}]")
    elif isinstance(value, str):
        if not (NUMBER.fullmatch(value) or HASH.fullmatch(value)
                or (WORD.fullmatch(value) and not LONG_HEX.search(value))):
            raise FactsError(f"facts: {where}: {value[:40]!r} is not a name, a number or a hash")
    elif value is not None and not isinstance(value, (bool, int, float)):
        raise FactsError(f"facts: {where}: {type(value).__name__} is not a fact")


def collect(source: Path, s: dict) -> dict[str, bytes]:
    """The spec's files under SOURCE: {path relative to the decomp: bytes}, each checked."""
    base = source / s["build"]
    out: dict[str, bytes] = {}
    for pattern in s["files"]:
        found = sorted(p for p in base.glob(pattern) if p.is_file())
        if not found:
            raise FactsError(f"facts: no {s['build']}/{pattern} in {source}: finish the full build first")
        for p in found:
            rel = p.relative_to(source).as_posix()
            data = p.read_bytes()
            try:
                check_value(json.loads(data), rel)
            except ValueError as e:
                raise FactsError(f"facts: {rel} is not JSON: {e}") from None
            out[rel] = data
    return out


def pack(decomp: str, version: str, source: Path, allow_dirty: bool = False) -> tuple[str, bytes]:
    """A bundle of SOURCE's build of VERSION: (its asset name, the zip)."""
    s = spec(decomp, version)
    commit = head(source)
    dirty = git(source, "status", "--porcelain", "--untracked-files=no")
    if dirty and not allow_dirty:
        raise FactsError(f"facts: {source} has changes ({dirty.splitlines()[0].strip()} ...): the build is not "
                         f"{commit[:10]}'s. Revert them (tools/carry.py rewrites config/: `git checkout config`).")
    files = collect(source, s)
    manifest = {
        "format": FORMAT,
        "repository": repo_url(source),
        "decomp": decomp,
        "commit": commit,
        "version": version,
        "tool": "tools/facts.py",
        "tool_version": TOOL_VERSION,
        "files": {rel: hashlib.sha256(data).hexdigest() for rel, data in sorted(files.items())},
    }
    buf = io.BytesIO()
    with zipfile.ZipFile(buf, "w", zipfile.ZIP_DEFLATED) as z:
        for rel, data in [("manifest.json", (json.dumps(manifest, indent=1) + "\n").encode())] + sorted(files.items()):
            info = zipfile.ZipInfo(rel, date_time=(1980, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o644 << 16
            z.writestr(info, data)
    return asset_name(decomp, commit, version), buf.getvalue()


def repo_url(source: Path) -> str:
    try:
        return git(source, "remote", "get-url", "origin")
    except FactsError:
        return ""


def unpack(data: bytes, decomp: str, commit: str, version: str, dest: Path) -> Path:
    """Check a bundle (its manifest's decomp, commit and version, every file's sha256) and write it
    to DEST, which then looks like the decomp's checkout. Returns DEST."""
    try:
        z = zipfile.ZipFile(io.BytesIO(data))
        manifest = json.loads(z.read("manifest.json"))
    except (zipfile.BadZipFile, KeyError, ValueError) as e:
        raise FactsError(f"facts: {asset_name(decomp, commit, version)} is not a bundle: {e}") from None
    for key, want in (("decomp", decomp), ("commit", commit), ("version", version), ("format", FORMAT)):
        if manifest.get(key) != want:
            raise FactsError(f"facts: {asset_name(decomp, commit, version)}: its manifest says {key} "
                             f"{manifest.get(key)!r}, not {want!r}")
    files = manifest.get("files") or {}
    names = set(z.namelist()) - {"manifest.json"}
    if names != set(files):
        raise FactsError(f"facts: {asset_name(decomp, commit, version)}: its files are not the manifest's")
    tmp = dest.with_name(dest.name + ".part")
    shutil.rmtree(tmp, ignore_errors=True)
    for rel, digest in sorted(files.items()):
        parts = Path(rel).parts
        if Path(rel).is_absolute() or ".." in parts or not parts or parts[0] != "build":
            raise FactsError(f"facts: {asset_name(decomp, commit, version)}: {rel} is outside build/")
        body = z.read(rel)
        if hashlib.sha256(body).hexdigest() != digest:
            raise FactsError(f"facts: {asset_name(decomp, commit, version)}: {rel} does not match its sha256")
        out = tmp / rel
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_bytes(body)
    (tmp / "manifest.json").write_text(json.dumps(manifest, indent=1) + "\n")
    shutil.rmtree(dest, ignore_errors=True)
    tmp.rename(dest)
    return dest


def download_url(name: str) -> str:
    base = os.environ.get("OPENRAC_FACTS_URL") or f"https://github.com/{REPOSITORY}/releases/download/{RELEASE_TAG}"
    return f"{base.rstrip('/')}/{name}"


def download(name: str) -> bytes | None:
    """The asset's bytes, or None when there is none."""
    try:
        with urllib.request.urlopen(download_url(name), timeout=120) as r:
            return r.read()
    except urllib.error.HTTPError as e:
        if e.code == 404:
            return None
        raise FactsError(f"facts: {download_url(name)}: HTTP {e.code}") from None
    except (urllib.error.URLError, FileNotFoundError) as e:
        if isinstance(e, FileNotFoundError) or isinstance(getattr(e, "reason", None), FileNotFoundError):
            return None
        raise FactsError(f"facts: {download_url(name)}: {e}") from None


def fetch(decomp: str, version: str, commit: str, cache: Path | None = None) -> Path:
    """The checked bundle of DECOMP at COMMIT for VERSION, unpacked under CACHE (build/facts/; downloaded once)."""
    dest = (cache or CACHE) / asset_name(decomp, commit, version)[:-len(".zip")]
    manifest = dest / "manifest.json"
    if manifest.is_file():
        m = json.loads(manifest.read_text())
        ok = m.get("commit") == commit and all(
            (dest / rel).is_file() and hashlib.sha256((dest / rel).read_bytes()).hexdigest() == h
            for rel, h in m.get("files", {}).items())
        if ok:
            return dest
    name = asset_name(decomp, commit, version)
    data = download(name)
    if data is None:
        raise FactsError(f"facts: no bundle {name}: build and publish facts for {decomp} {commit} "
                         f"(tools/build.py all, then `python3 tools/facts.py publish {decomp} {version}`)")
    return unpack(data, decomp, commit, version, dest)


def local(source: Path, s: dict) -> bool:
    """SOURCE has its own build of the version (its first file, the executable's): it is read as
    it is, as before there were bundles (a build without carry.json reads none)."""
    return any(p.is_file() for p in (source / s["build"]).glob(s["files"][0]))


def root(source: Path, decomp: str, version: str, commit: str | None = None) -> Path:
    """A folder that looks like DECOMP's checkout for reading VERSION's build facts: SOURCE when it
    has a build of them (a local build wins), else the bundle of SOURCE's HEAD (or COMMIT)."""
    s = spec(decomp, version)
    if local(source, s):
        return source
    return fetch(decomp, version, commit or head(source))


# --- publishing ---------------------------------------------------------------------------------

def token() -> str:
    for var in ("GH_TOKEN", "GITHUB_TOKEN"):
        if os.environ.get(var):
            return os.environ[var]
    try:
        r = subprocess.run(["gh", "auth", "token"], capture_output=True, text=True)
        if r.returncode == 0 and r.stdout.strip():
            return r.stdout.strip()
    except FileNotFoundError:
        pass
    raise FactsError("facts: no GitHub token: set GH_TOKEN, or log in with `gh auth login`")


def api(method: str, url: str, body: bytes | None = None, content_type: str = "application/json",
        auth: bool = True) -> dict | list | None:
    headers = {"Accept": "application/vnd.github+json", "User-Agent": "openrac-facts"}
    if auth:
        headers["Authorization"] = f"Bearer {token()}"
    if body is not None:
        headers["Content-Type"] = content_type
    req = urllib.request.Request(url, data=body, method=method, headers=headers)
    try:
        with urllib.request.urlopen(req, timeout=300) as r:
            text = r.read()
            return json.loads(text) if text else None
    except urllib.error.HTTPError as e:
        if e.code == 404:
            return None
        raise FactsError(f"facts: {method} {url}: HTTP {e.code}: {e.read()[:200]!r}") from None


def release(create: bool) -> dict | None:
    """The release that holds the bundles (made, as a pre-release, when CREATE)."""
    rel = api("GET", f"{API}/repos/{REPOSITORY}/releases/tags/{RELEASE_TAG}",
              auth=create or bool(os.environ.get("GH_TOKEN") or os.environ.get("GITHUB_TOKEN")))
    if rel is None and create:
        body = {"tag_name": RELEASE_TAG, "name": "Decomp build facts", "prerelease": True,
                "target_commitish": "develop",
                "body": "Build facts of the decomps (tools/facts.py): names, addresses, sizes and hashes; "
                        "no retail bytes. Do not delete."}
        rel = api("POST", f"{API}/repos/{REPOSITORY}/releases", json.dumps(body).encode())
    return rel


def published() -> set[str]:
    """The names of every published bundle."""
    rel = release(create=False)
    if rel is None:
        return set()
    names, page = set(), 1
    while True:
        batch = api("GET", f"{API}/repos/{REPOSITORY}/releases/{rel['id']}/assets?per_page=100&page={page}",
                    auth=bool(os.environ.get("GH_TOKEN") or os.environ.get("GITHUB_TOKEN"))) or []
        names |= {a["name"] for a in batch}
        if len(batch) < 100:
            return names
        page += 1


def missing(decomp: str, commit: str, versions: list[str] | None = None) -> list[str]:
    """The bundles of DECOMP at COMMIT not published, of VERSIONS (default: every version its spec has)."""
    have = published()
    return [asset_name(decomp, commit, v) for v in versions or sorted(SPECS.get(decomp, {}))
            if asset_name(decomp, commit, v) not in have]


def publish(decomp: str, version: str, source: Path, allow_dirty: bool = False) -> str:
    name, data = pack(decomp, version, source, allow_dirty)
    rel = release(create=True)
    existing = download(name)
    if existing is not None:
        old = json.loads(zipfile.ZipFile(io.BytesIO(existing)).read("manifest.json"))["files"]
        new = json.loads(zipfile.ZipFile(io.BytesIO(data)).read("manifest.json"))["files"]
        if old != new:
            raise FactsError(f"facts: {name} is published with other files: a build of one commit "
                             f"gave two results; delete the asset by hand if the new one is right")
        return f"{name}: already published, the same"
    upload = rel["upload_url"].split("{")[0] + f"?name={name}"
    api("POST", upload, data, "application/zip")
    return f"{name}: published ({len(data)} bytes)"


# --- command line -------------------------------------------------------------------------------

def default_source(decomp: str) -> Path:
    return ROOT / "games" / decomp


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0],
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest="cmd", required=True)
    p = sub.add_parser("pack", help="pack a bundle from a full build")
    p.add_argument("decomp"), p.add_argument("version")
    p.add_argument("--source", type=Path)
    p.add_argument("--out", type=Path, help="the zip (default: build/facts/<asset>)")
    p.add_argument("--allow-dirty", action="store_true", help="pack a checkout with changes (not to publish)")
    u = sub.add_parser("publish", help="pack and upload every version given (default: all)")
    u.add_argument("decomp"), u.add_argument("versions", nargs="*")
    u.add_argument("--source", type=Path)
    f = sub.add_parser("fetch", help="download and check a bundle into the cache")
    f.add_argument("decomp"), f.add_argument("version")
    f.add_argument("--commit", help="default: the pin")
    c = sub.add_parser("check", help="is a bundle published for each version")
    c.add_argument("decomp"), c.add_argument("versions", nargs="*")
    c.add_argument("--commit", help="default: the pin")
    args = ap.parse_args(argv)
    try:
        if args.cmd == "pack":
            source = (args.source or default_source(args.decomp)).resolve()
            name, data = pack(args.decomp, args.version, source, args.allow_dirty)
            out = args.out or CACHE / name
            out.parent.mkdir(parents=True, exist_ok=True)
            out.write_bytes(data)
            print(f"facts: {out} ({len(data)} bytes)")
        elif args.cmd == "publish":
            source = (args.source or default_source(args.decomp)).resolve()
            for v in args.versions or sorted(SPECS.get(args.decomp, {})):
                print(publish(args.decomp, v, source))
        elif args.cmd == "fetch":
            print(fetch(args.decomp, args.version, args.commit or pinned(args.decomp)))
        elif args.cmd == "check":
            commit = args.commit or pinned(args.decomp)
            gone = missing(args.decomp, commit, args.versions)
            if gone:
                print(f"facts: not published: {', '.join(gone)}: build and publish facts for "
                      f"{args.decomp} {commit}", file=sys.stderr)
                return 1
            print(f"facts: {args.decomp} {commit[:10]}: every bundle is published")
    except FactsError as e:
        print(e, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
