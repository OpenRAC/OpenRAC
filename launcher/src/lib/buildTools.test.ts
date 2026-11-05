import { beforeEach, describe, expect, it, vi } from "vitest";

const mocks = vi.hoisted(() => {
  Object.defineProperty(globalThis, "window", { value: { __TAURI_INTERNALS__: {} }, configurable: true });
  return { invoke: vi.fn(), confirm: vi.fn() };
});
vi.mock("@tauri-apps/api/core", () => ({ invoke: mocks.invoke }));
vi.mock("@tauri-apps/plugin-dialog", () => ({ confirm: mocks.confirm, open: vi.fn(), save: vi.fn() }));

import { api } from "./api";

const scope = { kind: "version" as const, key: "rac2/ntsc" };
const explanation = "LLVM (Clang compiler)\nRemove later: Windows Settings > Apps > LLVM > Uninstall";

beforeEach(() => {
  mocks.invoke.mockReset();
  mocks.confirm.mockReset();
});

describe("build prerequisites consent", () => {
  it("does not prompt or install when tools are ready", async () => {
    mocks.invoke.mockResolvedValueOnce(null).mockResolvedValueOnce({ id: 1 });
    await api.runAction(scope, "port-build");
    expect(mocks.confirm).not.toHaveBeenCalled();
    expect(mocks.invoke).toHaveBeenLastCalledWith("run_action", expect.objectContaining({ installTools: false }));
  });

  it("shows the tool names and removal instructions before passing consent", async () => {
    mocks.invoke.mockResolvedValueOnce(explanation).mockResolvedValueOnce({ id: 1 });
    mocks.confirm.mockResolvedValue(true);
    await api.runAction(scope, "port-build");
    expect(mocks.confirm).toHaveBeenCalledWith(
      explanation,
      expect.objectContaining({ okLabel: "Install listed tools and build" }),
    );
    expect(mocks.invoke).toHaveBeenLastCalledWith("run_action", expect.objectContaining({ installTools: true }));
  });

  it("cancelling starts neither an installer nor a build", async () => {
    mocks.invoke.mockResolvedValueOnce(explanation);
    mocks.confirm.mockResolvedValue(false);
    await expect(api.runAction(scope, "port-rebuild")).rejects.toThrow("Build cancelled");
    expect(mocks.invoke).toHaveBeenCalledTimes(1);
    expect(mocks.invoke).toHaveBeenCalledWith("build_tools_check", { scope, id: "port-rebuild" });
  });

  it("does not continue after an unsuccessful check", async () => {
    mocks.invoke.mockRejectedValueOnce(new Error("check failed"));
    await expect(api.runAction(scope, "port-build")).rejects.toThrow("check failed");
    expect(mocks.invoke).toHaveBeenCalledTimes(1);
    expect(mocks.confirm).not.toHaveBeenCalled();
  });

  it("playing does not require compilers", async () => {
    mocks.invoke.mockResolvedValue({ id: 1 });
    await api.runAction(scope, "play");
    expect(mocks.invoke).toHaveBeenCalledTimes(1);
    expect(mocks.invoke).toHaveBeenCalledWith(
      "run_action",
      expect.objectContaining({ id: "play", installTools: false }),
    );
  });
});
