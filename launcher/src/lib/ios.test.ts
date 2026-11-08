import { afterEach, describe, expect, it, vi } from "vitest";
import { iosInvoke, iosListen } from "./ios";

afterEach(() => vi.unstubAllGlobals());

describe("native iOS bridge", () => {
  it("matches out-of-order native replies to their requests and reports failures", async () => {
    const sent: { id: number; command: string; args: unknown }[] = [];
    const native = {
      webkit: { messageHandlers: { openrac: { postMessage: (request: (typeof sent)[number]) => sent.push(request) } } },
      __openracReply: undefined as ((id: number, ok: boolean, value: unknown) => void) | undefined,
    };
    vi.stubGlobal("window", native);
    const first = iosInvoke<string>("library");
    const second = iosInvoke<string>("install_game", { game: "rac1" });
    const failure = expect(second).rejects.toThrow("Invalid disc");
    const reply = native.__openracReply;
    const [request1, request2] = sent;
    if (!reply || !request1 || !request2) throw new Error("Native requests were not sent");
    reply(request2.id, false, "Invalid disc");
    reply(request1.id, true, "catalogue");
    reply(request1.id, true, "duplicate");
    expect(await first).toBe("catalogue");
    await failure;
    expect(sent[1]).toMatchObject({ command: "install_game", args: { game: "rac1" } });
  });

  it("sends a plain snapshot of nested reactive state to WebKit", async () => {
    const scope = new Proxy({ kind: "version", key: "rac1/ntsc" }, {});
    const postMessage = vi.fn();
    const native = {
      webkit: { messageHandlers: { openrac: { postMessage } } },
      __openracReply: undefined as ((id: number, ok: boolean, value: unknown) => void) | undefined,
    };
    vi.stubGlobal("window", native);
    const result = iosInvoke("run_action", { scope, id: "play" });
    const request = postMessage.mock.calls[0]?.[0] as { id: number; args: { scope: typeof scope } };
    expect(request.args.scope).not.toBe(scope);
    scope.key = "rac2/ntsc";
    expect(request.args.scope.key).toBe("rac1/ntsc");
    native.__openracReply?.(request.id, true, null);
    await result;
  });

  it("filters native events and removes the listener when its page leaves", async () => {
    const target = new EventTarget();
    vi.stubGlobal("window", target);
    const handler = vi.fn();
    const off = await iosListen("job-exit", handler);
    const emit = (name: string) =>
      target.dispatchEvent(Object.assign(new Event("openrac-event"), { detail: { name, payload: { id: 7 } } }));
    emit("job-output");
    emit("job-exit");
    expect(handler).toHaveBeenCalledExactlyOnceWith({ payload: { id: 7 } });
    off();
    emit("job-exit");
    expect(handler).toHaveBeenCalledTimes(1);
  });
});
