import { describe, expect, it } from "vitest";
import { discPrint } from "./discArt";

describe("discPrint", () => {
  it("picks the print of the version's region", () => {
    expect(discPrint("rac1", "ntsc")).toBe("/img/discs/rac1-ntsc.webp");
    expect(discPrint("rac1", "pal1")).toBe("/img/discs/rac1-pal.webp");
    expect(discPrint("rac2", "ntsc2")).toBe("/img/discs/rac2-ntsc.webp");
    expect(discPrint("rac2", "jp")).toBe("/img/discs/rac2-jp.webp");
  });

  it("shows the North American disc for a region without a print", () => {
    expect(discPrint("rac6", "jp")).toBe("/img/discs/rac6-ntsc.webp");
  });

  it("shows the Japanese print for Korean discs", () => {
    expect(discPrint("rac1", "jp")).toBe("/img/discs/rac1-jp.webp");
    expect(discPrint("rac3", "kr")).toBe("/img/discs/rac3-jp.webp");
  });

  it("has no print for another game", () => {
    expect(discPrint("jak1", "ntsc")).toBeNull();
  });
});
