# Adding a call to the mod API

This page shows how to add a new call to `openrac_api`, the table of functions
every mod gets. The design is in [MODS.md](MODS.md) (section 6 for the table,
section 8 for the SDK).

## The one rule

New calls go only at the end of `openrac_api`. In 1.x, never move, remove or
change an existing member. A mod built for 1.5 still reads its calls where 1.5
put them.

The files `sdk/api/openrac_api.lock`, `types.lock` and `macros.lock` record the
table. CI runs `sdk/tools/gen_api_lock.py --check --against origin/develop` and
fails if a change does more than add.

## A call, or a service?

A call is for what the port does. What one mod does for others (a mixer of
the mods' rumble, a shared movement state) is better a service of that mod
(`service_provide`, [sdk/docs/mods-together.md](../../sdk/docs/mods-together.md)):
it changes with the mod, not with the interface.

## Where the code goes

Each member has an **area**, written in `sdk/api/members.tsv`. The area decides
the files:

| Part | File |
|---|---|
| The member and its comment | `sdk/include/openrac/openrac_mod.h`, at the end of `openrac_api` |
| Its area, minor and status | `sdk/api/members.tsv`, last row |
| The port's code, `api_<name>` | `port/game/common/mod_api/<area>.cpp` |
| Its declaration | `port/game/common/mod_api/internal.h` |
| `g_api.<name> = api_<name>;` | `fill_api()` in `port/game/common/mods.cpp` |
| Game-specific code | a field of `openrac_game_play` (`port/game/common/include/openrac/game_host.h`), set in each game's `host/play.c` (rac3: `rac3-pal/host/play.c`) |
| The C++ wrapper | `sdk/include/openrac/mod.hpp`, in the area's class |
| The Luau name | `port/game/common/mod_scripts.cpp` |

The areas are: core, settings, hooks, input, graphics, menus, play, world, hero,
figures, saves, party, camera, visuals, sound, touch and system. If a call fits
none of them, it probably does too much. To add an area, add it to `AREAS` in
`sdk/tools/gen_api_lock.py` and add its file to `openrac_mods` in
`port/CMakeLists.txt`.

Hook points are not table members. They come from `sdk/bindings/<game>.json`,
and `sdk/tools/gen_binding.py` generates their names and types.

## Steps

```sh
# 1. Generate the standard parts.
python3 sdk/tools/new_api.py world moby_glow \
    --params "openrac_mod* mod, uint32_t moby, float strength" \
    --about "Makes a moby glow (0 to 1, 0 stops). Returns OPENRAC_OK, or OPENRAC_E_UNKNOWN if the moby is gone."
# 2. By hand: the body of api_moby_glow, the game part, the C++ wrapper, a test.
# 3. Write the changelog note, then set the version, the locks and the reference.
python3 sdk/tools/bump_api.py minor --note "a moby's glow (moby_glow)"
```

`new_api.py` writes the member, its `members.tsv` row, the declaration, a stub
and the `g_api` line. The stub returns `OPENRAC_E_UNKNOWN`, and `by-area.md`
lists the call as working on no game until you remove the stub's TODO line.
For several calls, run `new_api.py` once for each, then `bump_api.py minor`
once.

## Versions

You never choose the version. The interface's version lives in one place:
`OPENRAC_MOD_API_MAJOR` and `OPENRAC_MOD_API_MINOR` in `openrac_mod.h`. The
CMake package and the launcher read it from there, and nothing else holds a
copy.

SDK releases follow one rule: the patch is a dev release, the minor is a
stable one.

- A push to master (a dev release) publishes `sdk-v1.M.P`: the next patch of
  the last stable minor `M`, when the SDK archive changed.
- A stable launcher's tag (`launcher-v*`) publishes `sdk-v1.N.0` when the
  interface grew since the last stable release.

`bump_api.py` reads the last stable release (the highest `sdk-v1.M.0` tag of
OpenRAC/OpenRAC) and finds the open minor:

- `1.M+1` if your tree adds to the interface since that tag: a table member,
  or a new type, field or macro in `openrac_mod.h` or `keys.h`. Every pull
  request until the next stable release gets the same `1.M+1`, so they never
  renumber each other.
- `1.M` otherwise. Hook points (`games/*.h`), C++ helpers and docs are not a
  minor.

`bump_api.py` with no arguments sets the minor in the header, the new
members' `since` in `members.tsv`, their `Since 1.N.` in the header and the
`api_minor >= N` of the `mod.hpp` checks that read them. Then it regenerates
the locks and `sdk/docs/reference/`.

Each change of the interface needs a note: `sdk/changes/1.<minor>/<name>.md`,
one paragraph that names the new members. `bump_api.py minor --note "..."`
writes it (the name is your branch's last part, or `--name`). The changelog
is built from these notes, so two pull requests never edit the same text.

If a release run failed, run the `sdk` workflow on master with "release". A
tag `sdk-v<version>` pushed by hand still works.

If a stable release came out while your pull request was open, `bump_api.py --check`
fails after you merge develop in: your additions now belong to the next minor.
Run `bump_api.py`: it moves them, moves your note too, and prints the other
lines that still name the old minor (comments, the Luau types) for you to
check by hand.

A pull request that passed its checks just before a release and merged just
after it brings the old minor to develop. `dev_release` then stops at its
first step (`bump_api.py --check`): run `bump_api.py` on develop in a small
pull request, then start the release again.

Two pull requests that both add members still meet at the end of the table.
Keep both, develop's first. In `sdk/api/*.lock` and `sdk/docs/reference/`
take develop's side, then run `bump_api.py`: it writes them again.

## Naming

- `<thing>` reads, `<thing>_set` writes (`camera_set`). An action is a verb
  (`moby_spawn`).
- A callback is `on_<thing>[_<event>]`. Mods register it only in
  `openrac_mod_load`.
- The first parameter is `openrac_mod* mod`. Output goes last, in `out`.
- A call that can fail returns `int`: `OPENRAC_OK` or an `OPENRAC_E_*` code.
- A struct that may grow starts with `uint32_t size`, set by the mod.
- The C++ method and the Luau function use the member's name.
- Write pointers as `type* name`.

## Status

A new member starts as `experimental` (use `new_api.py --stable` if it copies
an existing pattern). Its behaviour may still change in a minor, with a note
in `sdk/changes/`. Its signature may not change. After a release where
mods used it, it becomes `stable`. A replaced member keeps working, and its row
says `deprecated:<new member>`.

## Tests

- Port code: the area's test if it has one (`mod_touch_test`,
  `mod_ui_menus_test`, `mod_visuals_test`, `mod_material_test`), otherwise
  `port/tests/mod_loader_test.cpp`. Its `every_call_of_the_table_is_filled`
  test fails if a member has no `g_api` line.
- C++ wrapper: `sdk/tests/mod_hpp_test.cpp`. Also test `has_<name>()` with a
  shorter table, as an older port gives.
- Luau name: `port/tests/mod_scripts_test.cpp`.
- SDK checks (run by CI in `sdk.yml`): `python3 -m unittest discover -s
  sdk/tools`, `gen_api_lock.py --check`, `gen_reference.py --check` and
  `bump_api.py --check`.

```sh
(cd port && cmake --build --preset release --target mod_loader_test && ctest --preset release -R "^mod_")
cmake -S sdk/tests -B build/sdk-tests && cmake --build build/sdk-tests && ctest --test-dir build/sdk-tests
```
