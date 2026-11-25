# Mods that work together

A mod can see which other mods are loaded, and a mod can offer the others a
**service**: a table of its own functions they call. A *base mod* can so own
something several mods want to change (the pads' rumble, the hero's movement,
the camera), and the others ask it instead of each hooking the same thing.

## Is a mod loaded?

| | |
|---|---|
| C++ | `mod.find("rumble")`: a `std::optional<openrac::ModEntry>` (`id`, `name`, `version`, `index`); `mod.loaded("rumble")` |
| C (1.14) | `api->mod_count()` and `api->mod_entry(index, &entry)`: the loaded mods in load order |
| Luau (1.40) | `openrac.mod_loaded("rumble")`: its version, or `nil` |

A mod is loaded when it is installed, switched on, made for this game and its
load succeeded.

**When to ask.** Mods load one by one, in the launcher's order. During your
load, only the mods loaded before yours are listed. Ask later:

- in `on_mods_loaded` (C and C++, 1.40): called once, after every mod's load
  and before the game's first tick;
- or in your first `on_frame` (script mods too).

Or fix the order in your [manifest](manifest.md): `"requires"` loads the other
mod first, and the launcher installs it with yours; `"load": {"after": [...]}`
asks for the order without needing the mod.

## Services (native mods, 1.40)

A service is a plain C struct of function pointers whose first member is its
`uint32_t size`. The provider ships a header with it for the mods that use it.
For example, a rumble mod that mixes every mod's vibration:

```c
/* rumble_effects.h, from the rumble mod. */
typedef struct rumble_effects {
    uint32_t size; /* sizeof(rumble_effects): the table grows at its end */
    /* The caller's levels for this tick: send them every tick while it rumbles. */
    int (*hold)(openrac_mod* caller, int port, int small_motor, int large_motor);
    /* A burst that ends by itself after `seconds`. */
    int (*burst)(openrac_mod* caller, int port, int small_motor, int large_motor, float seconds);
} rumble_effects;
```

The provider offers it during its load. The name starts with its own id and a
slash. The table must stay where it is until the game ends: make it `static`.

```cpp
static const rumble_effects effects = {sizeof effects, hold, burst};
mod.provide("rumble/effects", 1, effects);
```

A mod that uses it asks once every mod has loaded, and keeps the pointer:

```cpp
static const rumble_effects* rumble = nullptr;

if (mod.has_services()) {
    mod.on_mods_loaded([&mod] { rumble = mod.service<rumble_effects>("rumble/effects", 1); });
}
mod.on_frame([&mod] {
    if (rumble != nullptr) {
        rumble->hold(mod.handle(), 0, 0, 120); /* through the rumble mod's mixer and setting */
    } else {
        mod.set_motors(0, 0, 120); /* no rumble mod: straight to the pad */
    }
});
```

In C: `api->service_provide(mod, name, version, &table, sizeof table)` and
`api->service_find(mod, name, min_version, &found)` with an `openrac_service`
whose `size` you set; `found.table` is the table.

Rules:

- **Versions.** `version` starts at 1 and grows when the table gets new members
  at its end. A mod asks for the lowest version it needs; a newer table works,
  and its `size` says which members it has. A change that breaks callers takes a
  new name (`"rumble/effects2"`).
- **One name, one table.** The same name offered twice is refused; the first
  stays. A mod offers names under its own id only.
- **Who calls.** The port does not tell a provider which mod calls it. Pass
  your `openrac_mod*` handle (`mod.handle()`) where the provider asks for a
  caller, as `hold` does: it is unique to your mod and lasts until the game
  ends.
- **Lifetime.** Mods load once, when the game starts, and unload when it ends;
  a service found stays valid until then. A provider whose load fails takes its
  services with it before anyone can find them. Do not call another mod's
  service from your `openrac_mod_unload`.
- **Threads.** Call a service where the provider says, as a rule on the game's
  thread (load, `on_frame`, a hook).

Script mods can see other mods (`openrac.mod_loaded`) but cannot offer or call
services.
