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
# 3. Update the version, the locks and the reference.
python3 sdk/tools/bump_api.py minor --note "a moby's glow (moby_glow)"
```

`new_api.py` writes the member, its `members.tsv` row, the declaration, a stub
and the `g_api` line. The stub returns `OPENRAC_E_UNKNOWN`, and `by-area.md`
lists the call as working on no game until you remove the stub's TODO line.
For several calls in one minor, run `new_api.py` once for each, then run
`bump_api.py minor` once.

`bump_api.py` updates the version in all six places and regenerates the locks
and `sdk/docs/reference/`. For other changes that mods see, such as new hook
points or a new C++ helper, use `bump_api.py patch`.

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
in the header's history. Its signature may not change. After a release where
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
