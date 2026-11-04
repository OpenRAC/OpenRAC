// The Mods tab's updates panel rendered (svelte/server): what waits with its reason and
// changelog, what is held back and why, and what just went in.
import { render } from "svelte/server";
import { beforeEach, describe, expect, it } from "vitest";
import ModUpdatesPanel from "../components/ModUpdatesPanel.svelte";
import type { VersionView } from "./api";
import { autoUpdates } from "./modAutoUpdate.svelte";
import { mockUpdates } from "./mockIndex";
import { UPDATES, holdText } from "./modsText";

const version = { key: "rac1/pal", game: "rac1", name: "pal" } as VersionView;
const html = () => render(ModUpdatesPanel, { props: { version } }).body.replace(/<!--[^>]*-->/g, "");
const unescape = (s: string) =>
  s
    .replace(/&#39;/g, "'")
    .replace(/&quot;/g, '"')
    .replace(/&amp;/g, "&");

describe("the updates panel", () => {
  beforeEach(() => {
    autoUpdates.plans = {};
    autoUpdates.recent = {};
  });

  it("is not there with nothing to say", () => {
    autoUpdates.plans.rac1 = { silent: [], waiting: [], held: [] };
    expect(html().trim()).toBe("");
  });

  it("shows what waits for a click, with why and its changelog, and what is held back", () => {
    const plan = mockUpdates();
    autoUpdates.plans.rac1 = plan;
    const out = unescape(html());
    expect(out).toContain(UPDATES.waitingTitle);
    expect(out).toContain(holdText(plan.waiting[0]?.hold ?? null));
    expect(out).toContain(UPDATES.changelog);
    expect(out).toContain(UPDATES.heldTitle);
    expect(out).toContain("Needs a newer launcher (mod API 1.11)");
    expect(out).toMatch(/<button class="small"[^>]*>[\s\S]*?Update<\/button>/);
  });

  it("says what just went in", () => {
    autoUpdates.plans.rac1 = { silent: [], waiting: [], held: [] };
    autoUpdates.recent.rac1 = mockUpdates().silent;
    const out = html();
    expect(out).toContain(UPDATES.recentTitle);
    expect(out).toContain("1.1.0 → 1.2.0");
  });
});

describe("hold reasons", () => {
  it("read in a player's words", () => {
    expect(holdText({ kind: "heldBack", by: "user", byName: "Camera tweaks", wants: "=1.2.0" })).toBe(
      "Held back: Camera tweaks needs =1.2.0.",
    );
    expect(holdText({ kind: "major" })).toMatch(/major version/);
    expect(holdText(null)).toBe("");
  });
});
