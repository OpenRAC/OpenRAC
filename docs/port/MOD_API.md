# Adding to the mods' interface

How a new call joins `openrac_api` (the table every mod gets), where each part
of it goes, and the rules that keep every mod built before it working. The
design is in [MODS.md](MODS.md) (section 6 for the table, section 8 for the
SDK); this page is the how-to for OpenRAC's own code.

## The one rule

`openrac_api` only grows at its end. A member is never moved, removed or given
another signature in 1.x: a mod built against 1.5 reads its calls at the
offsets 1.5 had. `sdk/api/openrac_api.lock`, `types.lock` and `macros.lock`
hold what the table and its types are, and CI
(`sdk/tools/gen_api_lock.py --check --against origin/develop`) fails a change
that does more than add.

## Where new code goes

Every member has an **area** (`sdk/api/members.tsv`). The area decides each
file:

| Part | File |
|---|---|
| The member and its comment (the reference) | `sdk/include/openrac/openrac_mod.h`, last in `openrac_api` |
| Its area, minor and status | `sdk/api/members.tsv`, last row |
| The port's side, `api_<name>` | `port/game/common/mod_api/<area>.cpp` |
| Its declaration, and what two areas share | `port/game/common/mod_api/internal.h` |
| `g_api.<name> = api_<name>;` | `port/game/common/mods.cpp`, `fill_api()` |
| A game's part (reads or writes the game) | a field of `openrac_game_play` (`port/game/common/include/openrac/game_host.h`), set in each game's `port/game/<game>/host/play.c` (rac3: `rac3-pal/host/play.c`) for every disc |
| The C++ wrapper | `sdk/include/openrac/mod.hpp`, the area's class |
| The Luau name, for a script mod | `port/game/common/mod_scripts.cpp` |
| Tests | below |

The areas are core, settings, hooks, input, graphics, menus, play, world,
hero, figures, saves, party, camera, visuals, sound, touch and system. A call
that fits none is a sign it should be split; a new area is added to `AREAS` in
`sdk/tools/gen_api_lock.py`, with its file in `mod_api/` listed in `port/CMakeLists.txt`
(`openrac_mods`).

Hook points are not table members: they come from `sdk/bindings/<game>.json`
and `sdk/tools/gen_binding.py`, which writes their C names (the generated part
of `games/<game>.h`), the C++ and Luau types, and `ids.lock`.

## Steps

```sh
# 1. the parts that always look the same
python3 sdk/tools/new_api.py world moby_glow \
    --params "openrac_mod* mod, uint32_t moby, float strength" \
    --about "Makes a moby glow with strength 0 to 1, or stop with 0. Returns OPENRAC_OK, or OPENRAC_E_UNKNOWN for a moby that is gone."
# 2. by hand: the body of api_moby_glow, the game's part, the C++ wrapper, a test
# 3. the version, the locks and the reference
python3 sdk/tools/bump_api.py minor --note "a moby's glow (moby_glow)"
```

`new_api.py` writes the member (with "Since 1.<next>"), its `members.tsv` row,
the declaration, a stub that returns `OPENRAC_E_UNKNOWN` and the assignment.
Until the stub's TODO line is gone, `by-area.md` lists the call as working on
no game. Several calls for one minor are added one after another, then one
`bump_api.py minor` with a note naming them all.

`bump_api.py` writes the six places that hold the version (the header's minor
and its history, `sdk/VERSION`, the CMake version file, the launcher's
`API_MINOR`, both templates' `sdk.lock`) and regenerates the locks and
`sdk/docs/reference/`. Anything else a mod sees, such as new hook points or a
new C++ helper, is `bump_api.py patch`.

## Naming

- `<thing>` reads, `<thing>_set` writes (`camera_set`, `haze_set`); an action
  is a verb (`moby_spawn`, `prop_remove`).
- A callback is `on_<thing>[_<event>]` and is registered only in
  `openrac_mod_load`.
- The first parameter is `openrac_mod* mod`; output goes in `out`, last.
- A call that can fail returns `int`: `OPENRAC_OK` or an `OPENRAC_E_*` code
  (the same values as the older `OPENRAC_HOOK_*` names, which stay).
- A struct that may grow starts with `uint32_t size`, set by the mod, so it
  never needs a `2` version.
- The C++ method and the Luau function have the member's name.
- Pointers are written `type* name`.

## Status

A new member is `experimental` in `members.tsv` (`new_api.py --stable` when it
is a plain copy of a pattern already in use). It may still change in a minor,
with a note in the header's history. After a release in which mods used it, it
turns `stable`: kept and working through 1.x. A member replaced by a better
one stays and keeps working; its row says `deprecated:<new member>`.

## Tests

- The port's side: the area's test if it has one (`mod_touch_test`,
  `mod_ui_menus_test`, `mod_visuals_test`, `mod_material_test`), otherwise
  `port/tests/mod_loader_test.cpp`. `every_call_of_the_table_is_filled` there
  fails when a member has no `g_api` assignment.
- The C++ wrapper: `sdk/tests/mod_hpp_test.cpp`, with a table as an older port
  gives it (shorter `size`), so the wrapper's `has_<name>()` is checked too.
- A script name: `port/tests/mod_scripts_test.cpp`.
- The SDK's own checks (CI, `sdk.yml`): `python3 -m unittest discover -s
  sdk/tools`, `gen_api_lock.py --check`, `gen_reference.py --check`,
  `gen_binding.py --check`, `bump_api.py --check`.

```sh
cmake --build --preset release --target mod_loader_test && ctest --preset release -R "^mod_"
cmake -S sdk/tests -B build/sdk-tests && cmake --build build/sdk-tests && ctest --test-dir build/sdk-tests
```
