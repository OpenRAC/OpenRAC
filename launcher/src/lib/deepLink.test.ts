// openrac:// links reach the change set one at a time: the start's own link (getCurrent), then
// each one sent (onOpenUrl); one more waits while a change set is open, the rest are dropped.
import { beforeEach, describe, expect, it, vi } from "vitest";

const parsed: string[] = [];
vi.mock("./api", async (load) => {
  const real = await load<typeof import("./api")>();
  return {
    ...real,
    inTauri: false,
    api: {
      ...real.api,
      deeplinkParse: vi.fn((url: string) => {
        parsed.push(url);
        return Promise.resolve({ mods: [{ id: url.split("/").at(-1) ?? "", version: null }], game: null, disc: null });
      }),
      indexFetch: vi.fn(() => Promise.reject(new Error("not in this test"))),
    },
  };
});

const { changeSet, close, receive, startDeepLinks } = await import("./deepLink.svelte");

const flush = () => new Promise((r) => setTimeout(r, 0));

describe("deep links", () => {
  beforeEach(() => {
    parsed.length = 0;
    changeSet.link = null;
    changeSet.queued = null;
    changeSet.phase = "idle";
  });

  it("handles the cold start's link from getCurrent, and later ones from onOpenUrl", async () => {
    let send: (urls: string[]) => void = () => undefined;
    await startDeepLinks({
      getCurrent: () => Promise.resolve(["openrac://install/first"]),
      onOpenUrl: (handler) => {
        send = handler;
        return Promise.resolve(() => undefined);
      },
    });
    await flush();
    expect(changeSet.link).toBe("openrac://install/first");
    expect(parsed).toEqual(["openrac://install/first"]);
    send(["openrac://install/second"]);
    expect(changeSet.link).toBe("openrac://install/first");
    expect(changeSet.queued).toBe("openrac://install/second");
  });

  it("keeps one link waiting while a change set is open, and drops a flood", async () => {
    receive("openrac://install/a");
    await flush();
    receive("openrac://install/b");
    receive("openrac://install/c");
    receive("openrac://install/d");
    expect(changeSet.queued).toBe("openrac://install/b");
    // The same link again is not queued behind itself.
    close();
    await flush();
    expect(changeSet.link).toBe("openrac://install/b");
    receive("openrac://install/b");
    expect(changeSet.queued).toBeNull();
    close();
    await flush();
    expect(changeSet.link).toBeNull();
    expect(parsed).toEqual(["openrac://install/a", "openrac://install/b"]);
  });

  it("ignores no link at all", () => {
    receive(null);
    receive("");
    expect(changeSet.link).toBeNull();
  });
});
