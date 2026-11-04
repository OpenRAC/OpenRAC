// The change set rendered (svelte/server, no window), on plans from the preview's resolver over
// the shared fixture: one button when nothing needs a choice, the native warning, a blocking
// conflict with its fix, Switch to the dev build, and Play at the end.
import { render } from "svelte/server";
import { beforeEach, describe, expect, it } from "vitest";
import DeepLinkInstall from "../components/DeepLinkInstall.svelte";
import type { Config } from "./api";
import { app } from "./app.svelte";
import { changeSet } from "./deepLink.svelte";
import { mockResolve } from "./mockIndex";
import { CHANGE_SET, installCount, nativeWarning } from "./modsText";

const html = () => render(DeepLinkInstall).body.replace(/<!--[^>]*-->/g, "");

function show(wanted: string[], turnOff: string[] = [], api: [number, number] = [1, 10]) {
  const ws = wanted.map((id) => ({ id, version: null }));
  const plan = mockResolve(ws, turnOff, api);
  Object.assign(changeSet, {
    link: "openrac://install/x",
    phase: "ready",
    wanted: ws,
    game: "rac1",
    version: "pal",
    title: "Ratchet & Clank",
    plan,
    offers: { recommends: plan.recommends, suggests: plan.suggests },
    ticked: plan.recommends.map((o) => o.id),
  });
  return plan;
}

describe("the change set", () => {
  beforeEach(() => {
    app.config = { setupComplete: true } as Config;
    changeSet.result = null;
    changeSet.progress = {};
  });

  it("is one Install button when nothing needs a choice", () => {
    show(["ammo-max"]);
    const out = html();
    expect(out).toContain(installCount(1));
    expect(out).not.toContain("outside the game's sandbox");
    expect(out).not.toContain(CHANGE_SET.switchToDev);
    expect(out).not.toContain('role="alert"');
  });

  it("warns once about native code, naming the mods", () => {
    show(["lock-strafe"]);
    const out = html();
    expect(out).toContain(CHANGE_SET.alsoInstalls);
    expect(out.match(/outside the game(&#39;|')s sandbox/g)).toHaveLength(1);
    expect(out).toContain(nativeWarning(["Lock-strafe"]).split(" runs")[0]);
    expect(out).toContain(CHANGE_SET.recommended);
  });

  it("blocks on a conflict and offers its fix", () => {
    // The preview's ?conflict is not set here: the conflict is within the change set.
    const plan = show(["lock-strafe", "classic-controls"]);
    expect(plan.blocked).toBe(true);
    const out = html();
    expect(out).toMatch(/can(&#39;|')t be on with/);
    expect(out).toContain(CHANGE_SET.blocked);
    expect(out).toMatch(/<button class="primary"[^>]*disabled/);
  });

  it("offers Switch to the dev build when no version fits", () => {
    show(["archipelago"]);
    const out = html();
    expect(out).toContain(CHANGE_SET.switchToDev);
    expect(out).not.toContain(installCount(1));
  });

  it("ends with Play and the count", () => {
    show(["ammo-max"]);
    changeSet.phase = "done";
    changeSet.result = {
      done: ["ammo-max"],
      packs: [],
      failed: null,
      skipped: [],
      notOn: [],
      backup: "/b",
      rolledBack: false,
    };
    const out = html();
    expect(out).toContain(CHANGE_SET.play.replace("▸", ""));
    expect(out).toContain("Installed 1 mod");
    expect(out).toContain(CHANGE_SET.backedUp);
  });

  it("says how far it got when one failed", () => {
    show(["lock-strafe"]);
    changeSet.phase = "done";
    changeSet.result = {
      done: ["camera-tweaks"],
      packs: [],
      failed: { kind: "upstreamBad", modId: "lock-strafe", detail: "sha", action: null },
      skipped: [],
      notOn: [],
      backup: null,
      rolledBack: false,
    };
    const out = html();
    expect(out).toMatch(/Installed 1 of \d+\. Lock-strafe failed/);
    expect(out).toMatch(/couldn(&#39;|')t be verified/);
  });

  it("shows nothing until there is a link", () => {
    changeSet.link = null;
    expect(html().trim()).toBe("");
  });
});
