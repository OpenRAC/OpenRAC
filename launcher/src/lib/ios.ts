// The shared Svelte launcher in the iOS application's WKWebView. UIKit owns Files and the
// native engine; the filesystem commands use the same Rust core as the desktop launcher.
import type { UnlistenFn } from "@tauri-apps/api/event";

export const onIOS = typeof window !== "undefined" && "__OPENRAC_IOS__" in window;

type NativeWindow = Window & {
  webkit: { messageHandlers: { openrac: { postMessage(value: unknown): void } } };
  __openracReply?: (id: number, ok: boolean, value: unknown) => void;
};
const pending = new Map<number, { resolve(value: unknown): void; reject(error: Error): void }>();
let next = 0;

export function iosInvoke<T>(command: string, args?: unknown): Promise<T> {
  const native = window as unknown as NativeWindow;
  native.__openracReply ??= (id, ok, value) => {
    const request = pending.get(id);
    if (!request) return;
    pending.delete(id);
    if (ok) request.resolve(value);
    else request.reject(new Error(String(value)));
  };
  return new Promise<T>((resolve, reject) => {
    const id = ++next;
    pending.set(id, {
      resolve: (value) => {
        resolve(value as T);
      },
      reject,
    });
    try {
      // WebKit must receive plain JSON, including nested Svelte state proxies such as Scope.
      const request: unknown = JSON.parse(JSON.stringify({ id, command, args: args ?? {} }));
      native.webkit.messageHandlers.openrac.postMessage(request);
    } catch (error) {
      pending.delete(id);
      reject(error instanceof Error ? error : new Error(String(error)));
    }
  });
}

export function iosListen(name: string, handler: (event: { payload: unknown }) => void): Promise<UnlistenFn> {
  const listener = (event: Event) => {
    const detail = (event as CustomEvent<{ name: string; payload: unknown }>).detail;
    if (detail.name === name) handler({ payload: detail.payload });
  };
  window.addEventListener("openrac-event", listener);
  return Promise.resolve(() => {
    window.removeEventListener("openrac-event", listener);
  });
}

export const iosOpen = (options?: unknown) => iosInvoke<string | string[] | null>("dialog_open", { options });
export const iosConfirm = (message: string, options?: unknown) =>
  iosInvoke<boolean>("dialog_confirm", { message, options });
