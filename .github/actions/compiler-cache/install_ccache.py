"""Put ccache on PATH without the system package manager.

apt mirrors fail now and then (DNS errors), and a red job for a missing cache is wrong.
So we fetch ccache's own static build from GitHub (pinned, SHA-256 checked, with retries).
If that fails too, a pass-through `ccache` that only runs the compiler goes on PATH:
the build is slower, but it still passes.
"""

import hashlib
import io
import os
import platform
import stat
import sys
import tarfile
import time
import urllib.request
import zipfile
from pathlib import Path

VERSION = "4.13.6"
URL = f"https://github.com/ccache/ccache/releases/download/v{VERSION}/ccache-{VERSION}-{{name}}"
# SHA-256 of the ccache binary inside each archive (the same pins as hendrikmuhs/ccache-action).
BUILDS = {
    ("Linux", "x86_64"): ("linux-x86_64-musl-static.tar.xz",
                          "c4ca67395c175798c588ee5425c135e76bee5701add4d0631d8b4be0b161b322"),
    ("Linux", "aarch64"): ("linux-aarch64-musl-static.tar.xz",
                           "e678bcfe84a0fea0865307dca3f8ac38f4c881125afea91e3a9397ed3a087666"),
    ("Darwin", "x86_64"): ("darwin.tar.gz", "1d8cc4e68ed61355bc9b29191e05fab2b764d98a41fed794e7d6a9d34b5c3735"),
    ("Darwin", "arm64"): ("darwin.tar.gz", "1d8cc4e68ed61355bc9b29191e05fab2b764d98a41fed794e7d6a9d34b5c3735"),
    ("Windows", "AMD64"): ("windows-x86_64.zip",
                           "1f285645553e61463b000c6c440301724894b4ba0174dbef6b4818d54b54b68d"),
    ("Windows", "ARM64"): ("windows-aarch64.zip",
                           "01fd8dc542a97d71ba886a861ca26ba0343bd92356fde3c72db73da3e91840a6"),
}
SHIM = """#!/bin/sh
# Stand-in for ccache: options (stats, config) do nothing, a compile runs as is.
case "$1" in -*|"") exit 0 ;; esac
exec "$@"
"""


def download(url):
    """Fetch a URL, retrying network errors with a growing wait."""
    for attempt in range(1, 6):
        try:
            with urllib.request.urlopen(url, timeout=60) as response:
                return response.read()
        except OSError as error:
            print(f"Download attempt {attempt} failed: {error}", flush=True)
            if attempt == 5:
                raise
            time.sleep(5 * attempt)


def binary(archive, name, member):
    """Return the ccache binary's bytes from a release archive."""
    path = f"ccache-{VERSION}-{name.rsplit('.tar', 1)[0].removesuffix('.zip')}/{member}"
    if name.endswith(".zip"):
        with zipfile.ZipFile(io.BytesIO(archive)) as stream:
            return stream.read(path)
    with tarfile.open(fileobj=io.BytesIO(archive)) as stream:
        return stream.extractfile(path).read()


def main():
    folder = Path(sys.argv[1])
    folder.mkdir(parents=True, exist_ok=True)
    windows = platform.system() == "Windows"
    target = folder / ("ccache.exe" if windows else "ccache")
    build = BUILDS.get((platform.system(), platform.machine()))
    try:
        if not build:
            raise RuntimeError(f"no ccache build for {platform.system()} {platform.machine()}")
        name, sha256 = build
        data = binary(download(URL.format(name=name)), name, target.name)
        if hashlib.sha256(data).hexdigest() != sha256:
            raise RuntimeError("ccache binary SHA-256 does not match the pin")
        target.write_bytes(data)
    except Exception as error:  # noqa: BLE001 - any failure falls back to the stand-in
        print(f"::warning title=ccache::ccache {VERSION} not installed ({error}); "
              "building without a compiler cache.", flush=True)
        if windows:
            return 1  # A shell script can't stand in for an .exe; the step fails softly.
        target.write_text(SHIM)
        with open(os.environ["GITHUB_ENV"], "a", encoding="utf-8") as env:
            env.write("OPENRAC_CCACHE_SHIM=1\n")
    target.chmod(target.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)
    with open(os.environ["GITHUB_PATH"], "a", encoding="utf-8") as path:
        path.write(f"{folder}\n")
    print(f"ccache ready at {target}", flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
