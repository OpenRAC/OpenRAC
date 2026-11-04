import type { Rac2ExtraFields } from "./generated/Rac2ExtraFields";
import type { Rac2ExtraView } from "./generated/Rac2ExtraView";

/** The advanced fields' lists as core/src/save_edit_rac2_extra.rs gives them. */
const LEVELS: [number, string][] = [
  [0, "Aranos Tutorial"],
  [1, "Oozla"],
  [2, "Maktar Nebula"],
  [3, "Endako"],
  [4, "Barlow"],
  [5, "Feltzin System"],
  [6, "Notak"],
  [7, "Siberius"],
  [8, "Tabora"],
  [9, "Dobbo"],
  [10, "Hrugis Cloud"],
  [11, "Joba"],
  [12, "Todano"],
  [13, "Boldan"],
  [14, "Aranos Prison"],
  [15, "Gorn"],
  [16, "Snivelak"],
  [17, "Smolg"],
  [18, "Damosel"],
  [19, "Grelbin"],
  [20, "Yeedil"],
  [22, "Dobbo Orbit"],
  [23, "Damosel Orbit"],
  [24, "Ship Shack"],
  [25, "Wupash Nebula"],
  [26, "Jamming Array"],
];

const MOD_NAMES = ["Shock Mod", "Acid Mod", "Lock On Mod"];
const WEAPONS: Record<number, string> = {
  22: "Chopper",
  23: "Pulse Rifle",
  24: "Seeker Gun",
  25: "Hoverbomb Gun",
  26: "Blitz Gun",
  27: "Minirocket Tube",
  28: "Plasma Coil",
  29: "Lava Gun",
  30: "Lancer",
  32: "Spiderbot Glove",
  37: "Bouncer",
  41: "Miniturret Glove",
  42: "Gravity Bomb",
  44: "RYNO II",
};
const MODS: [number, number, number][] = [
  [22, 1, 2],
  [22, 2, 1],
  [23, 0, 1],
  [24, 0, 1],
  [24, 2, 1],
  [25, 1, 3],
  [25, 2, 1],
  [26, 1, 2],
  [26, 2, 1],
  [27, 1, 2],
  [27, 2, 2],
  [28, 1, 2],
  [28, 2, 1],
  [29, 1, 2],
  [29, 2, 1],
  [30, 0, 2],
  [30, 2, 3],
  [32, 1, 2],
  [37, 1, 3],
  [37, 2, 1],
  [41, 0, 2],
  [42, 1, 2],
  [42, 2, 1],
  [44, 2, 1],
];
const SHIP: [string, string[]][] = [
  ["Nose", ["Standard Nose", "Scoop Nose", "Split Nose"]],
  ["Wings", ["Standard Wings", "Hi-Lift Wings", "Heavy Ordinance Wings"]],
  ["Engine", ["Standard engine", "Triple Boost Acceleration Engine"]],
  ["Lasers", ["Standard lasers", "Fusion Laser Cannons"]],
  ["Mines", ["None", "Electro Mine Launcher", "Mega-Mine Launcher"]],
  ["Missiles", ["Standard Missile Launcher", "Fast-lock Missile Launcher"]],
  ["Torpedoes", ["None", "Torpedo Launcher", "Multi-Torpedo Launcher"]],
  ["Shields", ["Basic Shielding System", "Advanced Shielding System", "Ultra-Charge Shielding System"]],
  ["Nuclear Detonation Device", ["No", "Yes"]],
  ["Hyperspace Warp System", ["No", "Yes"]],
];

const saved: Record<string, Rac2ExtraFields> = {};
let writes = 0;

/** A slot on Oozla with the Lancer and Seeker Gun, two platinum bolts found. */
export function mockRac2Extra(serial: string, slot: number): Rac2ExtraView {
  const key = `${serial}/${String(slot)}`;
  const fields: Rac2ExtraFields = saved[key] ?? {
    level: 1,
    crystals: 0,
    moonstones: 0,
    platinumSpent: 0,
    ship: SHIP.map(() => 0),
    mods: MODS.map(([item, s]) => ({ item, slot: s, bought: item === 24 && s === 0 })),
  };
  return {
    serial,
    slot,
    fingerprint: `mock-${String(writes)}`,
    fields: structuredClone(fields),
    levels: LEVELS.map(([id, name]) => ({ id, name })),
    mods: MODS.map(([item, s, price]) => ({
      item,
      itemName: WEAPONS[item] ?? `Item ${String(item)}`,
      slot: s,
      name: MOD_NAMES[s] ?? "Mod",
      price,
      owned: item === 24 || item === 30,
    })),
    ship: SHIP.map(([name, choices]) => ({ name, choices })),
    platinumFound: 2,
    maxGems: 9999,
  };
}

export function mockWriteRac2Extra(serial: string, slot: number, fields: Rac2ExtraFields): Rac2ExtraView {
  if (fields.crystals > 9999 || fields.moonstones > 9999) throw new Error("crystals and moonstones go up to 9999");
  if (fields.platinumSpent > 2) throw new Error("only 2 platinum bolts are found in this save");
  saved[`${serial}/${String(slot)}`] = structuredClone(fields);
  writes += 1;
  return mockRac2Extra(serial, slot);
}
