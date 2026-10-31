// Browse's presets and what installs here, on the shared fixture.
import { describe, expect, it } from "vitest";
import { mockIndex } from "./mockIndex";
import { imageUrl, inPreset, PRESETS, siteUrl, upgradeable, versionHere, youtube, type Row } from "./modBrowse";

const index = mockIndex();
const revoked = (id: string, v: string) =>
  index.revoked.some((r) => r.id === id && (r.versions.includes("*") || r.versions.includes(v)));
const here = {
  game: "rac1",
  disc: "pal",
  platform: "linux-x86_64",
  api: [1, 10] as [number, number],
  prerelease: false,
};
function mod(id: string) {
  const m = index.mods.find((x) => x.id === id);
  if (!m) throw new Error(`no ${id} in the fixture`);
  return m;
}
const row = (id: string, installed: string | null = null, h = here): Row => ({
  mod: mod(id),
  here: versionHere(mod(id), h, revoked),
  installed,
});

describe("what installs here", () => {
  it("is the newest version that fits the API, never a revoked or test one", () => {
    expect(row("item-randomizer").here?.version).toBe("1.3.0");
    expect(row("item-randomizer", null, { ...here, api: [1, 11] }).here?.version).toBe("1.4.0");
    expect(row("item-randomizer", null, { ...here, api: [1, 11], prerelease: true }).here?.version).toBe(
      "2.0.0-beta.1",
    );
    expect(row("ammo-max").here?.version).toBe("1.0.1");
    expect(row("archipelago").here).toBeNull();
    expect(row("lock-strafe", null, { ...here, platform: "android-arm64" }).here).toBeNull();
    expect(row("camera-base", null, { ...here, platform: "android-arm64" }).here?.version).toBe("1.2.0");
  });
});

describe("presets", () => {
  it("sort rows into Compatible, Installed, Upgradeable, Not installed and All", () => {
    const rows = [row("camera-base", "1.1.0"), row("lock-strafe"), row("archipelago"), row("camera-tweaks", "2.0.0")];
    const by = (p: (typeof PRESETS)[number]) => rows.filter((r) => inPreset(r, p)).map((r) => r.mod.id);
    expect(by("compatible")).toEqual(["camera-base", "lock-strafe", "camera-tweaks"]);
    expect(by("installed")).toEqual(["camera-base", "camera-tweaks"]);
    expect(by("upgradeable")).toEqual(["camera-base"]);
    expect(by("notInstalled")).toEqual(["lock-strafe", "archipelago"]);
    expect(by("all")).toHaveLength(4);
    expect(upgradeable(row("camera-tweaks", "2.0.0"))).toBe(false);
  });
});

describe("media", () => {
  it("picks the smallest width that is enough, from the index's own address", () => {
    const icon = mod("ammo-max").media.banner;
    if (!icon) throw new Error("the fixture's ammo-max has a banner");
    expect(imageUrl(index.media, icon, 500)).toBe(`${index.media}${icon.hash}-960.webp`);
    expect(imageUrl(index.media, icon, 5000)).toBe(`${index.media}${icon.hash}-1600.webp`);
    expect(imageUrl("http://insecure/", icon, 10)).toBeNull();
    expect(imageUrl(index.media, { ...icon, hash: "../../x" }, 10)).toBeNull();
  });

  it("shows a video as its picture and a link, for real ids only", () => {
    expect(youtube("dQw4w9WgXcQ")).toEqual({
      thumb: "https://i.ytimg.com/vi/dQw4w9WgXcQ/hqdefault.jpg",
      watch: "https://www.youtube.com/watch?v=dQw4w9WgXcQ",
    });
    expect(youtube('x" onerror=1')).toBeNull();
    expect(siteUrl("lock-strafe")).toBe("https://openrac.dev/mods/lock-strafe");
  });
});
