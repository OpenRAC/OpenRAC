import { beforeAll, describe, expect, it, vi } from "vitest";

import type { Config } from "./generated/Config";
import type { Library } from "./generated/Library";
import { mockCall } from "./mock";
import { RAC3_GATE } from "./rac3Gate";

beforeAll(() => {
  vi.stubGlobal("window", {});
});

async function actionsWith(rac3Dev: boolean) {
  const config = (await mockCall("get_config")) as Config;

  await mockCall("save_config", { config: { ...config, rac3Dev } });

  const library = (await mockCall("library")) as Library;
  const byVersion = new Map<string, { runnable: boolean; blockers: string[] }[]>();

  for (const game of library.games) for (const v of game.versions) byVersion.set(v.key, v.actions);

  return byVersion;
}

describe("the Up Your Arsenal switch gates rac3's actions", () => {
  it("blocks every action of rac3/pal and rac3/ntsc, with a note, while it is off", async () => {
    const versions = await actionsWith(false);

    for (const key of ["rac3/pal", "rac3/ntsc"]) {
      const actions = versions.get(key) ?? [];

      expect(actions.length, key).toBeGreaterThan(0);

      for (const a of actions) {
        expect(a.runnable, key).toBe(false);
        expect(a.blockers, key).toEqual([RAC3_GATE]);
      }
    }
  });

  it("lifts the gate when it is on", async () => {
    const versions = await actionsWith(true);

    for (const key of ["rac3/pal", "rac3/ntsc"])
      for (const a of versions.get(key) ?? []) expect(a.blockers, key).not.toContain(RAC3_GATE);
  });

  it("never touches rac1, rac2 or rac4, on or off", async () => {
    for (const on of [false, true]) {
      const versions = await actionsWith(on);

      for (const [key, actions] of versions) {
        if (key.startsWith("rac3/")) continue;

        for (const a of actions) expect(a.blockers, key).not.toContain(RAC3_GATE);
      }
    }
  });
});
