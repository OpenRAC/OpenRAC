import { describe, expect, it } from "vitest";
import { news } from "./news";

describe("news", () => {
  it("is empty when nothing changed", () => {
    expect(news({ update: null, mods: [], games: [] })).toEqual([]);
  });

  it("lists the update, then mods, then crashes", () => {
    const list = news({
      update: { version: "0.10.0" },
      mods: [
        { game: "rac1", version: "pal", title: "Ratchet & Clank", updates: [{}, {}] },
        { game: "rac2", version: "ntsc", title: "Going Commando", updates: [] },
        { game: "rac3", version: "ntsc", title: "Up Your Arsenal", updates: [{}] },
      ],
      games: [
        { id: "rac1", title: "Ratchet & Clank", version: "rac1/pal", crashes: 1 },
        { id: "rac2", title: "Going Commando", version: "rac2/ntsc", crashes: 0 },
        { id: "rac3", title: "Up Your Arsenal", version: null, crashes: 2 },
      ],
    });
    expect(list.map((n) => n.key)).toEqual(["update", "mods-rac1", "mods-rac3", "crash-rac1"]);
    expect(list[0]?.title).toBe("OpenRAC 0.10.0 is ready");
    expect(list[1]).toMatchObject({ title: "2 mod updates", text: "Ratchet & Clank", version: "rac1/pal" });
    expect(list[2]?.title).toBe("1 mod update");
    expect(list[3]).toMatchObject({ kind: "crash", game: "rac1", version: "rac1/pal" });
  });
});
