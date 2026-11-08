import { afterEach, expect, it, vi } from "vitest";
import { gameArt, gameThumb, gameTileArt } from "./media";

afterEach(() => vi.unstubAllGlobals());

it("retains the desktop artwork outside the Store variant", () => {
  vi.stubGlobal("window", {});
  expect(gameArt("rac2")).toBe("/img/gc-bg.webp");
  expect(gameThumb("rac2")).toBe("/img/gc-thumb.webp");
  expect(gameTileArt("rac2")).toBe("/img/gc-tile.webp");
});

it("uses original vector illustrations for every Store catalogue image", () => {
  vi.stubGlobal("window", { __OPENRAC_STORE_ARTWORK__: true });
  for (let i = 1; i <= 6; i++) {
    const game = `rac${i}`;
    expect(gameArt(game)).toBe(`/img/store/${game}.svg`);
    expect(gameThumb(game)).toBe(gameArt(game));
    expect(gameTileArt(game)).toBe(gameArt(game));
  }
  expect(gameArt("unknown")).toBeNull();
});
