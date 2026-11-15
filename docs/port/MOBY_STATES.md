# Moby start states (`--moby-states`)

OpenRAC Editor shows each moby as the game starts it: spawned or not, hidden, its state and
place. It asks the game itself, with a run of the port's states program, so the editor never
copies game logic. The command line and the file below are a contract between the two
repositories: change either only together with the editor's `openrac/tools/moby_states.py`, and
raise `format` when the file changes.

## The run

```sh
openrac-rac1-<version>-states --data DIR --levels DIR --level N \
    --moby-states OUT.json --moby-states-ticks TICKS (--mods DIR | --no-mods)
```

The program loads level `N`, runs its mobys' updates for `TICKS` ticks (at least 1; the editor
asks for 300, five seconds) as fast as the machine allows, writes `OUT.json` and stops. It exits
with status 1 when it cannot write the file. `--levels` may name a map's packed levels, so a
packed level is worked out as it plays.

The program is built with the port (`port/cmake/Games.cmake`, one per version; the launcher passes
its path to the editor's pack as `--states`). Code: `port/game/common/program/moby_states.{h,cpp}`.

## The file

```json
{"format": 1, "game": "rac1", "level": 3, "ticks": 300, "mobys": [
  {"key": 12, "class": 500, "state": 0, "hidden": 0, "dead": 0, "pos": [10.5, 2.0, 30.25]}
]}
```

- `key`: the moby's record in the level's moby table (its place, not its spawn id); only the
  level's own placed mobys are listed.
- `class`: the moby's class number. `state`: its state byte.
- `hidden`, `dead`: 1 when the game hides it or it is dead at the end of the run.
- `pos`: its position in game units (z up); a non-finite coordinate is written as 0.
