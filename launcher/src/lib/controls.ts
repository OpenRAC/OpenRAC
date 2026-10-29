// What a controls page needs beside the commands (core/src/controls.rs): the pad inputs' names as
// a player reads them, a browser controller's press as the port names it, and a control's new pad
// inputs written so that its keys stay as the file had them. Pure functions, tested without a page.
//
// No page shows the controls yet: these are for the one that will (the game's own window does it
// today, port/game/common/debug/controls_window.cpp).

import type { BindingRow } from "./generated/BindingRow";
import type { ControlsLayer } from "./generated/ControlsLayer";

/** port/platform/bindings.cpp's values that bind nothing, and no pad input. */
export const UNBOUND = "none";
export const PAD_NONE = "PadNone";

/** The most of a kind one control holds (the port's kMostOfAKind). */
export const MOST_OF_A_KIND = 4;

/** The pad inputs as a player knows them: by position, with what the common pads print. */
const PAD_LABELS: Record<string, string> = {
  PadSouth: "South (A, Cross)",
  PadEast: "East (B, Circle)",
  PadWest: "West (X, Square)",
  PadNorth: "North (Y, Triangle)",
  PadBack: "Back / Select / Share",
  PadGuide: "Guide / Home",
  PadStart: "Start / Options",
  PadLeftStick: "Left stick click",
  PadRightStick: "Right stick click",
  PadLeftShoulder: "Left shoulder",
  PadRightShoulder: "Right shoulder",
  PadDpadUp: "D-pad up",
  PadDpadDown: "D-pad down",
  PadDpadLeft: "D-pad left",
  PadDpadRight: "D-pad right",
  PadMisc1: "Misc (mic, capture, share)",
  PadRightPaddle1: "Right paddle 1",
  PadLeftPaddle1: "Left paddle 1",
  PadRightPaddle2: "Right paddle 2",
  PadLeftPaddle2: "Left paddle 2",
  PadTouchpad: "Touchpad click",
  PadMisc2: "Misc 2",
  PadMisc3: "Misc 3",
  PadMisc4: "Misc 4",
  PadMisc5: "Misc 5",
  PadMisc6: "Misc 6",
  PadLeftTrigger: "Left trigger",
  PadRightTrigger: "Right trigger",
  PadLeftStickUp: "Left stick up",
  PadLeftStickDown: "Left stick down",
  PadLeftStickLeft: "Left stick left",
  PadLeftStickRight: "Left stick right",
  PadRightStickUp: "Right stick up",
  PadRightStickDown: "Right stick down",
  PadRightStickLeft: "Right stick left",
  PadRightStickRight: "Right stick right",
};

/** A pad input's or a chord's name as a player reads it ("Left shoulder + South (A, Cross)"). */
export function padLabel(name: string): string {
  return name
    .split("+")
    .map((n) => PAD_LABELS[n.trim()] ?? n.trim())
    .join(" + ");
}

/** Where a value came from, in words. */
export function layerLabel(layer: ControlsLayer): string {
  switch (layer) {
    case "default":
      return "Default";
    case "global":
      return "All games";
    case "globalPlayer":
      return "All games, this player";
    case "game":
      return "This game";
    case "gamePlayer":
      return "This game, this player";
  }
}

/** A key as a page edits it: one player's own (`p2.bind_cross`) while a player is chosen. */
export function playerKey(key: string, player: number | null): string {
  return player === null ? key : `p${player + 1}.${key}`;
}

/** The browser's standard mapping's buttons (w3c.github.io/gamepad) by the port's names: both are
 *  by position, so the bottom face button is PadSouth whatever is printed on it. */
const STANDARD_BUTTONS: readonly string[] = [
  "PadSouth",
  "PadEast",
  "PadWest",
  "PadNorth",
  "PadLeftShoulder",
  "PadRightShoulder",
  "PadLeftTrigger",
  "PadRightTrigger",
  "PadBack",
  "PadStart",
  "PadLeftStick",
  "PadRightStick",
  "PadDpadUp",
  "PadDpadDown",
  "PadDpadLeft",
  "PadDpadRight",
  "PadGuide",
];

/** The standard mapping's axes, each way, by the port's names. */
const STANDARD_AXES: readonly [string, string][] = [
  ["PadLeftStickLeft", "PadLeftStickRight"],
  ["PadLeftStickUp", "PadLeftStickDown"],
  ["PadRightStickLeft", "PadRightStickRight"],
  ["PadRightStickUp", "PadRightStickDown"],
];

/** A controller as the browser shows it, what of it a capture reads. */
export interface PadReading {
  mapping: string;
  buttons: readonly { pressed: boolean; value: number }[];
  axes: readonly number[];
}

/** How far a stick goes before a capture counts it, and a trigger. */
const STICK_FAR = 0.75;
const TRIGGER_PRESS = 0.5;

/** The pad inputs `pad` holds, by the port's names; none for a controller the browser does not
 *  map as standard (the game's own window captures every controller). */
export function heldInputs(pad: PadReading | null): string[] {
  if (pad?.mapping !== "standard") return [];
  const out: string[] = [];
  STANDARD_BUTTONS.forEach((name, i) => {
    const b = pad.buttons[i];
    if (!b) return;
    const trigger = name === "PadLeftTrigger" || name === "PadRightTrigger";
    if (trigger ? b.value >= TRIGGER_PRESS : b.pressed) out.push(name);
  });
  STANDARD_AXES.forEach(([negative, positive], i) => {
    const v = pad.axes[i] ?? 0;
    if (v <= -STICK_FAR) out.push(negative);
    if (v >= STICK_FAR) out.push(positive);
  });
  return out;
}

/** The first pad input pressed in `now` that `before` did not hold: what a capture binds. */
export function newlyPressed(before: readonly string[], now: readonly string[]): string | null {
  return now.find((n) => !before.includes(n)) ?? null;
}

/** A control's value as the port writes it: keys, then pad inputs; PadNone for keys without a pad
 *  input, "none" for nothing. */
export function bindingValue(keys: readonly string[], pad: readonly string[]): string {
  if (keys.length === 0 && pad.length === 0) return UNBOUND;
  const names = [...keys, ...pad];
  if (pad.length === 0) names.push(PAD_NONE);
  return names.join(",");
}

/** The keys a file's value names for a control, or null when it names none (they come from below). */
function namedKeys(value: string | null): string[] | null {
  if (value === null) return null;
  if (value.trim() === UNBOUND) return [];
  const keys = value
    .split(",")
    .map((n) => n.trim())
    .filter((n) => n && n !== PAD_NONE && !n.startsWith("Pad") && !n.includes("+"));
  return keys.length > 0 ? keys : null;
}

/**
 * What to write for `row` in a file whose value for it is `fileValue` (the row's `global` or
 * `game`) so that its pad inputs become `pad`, at most four: the keys that file names stay, and
 * keys it does not name go on coming from below it.
 */
export function withPad(fileValue: string | null, pad: readonly string[]): string {
  const keys = namedKeys(fileValue);
  const kept = [...new Set(pad)].slice(0, MOST_OF_A_KIND);
  if (keys === null) return kept.length > 0 ? kept.join(",") : PAD_NONE;
  return bindingValue(keys, kept);
}

/** The pad inputs `row` has now with `input` added (once, and not past four), or replaced by it. */
export function padWith(row: BindingRow, input: string, replace: boolean): string[] {
  if (replace) return [input];
  if (row.pad.includes(input)) return [...row.pad];
  return [...row.pad, input].slice(0, MOST_OF_A_KIND);
}

/** The other controls a lone pad input presses too, by their labels. */
export function alsoPressing(rows: readonly BindingRow[], key: string, input: string): string[] {
  if (input.includes("+")) return [];
  return rows.filter((r) => r.key !== key && r.pad.includes(input)).map((r) => r.label);
}
