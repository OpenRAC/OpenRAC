import { describe, expect, it } from "vitest";

import {
  levelName,
  levelsOf,
  regionLabel,
  shortTitle,
  titleLines,
  withShownTitles,
} from "./labels";

describe("labels", () => {
  it("shortens a game's name for a narrow list", () => {
    expect(shortTitle("Ratchet & Clank")).toBe("Ratchet & Clank");
    expect(shortTitle("Ratchet & Clank: Going Commando")).toBe("Going Commando");
    expect(shortTitle("Ratchet: Deadlocked")).toBe("Deadlocked");
    expect(shortTitle("Ratchet & Clank: Size Matters")).toBe("Size Matters");
    expect(shortTitle("Ratchet & Clank: Secret Agent Clank")).toBe("Secret Agent Clank");
  });

  it("lists each game's levels by their numbers", () => {
    expect(levelsOf("rac1")).toHaveLength(19);
    expect(levelsOf("rac1")[18]).toEqual({ number: 18, name: "Veldin (the return)" });
    // The later games number their levels as their level headers do: gaps, and Going Commando's museum is 30.
    expect(levelsOf("rac2").map((l) => l.number)).toContain(30);
    expect(levelsOf("rac3")[0]).toEqual({ number: 1, name: "Veldin" });
    expect(levelsOf("rac4")).toHaveLength(47);
    expect(levelName("rac2", 30)).toBe("Insomniac Museum");
    expect(levelName("rac4", 99)).toBe("Level 99");
  });

  it("names regions", () => {
    expect(regionLabel("ntsc")).toBe("NTSC-U");
    expect(regionLabel("pal")).toBe("PAL");
    expect(regionLabel("pal1")).toBe("PAL v1.00");
    expect(regionLabel("ntsc2")).toBe("NTSC-U v2.00");
    expect(regionLabel("jp")).toBe("NTSC-J");
    expect(regionLabel("ntsc-j")).toBe("NTSC-J");
  });
});

describe("titleLines", () => {
  it("puts the game's own name on a second line", () => {
    expect(titleLines("Ratchet & Clank")).toEqual({ series: "Ratchet & Clank", game: "" });
    expect(titleLines("Ratchet & Clank: Going Commando")).toEqual({
      series: "Ratchet & Clank:",
      game: "Going Commando",
    });
    expect(titleLines("Ratchet: Deadlocked")).toEqual({ series: "Ratchet: Deadlocked", game: "" });
  });
});

describe("withShownTitles", () => {
  it("shows Secret Agent Clank without the series", () => {
    const game = (title: string) => ({
      id: "x",
      title,
      year: null,
      install: null,
      versions: [{ title }],
    });
    const library = {
      games: [game("Ratchet & Clank: Secret Agent Clank"), game("Ratchet & Clank: Going Commando")],
    } as never;
    const [sac, rac2] = withShownTitles(library).games;

    expect(sac?.title).toBe("Secret Agent Clank");
    expect(sac?.versions[0]?.title).toBe("Secret Agent Clank");
    expect(rac2?.title).toBe("Ratchet & Clank: Going Commando");
  });
});
