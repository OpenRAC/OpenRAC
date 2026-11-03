import { describe, expect, it } from "vitest";
import { extraChanged, extraErrors, modCost, modGroups } from "./saveEditRac2Extra";
import { mockRac2Extra } from "./mockRac2Extra";

describe("the Going Commando editor's advanced fields", () => {
  const view = mockRac2Extra("SCUS_972.68", 0);

  it("takes the save as read", () => {
    expect(extraErrors(view, structuredClone(view.fields))).toEqual({});
    expect(extraChanged(view, structuredClone(view.fields))).toBe(false);
  });

  it("keeps the counts in their limits", () => {
    const f = structuredClone(view.fields);
    f.crystals = 10_000;
    f.moonstones = -1;
    f.platinumSpent = 3;
    const e = extraErrors(view, f);
    expect(Object.keys(e).sort()).toEqual(["crystals", "moonstones", "platinumSpent"]);
    expect(e.platinumSpent).toContain("found");
    f.platinumSpent = 2;
    expect(extraErrors(view, f).platinumSpent).toBeUndefined();
  });

  it("groups the mods by weapon and prices the change", () => {
    const groups = modGroups(view);
    expect(groups.find((g) => g.item === 30)?.mods.map((k) => view.mods[k]?.name)).toEqual([
      "Shock Mod",
      "Lock On Mod",
    ]);
    const f = structuredClone(view.fields);
    for (const m of f.mods) {
      if (m.item === 30 && m.slot === 2) m.bought = true;
      if (m.item === 24 && m.slot === 0) m.bought = false;
    }
    expect(modCost(view, f)).toBe(3 - 1);
    expect(extraChanged(view, f)).toBe(true);
  });
});
