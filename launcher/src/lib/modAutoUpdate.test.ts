// When mods update by themselves: never while the game runs, never with the setting off, and
// Play never waits on it for long.
import { beforeEach, describe, expect, it, vi } from "vitest";

const calls: { running: boolean; ids: string[] | null }[] = [];
let hang = false;
vi.mock("./api", async (load) => {
  const real = await load<typeof import("./api")>();
  return {
    ...real,
    api: {
      ...real.api,
      modsAutoUpdate: vi.fn((_g: string, _v: string, ids: string[] | null, running: boolean) => {
        calls.push({ running, ids });
        if (hang) return new Promise(() => undefined);
        return Promise.resolve({
          updated: [{ id: "a", name: "A", from: "1.0.0", to: "1.1.0", native: false, changelog: "", hold: null }],
          plan: { silent: [], waiting: [], held: [] },
          result: null,
        });
      }),
    },
  };
});

const { app, jobs } = await import("./app.svelte");
const auto = await import("./modAutoUpdate.svelte");

const version = {
  key: "rac1/pal",
  game: "rac1",
  name: "pal",
  actions: [{ id: "play", state: "unverified", thisPlatform: true }],
} as never;

describe("auto-update", () => {
  beforeEach(() => {
    calls.length = 0;
    hang = false;
    app.config = { autoUpdateMods: true } as never;
    app.library = { games: [{ id: "rac1", title: "R&C", versions: [version], install: null }] } as never;
    app.discs = [];
    app.discChoices = [];
    app.game = null;
    jobs.list = [];
    auto.autoUpdates.recent = {};
  });

  it("updates each playable game's mods and remembers what went in", async () => {
    await auto.autoUpdateAll();
    expect(calls).toEqual([{ running: false, ids: null }]);
    expect(auto.autoUpdates.recent.rac1?.[0]?.id).toBe("a");
  });

  it("does nothing while the game runs, or with the setting off", async () => {
    app.game = { running: true };
    await auto.autoUpdateAll();
    await auto.beforePlay("rac1/pal");
    app.game = null;
    app.config = { autoUpdateMods: false } as never;
    await auto.autoUpdateAll();
    expect(calls).toEqual([]);
  });

  it("lets Play go on when the update takes too long", async () => {
    vi.useFakeTimers();
    hang = true;
    let done = false;
    void auto.beforePlay("rac1/pal").then(() => (done = true));
    await vi.advanceTimersByTimeAsync(auto.BEFORE_PLAY_MS + 10);
    expect(done).toBe(true);
    vi.useRealTimers();
  });
});
