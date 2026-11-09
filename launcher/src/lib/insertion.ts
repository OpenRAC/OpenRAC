// SPDX-License-Identifier: GPL-3.0-or-later
// How a disc goes into the console when Play is pressed on Android's Games tab (DiscLaunch.svelte),
// as pure functions of the time since the press. Adapted from Cartouche's insertion.gd
// (github.com/Hakimba/cartouche, GPL-3.0-or-later) and its DVD timings (appearance.json): the disc
// comes to the console and turns to face the player, comes forward as if taken in hand, goes back
// onto the spindle with a small rebound, and spins up; then the lid closes, the light comes on and
// the screen goes black from the disc's centre. Times in seconds.

export const INSERTION = {
  /** The shelf leaves and the console comes in (ours). */
  intro_s: 0.35,
  face_s: 0.25,
  grab_s: 0.24,
  /** How far the disc comes forward, as a share of its size. */
  grab_lift: 0.12,
  seat_s: 0.22,
  bounce: 0.03,
  bounce_s: 0.08,
  spin_up_s: 0.7,
  turns_per_s: 15,
  /** The lid (ours): it starts closing while the disc spins up. */
  lid_after_s: 0.22,
  lid_s: 0.4,
  black_after_s: 0.1,
  black_s: 0.35,
} as const;

const I = INSERTION;

/** How far `t` is through the step from `from` lasting `length`, from 0 to 1. */
export function progress(t: number, from: number, length: number): number {
  if (length <= 0) return t >= from ? 1 : 0;
  return Math.min(1, Math.max(0, (t - from) / length));
}

const smooth = (x: number) => x * x * (3 - 2 * x);

/** When the disc reaches the console and starts to turn towards the player. */
export const faceAt = () => I.intro_s;
/** When the disc touches the spindle: the click, and the drive starts. */
export const seatedAt = () => I.intro_s + I.face_s + I.grab_s + I.seat_s;
/** When the light comes on: the lid is shut. */
export const shutAt = () => seatedAt() + I.lid_after_s + I.lid_s;
/** When the screen is all black: the game can start. */
export const duration = () => shutAt() + I.black_after_s + I.black_s;

/** How far the disc has come from the shelf to the console's spindle, 0 to 1. */
export function travelAt(t: number): number {
  return smooth(progress(t, 0, I.intro_s + I.face_s));
}

/** How far the disc stands out of the console, as a share of its size: forward as it is taken in
 *  hand, back past where it was onto the spindle, then a small rebound. */
export function liftAt(t: number): number {
  const grabbed = progress(t, faceAt() + I.face_s, I.grab_s);
  const pushed = progress(t, faceAt() + I.face_s + I.grab_s, I.seat_s);
  const rebound = progress(t, seatedAt(), I.bounce_s);
  const lift = I.grab_lift * (1 - (1 - grabbed) ** 2);
  return lift * (1 - pushed * pushed) + I.bounce * Math.sin(Math.PI * rebound);
}

/** How far the drive has turned the disc, in degrees: speeding up evenly from the click, then
 *  turning `turns_per_s` times a second. */
export function spinAt(t: number): number {
  const spun = Math.max(0, t - seatedAt());
  const ramp = Math.min(spun, I.spin_up_s);
  const full = 360 * I.turns_per_s;
  return (full * ramp * ramp) / (2 * I.spin_up_s) + full * (spun - ramp);
}

/** How fast the disc turns, 0 (still) to 1 (full speed): for the blur of its print. */
export function speedAt(t: number): number {
  return progress(t, seatedAt(), I.spin_up_s);
}

/** How fast the disc turns, in turns a second: how far its print smears in one frame. */
export function turnsAt(t: number): number {
  return I.turns_per_s * speedAt(t);
}

/** How shut the lid is, 0 (open) to 1 (shut), easing in and out. */
export function lidAt(t: number): number {
  return smooth(progress(t, seatedAt() + I.lid_after_s, I.lid_s));
}

/** How much of the screen is black, 0 to 1 (all of it), growing faster and faster from the centre. */
export function blackAt(t: number): number {
  return progress(t, shutAt() + I.black_after_s, I.black_s) ** 2;
}

/** How visible the console is: it comes in with the intro. */
export function consoleAt(t: number): number {
  return smooth(progress(t, 0, I.intro_s));
}
