import { beforeEach, describe, expect, it } from "vitest";
import { checkDue, forgetModUpdates, lastChecked, markChecked, modUpdates } from "./modUpdates.svelte";

const DAY = 24 * 60 * 60 * 1000;

describe("the day between looks", () => {
  // The look remembers in the page's own storage, which node's test environment has none of.
  beforeEach(() => {
    const kept = new Map<string, string>();
    globalThis.localStorage = {
      getItem: (k: string) => kept.get(k) ?? null,
      setItem: (k: string, v: string) => void kept.set(k, v),
    } as Storage;
  });

  it("looks when a game was never looked at, and not again until a day has passed", () => {
    const now = Date.UTC(2026, 9, 29, 12);
    expect(lastChecked("rac1")).toBe(0);
    expect(checkDue("rac1", now)).toBe(true);
    markChecked("rac1", now);
    expect(lastChecked("rac1")).toBe(now);
    expect(checkDue("rac1", now + DAY)).toBe(false);
    expect(checkDue("rac1", now + DAY + 1)).toBe(true);
  });

  it("keeps a game's own answer: one looked at today does not hold another back", () => {
    const now = Date.UTC(2026, 9, 29, 12);
    markChecked("rac1", now);
    expect(checkDue("rac1", now)).toBe(false);
    expect(checkDue("rac2", now)).toBe(true);
  });
});

describe("what the look found", () => {
  it("forgets one game's updates and closes the prompt that showed them", () => {
    const offer = { game: "rac1", version: "pal", title: "Ratchet & Clank", updates: [] };
    modUpdates.found = { rac1: offer, rac2: { ...offer, game: "rac2" } };
    modUpdates.offer = offer;
    forgetModUpdates("rac1");
    expect(Object.keys(modUpdates.found)).toEqual(["rac2"]);
    expect(modUpdates.offer).toBeNull();
  });
});
