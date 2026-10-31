// "Share my setup": the mods that are on and in the index, as openrac.dev's set link.
import { describe, expect, it } from "vitest";
import type { PluginView } from "./api";
import { SET_URL, shareLink } from "./shareSetup";

const mod = (id: string, version: string, enabled = true, kind: PluginView["kind"] = "script") =>
  ({ id, version, enabled, kind }) as PluginView;

describe("shareLink", () => {
  it("lists the mods that are on, in the index, with exact versions, the game and disc", () => {
    const mods = [
      mod("lock-strafe", "1.2.0"),
      mod("camera-base", "1.1.0"),
      mod("off-one", "1.0.0", false),
      mod("mine", "1.0.0"),
    ];
    const link = shareLink(mods, (id) => id !== "mine", "rac1", "pal");
    expect(link).toBe(`${SET_URL}?m=lock-strafe@1.2.0,camera-base@1.1.0&g=rac1&d=pal`);
    const url = new URL(link ?? "");
    expect(url.searchParams.get("m")?.split(",")).toHaveLength(2);
  });

  it("leaves out built-ins and odd versions, keeps at most 32, and is null when nothing is left", () => {
    expect(shareLink([mod("enhanced", "1.0.0", true, "built-in")], () => true, "rac1", "pal")).toBeNull();
    expect(shareLink([mod("x", "latest")], () => true, "rac1", "pal")).toBeNull();
    const many = Array.from({ length: 40 }, (_, i) => mod(`m${i}`, "1.0.0"));
    const link = shareLink(many, () => true, "rac1", "ntsc") ?? "";
    expect(new URL(link).searchParams.get("m")?.split(",")).toHaveLength(32);
    expect(link.endsWith("&g=rac1&d=ntsc")).toBe(true);
  });
});
