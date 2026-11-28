import { describe, expect, it } from "vitest";

import {
  shortcutClashes,
  shortcutKeys,
  shortcutLabel,
  shortcutOfKey,
  shortcutValue,
} from "./keyShortcuts";

const press = (
  code: string,
  mods: Partial<Record<"ctrlKey" | "shiftKey" | "altKey", boolean>> = {},
) => ({
  code,
  ctrlKey: false,
  shiftKey: false,
  altKey: false,
  ...mods,
});

describe("key shortcuts", () => {
  it("reads and writes a value as the game's controls file has it", () => {
    expect(shortcutKeys(" F11 , Alt+Enter ")).toEqual(["F11", "Alt+Enter"]);
    expect(shortcutValue(["Ctrl+F3", ""])).toBe("Ctrl+F3");
    expect(shortcutKeys("")).toEqual([]);
  });

  it("labels a key for the player", () => {
    expect(shortcutLabel("Ctrl+Shift+KeyP")).toBe("Ctrl+Shift+P");
    expect(shortcutLabel("F12")).toBe("F12");
  });

  it("takes a press with its modifiers, never a modifier alone", () => {
    expect(shortcutOfKey(press("F3", { ctrlKey: true }))).toBe("Ctrl+F3");
    expect(shortcutOfKey(press("ControlLeft", { ctrlKey: true }))).toBeNull();
    expect(shortcutOfKey(press("Unidentified"))).toBeNull();
  });

  it("names the shortcuts that share a key", () => {
    const clashes = shortcutClashes([
      { id: "a", name: "Screenshot", keys: "F12" },
      { id: "b", name: "Pause", keys: "F5,F12" },
      { id: "c", name: "Slower", keys: "F7" },
    ]);

    expect(clashes).toEqual({ a: ["Pause"], b: ["Screenshot"] });
  });
});
