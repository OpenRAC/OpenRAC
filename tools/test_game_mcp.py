"""tools/game_mcp.py, the game's MCP server, against a stand-in for the port's --control channel
(no disc): the MCP handshake, the tools, the pad's buttons, pictures, and an editor cutscene
started by its trigger. With OPENRAC_GAME, OPENRAC_DATA and OPENRAC_LEVELS (and a display or
SDL_VIDEODRIVER=offscreen) it also drives the real game on Kerwan."""

import base64
import json
import os
import subprocess
import sys
import tempfile
import textwrap
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent

# The port's side, as control.cpp answers: a hero, two mobys, pictures written on request.
FAKE = textwrap.dedent('''
    import json, sys, zlib, struct
    P = "@openrac-control "
    hero = [10.0, 20.0, 5.0]
    tick = 0
    log = open(sys.argv[1], "a")
    def png(path):
        raw = b"\\x00\\xff\\x00\\x00"
        body = b"".join(struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d))
                        for t, d in ((b"IHDR", struct.pack(">IIBBBBB", 1, 1, 8, 2, 0, 0, 0)),
                                     (b"IDAT", zlib.compress(raw)), (b"IEND", b"")))
        open(path, "wb").write(b"\\x89PNG\\r\\n\\x1a\\n" + body)
    def status():
        return {"ok": True, "tick": tick, "phase": "playing", "level": 3, "hero": {"position": hero, "yaw": 0}}
    print("some log line"); print(P + json.dumps({"id": None, "ok": True, "ready": True, "game": "rac1-pal"}), flush=True)
    for line in sys.stdin:
        c = json.loads(line); log.write(line); log.flush()
        out = {"ok": True}
        if c["cmd"] in ("status",): out = status()
        elif c["cmd"] in ("step", "wait"):
            tick += c.get("ticks", 1) if c["cmd"] == "step" else 1
            out = dict(status(), ran=1, reached=True)
        elif c["cmd"] == "hero_place": hero = c["position"]
        elif c["cmd"] == "shot": png(c["path"]); out = {"ok": True, "path": c["path"]}
        elif c["cmd"] == "mobys": out = {"ok": True, "count": 1, "mobys": [{"slot": 4, "spawn_id": 77, "class": 11, "position": [1, 2, 3]}]}
        elif c["cmd"] == "quit": print(P + json.dumps({"id": c["id"], "ok": True}), flush=True); break
        else: out = {"ok": False, "error": "unknown"}
        print(P + json.dumps(dict(out, id=c["id"])), flush=True)
''')


class Client:
    def __init__(self, *args: str):
        self.process = subprocess.Popen([sys.executable, str(HERE / "game_mcp.py"), *args], stdin=subprocess.PIPE,
                                        stdout=subprocess.PIPE, text=True)
        self.next = 0

    def rpc(self, method: str, params: dict | None = None) -> dict:
        self.next += 1
        self.process.stdin.write(json.dumps({"jsonrpc": "2.0", "id": self.next, "method": method,
                                             "params": params or {}}) + "\n")
        self.process.stdin.flush()
        return json.loads(self.process.stdout.readline())

    def tool(self, tool_name: str, **arguments) -> dict:
        return self.rpc("tools/call", {"name": tool_name, "arguments": arguments})["result"]

    def json(self, tool_name: str, **arguments) -> dict:
        result = self.tool(tool_name, **arguments)
        assert not result.get("isError"), result
        return json.loads(result["content"][0]["text"])

    def close(self) -> None:
        self.process.stdin.close()
        self.process.wait(20)


@unittest.skipIf(sys.platform == "win32", "the stand-in is a shell script")
class WithAStandIn(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        root = Path(self.temp.name)
        (root / "fake.py").write_text(FAKE, encoding="utf-8")
        self.sent = root / "sent.jsonl"
        game = root / "game"
        game.write_text(f"#!/bin/sh\nexec {sys.executable} {root / 'fake.py'} {self.sent}\n", encoding="utf-8")
        game.chmod(0o755)
        self.packed = root / "packed"
        (self.packed / "level_03" / "cutscenes").mkdir(parents=True)
        (self.packed / "level_03" / "cutscenes" / "meeting.cutscene").write_text(json.dumps(
            {"format": "openrac-cutscene", "trigger": {"kind": "volume", "sphere": {"position": [50, 60, 7], "radius": 4}}}))
        (self.packed / "level_03" / "cutscenes" / "talk.cutscene").write_text(json.dumps(
            {"format": "openrac-cutscene", "trigger": {"kind": "talk", "spawn_id": 77}}))
        self.client = Client("--game", str(game), "--data", str(root))

    def tearDown(self):
        self.client.close()
        self.temp.cleanup()

    def sent_commands(self) -> list[dict]:
        return [json.loads(line) for line in self.sent.read_text().splitlines()]

    def test_tools_and_a_session(self):
        init = self.client.rpc("initialize", {"protocolVersion": "2025-06-18"})["result"]
        self.assertEqual(init["serverInfo"]["name"], "openrac-game")
        names = {t["name"] for t in self.client.rpc("tools/list")["result"]["tools"]}
        self.assertLessEqual({"game_start", "game_step", "game_shot", "game_mobys", "game_hero_place",
                              "game_scene", "game_camera", "game_wait", "game_log"}, names)
        self.assertTrue(self.client.tool("game_status")["isError"])  # nothing runs yet

        started = self.client.json("game_start", level=3, mods=str(self.packed))
        self.assertEqual(started["level"], 3)
        self.assertEqual(self.client.json("game_step", ticks=10, buttons=["cross", "l1"], left=[128, 0])["tick"], 11)
        step = [c for c in self.sent_commands() if c["cmd"] == "step"][0]
        self.assertEqual(step["buttons"], 0x4000 | 0x400)
        self.assertEqual(step["left"], [128, 0])
        self.assertIn("No button", self.client.tool("game_step", buttons=["jump"])["content"][0]["text"])

        shot = self.client.tool("game_shot")
        self.assertEqual(shot["content"][0]["type"], "image")
        self.assertTrue(base64.b64decode(shot["content"][0]["data"]).startswith(b"\x89PNG"))

        self.assertEqual(self.client.json("game_mobys", near=[0, 0, 0])["mobys"][0]["spawn_id"], 77)
        self.assertIn("some log line", self.client.json("game_log")["lines"])

        # An editor cutscene: the hero is put where its trigger fires.
        self.client.json("game_scene", name="meeting")
        places = [c["position"] for c in self.sent_commands() if c["cmd"] == "hero_place"]
        self.assertEqual(places[-1], [50, 60, 7])
        self.client.json("game_scene", name="talk")
        steps = [c for c in self.sent_commands() if c["cmd"] == "step"]
        self.assertEqual(steps[-1]["buttons"], 0x2000)  # circle talks
        self.assertIn("No cutscene", self.client.tool("game_scene", name="gone")["content"][0]["text"])

        self.client.json("game_stop")
        self.assertEqual(self.sent_commands()[-1]["cmd"], "quit")


@unittest.skipUnless(os.environ.get("OPENRAC_GAME") and os.environ.get("OPENRAC_DATA"),
                     "set OPENRAC_GAME, OPENRAC_DATA and OPENRAC_LEVELS for the real game")
class InTheGame(unittest.TestCase):
    def test_kerwan(self):
        client = Client()
        try:
            started = client.json("game_start", level=3)
            self.assertEqual(started["phase"], "playing", started)
            hero = started["hero"]["position"]
            moved = client.json("game_step", ticks=60, left=[128, 0])
            self.assertNotEqual(moved["hero"]["position"], hero)
            shot = client.tool("game_shot")
            self.assertGreater(len(base64.b64decode(shot["content"][0]["data"])), 10000)
            self.assertGreater(client.json("game_mobys", near=hero, radius=30)["count"], 0)
            client.json("game_hero_place", position=[hero[0] + 5, hero[1], hero[2] + 2], yaw=90)
            placed = client.json("game_step", ticks=30)
            self.assertAlmostEqual(placed["hero"]["position"][0], hero[0] + 5, delta=0.5)
        finally:
            client.json("game_stop")
            client.close()


if __name__ == "__main__":
    unittest.main()
