# The game's MCP server

`tools/game_mcp.py` lets an AI assistant play and look at a level in OpenRAC's port, through the
[Model Context Protocol](https://modelcontextprotocol.io) over stdio. OpenRAC Editor's own MCP
server edits a level (its `docs/MCP.md`); this one shows the result in the game: start the port
on the level with the editor's packed levels, step it a tick at a time with the pad held, take
pictures, list the mobys, put the hero somewhere, start a cutscene, move the camera.

It needs only Python's standard library and a built port.

## Connect

```sh
claude mcp add openrac-game -- python3 tools/game_mcp.py \
  --game port/build/release/openrac-rac1-pal --data <game data> --levels <levels>
```

`--game`, `--data` and `--levels` (or `OPENRAC_GAME`, `OPENRAC_DATA`, `OPENRAC_LEVELS`) are the
defaults `game_start` uses; `--plugins` names a `plugins.json` with the mods to run (a map's cutscenes and behaviours
play without one: the game has their player built in). On Linux without a display the game
draws off screen (`SDL_VIDEODRIVER=offscreen`).

## Tools

| Tool | What it does |
|---|---|
| `game_start` | Starts the game on a level (with `mods`, the editor's packed levels) and waits until the player has control, skipping the scenes on the way |
| `game_stop` | Stops it |
| `game_status` | Tick, phase (`front_end`, `loading`, `playing`, `scene`, `menu`), level, the hero (position, heading, state, health), the game's scene playing, the camera |
| `game_step` | Runs ticks (60 a second) with buttons held (`cross`, `circle`, `square`, `triangle`, `start`, `select`, `l1` ... `r3`, `up` ... `right`) and the sticks (`[x, y]`, 0 to 255, 128 centred) |
| `game_wait` | Runs ticks until `playing`, `level`, `scene` or `scene_end`, at most `max_ticks` |
| `game_shot` | A picture of the window as the player sees it (subtitles and the mods' drawing included), a tick on |
| `game_mobys` | The live mobys: slot, spawn number, class, state, hidden, dead, position, heading; near a point |
| `game_hero_place` | Puts the hero at a place and heading, on the ground below |
| `game_travel` | Leaves for another level, as the ship does |
| `game_scene` | Starts an editor cutscene by name (the hero is put where its trigger fires: a volume, or talking to its moby) or a scene of the game's own by number |
| `game_camera` | Puts the camera at an eye looking at a point, or gives it back |
| `game_line`, `game_ground` | Whether walls stand between two points; the floor under a point |
| `game_log` | The game's last output lines |

The game waits between calls: only `game_step`, `game_wait` and `game_shot` run ticks, as fast as
the machine allows, so two runs with the same calls see the same game. Coordinates are the game's:
x and y across, z up, headings in degrees about z (0 along +x).

## The port's control channel

The server drives the port started with `--control` (`port/game/common/program/control.h`). Each
line on its stdin is a JSON command, `{"id": 1, "cmd": "status"}`; each answer is one line on its
stdout, `@openrac-control {"id": 1, "ok": true, ...}`, among its other output. The first line it
writes is `{"id": null, "ready": true}`. The game is held at the end of a tick until a command
asks for ticks.

| Command | Fields | Answer |
|---|---|---|
| `status` | | `tick`, `phase`, `level`, `level_name`, `loads`, `hero`, `scene`, `camera` |
| `step` | `ticks`, `buttons` (a mask, pressed bits: 0x4000 cross, 0x8 start), `left`, `right` | the status after, `ran` |
| `wait` | `until` (`playing`, `level`, `scene`, `scene_end`), `level`, `max_ticks`, `skip` | the status, `reached`, `ran` |
| `shot` | `path` (a PNG to write) | `path`, once the window's next picture is handed to the writer (it is written beside the game) |
| `mobys` | `near`, `radius`, `class`, `limit` | `count`, `mobys` |
| `hero_place` | `position`, `yaw` (degrees), `ground` | asked, there after the next tick |
| `travel` | `level` | asked |
| `scene` | `number` | asked: it starts once the player has control |
| `camera` | `eye` and `look_at`, or `release` | |
| `line` | `from`, `to` | `blocked` |
| `ground` | `position`, `above`, `depth` | `found`, `position`, `normal`, `surface`, `moby` |
| `quit` | | the program ends |

It reads the game through the play host table (`game_host.h`) and asks only for what a player or
a mod could do (the pad, the hero placed, a level, a scene, the camera): the game's logic stays the
original's. The program without a window (`openrac-<game>-states`) takes every command but `shot`.

## Tests

`tools/test_game_mcp.py` checks the server against a stand-in for the port. With
`OPENRAC_GAME`, `OPENRAC_DATA` and `OPENRAC_LEVELS` set it also plays Kerwan in the real game:

```sh
OPENRAC_GAME=port/build/release/openrac-rac1-pal OPENRAC_DATA=<data> OPENRAC_LEVELS=<levels> \
  SDL_VIDEODRIVER=offscreen python3 -m unittest tools/test_game_mcp.py
```
