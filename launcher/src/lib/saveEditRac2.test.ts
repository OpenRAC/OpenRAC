import { describe, expect, it } from "vitest";
import type { Rac2SaveFields, Rac2SaveView } from "./api";
import type { Rac2VersionInfo } from "./generated/Rac2VersionInfo";
import {
  changed,
  editList,
  fieldErrors,
  flagGroups,
  ownedCount,
  platinumCount,
  setOwned,
  setVersion,
  versionAllowed,
  versionHow,
  versionLabel,
  versionName,
} from "./saveEditRac2";

const MAX = 9_999_999;

/** A four-version weapon as the core describes it: V2 at `step1`, V3 bought, V4 at `step3`. */
function four(name: string, up: string, step1: number, step3: number, ammo: number[]): Rac2VersionInfo[] {
  const v = (label: string, how: Rac2VersionInfo["how"], k: number, min: number, max: number) => ({
    label,
    name: k === 0 ? name : up,
    how,
    stepXp: how === "experience" ? min : null,
    challengeOnly: k >= 2,
    maxAmmo: ammo[k] ?? null,
    minXp: min,
    maxXp: max,
  });
  return [
    v("V1", "first", 0, 0, step1 - 1),
    v("V2", "experience", 1, step1, MAX),
    v("V3", "bought", 2, 0, step3 - 1),
    v("V4", "experience", 3, step3, MAX),
  ];
}

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
        { id: 2, owned: false, ammo: null, xp: null, version: null },
        { id: 30, owned: true, ammo: 27, xp: 100, version: 0 },
        { id: 24, owned: false, ammo: 0, xp: 0, version: 0 },
        { id: 12, owned: true, ammo: 10, xp: null, version: 0 },
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
        versions: [],
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
        versions: four("Lancer", "Heavy Lancer", 8960, 256_000, [200, 300, 400, 500]),
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
        versions: four("Seeker Gun", "HK-22", 33_600, 339_200, [25, 20, 40, 50]),
        worn: false,
      },
      {
        id: 12,
        name: "Bomb Glove",
        kind: "weapon",
        maxAmmo: 40,
        upgradeName: null,
        upgradeXp: null,
        maxXp: null,
        versions: [
          {
            label: "V1",
            name: "Bomb Glove",
            how: "first",
            stepXp: null,
            challengeOnly: false,
            maxAmmo: 40,
            minXp: null,
            maxXp: null,
          },
          {
            label: "V2",
            name: "Bomb Glove",
            how: "bought",
            stepXp: null,
            challengeOnly: true,
            maxAmmo: 60,
            minXp: null,
            maxXp: null,
          },
        ],
        worn: false,
      },
    ],
    planets: [{ id: 1, name: "Oozla" }],
    maxBolts: 9_999_999,
    maxXp: 9_999_999,
    challengeMode: 0,
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
    expect(ownedCount(v, f, "weapon")).toEqual([3, 3]);
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

  it("names the version chosen", () => {
    const v = view();
    const f = structuredClone(v.fields);
    expect(versionName(v, f, 2)).toBeNull();
    expect(versionName(v, f, 24)).toBe("Seeker Gun");
    at(f, 24).version = 1;
    expect(versionName(v, f, 24)).toBe("HK-22");
    const [v1, , v3, v4] = v.items[2]?.versions ?? [];
    expect(v1 && versionLabel(v1)).toBe("V1");
    expect(v3 && versionLabel(v3)).toBe("V3 Mega");
    expect(v4 && versionHow(v4)).toBe("Upgrades itself at 339,200 experience, in challenge mode");
    expect(v3 && versionHow(v3)).toContain("challenge mode");
  });

  it("moves a weapon's version with the experience and ammo the game leaves", () => {
    const v = view();
    v.challengeMode = 1;
    const f = structuredClone(v.fields);
    const seeker = at(f, 24);
    seeker.owned = true;
    seeker.xp = 1000;
    // V1 to V2: experience raised to the step, full ammo of the new version.
    setVersion(v, f, 24, 1);
    expect(seeker).toMatchObject({ version: 1, xp: 33_600, ammo: 20 });
    // V2 to V3, the purchase: experience starts again at 0.
    setVersion(v, f, 24, 2);
    expect(seeker).toMatchObject({ version: 2, xp: 0, ammo: 40 });
    // V3 to V4 by experience.
    setVersion(v, f, 24, 3);
    expect(seeker).toMatchObject({ version: 3, xp: 339_200, ammo: 50 });
    expect(fieldErrors(v, f)).toEqual({});
    // Down again: what fits is kept, and V1 is back at the save's own experience.
    setVersion(v, f, 24, 2);
    expect(seeker).toMatchObject({ version: 2, xp: 339_199, ammo: 40 });
    setVersion(v, f, 24, 0);
    expect(seeker).toMatchObject({ version: 0, xp: 0, ammo: 25 });
    // Straight from V1 to V4: through the purchase, so at the V4 step.
    setVersion(v, f, 24, 3);
    expect(seeker).toMatchObject({ version: 3, xp: 339_200, ammo: 50 });
    expect(editList(v, f).map((e) => e.text)).toContain("Version: Seeker Gun V1 → HK-22 V4 Mega");
    expect(changed(v, f)).toBe(true);
  });

  it("offers the Mega versions only in challenge mode", () => {
    const v = view();
    const f = structuredClone(v.fields);
    const glove = v.items[3]?.versions ?? [];
    expect(glove.map((x) => versionAllowed(v, x))).toEqual([true, false]);
    at(f, 12).version = 1;
    expect(fieldErrors(v, f)["version-12"]).toContain("challenge mode");
    v.challengeMode = 2;
    expect(glove.map((x) => versionAllowed(v, x))).toEqual([true, true]);
    expect(fieldErrors(v, f)).toEqual({});
  });

  it("keeps experience in the chosen version's range and the held weapon's version", () => {
    const v = view();
    v.challengeMode = 1;
    const f = structuredClone(v.fields);
    const seeker = at(f, 24);
    seeker.owned = true;
    seeker.version = 1;
    seeker.ammo = 20;
    seeker.xp = 33_599;
    expect(fieldErrors(v, f)["xp-24"]).toBe("From 33,600 to 9,999,999");
    seeker.xp = 33_600;
    seeker.ammo = 21;
    expect(Object.keys(fieldErrors(v, f))).toEqual(["ammo-24"]);
    seeker.ammo = 20;
    seeker.owned = false;
    expect(fieldErrors(v, f)["version-24"]).toContain("Give");
    // The Lancer is in Ratchet's hand.
    at(f, 30).version = 1;
    at(f, 30).xp = 8960;
    expect(fieldErrors(v, f)["version-30"]).toContain("hand");
  });
});
