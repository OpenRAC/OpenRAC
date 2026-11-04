import { describe, expect, it } from "vitest";
import type { Rac2SaveFields, Rac2SaveView } from "./api";
import {
  changed,
  editList,
  fieldErrors,
  flagGroups,
  ownedCount,
  platinumCount,
  setOwned,
  versionName,
} from "./saveEditRac2";

function view(): Rac2SaveView {
  return {
    serial: "SCUS_972.68",
    slot: 0,
    fingerprint: "x",
    planetName: "Oozla",
    savedAt: null,
    maxHealth: 4,
    fields: {
      bolts: 10,
      raritanium: 0,
      nanotechXp: 0,
      boosts: Array<boolean>(10).fill(false),
      armor: 0,
      shipPaint: 0,
      items: [
        { id: 2, owned: false, ammo: null, xp: null },
        { id: 30, owned: true, ammo: 27, xp: 100 },
        { id: 24, owned: false, ammo: 0, xp: 0 },
      ],
      planets: [{ id: 1, available: true, visited: true, platinum: [false, false, false, false] }],
      skillPoints: Array<boolean>(30).fill(false),
      flags: [{ id: "maktar-arena-entered", on: false }],
    },
    items: [
      {
        id: 2,
        name: "Heli-Pack",
        kind: "gadget",
        maxAmmo: null,
        upgradeName: null,
        upgradeXp: null,
        maxXp: null,
        version: null,
        worn: false,
      },
      {
        id: 30,
        name: "Lancer",
        kind: "weapon",
        maxAmmo: 200,
        upgradeName: "Heavy Lancer",
        upgradeXp: 8960,
        maxXp: 8959,
        version: 0,
        worn: true,
      },
      {
        id: 24,
        name: "Seeker Gun",
        kind: "weapon",
        maxAmmo: 25,
        upgradeName: "HK-22",
        upgradeXp: 33600,
        maxXp: 33599,
        version: 1,
        worn: false,
      },
    ],
    planets: [{ id: 1, name: "Oozla" }],
    maxBolts: 9_999_999,
    maxXp: 9_999_999,
    flags: [{ id: "maktar-arena-entered", level: 1, name: "Arena entered", description: "", confidence: "proven" }],
  };
}

const at = (f: Rac2SaveFields, id: number) => {
  const i = f.items.find((x) => x.id === id);
  if (!i) throw new Error(`no item ${String(id)}`);
  return i;
};

describe("the Going Commando save editor's page checks", () => {
  it("flags values past the limits, only where they were changed", () => {
    const v = view();
    const f = structuredClone(v.fields);
    expect(fieldErrors(v, f)).toEqual({});
    f.bolts = 10_000_000;
    at(f, 30).xp = 8960;
    at(f, 30).ammo = 201;
    expect(Object.keys(fieldErrors(v, f)).sort()).toEqual(["ammo-30", "bolts", "xp-30"]);
  });

  it("gives with full ammo and never takes what is in hand", () => {
    const v = view();
    const f = structuredClone(v.fields);
    setOwned(v, f, 24, true);
    expect(f.items[2]).toMatchObject({ owned: true, ammo: 25 });
    setOwned(v, f, 30, false);
    expect(at(f, 30).owned).toBe(true);
    expect(ownedCount(v, f, "weapon")).toEqual([2, 2]);
    expect(changed(v, f)).toBe(true);
  });

  it("lists the changes in a player's words", () => {
    const v = view();
    const f = structuredClone(v.fields);
    f.raritanium = 500;
    f.boosts[0] = true;
    at(f, 2).owned = true;
    f.planets.forEach((p) => (p.platinum[2] = true));
    f.skillPoints[3] = true;
    expect(platinumCount(f)).toBe(1);
    expect(editList(v, f).map((e) => e.text)).toEqual([
      "Raritanium 0 → 500",
      "Nanotech Boosts 0 → 1",
      "Given: Heli-Pack",
      "Platinum bolts: Oozla",
      "Skill points 0 → 1",
    ]);
    f.flags.forEach((x) => (x.on = true));
    expect(editList(v, f).at(-1)?.text).toBe("Oozla: Arena entered set");
    expect(flagGroups(v)).toEqual([{ planet: "Oozla", flags: v.flags }]);
  });

  it("names the version in the save", () => {
    const v = view();
    expect(versionName(v, 30)).toBeNull();
    expect(versionName(v, 24)).toBe("HK-22");
  });
});
