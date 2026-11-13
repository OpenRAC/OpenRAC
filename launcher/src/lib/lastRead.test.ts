import { describe, expect, it } from "vitest";
import { hasRead, keepRead, lastRead } from "./lastRead";

describe("lastRead", () => {
  it("gives the fallback until something is kept, then the kept value", () => {
    expect(hasRead("t:a")).toBe(false);
    expect(lastRead("t:a", [])).toEqual([]);
    keepRead("t:a", [1, 2]);
    expect(hasRead("t:a")).toBe(true);
    expect(lastRead<number[]>("t:a", [])).toEqual([1, 2]);
  });

  it("keeps a null read apart from nothing read", () => {
    keepRead("t:b", null);
    expect(hasRead("t:b")).toBe(true);
    expect(lastRead<string | null>("t:b", "none")).toBeNull();
  });
});
