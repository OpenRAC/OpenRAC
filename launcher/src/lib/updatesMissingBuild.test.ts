import { afterEach, describe, expect, it, vi } from "vitest";

// A desktop launcher whose updater finds a newer release with no build for this system.
vi.mock("./api", async (actual) => ({
  ...(await actual<typeof import("./api")>()),
  inTauri: true,
  onAndroid: false,
  api: { launcherUpdateCheck: vi.fn().mockRejectedValue("no build for this platform") },
}));

const { checkForUpdate } = await import("./updates");

describe("a release without a build for this system", () => {
  const source = {
    repository: "OpenRAC/OpenRAC",
    fork: false,
    channel: null,
    manifest: "",
    page: "https://github.com/OpenRAC/OpenRAC/releases",
  };

  afterEach(() => vi.unstubAllGlobals());

  it("is announced from GitHub's release, with nothing to install", async () => {
    const body = JSON.stringify({ tag_name: "launcher-v0.9.0", name: "OpenRAC Launcher 0.9.0" });

    vi.stubGlobal(
      "fetch",
      vi.fn(() => Promise.resolve(new Response(body, { status: 200 }))),
    );
    const update = await checkForUpdate("0.8.0", source);

    expect(update?.version).toBe("0.9.0");
    expect(update?.missingBuild).toBe(true);
    expect(update?.canInstallDirectly).toBe(false);
  });

  it("is nothing when GitHub's release is not newer either", async () => {
    const body = JSON.stringify({ tag_name: "launcher-v0.8.0", name: "OpenRAC Launcher 0.8.0" });

    vi.stubGlobal(
      "fetch",
      vi.fn(() => Promise.resolve(new Response(body, { status: 200 }))),
    );
    expect(await checkForUpdate("0.8.0", source)).toBeNull();
  });
});
