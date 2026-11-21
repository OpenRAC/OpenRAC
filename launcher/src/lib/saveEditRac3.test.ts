import type {Rac3SaveView} from "$lib/api";
import {changedRac3, fieldErrorsRac3, playTime} from "$lib/saveEditRac3";
import {describe, expect, it} from "vitest";

function weapon(f: Rac3SaveView["fields"], i: number) {
  const w = f.weapons[i];

  if (!w)
    throw new Error(`no weapon ${String(i)}`);

  return w;
}

function view(): Rac3SaveView {
  return {
    serial : "SCES_524.56",
    slot : 1,
    fingerprint : "f",
    planetName : "Veldin",
    savedAt : null,
    playSeconds : 3723,
    fields : {
      bolts : 12345,
      weapons : [
        {id : 39, owned : true, ammo : 20},
        {id : 47, owned : false, ammo : 0},
      ],
    },
    weapons : [
      {
        id : 39,
        name : "Shock Blaster",
        versionName : "Shock Blaster V2",
        version : 2,
        maxAmmo : 35,
        xp : 0,
      },
      {
        id : 47,
        name : "N60 Storm",
        versionName : "N60 Storm",
        version : 1,
        maxAmmo : 150,
        xp : 0
      },
    ],
    maxBolts : 9_999_999,
  };
}

describe("the Up Your Arsenal save editor", () => {
  it("sees a change to the bolts or an owned weapon's ammo", () => {
    const v = view();
    const f = structuredClone(v.fields);

    expect(changedRac3(v, f)).toBe(false);
    f.bolts = 1;
    expect(changedRac3(v, f)).toBe(true);
    f.bolts = v.fields.bolts;
    weapon(f, 0).ammo = 35;
    expect(changedRac3(v, f)).toBe(true);
    expect(fieldErrorsRac3(v, f)).toEqual({});
  });

  it("refuses what the core would", () => {
    const v = view();
    const f = structuredClone(v.fields);

    f.bolts = 10_000_000;
    weapon(f, 0).ammo = 36;
    weapon(f, 1).ammo = 5;
    expect(Object.keys(fieldErrorsRac3(v, f)).sort()).toEqual([
      "ammo-39", "ammo-47", "bolts"
    ]);
  });

  it("shows the play time as the Load page does", () => {
    expect(playTime(3723)).toBe("1:02");
    expect(playTime(59)).toBe("0:00");
  });
});
