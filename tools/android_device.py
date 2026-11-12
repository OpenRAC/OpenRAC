#!/usr/bin/env python3
"""
Drives a debug build of OpenRAC on an Android device over adb (USB or wireless
debugging), to measure it without touching the device (port/android/README.md).

  python tools/android_device.py status
  python tools/android_device.py start [--env NAME=VALUE ...] [--arg TEXT ...] [--lib FILE]
  python tools/android_device.py sweep [--level N] [--at TICK] [--out DIR] [--env ...] [--lib FILE]
  python tools/android_device.py record SECONDS [--level N] [--at TICK] [--out DIR] [--env ...]
  python tools/android_device.py gpu [--seconds N]
  python tools/android_device.py pull PATH [--out DIR]
  python tools/android_device.py log [--seconds N]
  python tools/android_device.py stop | reset

The game is started as the launcher last started it (play once from the launcher
first), with what is given here added: environment variables (port/common/env.cpp lists them),
arguments, and a library file to load in place of the APK's (--lib, the game's
libopenrac-GAME.so from port/build/android-arm64). `sweep` runs the Graphics
sweep by script and copies its report here; `record` a recording of that many
seconds. `gpu` prints the GPU's clock, load and temperature once a second.
`reset` removes what was added, so the launcher starts the game as usual again.

-s SERIAL (or ANDROID_SERIAL) names the device, as for adb; --package the app
when more than one OpenRAC is installed. Standard library only.
"""

import argparse
import os
import shlex
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

ALIAS = "dev.openrac.game.DebugStart"
STAGING = "/data/local/tmp/openrac-debug"
# Adreno's kernel driver, as far as the shell may read it.
KGSL = "/sys/class/kgsl/kgsl-3d0"
KGSL_FILES = ("gpuclk", "max_gpuclk", "gpu_busy_percentage", "temp", "devfreq/governor",
              "devfreq/min_freq", "devfreq/max_freq", "thermal_pwrlevel")


class Device:
    def __init__(self, serial: str | None, package: str | None):
        self.adb = find_adb()
        self.serial = serial
        self._package = package

    def run(self, *args: str, data: bytes | None = None, check: bool = True, timeout: float = 120) -> bytes:
        command = [self.adb] + (["-s", self.serial] if self.serial else []) + list(args)
        done = subprocess.run(command, input=data, capture_output=True, timeout=timeout)
        if check and done.returncode != 0:
            sys.exit(f"{' '.join(command)}\n{(done.stderr or done.stdout).decode(errors='replace').strip()}")
        return done.stdout

    def shell(self, command: str, check: bool = True, timeout: float = 120) -> str:
        """Runs one command line in the device's shell."""
        return self.run("shell", command, check=check, timeout=timeout).decode(errors="replace").replace("\r\n", "\n")

    @property
    def package(self) -> str:
        if self._package is None:
            found = [line.split(":", 1)[1].strip()
                     for line in self.shell("pm list packages dev.openrac.launcher").splitlines() if ":" in line]
            if not found:
                sys.exit("no OpenRAC on the device (pm list packages dev.openrac.launcher)")
            if len(found) > 1:
                sys.exit("more than one OpenRAC is installed, name one with --package: " + ", ".join(found))
            self._package = found[0]
        return self._package

    def app(self, command: str, check: bool = True, timeout: float = 120) -> str:
        """Runs a command as the app, in its data folder (a debug build's run-as)."""
        return self.shell(f"run-as {self.package} sh -c {shlex.quote(command)}", check=check, timeout=timeout)

    def data_dir(self) -> str:
        return self.app("pwd").strip()

    def put(self, local: Path, name: str) -> str:
        """Copies a file into the app's files/debug; returns its path there."""
        staged = f"{STAGING}-{name}"
        self.run("push", str(local), staged, timeout=1800)
        self.shell(f"chmod 644 {shlex.quote(staged)}")
        self.app(f"mkdir -p files/debug && cp {shlex.quote(staged)} files/debug/{shlex.quote(name)}", timeout=600)
        self.shell(f"rm -f {shlex.quote(staged)}")
        return f"{self.data_dir()}/files/debug/{name}"

    def read(self, path: str) -> bytes:
        return self.run("exec-out", f"run-as {self.package} cat {shlex.quote(path)}", timeout=1800)


def find_adb() -> str:
    found = shutil.which("adb")
    if found:
        return found
    name = "adb.exe" if os.name == "nt" else "adb"
    for home in (os.environ.get("ANDROID_HOME"), os.environ.get("ANDROID_SDK_ROOT")):
        if home and (Path(home) / "platform-tools" / name).is_file():
            return str(Path(home) / "platform-tools" / name)
    sys.exit("adb was not found: put Android's platform-tools on PATH or set ANDROID_HOME")


def start(device: Device, env: list[str], args: list[str], lib: str | None) -> None:
    """Starts the game as the launcher last did, with `env`, `args` and the library `lib` added."""
    if "last_launch" not in device.app("ls files/debug 2>/dev/null", check=False):
        sys.exit("the game has not been started from the launcher with this build yet: play once, quit, try again")
    lines = [f"env {pair}" for pair in env] + [f"arg {arg}" for arg in args]
    if lib:
        local = Path(lib)
        if not local.is_file():
            sys.exit(f"no library at {local}")
        print(f"copying {local.name} ({local.stat().st_size / 1e6:.0f} MB)")
        lines.append("load " + device.put(local, "library.so"))
    device.shell(f"am force-stop {device.package}")
    with tempfile.TemporaryDirectory() as folder:
        extra = Path(folder) / "extra"
        extra.write_bytes(("\n".join(lines) + "\n").encode())
        device.put(extra, "extra")
    print(device.shell(f"am start -n {device.package}/{ALIAS}").strip())


def wait_and_pull(device: Device, remote: str, out: Path, timeout: float) -> None:
    """Waits for the report `remote` (the app's path) and copies it and its companions to `out`."""
    folder, name = remote.rsplit("/", 1)
    stem = name.rsplit(".", 1)[0]
    began = time.time()
    while name not in device.app(f"ls {shlex.quote(folder)} 2>/dev/null", check=False).split():
        if time.time() - began > timeout:
            sys.exit(f"no report after {timeout:.0f} s; see `log`")
        if "RESUMED" not in device.shell(f"dumpsys activity activities | grep -i {device.package} | grep -i state",
                                         check=False) and time.time() - began > 20:
            print("the game is not in front (crashed or closed?); still waiting")
        time.sleep(3)
    time.sleep(2)  # the frames file is closed after the report
    out.mkdir(parents=True, exist_ok=True)
    for entry in device.app(f"ls {shlex.quote(folder)}").split():
        if entry.startswith(stem):
            data = device.read(f"{folder}/{entry}")
            (out / entry).write_bytes(data)
            print(f"{out / entry} ({len(data) / 1e6:.1f} MB)")


def capture(device: Device, what: str, options: argparse.Namespace) -> None:
    stamp = time.strftime("%Y%m%d-%H%M%S")
    remote = f"{device.data_dir()}/files/debug/out/{what}-{stamp}.md"
    device.app("rm -rf files/debug/out && mkdir -p files/debug/out")
    env = [f"OPENRAC_PERF_CAPTURE={options.at}:{what if what == 'sweep' else options.seconds}:{remote}"] + options.env
    args = list(options.arg)
    if options.level is not None:
        args += ["--level", str(options.level), "--skip-movies", "--no-card"]
    start(device, env, args, options.lib)
    length = 160 + 90 if what == "sweep" else float(options.seconds)
    wait_and_pull(device, remote, Path(options.out), timeout=length + 240)
    device.shell(f"am force-stop {device.package}")


def gpu(device: Device, seconds: int) -> None:
    probe = "; ".join(f"echo {name}=$(cat {KGSL}/{name} 2>/dev/null)" for name in KGSL_FILES)
    for _ in range(max(seconds, 1)):
        values = dict(line.split("=", 1) for line in device.shell(probe, check=False).splitlines() if "=" in line)
        print(time.strftime("%H:%M:%S"), "  ".join(f"{name.rsplit('/', 1)[-1]} {value or '?'}"
                                                   for name, value in values.items()), flush=True)
        time.sleep(1)


def status(device: Device) -> None:
    print("device:", device.shell("getprop ro.product.manufacturer; getprop ro.product.model; "
                                  "getprop ro.soc.model; getprop ro.build.version.release").replace("\n", " ").strip())
    print("package:", device.package)
    who = device.shell(f"run-as {device.package} id", check=False).strip()
    print("debug build (run-as):", "yes" if who.startswith("uid=") else f"no ({who})")
    print("files/debug:", " ".join(device.app("ls files/debug 2>/dev/null", check=False).split()) or "empty")
    print("last launch:\n" + (device.app("cat files/debug/last_launch 2>/dev/null", check=False).strip() or "  none"))
    print("thermal status:", device.shell("dumpsys thermalservice | grep -i -m1 'thermal status'", check=False).strip())
    gpu(device, 1)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("-s", "--serial", default=os.environ.get("ANDROID_SERIAL"))
    parser.add_argument("--package")
    commands = parser.add_subparsers(dest="command", required=True)

    def launch_options(sub: argparse.ArgumentParser) -> None:
        sub.add_argument("--env", action="append", default=[], metavar="NAME=VALUE")
        sub.add_argument("--arg", action="append", default=[], metavar="TEXT")
        sub.add_argument("--lib", metavar="FILE")

    def capture_options(sub: argparse.ArgumentParser) -> None:
        launch_options(sub)
        sub.add_argument("--level", type=int, help="start in this level, without the memory card")
        sub.add_argument("--at", type=int, default=900, help="the game tick the recording starts at")
        sub.add_argument("--out", default="android-captures")

    commands.add_parser("status")
    launch_options(commands.add_parser("start"))
    capture_options(commands.add_parser("sweep"))
    record = commands.add_parser("record")
    record.add_argument("seconds", type=int)
    capture_options(record)
    commands.add_parser("gpu").add_argument("--seconds", type=int, default=30)
    pull = commands.add_parser("pull")
    pull.add_argument("path")
    pull.add_argument("--out", default="android-captures")
    commands.add_parser("log").add_argument("--seconds", type=int, default=0)
    commands.add_parser("stop")
    commands.add_parser("reset")
    options = parser.parse_args()
    device = Device(options.serial, options.package)

    if options.command == "status":
        status(device)
    elif options.command == "start":
        start(device, options.env, options.arg, options.lib)
    elif options.command in ("sweep", "record"):
        capture(device, options.command, options)
    elif options.command == "gpu":
        gpu(device, options.seconds)
    elif options.command == "pull":
        out = Path(options.out)
        out.mkdir(parents=True, exist_ok=True)
        (out / Path(options.path).name).write_bytes(device.read(options.path))
        print(out / Path(options.path).name)
    elif options.command == "log":
        command = [device.adb] + (["-s", device.serial] if device.serial else []) + \
                  ["logcat", "-v", "time", "-s", "OpenRAC", "SDL", "SDL/APP", "openrac"]
        try:
            subprocess.run(command + (["-d"] if options.seconds == 0 else []), timeout=options.seconds or None)
        except subprocess.TimeoutExpired:
            pass
    elif options.command == "stop":
        device.shell(f"am force-stop {device.package}")
    elif options.command == "reset":
        device.shell(f"am force-stop {device.package}")
        device.app("rm -rf files/debug/extra files/debug/library.so files/debug/out")


if __name__ == "__main__":
    main()
