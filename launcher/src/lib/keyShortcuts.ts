// The game's keyboard shortcuts as Settings, Controls changes them (core/src/key_shortcuts.rs): a
// value is keys by comma, each with its Ctrl+, Shift+ and Alt+ first ("F11,Alt+Enter"), the key
// by where it is (event.code), as the game's controls file writes them. Pure, tested alone.

import { bindLabel, bindOfKey } from "./keybinds";

const MODIFIERS = ["Ctrl+", "Shift+", "Alt+"] as const;

/** A value's keys. */
export const shortcutKeys = (value: string): string[] =>
  value
    .split(",")
    .map((k) => k.trim())
    .filter(Boolean);

export const shortcutValue = (keys: string[]): string => keys.filter(Boolean).join(",");

/** One key as the player reads it: "Ctrl+F3", "Alt+Enter", "Shift+P". */
export function shortcutLabel(key: string): string {
  let rest = key;
  let mods = "";

  for (let found = true; found;) {
    found = false;

    for (const m of MODIFIERS)
      if (rest.startsWith(m)) {
        mods += m;
        rest = rest.slice(m.length);
        found = true;
      }
  }

  return mods + bindLabel(rest);
}

/** A key press as a shortcut's key; null for a modifier alone (it waits for the key) or a key
 *  the game does not know. */
export function shortcutOfKey(e: {
  code: string;
  ctrlKey: boolean;
  shiftKey: boolean;
  altKey: boolean;
}): string | null {
  if (/^(Control|Shift|Alt|Meta)(Left|Right)$/.test(e.code)) return null;
  const key = bindOfKey(e);

  if (!key) return null;

  return `${e.ctrlKey ? "Ctrl+" : ""}${e.shiftKey ? "Shift+" : ""}${e.altKey ? "Alt+" : ""}${key}`;
}

/** For each shortcut, the names of the others that have one of its keys. */
export function shortcutClashes(
  rows: { id: string; name: string; keys: string }[],
): Record<string, string[]> {
  const out: Record<string, string[]> = {};

  for (const row of rows) {
    const mine = shortcutKeys(row.keys);
    const others = rows
      .filter((o) => o.id !== row.id && shortcutKeys(o.keys).some((k) => mine.includes(k)))
      .map((o) => o.name);

    if (others.length) out[row.id] = others;
  }

  return out;
}
