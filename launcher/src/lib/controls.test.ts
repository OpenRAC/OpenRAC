import { describe, expect, it } from "vitest";
import bindingsCpp from "../../../port/platform/bindings.cpp?raw";
import bindingsH from "../../../port/platform/bindings.h?raw";
import type { BindingRow } from "./generated/BindingRow";
import {
  alsoPressing,
  bindingValue,
  heldInputs,
  layerLabel,
  MOST_OF_A_KIND,
  newlyPressed,
  PAD_NONE,
  padLabel,
  padWith,
  playerKey,
  withPad,
  type PadReading,
} from "./controls";

const row = (key: string, label: string, pad: string[], keys: string[] = []): BindingRow => ({
  key,
  label,
  button: true,
  hint: null,
  keys,
  pad,
  keysFrom: "default",
  padFrom: "default",
  global: null,
  game: null,
  shared: [],
});

const standard = (
  pressed: number[] = [],
  axes: number[] = [0, 0, 0, 0],
  triggers: [number, number] = [0, 0],
): PadReading => ({
  mapping: "standard",
  buttons: Array.from({ length: 17 }, (_, i) => {
    const value = i === 6 ? triggers[0] : i === 7 ? triggers[1] : pressed.includes(i) ? 1 : 0;
    return { pressed: value > 0, value };
  }),
  axes,
});

describe("the port's names", () => {
  it("label every pad input the port knows", () => {
    const names = [...bindingsCpp.matchAll(/\{"(Pad[A-Za-z0-9]+)", (?:SDL_|kNoButton)/g)].map((m) => m[1] ?? "");
    expect(names.length).toBe(36);
    for (const name of names) expect(padLabel(name)).not.toBe(name);
  });

  it("hold four of a kind", () => {
    expect(bindingsH).toContain(`kMostOfAKind = ${MOST_OF_A_KIND};`);
    expect(bindingsH).toContain(`kPadNone = "${PAD_NONE}"`);
  });

  it("read chords and layers", () => {
    expect(padLabel("PadLeftShoulder+PadSouth")).toBe("Left shoulder + South (A, Cross)");
    expect(padLabel("Unknown")).toBe("Unknown");
    expect(layerLabel("game")).toBe("This game");
    expect(layerLabel("gamePlayer")).toBe("This game, this player");
    expect(playerKey("bind_cross", 1)).toBe("p2.bind_cross");
    expect(playerKey("bind_cross", null)).toBe("bind_cross");
  });
});

describe("a browser controller's press", () => {
  it("is named by position, as SDL names it", () => {
    expect(heldInputs(standard([0, 9, 16]))).toEqual(["PadSouth", "PadStart", "PadGuide"]);
    expect(heldInputs(standard([], [0, -0.9, 0.8, 0]))).toEqual(["PadLeftStickUp", "PadRightStickRight"]);
    expect(heldInputs(standard([], undefined, [0.2, 0.9]))).toEqual(["PadRightTrigger"]);
    expect(heldInputs({ ...standard([0]), mapping: "" })).toEqual([]);
    expect(heldInputs(null)).toEqual([]);
  });

  it("counts only when newly pressed", () => {
    expect(newlyPressed(["PadSouth"], ["PadSouth"])).toBeNull();
    expect(newlyPressed(["PadSouth"], ["PadSouth", "PadEast"])).toBe("PadEast");
    expect(newlyPressed([], [])).toBeNull();
  });
});

describe("a control's new pad inputs", () => {
  it("keep the keys the file names, and leave the rest to the file below", () => {
    expect(withPad(null, ["PadEast"])).toBe("PadEast");
    expect(withPad("Space,PadSouth", ["PadEast", "PadWest"])).toBe("Space,PadEast,PadWest");
    expect(withPad("PadSouth", [])).toBe(PAD_NONE);
    expect(withPad("Space", [])).toBe("Space,PadNone");
    expect(withPad("none", [])).toBe("none");
    expect(withPad(null, ["PadEast", "PadEast", "PadWest", "PadNorth", "PadBack", "PadStart"])).toBe(
      "PadEast,PadWest,PadNorth,PadBack",
    );
  });

  it("are written as the port writes them", () => {
    expect(bindingValue([], [])).toBe("none");
    expect(bindingValue(["Space"], [])).toBe("Space,PadNone");
    expect(bindingValue(["Space"], ["PadSouth"])).toBe("Space,PadSouth");
  });

  it("add, replace, and say what else a press presses", () => {
    const cross = row("bind_cross", "Cross", ["PadSouth"]);
    expect(padWith(cross, "PadEast", false)).toEqual(["PadSouth", "PadEast"]);
    expect(padWith(cross, "PadSouth", false)).toEqual(["PadSouth"]);
    expect(padWith(cross, "PadEast", true)).toEqual(["PadEast"]);
    const rows = [cross, row("bind_circle", "Circle", ["PadEast"]), row("bind_select", "Select", ["PadEast+PadSouth"])];
    expect(alsoPressing(rows, "bind_cross", "PadEast")).toEqual(["Circle"]);
    expect(alsoPressing(rows, "bind_cross", "PadEast+PadSouth")).toEqual([]);
  });
});
