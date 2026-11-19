# Mods built on other mods: tables offered by name

A native mod can offer the other mods a table of its own, under a name: functions, data, or both.
A library mod holds what several mods share, and the mods built on it find its table and call it.
For example, a mod that keeps a list of new weapons could be the base for mods that each add one.
Native mods, interface 1.29.

## The calls

| Call | Since | What |
|---|---|---|
| `interface_provide(mod, name, version, table)` | 1.29 | Offers `table` under `name` at `version`. Only from `openrac_mod_load` |
| `interface_find(name, min_version)` | 1.29 | The table offered under `name`, when its version is `min_version` or more; null otherwise. At any time |

The C++ layer has them as `mod.interface_provide(name, version, &table)`,
`mod.interface_find<T>(name, min_version)` and `mod.has_interfaces()`.

## The library mod

The table is a struct the library mod publishes in a header of its own. Its version grows each
time a field is added at the end, so a mod built against an older header still finds what it
knows:

```cpp
// weapons-base/include/weapons_base.h, published with the library mod
struct WeaponsBase {
    int (*add_weapon)(const char* name);     // version 1
    int (*weapon_count)();                   // version 1
    void (*set_icon)(int weapon, int icon);  // version 2: added at the end
};
```

```cpp
static const WeaponsBase kTable{add_weapon, weapon_count, set_icon};

static void load(openrac::Mod& mod) {
    if (!mod.interface_provide("weapons-base", 2, &kTable)) {
        throw openrac::Error("weapons-base is offered by another mod, or OpenRAC is older than 1.29");
    }
}

OPENRAC_MOD_CPP("weapons-base", "1.1.0", load)
```

The table must stay where it is until the mod is unloaded: a `static` object, never a local one.
A name belongs to one mod: another mod offering it gets `OPENRAC_HOOK_TAKEN`. The same mod
offering it again replaces its own offer. A mod that fails to load takes its offers back.

## The mods built on it

A mod built on the library lists it in its manifest's `requires`. OpenRAC then refuses the mod
when the library is missing, off, too old, or after it in the list of mods. Mods unload in the
reverse of their load order, so the mod goes before the library, and the table outlives every
mod that uses it:

```json
"requires": [ { "id": "weapons-base", "version": ">=1.1.0" } ]
```

```cpp
static void load(openrac::Mod& mod) {
    const WeaponsBase* base = mod.interface_find<WeaponsBase>("weapons-base", 1);
    if (base == nullptr) {
        throw openrac::Error("needs weapons-base's interface 1 or later");
    }
    base->add_weapon("Lancer");
}

OPENRAC_MOD_CPP("lancer", "0.1.0", load)
```

The manifest's version (`>=1.1.0`) is the library mod's release. The table's version is the
interface the code needs. Both are checked: the first when the mod is chosen, the second when it
loads.

## Limits

- A mod that does not `require` the library can still call `interface_find`. But if it loads
  first, it finds nothing during its own load, and nothing guarantees that the table outlives it.
- What the table holds is the two mods' agreement. OpenRAC only keeps the pointer, the name and
  the version, and never reads the table.
- Script mods have no tables to offer or find.
