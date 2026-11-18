import { describe, expect, it } from "vitest";
import { dropIndex } from "./dragSort";
import { arrange, groupOf, matches, steps } from "./modsList";

describe("My mods' lists", () => {
  it("put each kind of mod in its group", () => {
    expect(groupOf("native")).toBe("gameplay");
    expect(groupOf("script")).toBe("gameplay");
    expect(groupOf("data")).toBe("gameplay");
    expect(groupOf("tool")).toBe("tools");
    expect(groupOf("built-in")).toBe("looks");
  });

  it("find a row by every word searched, in any case and order", () => {
    expect(matches("", "Lock-strafe")).toBe(true);
    expect(matches("strafe LOCK", "Lock-strafe", "Strafe as in the later games")).toBe(true);
    expect(matches("strafe camera", "Lock-strafe")).toBe(false);
    expect(matches("someone", "Bolt magnet", null, "someone")).toBe(true);
  });

  it("keep load order unless sorted by name", () => {
    const rows = ["Zed", "alpha", "Mid"];
    expect(arrange(rows, "", "order", (r) => [r])).toEqual(["Zed", "alpha", "Mid"]);
    expect(arrange(rows, "", "name", (r) => [r])).toEqual(["alpha", "Mid", "Zed"]);
    expect(arrange(rows, "i", "order", (r) => [r])).toEqual(["Mid"]);
    // The list given is never changed.
    expect(rows).toEqual(["Zed", "alpha", "Mid"]);
  });

  it("move a row one place at a time", () => {
    expect(steps(3, 1)).toEqual([true, true]);
    expect(steps(0, 2)).toEqual([false, false]);
    expect(steps(2, 2)).toEqual([]);
  });

  it("drop a dragged row where the pointer is", () => {
    const middles = [10, 30, 50, 70];
    expect(dropIndex(middles, 0, 5)).toBe(0);
    expect(dropIndex(middles, 0, 35)).toBe(1);
    expect(dropIndex(middles, 0, 99)).toBe(3);
    expect(dropIndex(middles, 3, 0)).toBe(0);
    expect(dropIndex(middles, 3, 40)).toBe(2);
    expect(dropIndex(middles, 2, 40)).toBe(2);
  });
});
