import { describe, expect, it, vi } from "vitest";
import { startGameRequests } from "./gameRequest";
import { shortcutOptions } from "./shortcuts";

describe("game requests", () => {
  it("opens the game the start could not play, then each one sent", async () => {
    const opened: string[] = [];
    let send: (key: string) => void = () => undefined;
    const stop = vi.fn();
    const off = await startGameRequests((key) => opened.push(key), {
      startupGame: () => Promise.resolve("rac1/pal"),
      onOpenGame: (handler) => {
        send = handler;
        return Promise.resolve(stop);
      },
    });
    send("rac2/ntsc");
    expect(opened).toEqual(["rac1/pal", "rac2/ntsc"]);
    expect(off).toBe(stop);
  });

  it("does nothing for a start with no request, or one that cannot be read", async () => {
    const opened: string[] = [];
    const source = (startupGame: () => Promise<string | null>) => ({
      startupGame,
      onOpenGame: () => Promise.resolve(() => undefined),
    });
    await startGameRequests(
      (key) => opened.push(key),
      source(() => Promise.resolve(null)),
    );
    await startGameRequests(
      (key) => opened.push(key),
      source(() => Promise.reject(new Error("no"))),
    );
    expect(opened).toEqual([]);
  });
});

describe("shortcut options", () => {
  it("offers Steam on Linux and Windows, the desktop file on Linux only", () => {
    expect(shortcutOptions("linux")).toEqual({ steam: true, desktop: true });
    expect(shortcutOptions("windows")).toEqual({ steam: true, desktop: false });
    expect(shortcutOptions("macos")).toEqual({ steam: false, desktop: false });
    expect(shortcutOptions("android")).toEqual({ steam: false, desktop: false });
    expect(shortcutOptions(undefined)).toEqual({ steam: false, desktop: false });
  });
});
