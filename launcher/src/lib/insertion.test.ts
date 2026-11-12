import { describe, expect, it } from "vitest";
import { blackAt, duration, lidAt, liftAt, seatedAt, shutAt, spinAt, speedAt, travelAt } from "./insertion";

describe("insertion", () => {
  it("lasts a little over two seconds, each step after the last", () => {
    expect(seatedAt()).toBeCloseTo(1.06);
    expect(shutAt()).toBeGreaterThan(seatedAt());
    expect(duration()).toBeCloseTo(2.13);
  });

  it("brings the disc over, forward, then down onto the spindle, with no rebound", () => {
    expect(travelAt(0)).toBe(0);
    expect(travelAt(0.6)).toBe(1);
    expect(liftAt(0)).toBe(0);
    expect(liftAt(0.84)).toBeGreaterThan(0.1);
    expect(liftAt(seatedAt())).toBeCloseTo(0);
    expect(liftAt(seatedAt() + 0.04)).toBe(0);
    // No kink: its speed never jumps from one frame to the next (a jump read as a hop).
    const h = 1 / 120;
    for (let t = h; t < seatedAt() + 0.1; t += h) {
      expect(Math.abs(liftAt(t + h) - 2 * liftAt(t) + liftAt(t - h))).toBeLessThan(0.002);
    }
    expect(liftAt(2)).toBeCloseTo(0);
  });

  it("spins only once seated, ever faster up to full speed", () => {
    expect(spinAt(seatedAt())).toBe(0);
    const a = spinAt(seatedAt() + 0.2) - spinAt(seatedAt() + 0.1);
    const b = spinAt(seatedAt() + 0.6) - spinAt(seatedAt() + 0.5);
    expect(b).toBeGreaterThan(a);
    expect(speedAt(seatedAt() + 1)).toBe(1);
  });

  it("shuts the lid, then goes all black", () => {
    expect(lidAt(seatedAt())).toBe(0);
    expect(lidAt(shutAt())).toBe(1);
    expect(blackAt(shutAt())).toBe(0);
    expect(blackAt(duration())).toBe(1);
  });
});
