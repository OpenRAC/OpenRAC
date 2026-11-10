// SPDX-License-Identifier: GPL-3.0-or-later
// The two sounds of Play's disc animation (DiscLaunch.svelte), made on the spot with Web Audio: no
// files. First the disc clicking onto the spindle (a soft knock, then the clamp's snap); then the
// drive: its motor winding up under the rush of air of a spinning disc, and the lens's little
// ticks as it looks for the track. Quiet, and cut above 3 kHz, so it never sounds harsh.

/** The sounds of one Play, from the moment it is pressed. `stop` ends them (a fade, not a cut). */
export interface DriveSounds {
  stop: (fade?: number) => void;
}

/** One short burst of white noise, shared by every sound below. */
function noise(ctx: BaseAudioContext, seconds: number): AudioBuffer {
  const buffer = ctx.createBuffer(1, Math.ceil(ctx.sampleRate * seconds), ctx.sampleRate);
  const data = buffer.getChannelData(0);
  // A fixed seed: the same sound every time.
  let seed = 22222;
  for (let i = 0; i < data.length; i++) {
    seed = (seed * 16807) % 2147483647;
    data[i] = (seed / 2147483647) * 2 - 1;
  }
  return buffer;
}

/** A click: noise through a band around `hz`, gone in `decay` seconds. */
function click(
  ctx: BaseAudioContext,
  out: AudioNode,
  buf: AudioBuffer,
  at: number,
  hz: number,
  level: number,
  decay: number,
) {
  const src = ctx.createBufferSource();
  src.buffer = buf;
  const band = ctx.createBiquadFilter();
  band.type = "bandpass";
  band.frequency.value = hz;
  band.Q.value = 1.4;
  const env = ctx.createGain();
  env.gain.setValueAtTime(0, at);
  env.gain.linearRampToValueAtTime(level, at + 0.002);
  env.gain.exponentialRampToValueAtTime(0.0001, at + decay);
  src.connect(band).connect(env).connect(out);
  src.start(at, Math.random() * 0.5, decay + 0.02);
}

/** A low knock: a sine falling from `from` to `to` Hz. */
function knock(
  ctx: BaseAudioContext,
  out: AudioNode,
  at: number,
  from: number,
  to: number,
  level: number,
  decay: number,
) {
  const osc = ctx.createOscillator();
  osc.frequency.setValueAtTime(from, at);
  osc.frequency.exponentialRampToValueAtTime(to, at + decay);
  const env = ctx.createGain();
  env.gain.setValueAtTime(0, at);
  env.gain.linearRampToValueAtTime(level, at + 0.004);
  env.gain.exponentialRampToValueAtTime(0.0001, at + decay);
  osc.connect(env).connect(out);
  osc.start(at);
  osc.stop(at + decay + 0.02);
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
  const buf = noise(ctx, 2);

  // Everything through one soft top cut and one volume.
  const master = ctx.createGain();
  master.gain.value = 0.8;
  const soft = ctx.createBiquadFilter();
  soft.type = "lowpass";
  soft.frequency.value = 3200;
  soft.connect(master).connect(ctx.destination);

  const t0 = ctx.currentTime + seat;

  // ---- 1. The disc goes onto the spindle: a soft knock of plastic, then the clamp snaps. ----
  knock(ctx, soft, t0, 170, 70, 0.32, 0.09);
  click(ctx, soft, buf, t0, 1900, 0.35, 0.03);
  click(ctx, soft, buf, t0 + 0.05, 2600, 0.22, 0.025);
  knock(ctx, soft, t0 + 0.05, 240, 120, 0.12, 0.05);

  // ---- 2. The drive: the motor winds up, the air rushes, the lens ticks as it finds the track. ----
  const start = t0 + 0.09;
  const end = start + spin;

  const motor = ctx.createOscillator();
  motor.type = "triangle";
  motor.frequency.setValueAtTime(38, start);
  motor.frequency.exponentialRampToValueAtTime(190, start + 0.9);
  const hum = ctx.createBiquadFilter();
  hum.type = "lowpass";
  hum.frequency.value = 700;
  const motorGain = ctx.createGain();
  motorGain.gain.setValueAtTime(0, start);
  motorGain.gain.linearRampToValueAtTime(0.16, start + 0.25);
  motor.connect(hum).connect(motorGain).connect(soft);
  motor.start(start);
  motor.stop(end + 1);

  // An octave over the motor, quieter: the whine a drive makes as it reaches speed.
  const whine = ctx.createOscillator();
  whine.type = "sine";
  whine.frequency.setValueAtTime(90, start);
  whine.frequency.exponentialRampToValueAtTime(760, start + 1);
  const whineGain = ctx.createGain();
  whineGain.gain.setValueAtTime(0, start);
  whineGain.gain.linearRampToValueAtTime(0.025, start + 0.6);
  whine.connect(whineGain).connect(soft);
  whine.start(start);
  whine.stop(end + 1);

  const air = ctx.createBufferSource();
  air.buffer = buf;
  air.loop = true;
  const rush = ctx.createBiquadFilter();
  rush.type = "bandpass";
  rush.Q.value = 0.9;
  rush.frequency.setValueAtTime(260, start);
  rush.frequency.exponentialRampToValueAtTime(1300, start + 0.9);
  const airGain = ctx.createGain();
  airGain.gain.setValueAtTime(0, start);
  airGain.gain.linearRampToValueAtTime(0.11, start + 0.5);
  air.connect(rush).connect(airGain).connect(soft);
  air.start(start);
  air.stop(end + 1);

  const ticks: [number, number][] = [
    [0.22, 0.1],
    [0.29, 0.07],
    [0.38, 0.09],
    [0.62, 0.05],
  ];
  for (const [after, level] of ticks) {
    click(ctx, soft, buf, start + after, 2300, level, 0.012);
  }

  // The drive dies away as the screen goes black.
  for (const g of [motorGain, whineGain, airGain]) {
    g.gain.setValueAtTime(g === motorGain ? 0.16 : g === whineGain ? 0.025 : 0.11, end - 0.35);
    g.gain.linearRampToValueAtTime(0, end);
  }

  return master;
}
