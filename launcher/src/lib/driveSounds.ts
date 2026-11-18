// SPDX-License-Identifier: GPL-3.0-or-later
// The two sounds of Play's disc animation (DiscLaunch.svelte), worked out sample by sample here (no
// files), after what a disc drive does:
//  1. The disc onto the spindle: its hub slides down the spindle, knocks on the turntable (the disc
//     and the tray ring at their own pitches), the clamp snaps shut with a little metal ring, and
//     the disc rattles once or twice as it settles.
//  2. The drive: the focus coil jumps twice as the lens looks for the disc; the spindle motor winds
//     up (its commutation whine, a few harmonics, rising with the speed) under the rush of air of
//     the spinning disc and the disc's slight wobble; the laser sled buzzes out along its gear and
//     stops with a tick; the lens chatters as it reads. All of it through the console's plastic case
//     (a few resonances), with nothing sharp above 3 kHz.
// The same seed every time: the same sound every time.

/** The sounds of one Play, from the moment it is pressed. `stop` ends them (a fade, not a cut). */
export interface DriveSounds {
  stop: (fade?: number) => void;
}

const TAU = Math.PI * 2;

/** A seeded random source, -1 to 1. */
function random(seed: number) {
  let s = seed;
  return () => {
    s = (s * 16807) % 2147483647;
    return (s / 2147483647) * 2 - 1;
  };
}

/** A two-pole resonator: rings at `hz`, its ringing down to 1/1000 in `decay` seconds. */
function resonator(rate: number, hz: number, decay: number) {
  const r = Math.exp(-6.9 / (decay * rate));
  const a1 = 2 * r * Math.cos((TAU * hz) / rate);
  const a2 = -r * r;
  const g = 1 - r;
  let y1 = 0,
    y2 = 0;
  return (x: number) => {
    const y = g * x + a1 * y1 + a2 * y2;
    y2 = y1;
    y1 = y;
    return y;
  };
}

/** A band-pass filter (constant peak gain), its centre settable per sample. */
function bandpass(rate: number, q: number) {
  let x1 = 0,
    x2 = 0,
    y1 = 0,
    y2 = 0;
  return (x: number, hz: number) => {
    const w = (TAU * Math.min(hz, rate * 0.45)) / rate;
    const alpha = Math.sin(w) / (2 * q);
    const a0 = 1 + alpha;
    const y = (alpha * x - alpha * x2 - -2 * Math.cos(w) * y1 - (1 - alpha) * y2) / a0;
    x2 = x1;
    x1 = x;
    y2 = y1;
    y1 = y;
    return y;
  };
}

/** A one-pole low-pass. */
function lowpass(rate: number, hz: number) {
  const a = Math.exp((-TAU * hz) / rate);
  let y = 0;
  return (x: number) => (y = (1 - a) * x + a * y);
}

/** A bank of resonators struck together: a piece of plastic, or of metal, ringing. */
function body(rate: number, modes: [number, number, number][]) {
  const rs = modes.map(([hz, decay]) => resonator(rate, hz, decay));
  return (x: number) => {
    let y = 0;
    for (let i = 0; i < rs.length; i++) y += (rs[i]?.(x) ?? 0) * (modes[i]?.[2] ?? 0);
    return y;
  };
}

/** Peak to `peak`, so each sound has its own loudness whatever its parts add up to. */
function normalise(data: Float32Array, peak: number) {
  let most = 0;
  for (const v of data) most = Math.max(most, Math.abs(v));
  if (most > 0) for (let i = 0; i < data.length; i++) data[i] = ((data[i] ?? 0) * peak) / most;
}

/** 1. The disc onto the spindle; its contact with the turntable `lead` seconds into it. */
export function seatSamples(rate: number, lead = 0.05): Float32Array {
  const out = new Float32Array(Math.ceil(rate * (lead + 0.4)));
  const rnd = random(31337);
  const disc = body(rate, [
    [410, 0.07, 1],
    [1120, 0.045, 0.7],
    [2350, 0.025, 0.45],
    [3700, 0.016, 0.25],
    [5300, 0.01, 0.12],
  ]);
  const tray = body(rate, [
    [180, 0.09, 1.2],
    [520, 0.05, 0.6],
    [1650, 0.03, 0.3],
  ]);
  const clamp = body(rate, [
    [1900, 0.02, 0.6],
    [3150, 0.09, 0.35],
    [4400, 0.03, 0.3],
    [6200, 0.012, 0.15],
  ]);
  const slide = bandpass(rate, 3);
  const at = (s: number) => Math.round((lead + s) * rate);
  // What strikes what, and how hard: the knock, the clamp, the disc settling.
  const strikes = new Map<number, [number, number, number]>([
    [at(0), [1, 0.9, 0]],
    [at(0.004), [0.35, 0.2, 0]],
    [at(0.036), [0.15, 0.1, 1]],
    [at(0.058), [0.12, 0.05, 0.25]],
    [at(0.071), [0.06, 0.03, 0.1]],
  ]);
  let hit: [number, number, number] = [0, 0, 0];
  let left = 0;
  for (let i = 0; i < out.length; i++) {
    const t = i / rate - lead;
    const s = strikes.get(i);
    if (s) {
      hit = s;
      left = Math.round(rate * 0.0012);
    }
    // A strike: a burst of noise a little over a millisecond long.
    const n = rnd();
    const e = left-- > 0 ? n : 0;
    // The hub sliding down the spindle just before it lands: a short rising whisper.
    const sliding = t < 0 && t > -lead ? (1 + t / lead) ** 2 * 0.05 : 0;
    const thump = t >= 0 ? Math.sin(TAU * 95 * t) * Math.exp(-t / 0.025) * 0.5 : 0;
    out[i] =
      disc(e * hit[0]) * 2.2 +
      tray(e * hit[1]) * 2.6 +
      clamp(e * hit[2]) * 2 +
      slide(n * sliding, 2600 + 3000 * (1 + t / lead)) +
      thump;
  }
  normalise(out, 0.6);
  return out;
}

/** 2. The drive, `seconds` long, fading out over its last 0.35 s. */
export function spinSamples(rate: number, seconds: number): Float32Array {
  const n = Math.ceil(rate * seconds);
  const out = new Float32Array(n);
  const rnd = random(4242);
  const air = bandpass(rate, 1.6);
  const air2 = bandpass(rate, 3);
  const sledGear = bandpass(rate, 6);
  const tick = body(rate, [
    [700, 0.014, 1],
    [1350, 0.009, 0.5],
    [2300, 0.006, 0.2],
  ]);
  const focus = body(rate, [
    [260, 0.03, 1],
    [610, 0.02, 0.5],
  ]);
  const caseBody = body(rate, [
    [170, 0.12, 0.5],
    [330, 0.08, 0.4],
    [640, 0.06, 0.3],
    [980, 0.05, 0.2],
  ]);
  const top = lowpass(rate, 2200);
  const top2 = lowpass(rate, 3500);
  const rumbleLp = lowpass(rate, 140);

  // The focus coil's two jumps, the sled's one run, then the lens reading.
  const jumps = [0.04, 0.11];
  const sled: [number, number][] = [[0.16, 0.42]];
  const reads: number[] = [];
  for (let t = 0.34, k = 0; t < seconds - 0.4; k++) {
    reads.push(t);
    t += 0.035 + ((((Math.sin(k * 12.9898) * 43758.5453) % 1) + 1) % 1) * 0.07;
  }
  const strikes = new Map<number, [number, number]>();
  jumps.forEach((t, k) => strikes.set(Math.round(t * rate), [0, k ? 0.7 : 1]));
  sled.forEach(([, stop]) => strikes.set(Math.round(stop * rate), [0.8, 0]));
  reads.forEach((t, k) => strikes.set(Math.round(t * rate), [0.12 + 0.08 * ((k * 7) % 3), 0]));

  let phase = 0,
    turn = 0,
    sledPhase = 0,
    left = 0;
  let hit: [number, number] = [0, 0];
  for (let i = 0; i < n; i++) {
    const t = i / rate;
    // Turns a second: up towards 38 (a little over 2,000 rpm), quickly at first.
    const fr = 38 * (1 - Math.exp(-Math.max(0, t - 0.06) / 0.38));
    const speed = fr / 38;
    turn += (TAU * fr) / rate;
    // The motor's hum: 6 steps a turn, and its harmonics, louder as it pulls harder.
    phase += (TAU * 6 * fr) / rate;
    const pull = 0.4 + 0.6 * Math.exp(-t / 0.5);
    let whine = 0;
    for (const [k, a] of [
      [1, 1],
      [2, 0.45],
      [3, 0.22],
      [5, 0.08],
    ] as const) {
      whine += Math.sin(phase * k) * a;
    }
    whine *= 0.09 * pull * Math.min(1, speed * 3);
    // The disc's wobble: a low hum at its turning, and everything swells a little once a turn.
    const wobble = 1 + 0.18 * Math.sin(turn) * speed;
    const rumble = rumbleLp(Math.sin(turn) + 0.4 * Math.sin(2 * turn + 1)) * 0.03 * speed;
    // The air: rising in pitch and loudness with the speed, with a narrower whistle in it.
    const noise = rnd();
    const rush = air(noise, 120 + 9 * fr) * 0.5 * speed ** 1.5 + air2(noise, 380 + 14 * fr) * 0.04 * speed ** 2;
    // The sled: a small motor buzzing through its gear while it runs.
    let buzz = 0;
    for (const [from, to] of sled) {
      if (t >= from && t < to) {
        const u = (t - from) / (to - from);
        sledPhase += (TAU * (90 + 25 * u)) / rate;
        const env = Math.min(1, u * 12, (1 - u) * 12);
        const saw = (sledPhase / TAU) % 1;
        buzz = (sledGear(saw * 2 - 1 + noise * 0.4, 650) * 0.9 + (saw - 0.5) * 0.25) * env;
      }
    }
    const s = strikes.get(i);
    if (s) {
      hit = s;
      left = Math.round(rate * 0.0009);
    }
    const e = left-- > 0 ? rnd() : 0;
    const ticks = tick(e * hit[0]) * 3 + focus(e * hit[1]) * 1.4;
    const dry = (whine + rush) * wobble + rumble + buzz * 0.5 + ticks;
    out[i] = top2(top(dry + caseBody(dry) * 0.6));
  }
  // Out over the last 0.35 s, in over the first 20 ms.
  for (let i = 0; i < n; i++) {
    const t = i / rate;
    const g = Math.min(1, t / 0.02, Math.max(0, (seconds - t) / 0.35));
    out[i] = (out[i] ?? 0) * g;
  }
  normalise(out, 0.45);
  return out;
}

/** The disc onto the spindle at `seat` seconds from now, then the drive spinning it up for `spin`
 *  seconds. Null where there is no Web Audio, or the page may not play sound yet. */
export function driveSounds(seat: number, spin: number): DriveSounds | null {
  const Ctx = typeof window === "undefined" ? undefined : window.AudioContext;
  if (!Ctx) return null;
  let ctx: AudioContext;
  try {
    ctx = new Ctx();
  } catch {
    return null;
  }
  void ctx.resume().catch(() => undefined);
  const master = playInto(ctx, seat, spin);
  let stopped = false;
  return {
    stop(fade = 0.08) {
      if (stopped) return;
      stopped = true;
      const now = ctx.currentTime;
      master.gain.cancelScheduledValues(now);
      master.gain.setValueAtTime(master.gain.value, now);
      master.gain.linearRampToValueAtTime(0, now + fade);
      window.setTimeout(() => void ctx.close().catch(() => undefined), (fade + 0.1) * 1000);
    },
  };
}

/** Both sounds into `ctx` (a live one, or an offline one to hear them in a file); its volume. */
export function playInto(ctx: BaseAudioContext, seat: number, spin: number): GainNode {
  const master = ctx.createGain();
  master.gain.value = 0.9;
  master.connect(ctx.destination);
  const lead = 0.05;
  const play = (data: Float32Array, at: number) => {
    const buffer = ctx.createBuffer(1, data.length, ctx.sampleRate);
    buffer.copyToChannel(data as Float32Array<ArrayBuffer>, 0);
    const src = ctx.createBufferSource();
    src.buffer = buffer;
    src.connect(master);
    src.start(Math.max(ctx.currentTime, at));
  };
  const t0 = ctx.currentTime + seat;
  play(seatSamples(ctx.sampleRate, lead), t0 - lead);
  play(spinSamples(ctx.sampleRate, spin), t0 + 0.07);
  return master;
}
