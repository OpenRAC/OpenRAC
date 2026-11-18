import { describe, expect, it } from "vitest";
import type { ModSaveData, ModSaveField, Rac2SaveFields, Rac2SaveView } from "./api";
import type { Rac2VersionInfo } from "./generated/Rac2VersionInfo";
import {
  activeTiers,
  attachments,
  isAttached,
  pickVersion,
  pickedValue,
  tierOptions,
  type ModValues,
} from "./modSaveAttach";

const ver = (label: string): Rac2VersionInfo => ({
  label,
  name: "Lancer",
  how: "first",
  stepXp: null,
  challengeOnly: false,
  maxAmmo: 200,
  minXp: 0,
  maxXp: 100,
});

function view(): Rac2SaveView {
  const fields: Rac2SaveFields = {
    bolts: 0,
    raritanium: 0,
    nanotechXp: 0,
    boosts: [],
    armor: 0,
    shipPaint: 0,
    items: [
      { id: 30, owned: true, ammo: 100, xp: 10, version: 1 },
      { id: 12, owned: true, ammo: 5, xp: null, version: 0 },
    ],
    planets: [],
    skillPoints: [],
    flags: [],
  };
  const info = (id: number, n: number) => ({
    id,
    name: id === 30 ? "Lancer" : "Bomb Glove",
    kind: "weapon" as const,
    maxAmmo: 200,
    upgradeName: null,
    upgradeXp: null,
    maxXp: 100,
    versions: ["V1", "V2", "V3", "V4"].slice(0, n).map(ver),
    worn: false,
  });
  return {
    serial: "SCUS_972.68",
    slot: 0,
    fingerprint: "x",
    planetName: null,
    savedAt: null,
    maxHealth: 4,
    fields: structuredClone(fields),
    items: [info(30, 4), info(12, 2)],
    planets: [],
    maxBolts: 0,
    maxXp: 0,
    challengeMode: 0,
    flags: [],
  };
}

const tiers = ["None", "Alpha", "Beta"];
function field(key: string, id: number, as: string, requires = true): ModSaveField {
  return {
    key,
    name: key,
    offset: id,
    mask: 127,
    min: 0,
    max: 2,
    choices: tiers,
    value: 0,
    attach: { to: "item", id, as },
    ...(requires ? { requires: { version: "last" } } : {}),
  };
}
const mod = (id: string, fields: ModSaveField[]): ModSaveData => ({ id, name: id, title: id, found: true, fields });

describe("mod fields attached to items", () => {
  const ids = new Set([30, 12]);

  it("attaches only to known items and ways", () => {
    expect(isAttached(field("a", 30, "version"), ids)).toBe(true);
    expect(isAttached(field("a", 99, "version"), ids)).toBe(false);
    expect(isAttached(field("a", 30, "side"), ids)).toBe(false);
    expect(isAttached({ ...field("a", 30, "row"), attach: { to: "planet", id: 30, as: "row" } }, ids)).toBe(false);
  });

  it("stacks several mods on one item", () => {
    const a = attachments(
      [mod("ng", [field("l", 30, "version")]), mod("x", [field("l", 30, "version"), field("r", 30, "row", false)])],
      ids,
    );
    expect(a.get(30)?.version.map((x) => x.mod.id)).toEqual(["ng", "x"]);
    expect(a.get(30)?.row).toHaveLength(1);
    expect(tierOptions(a.get(30)?.version ?? []).map((o) => o.kind === "mod" && o.name)).toEqual([
      "Alpha",
      "Beta",
      "Alpha",
      "Beta",
    ]);
  });

  it("a tier moves the weapon to its last version; a game version clears the tier", () => {
    const v = view();
    const f = structuredClone(v.fields);
    const values: ModValues = {};
    const at = attachments([mod("ng", [field("l", 30, "version"), field("b", 12, "version")])], ids);
    const lancer = at.get(30)?.version ?? [];
    pickVersion(v, f, values, 30, lancer, tierOptions(lancer)[0]?.value ?? "");
    expect(values.ng?.l).toBe(1);
    expect(f.items[0]?.version).toBe(3);
    expect(pickedValue(values, lancer, 3)).toBe("m:ng:l:1");
    pickVersion(v, f, values, 30, lancer, "g:1");
    expect(values.ng?.l).toBe(0);
    expect(f.items[0]?.version).toBe(1);
    expect(pickedValue(values, lancer, 1)).toBe("g:1");

    // Two versions: the last is V2.
    const glove = at.get(12)?.version ?? [];
    pickVersion(v, f, values, 12, glove, tierOptions(glove)[1]?.value ?? "");
    expect(f.items[1]?.version).toBe(1);
    expect(values.ng?.b).toBe(2);
  });

  it("a tier without requires leaves the game version", () => {
    const v = view();
    const f = structuredClone(v.fields);
    const values: ModValues = {};
    const list = attachments([mod("m", [field("l", 30, "version", false)])], ids).get(30)?.version ?? [];
    pickVersion(v, f, values, 30, list, tierOptions(list)[0]?.value ?? "");
    expect(f.items[0]?.version).toBe(1);
  });

  it("shows both mods' tiers when they disagree", () => {
    const list =
      attachments([mod("a", [field("l", 30, "version")]), mod("b", [field("l", 30, "version")])], ids).get(30)
        ?.version ?? [];
    expect(activeTiers({ a: { l: 1 }, b: { l: 2 } }, list)).toHaveLength(2);
    // Picking one through the picker puts the other back.
    const v = view();
    const values: ModValues = { a: { l: 1 }, b: { l: 2 } };
    pickVersion(v, structuredClone(v.fields), values, 30, list, "m:b:l:1");
    expect(values).toEqual({ a: { l: 0 }, b: { l: 1 } });
  });
});
