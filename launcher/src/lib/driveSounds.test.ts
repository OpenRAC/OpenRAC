import { describe, expect, it } from "vitest";
import { seatSamples, spinSamples } from "./driveSounds";

const peak = (d: Float32Array) => d.reduce((m, v) => Math.max(m, Math.abs(v)), 0);

describe("drive sounds", () => {
  it("makes the click and the drive, never clipping and the same every time", () => {
    const seat = seatSamples(44100);
    const spin = spinSamples(44100, 1.07);
    expect(seat.every(Number.isFinite) && spin.every(Number.isFinite)).toBe(true);
    expect(peak(seat)).toBeCloseTo(0.6);
    expect(peak(spin)).toBeCloseTo(0.45);
    expect(spinSamples(44100, 1.07)).toEqual(spin);
  });

  it("ends the drive in silence, faded, not cut", () => {
    const spin = spinSamples(44100, 1);
    expect(peak(spin.subarray(spin.length - 200))).toBeLessThan(0.01);
  });
});
