// The browser preview's controls files (core/src/controls.rs), kept in memory: the defaults, each
// file's values, and a value read over the one below it kind by kind, as the port does. Values
// are taken as given; the real checks are the core's.

import type { BindingRow } from "./generated/BindingRow";
import type { ControlsLayer } from "./generated/ControlsLayer";
import type { ControlsScope } from "./generated/ControlsScope";
import type { ControlsView } from "./generated/ControlsView";
import { PAD_NONE, UNBOUND } from "./controls";

// key, label, the default keys, the default pad inputs.
const TARGETS: [string, string, string[], string[]][] = [
  ["bind_cross", "Cross", ["Space"], ["PadSouth"]],
  ["bind_circle", "Circle", ["KeyE"], ["PadEast"]],
  ["bind_square", "Square", ["KeyF"], ["PadWest"]],
  ["bind_triangle", "Triangle", ["KeyR"], ["PadNorth"]],
  ["bind_l1", "L1", ["ShiftLeft"], ["PadLeftShoulder"]],
  ["bind_r1", "R1", ["KeyQ"], ["PadRightShoulder"]],
  ["bind_l2", "L2", ["Digit1"], ["PadLeftTrigger"]],
  ["bind_r2", "R2", ["Digit3"], ["PadRightTrigger"]],
  ["bind_l3", "L3", ["KeyC"], ["PadLeftStick"]],
  ["bind_r3", "R3", ["KeyV"], ["PadRightStick"]],
  ["bind_start", "Start", ["Enter"], ["PadStart"]],
  ["bind_select", "Select", ["Backspace"], ["PadBack"]],
  ["bind_up", "D-pad up", ["ArrowUp"], ["PadDpadUp"]],
  ["bind_down", "D-pad down", ["ArrowDown"], ["PadDpadDown"]],
  ["bind_left", "D-pad left", ["ArrowLeft"], ["PadDpadLeft"]],
  ["bind_right", "D-pad right", ["ArrowRight"], ["PadDpadRight"]],
  ["bind_lstick_up", "Left stick up", [], ["PadLeftStickUp"]],
  ["bind_lstick_down", "Left stick down", [], ["PadLeftStickDown"]],
  ["bind_lstick_left", "Left stick left", [], ["PadLeftStickLeft"]],
  ["bind_lstick_right", "Left stick right", [], ["PadLeftStickRight"]],
  ["bind_rstick_up", "Right stick up", [], ["PadRightStickUp"]],
  ["bind_rstick_down", "Right stick down", [], ["PadRightStickDown"]],
  ["bind_rstick_left", "Right stick left", [], ["PadRightStickLeft"]],
  ["bind_rstick_right", "Right stick right", [], ["PadRightStickRight"]],
];

const files = new Map<string, Map<string, string>>();

const fileKey = (scope: ControlsScope) => (scope.kind === "global" ? "global" : scope.game);

function file(name: string): Map<string, string> {
  let f = files.get(name);
  if (!f) {
    f = new Map();
    files.set(name, f);
  }
  return f;
}

function read(value: string): { keys: string[] | null; pad: string[] | null } {
  if (value.trim() === UNBOUND) return { keys: [], pad: [] };
  const names = value
    .split(",")
    .map((n) => n.trim())
    .filter(Boolean);
  const pad = names.filter((n) => n.startsWith("Pad") && n !== PAD_NONE);
  const keys = names.filter((n) => !n.startsWith("Pad"));
  return {
    keys: keys.length > 0 ? keys : null,
    pad: pad.length > 0 || names.includes(PAD_NONE) ? pad : null,
  };
}

/** One player's own keys in a file, without their prefix. */
function own(values: Map<string, string>, player: number): Map<string, string> {
  const prefix = `p${player + 1}.`;
  return new Map([...values].filter(([k]) => k.startsWith(prefix)).map(([k, v]) => [k.slice(prefix.length), v]));
}

export function mockControlsView(game: string | null, player: number | null = null): ControlsView {
  const layers: [ControlsLayer, Map<string, string>][] = [["global", file("global")]];
  if (player !== null) layers.push(["globalPlayer", own(file("global"), player)]);
  if (game) layers.push(["game", file(game)]);
  if (game && player !== null) layers.push(["gamePlayer", own(file(game), player)]);
  const edited = (key: string) => (player === null ? key : `p${player + 1}.${key}`);
  const bindings: BindingRow[] = TARGETS.map(([key, label, keys, pad]) => {
    const row: BindingRow = {
      key,
      label,
      button: keys.length > 0,
      hint: game === "rac1" && key === "bind_cross" ? "Jump" : null,
      keys: [...keys],
      pad: [...pad],
      keysFrom: "default",
      padFrom: "default",
      global: file("global").get(edited(key)) ?? null,
      game: game ? (file(game).get(edited(key)) ?? null) : null,
      shared: [],
    };
    for (const [layer, values] of layers) {
      const value = values.get(key);
      if (value === undefined) continue;
      const got = read(value);
      if (got.keys) [row.keys, row.keysFrom] = [got.keys, layer];
      if (got.pad) [row.pad, row.padFrom] = [got.pad, layer];
    }
    return row;
  });
  for (const row of bindings) {
    row.shared = row.pad.filter((p) => !p.includes("+") && bindings.some((o) => o !== row && o.pad.includes(p)));
  }
  return {
    game,
    player,
    globalFile: "~/.local/share/openrac/controls/global.ini",
    gameFile: game ? `~/.local/share/openrac/controls/${game}.ini` : null,
    globalReadOnly: false,
    gameReadOnly: false,
    bindings,
    tuning: [
      ["pad_left_deadzone", "Left stick dead zone", "0", 0, 50],
      ["pad_right_deadzone", "Right stick dead zone", "0", 0, 50],
      ["pad_trigger_threshold", "Trigger press point", "13", 1, 100],
    ].map(([key, label, value, low, high]) => {
      const set = layers.reduce<[string, ControlsLayer] | null>(
        (got, [layer, values]) => (values.has(String(key)) ? [values.get(String(key)) ?? "", layer] : got),
        null,
      );
      return {
        key: String(key),
        label: String(label),
        value: set ? set[0] : String(value),
        from: set ? set[1] : "default",
        low: Number(low),
        high: Number(high),
        words: [],
      };
    }),
    players: [
      {
        key: "player1",
        wants: (file("global").get("player1") ?? "keyboard, auto").split(", "),
        set: file("global").has("player1"),
      },
      {
        key: "player2",
        wants: (file("global").get("player2") ?? "auto").split(", "),
        set: file("global").has("player2"),
      },
    ],
    devices: [{ id: "030000004c050000e60c000000016800", name: "DualSense Wireless Controller" }],
    padNames: [...new Set(TARGETS.flatMap(([, , , pad]) => pad))],
    issues: [],
  };
}

export function mockControlsSet(scope: ControlsScope, key: string, value: string | null): ControlsView {
  const f = file(fileKey(scope));
  if (value === null) f.delete(key);
  else f.set(key, value);
  const player = /^p([12])\./.exec(key);
  return mockControlsView(scope.kind === "game" ? scope.game : null, player ? Number(player[1]) - 1 : null);
}

export function mockControlsReset(scope: ControlsScope): ControlsView {
  const name = fileKey(scope);
  if (scope.kind === "game") files.delete(name);
  else for (const key of [...file(name).keys()]) if (/^(p[12]\.)?(bind_|pad_)/.test(key)) file(name).delete(key);
  return mockControlsView(scope.kind === "game" ? scope.game : null);
}

export function mockControlsResetPlayers(): ControlsView {
  file("global").delete("player1");
  file("global").delete("player2");
  return mockControlsView(null);
}

export function mockControlsCopy(from: string, to: string): ControlsView {
  files.set(to, new Map(file(from)));
  return mockControlsView(to);
}

export function mockControlsResetPlayer(scope: ControlsScope, player: number): ControlsView {
  const f = file(fileKey(scope));
  for (const key of [...f.keys()]) if (key.startsWith(`p${player + 1}.`)) f.delete(key);
  return mockControlsView(scope.kind === "game" ? scope.game : null, player);
}
