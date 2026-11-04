import { describe, expect, it } from "vitest";
import { pageOrigin, pageUrl, readRequest, reply, type ToolBackend, type ToolInfo } from "./modTool";

const info: ToolInfo = {
  game: "rac1",
  version: "pal",
  packId: "rac1-pal",
  mod: { id: "nanotech-upscaler", name: "Nanotech Upscaler", version: "0.1.0" },
  system: "desktop",
};

/** A backend that records what it was asked. */
function backend(): ToolBackend & { asked: unknown[][] } {
  const asked: unknown[][] = [];
  return {
    asked,
    list: (...a) => (asked.push(["list", ...a]), Promise.resolve([{ path: "level_00/textures/a.png", size: 3 }])),
    read: (...a) => (asked.push(["read", ...a]), Promise.resolve(new Uint8Array([1, 2, 3]).buffer)),
    storeGet: (...a) => (asked.push(["storeGet", ...a]), Promise.resolve(undefined)),
    storeSet: (...a) => (asked.push(["storeSet", ...a]), Promise.resolve(null)),
    save: (id, name, extension, bytes) => (
      asked.push(["save", id, name, extension, bytes.byteLength]),
      Promise.resolve("~/Nanotech.zip")
    ),
    open: () => Promise.resolve(new ArrayBuffer(0)),
  };
}

describe("a mod page's messages", () => {
  it("leaves out what is not one of ours", () => {
    expect(readRequest(null)).toBeNull();
    expect(readRequest("info")).toBeNull();
    expect(readRequest({ openrac: 2, id: 1, call: "info" })).toBeNull();
    expect(readRequest({ openrac: 1, call: "info" })).toBeNull();
    expect(readRequest({ openrac: 1, id: 1.5, call: "info" })).toBeNull();
    // Our own answers, echoed back, are not requests.
    expect(readRequest({ openrac: 1, id: 1, ok: true, value: 1 })).toBeNull();
  });

  it("refuses calls and arguments that are not allowed", () => {
    expect(readRequest({ openrac: 1, id: 1, call: "run", args: {} })).toEqual({ id: 1, error: 'no call "run"' });
    expect(readRequest({ openrac: 1, id: 2, call: "list", args: { root: "saves" } })).toHaveProperty("error");
    expect(readRequest({ openrac: 1, id: 3, call: "read", args: { root: "levels" } })).toHaveProperty("error");
    expect(readRequest({ openrac: 1, id: 4, call: "storeSet", args: { key: "../x", value: 1 } })).toHaveProperty(
      "error",
    );
    expect(
      readRequest({
        openrac: 1,
        id: 5,
        call: "save",
        args: { name: "a", extension: "exe", bytes: new ArrayBuffer(1) },
      }),
    ).toHaveProperty("error");
    expect(
      readRequest({
        openrac: 1,
        id: 6,
        call: "save",
        args: { name: "a", extension: "zip", bytes: new ArrayBuffer(0) },
      }),
    ).toHaveProperty("error");
  });

  it("answers with the game it runs for, never one the page names", async () => {
    const b = backend();
    const listed = await reply({ openrac: 1, id: 7, call: "list", args: { root: "levels", game: "rac2" } }, info, b);
    expect(listed?.message).toEqual({
      openrac: 1,
      id: 7,
      ok: true,
      value: [{ path: "level_00/textures/a.png", size: 3 }],
    });
    expect(b.asked[0]).toEqual(["list", "rac1", "pal", "levels"]);
  });

  it("hands bytes over instead of copying them", async () => {
    const read = await reply(
      { openrac: 1, id: 8, call: "read", args: { root: "dump", path: "Interface/a.png" } },
      info,
      backend(),
    );
    expect(read?.transfer).toHaveLength(1);
    expect(read?.message.value).toBeInstanceOf(ArrayBuffer);
  });

  it("stores under the mod's own id and saves its files", async () => {
    const b = backend();
    expect(
      (await reply({ openrac: 1, id: 9, call: "storeGet", args: { key: "presets" } }, info, b))?.message.value,
    ).toBe(null);
    await reply({ openrac: 1, id: 10, call: "storeSet", args: { key: "presets", value: [1] } }, info, b);
    const saved = await reply(
      { openrac: 1, id: 11, call: "save", args: { name: "Nanotech", extension: "zip", bytes: new ArrayBuffer(4) } },
      info,
      b,
    );
    expect(saved?.message.value).toBe("~/Nanotech.zip");
    expect(b.asked).toEqual([
      ["storeGet", "nanotech-upscaler", "presets"],
      ["storeSet", "nanotech-upscaler", "presets", [1]],
      ["save", "nanotech-upscaler", "Nanotech", "zip", 4],
    ]);
  });

  it("says why a call failed", async () => {
    const b = backend();
    b.read = () => Promise.reject(new Error("level_00/x.png is not there"));
    const failed = await reply({ openrac: 1, id: 12, call: "read", args: { root: "levels", path: "x" } }, info, b);
    expect(failed?.message).toEqual({ openrac: 1, id: 12, ok: false, error: "level_00/x.png is not there" });
  });

  it("an open the player backed out of is null", async () => {
    const opened = await reply({ openrac: 1, id: 13, call: "open", args: { extension: "json" } }, info, backend());
    expect(opened?.message.value).toBeNull();
  });
});

describe("a mod page's address", () => {
  it("escapes each part", () => {
    expect(pageUrl("openrac-mod://localhost/", "rac1", "nanotech-upscaler", "ui/index.html")).toBe(
      "openrac-mod://localhost/rac1/nanotech-upscaler/ui/index.html",
    );
    expect(pageUrl("http://openrac-mod.localhost", "rac1", "a", "my page/index.html")).toBe(
      "http://openrac-mod.localhost/rac1/a/my%20page/index.html",
    );
  });
});

describe("a mod page's site", () => {
  it("is the scheme and host", () => {
    expect(pageOrigin("openrac-mod://localhost/rac1/a/ui/index.html")).toBe("openrac-mod://localhost");
    expect(pageOrigin("http://openrac-mod.localhost/rac1/a/ui/index.html")).toBe("http://openrac-mod.localhost");
  });
});
