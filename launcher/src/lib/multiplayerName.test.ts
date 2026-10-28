import { describe, expect, it } from "vitest";
import { nameProblem } from "./multiplayerName";

describe("multiplayer name", () => {
  it("takes 3 to 16 letters, digits and underscores", () => {
    for (const good of ["abc", "Brave_Lombax_42", "A1_", "x234567890123456"]) expect(nameProblem(good)).toBeNull();
  });

  it("says what is wrong with any other", () => {
    expect(nameProblem("ab")).toBe("At least 3 characters.");
    expect(nameProblem("x2345678901234567")).toBe("At most 16 characters.");
    for (const bad of ["two words", "dash-name", "émile", "semi;colon"])
      expect(nameProblem(bad)).toBe("Only letters, digits and underscores.");
  });
});
