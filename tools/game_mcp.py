#!/usr/bin/env python3
"""OpenRAC's game as an MCP server (Model Context Protocol, over stdio), for AI assistants: start
the port on a level, step it a tick at a time with the pad held, take pictures, list the mobys,
place the hero, travel, start scenes and move the camera. OpenRAC Editor's own server edits a
level; this one shows it in the game. The port is driven through its --control channel
(port/game/common/program/control.h). Commands and tools: docs/port/GAME_MCP.md.

    claude mcp add openrac-game -- python3 tools/game_mcp.py \\
        --game port/build/release/openrac-rac1-pal --data <data> --levels <levels>

Python's standard library only.
"""

from __future__ import annotations

import argparse
import base64
import json
import os
import queue
import subprocess
import sys
import tempfile
import threading
import time
from pathlib import Path

PROTOCOL = "2025-06-18"
PREFIX = "@openrac-control "

# The pad's buttons as the game reads them (active low in the report; here, pressed).
BUTTONS = {
    "select": 0x1, "l3": 0x2, "r3": 0x4, "start": 0x8, "up": 0x10, "right": 0x20, "down": 0x40,
    "left": 0x80, "l2": 0x100, "r2": 0x200, "l1": 0x400, "r1": 0x800, "triangle": 0x1000,
    "circle": 0x2000, "cross": 0x4000, "square": 0x8000,
}

LOG_LINES = 400


def prop(kind: str, description: str, **more) -> dict:
    return {"type": kind, "description": description, **more}


VECTOR = {"type": "array", "items": {"type": "number"}, "minItems": 3, "maxItems": 3}
STICK = {"type": "array", "items": {"type": "number"}, "minItems": 2, "maxItems": 2}


class Game:
    """One running port under --control: its commands, answers and output lines."""

    def __init__(self, command: list[str], env: dict, cwd: str | None = None):
        self.command = command
        self.process = subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                        stderr=subprocess.STDOUT, text=True, encoding="utf-8",
                                        errors="replace", env=env, cwd=cwd, bufsize=1)
        self.answers: queue.Queue = queue.Queue()
        self.log: list[str] = []
        self.log_total = 0
        self.lock = threading.Lock()
        self.next_id = 0
        self.shots = Path(tempfile.mkdtemp(prefix="openrac-game-shots-"))
        threading.Thread(target=self.read, daemon=True).start()
        ready = self.answer(None, 60)
        if not ready.get("ready"):
            raise RuntimeError("the game did not start its control channel: " + self.tail(30))
        self.game = ready.get("game", "")

    def read(self) -> None:
        for line in self.process.stdout:
            line = line.rstrip("\n")
            if line.startswith(PREFIX):
                try:
                    self.answers.put(json.loads(line[len(PREFIX):]))
                except json.JSONDecodeError:
                    pass
                continue
            with self.lock:
                self.log.append(line)
                self.log_total += 1
                del self.log[:-LOG_LINES]
        self.answers.put({"id": None, "ok": False, "error": "the game ended", "ended": True})

    def answer(self, wanted, timeout: float) -> dict:
        until = time.monotonic() + timeout
        while True:
            left = until - time.monotonic()
            if left <= 0:
                raise TimeoutError(f"no answer from the game in {timeout:.0f} s")
            try:
                got = self.answers.get(timeout=left)
            except queue.Empty:
                continue
            if got.get("ended"):
                raise RuntimeError("the game ended: " + self.tail(20))
            if got.get("id") == wanted:
                return got

    def call(self, command: dict, timeout: float = 120) -> dict:
        if self.process.poll() is not None:
            raise RuntimeError("the game is not running: " + self.tail(20))
        self.next_id += 1
        command = dict(command, id=self.next_id)
        self.process.stdin.write(json.dumps(command) + "\n")
        self.process.stdin.flush()
        return self.answer(self.next_id, timeout)

    def tail(self, count: int) -> str:
        with self.lock:
            return "\n".join(self.log[-count:])

    def stop(self) -> None:
        if self.process.poll() is None:
            try:
                self.process.stdin.write(json.dumps({"id": 0, "cmd": "quit"}) + "\n")
                self.process.stdin.close()
                self.process.wait(10)
            except (OSError, subprocess.TimeoutExpired, ValueError):
                self.process.kill()
                self.process.wait(5)


class Server:
    def __init__(self, defaults: argparse.Namespace):
        self.defaults = defaults
        self.game: Game | None = None
        self.started_with: dict = {}

    # The tools: name, description, input schema, handler.
    def tools(self) -> list[tuple]:
        return [
            ("game_start", "Starts the game on a level (stopping one that runs), with the editor's packed levels, "
             "and waits until the player has control (Start skips the scenes on the way). The game then waits "
             "between calls: it runs only the ticks game_step and game_wait ask for (60 a second).", {
                "level": prop("integer", "The level (Veldin 1 ... Drek's fleet 18)."),
                "mods": prop("string", "The packed levels folder (OpenRAC Editor's project_info folders.packed)."),
                "plugins": prop("string", "A plugins.json with the mods to run."),
                "game": prop("string", "The port's program (default: the server's --game)."),
                "data": prop("string", "The game data folder (default: --data)."),
                "levels": prop("string", "The extracted levels folder (default: --levels)."),
                "wait": prop("boolean", "Wait until the player has control (default true)."),
                "env": prop("object", "Extra environment for the game, e.g. {\"SDL_VIDEODRIVER\": \"offscreen\"}.")},
             ["level"], self.start),
            ("game_stop", "Stops the game.", {}, [], self.stop),
            ("game_status", "The game now: tick, phase (front_end, loading, playing, scene, menu), level, the hero "
             "(position, heading, state, health), the game's scene playing and the camera.", {}, [], self.status),
            ("game_step", "Runs ticks with the pad held: buttons (cross, circle, square, triangle, start, select, "
             "l1, r1, l2, r2, l3, r3, up, down, left, right) and sticks [x, y] from 0 to 255 (128 centred, 0 left "
             "or up). Answers with the status after.", {
                "ticks": prop("integer", "How many (60 a second; default 1)."),
                "buttons": {"type": "array", "items": {"type": "string"}, "description": "Buttons held."},
                "left": dict(STICK, description="The left stick [x, y] (walk)."),
                "right": dict(STICK, description="The right stick [x, y] (camera).")}, [], self.step),
            ("game_wait", "Runs ticks until: playing (the player has control), level (with level), scene (a game "
             "scene plays) or scene_end; at most max_ticks.", {
                "until": prop("string", "playing, level, scene or scene_end."),
                "level": prop("integer", "For until level."),
                "max_ticks": prop("integer", "The most to run (default 3600)."),
                "skip": prop("boolean", "Press Start now and then while a scene plays, to skip it.")}, ["until"], self.wait),
            ("game_shot", "A picture of the game as the player sees it, subtitles included (the next frame drawn, a tick on).", {}, [], self.shot),
            ("game_mobys", "The level's live mobys: slot, spawn number, class, state, hidden, dead, position, heading. "
             "Near a point, nearest first.", {
                "near": dict(VECTOR, description="[x, y, z]."), "radius": prop("number", "Only within this distance."),
                "class": prop("integer", "Only this class."), "limit": prop("integer", "At most (default 50).")}, [], self.mobys),
            ("game_hero_place", "Puts the hero at a place, facing a heading (degrees about z, 0 along +x); there "
             "after the next tick.", {
                "position": dict(VECTOR, description="[x, y, z] in the game's coordinates."),
                "yaw": prop("number", "Heading in degrees."),
                "ground": prop("boolean", "Drop him to the ground below (default true).")}, ["position"], self.hero_place),
            ("game_travel", "Leaves the level for another, as the ship does; then game_wait until level.",
             {"level": prop("integer", "The level.")}, ["level"], self.travel),
            ("game_scene", "Starts a cutscene: one the editor made (by name, from the packed levels: the hero is put "
             "where its trigger fires and it runs until it plays), or one of the game's own (number).", {
                "name": prop("string", "The editor's cutscene (its name in the level)."),
                "number": prop("integer", "A scene of the game's own, by its number in the level.")}, [], self.scene),
            ("game_camera", "Puts the camera at an eye looking at a point (it stays until released), or lets it go.", {
                "eye": dict(VECTOR, description="[x, y, z]."), "look_at": dict(VECTOR, description="[x, y, z]."),
                "release": prop("boolean", "Give the camera back to the game.")}, [], self.camera),
            ("game_line", "Whether the level's walls and ground stand between two points.", {
                "from": dict(VECTOR, description="[x, y, z]."), "to": dict(VECTOR, description="[x, y, z].")},
             ["from", "to"], self.line),
            ("game_ground", "The floor under a point in the running level (its collision as the game has it now).", {
                "position": dict(VECTOR, description="[x, y, z].")}, ["position"], self.ground),
            ("game_log", "The game's last output lines.", {"lines": prop("integer", "How many (default 60).")}, [], self.log),
        ]

    # --- tools ---------------------------------------------------------------------------------

    def need(self) -> Game:
        if self.game is None or self.game.process.poll() is not None:
            raise RuntimeError("No game is running: game_start first.")
        return self.game

    def start(self, a: dict) -> dict:
        self.stop({})
        game = a.get("game") or self.defaults.game
        data = a.get("data") or self.defaults.data
        levels = a.get("levels") or self.defaults.levels
        if not game or not data:
            raise ValueError("Say where the game and its data are (game and data, or the server's --game and --data).")
        level = int(a["level"])
        command = [game, "--control", "--window", "--data", data, "--level", str(level), "--no-card"]
        if levels:
            command += ["--levels", levels]
        if a.get("mods"):
            command += ["--mods", a["mods"]]
        if a.get("plugins") or self.defaults.plugins:
            command += ["--plugins", a.get("plugins") or self.defaults.plugins]
        env = dict(os.environ)
        # Straight into the level, past the title's notices (rac1/host/boot.c).
        env["OPENRAC_DIRECT"] = "1"
        env.update({str(k): str(v) for k, v in (a.get("env") or {}).items()})
        if sys.platform.startswith("linux") and not env.get("DISPLAY") and not env.get("WAYLAND_DISPLAY"):
            env.setdefault("SDL_VIDEODRIVER", "offscreen")
            env.setdefault("SDL_AUDIO_DRIVER", "dummy")
        self.game = Game(command, env)
        self.started_with = {"level": level, "mods": a.get("mods", "")}
        if a.get("wait", True):
            answer = self.game.call({"cmd": "wait", "until": "level", "level": level, "max_ticks": 7200, "skip": True}, 600)
            if not answer.get("reached"):
                answer["note"] = "The player has no control yet: game_wait again, or game_shot to see why."
            return answer
        return self.game.call({"cmd": "status"})

    def stop(self, _a: dict) -> dict:
        if self.game is not None:
            self.game.stop()
            self.game = None
        return {"ok": True}

    def status(self, _a: dict) -> dict:
        return self.need().call({"cmd": "status"})

    def step(self, a: dict) -> dict:
        mask = 0
        for name in a.get("buttons", []):
            if str(name).lower() not in BUTTONS:
                raise ValueError(f"No button {name}: " + ", ".join(BUTTONS))
            mask |= BUTTONS[str(name).lower()]
        command = {"cmd": "step", "ticks": int(a.get("ticks", 1)), "buttons": mask}
        for stick in ("left", "right"):
            if stick in a:
                command[stick] = a[stick]
        return self.need().call(command, 60 + int(a.get("ticks", 1)) / 10)

    def wait(self, a: dict) -> dict:
        command = {"cmd": "wait", "until": a["until"], "max_ticks": int(a.get("max_ticks", 3600)), "skip": bool(a.get("skip", False))}
        if "level" in a:
            command["level"] = int(a["level"])
        return self.need().call(command, 60 + command["max_ticks"] / 10)

    def shot(self, _a: dict) -> dict:
        game = self.need()
        path = game.shots / f"shot_{game.next_id + 1}.png"
        answer = game.call({"cmd": "shot", "path": str(path)})
        if not answer.get("ok"):
            return answer
        # The picture is written beside the game's work: wait until the whole file is there.
        until = time.monotonic() + 20
        data = b""
        while time.monotonic() < until:
            if path.is_file():
                data = path.read_bytes()
                if data.endswith(b"IEND\xaeB`\x82"):
                    break
            time.sleep(0.05)
        else:
            raise RuntimeError("the picture was not written")
        status = game.call({"cmd": "status"})
        return {"content": [{"type": "image", "mimeType": "image/png", "data": base64.b64encode(data).decode()},
                            {"type": "text", "text": json.dumps(status)}]}

    def mobys(self, a: dict) -> dict:
        command = {"cmd": "mobys", "limit": int(a.get("limit", 50))}
        for key in ("near", "radius", "class"):
            if key in a:
                command[key] = a[key]
        return self.need().call(command)

    def hero_place(self, a: dict) -> dict:
        return self.need().call({"cmd": "hero_place", "position": a["position"], "yaw": float(a.get("yaw", 0)),
                                 "ground": bool(a.get("ground", True))})

    def travel(self, a: dict) -> dict:
        return self.need().call({"cmd": "travel", "level": int(a["level"])})

    def scene(self, a: dict) -> dict:
        game = self.need()
        if "number" in a:
            return game.call({"cmd": "scene", "number": int(a["number"])})
        name = str(a.get("name", ""))
        if not name:
            raise ValueError("Give name (the editor's cutscene) or number (the game's).")
        doc = self.cutscene(name)
        trigger = doc.get("trigger", {})
        kind = trigger.get("kind", "manual")
        if kind == "volume":
            centre = trigger.get("sphere", {}).get("position") or trigger.get("box", {}).get("position")
            if not centre:
                raise ValueError(f"The cutscene {name}'s volume has no position.")
            # Out of the volume first, so stepping in fires it.
            game.call({"cmd": "hero_place", "position": [centre[0], centre[1], centre[2] + 200], "yaw": 0, "ground": False})
            game.call({"cmd": "step", "ticks": 2})
            game.call({"cmd": "hero_place", "position": centre, "yaw": 0, "ground": True})
            answer = game.call({"cmd": "step", "ticks": 30})
        elif kind == "talk":
            spawn = trigger.get("spawn_id")
            found = [m for m in game.call({"cmd": "mobys", "limit": 5000})["mobys"] if m["spawn_id"] == spawn]
            if not found:
                raise ValueError(f"No moby with spawn number {spawn} in the level now.")
            at = found[0]["position"]
            game.call({"cmd": "hero_place", "position": [at[0] + 1.5, at[1], at[2]], "yaw": 180, "ground": True})
            game.call({"cmd": "step", "ticks": 20})
            answer = game.call({"cmd": "step", "ticks": 6, "buttons": BUTTONS["circle"]})
        elif kind == "level_start":
            level = self.started_with.get("level", -1)
            game.call({"cmd": "travel", "level": level})
            answer = game.call({"cmd": "wait", "until": "level", "level": level, "max_ticks": 7200}, 600)
        else:
            raise ValueError(f"The cutscene {name} starts on a {kind} trigger: play it in the game as a player would.")
        answer["note"] = "Its trigger fired: game_step and game_shot to watch it (Start skips it)."
        return answer

    def cutscene(self, name: str) -> dict:
        packed = self.started_with.get("mods", "")
        level = self.started_with.get("level", -1)
        if not packed:
            raise ValueError("game_start with mods (the packed levels) to play the editor's cutscenes.")
        for folder in sorted(Path(packed).glob(f"*{level:02d}*")) + [Path(packed)]:
            file = folder / "cutscenes" / f"{name}.cutscene"
            if file.is_file():
                return json.loads(file.read_text(encoding="utf-8"))
        raise ValueError(f"No cutscene {name} packed for level {level} (level_pack in the editor first).")

    def camera(self, a: dict) -> dict:
        if a.get("release"):
            return self.need().call({"cmd": "camera", "release": True})
        return self.need().call({"cmd": "camera", "eye": a.get("eye"), "look_at": a.get("look_at")})

    def line(self, a: dict) -> dict:
        return self.need().call({"cmd": "line", "from": a["from"], "to": a["to"]})

    def ground(self, a: dict) -> dict:
        return self.need().call({"cmd": "ground", "position": a["position"]})

    def log(self, a: dict) -> dict:
        if self.game is None:
            return {"lines": []}
        return {"lines": self.game.tail(int(a.get("lines", 60))).splitlines()}

    # --- MCP -----------------------------------------------------------------------------------

    def handle(self, message: dict) -> dict | None:
        method = message.get("method", "")
        mid = message.get("id")
        if mid is None:
            return None  # a notification
        params = message.get("params") or {}
        if method == "initialize":
            result = {"protocolVersion": PROTOCOL, "capabilities": {"tools": {}},
                      "serverInfo": {"name": "openrac-game", "version": "1.0"},
                      "instructions": "OpenRAC's game, driven a tick at a time. game_start on a level (with the "
                      "editor's packed levels as mods), then game_step, game_shot, game_mobys. Coordinates are "
                      "the game's: x and y across, z up, headings in degrees. docs/port/GAME_MCP.md."}
        elif method == "ping":
            result = {}
        elif method == "tools/list":
            result = {"tools": [{"name": name, "description": about,
                                 "inputSchema": {"type": "object", "properties": props, "required": required}}
                                for name, about, props, required, _ in self.tools()]}
        elif method == "tools/call":
            result = self.call(params.get("name", ""), params.get("arguments") or {})
        else:
            return {"jsonrpc": "2.0", "id": mid, "error": {"code": -32601, "message": f"No method {method}"}}
        return {"jsonrpc": "2.0", "id": mid, "result": result}

    def call(self, name: str, arguments: dict) -> dict:
        for tool, _, _, _, handler in self.tools():
            if tool == name:
                try:
                    out = handler(arguments)
                except (ValueError, RuntimeError, TimeoutError, OSError, KeyError) as error:
                    return {"content": [{"type": "text", "text": str(error)}], "isError": True}
                if isinstance(out, dict) and "content" in out:
                    return out
                failed = isinstance(out, dict) and out.get("ok") is False
                return {"content": [{"type": "text", "text": json.dumps(out)}], "isError": failed}
        return {"content": [{"type": "text", "text": f"No tool {name}."}], "isError": True}


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--game", default=os.environ.get("OPENRAC_GAME", ""), help="the port's program")
    parser.add_argument("--data", default=os.environ.get("OPENRAC_DATA", ""), help="the game data folder")
    parser.add_argument("--levels", default=os.environ.get("OPENRAC_LEVELS", ""), help="the extracted levels folder")
    parser.add_argument("--plugins", default="", help="a plugins.json with the mods to run")
    server = Server(parser.parse_args(argv))
    try:
        for line in sys.stdin:
            if not line.strip():
                continue
            try:
                message = json.loads(line)
            except json.JSONDecodeError:
                reply = {"jsonrpc": "2.0", "id": None, "error": {"code": -32700, "message": "Parse error"}}
            else:
                reply = server.handle(message) if isinstance(message, dict) else None
            if reply is not None:
                sys.stdout.write(json.dumps(reply) + "\n")
                sys.stdout.flush()
    finally:
        server.stop({})
    return 0


if __name__ == "__main__":
    sys.exit(main())
