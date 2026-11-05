// What a mod's page (core/src/mod_tools.rs) may ask the launcher, by messages from its frame
// (ModTool.svelte). The page is the mod's code, so every message is checked here first.
//
// The page sends   { openrac: 1, id, call, args }
// and gets back    { openrac: 1, id, ok: true, value }  or  { openrac: 1, id, ok: false, error }
//
// | call      | args                                   | value                                         |
// |-----------|----------------------------------------|-----------------------------------------------|
// | info      |                                        | the game, its disc, its texture pack's id, the mod, the system |
// | list      | root ("levels" or "dump")              | the files there: [{ path, size }]             |
// | read      | root, path, start?, length?            | the file's bytes, or that part (ArrayBuffer)  |
// | storeGet  | key                                    | what the mod stored under key, or null        |
// | storeSet  | key, value (null removes it)           | null                                          |
// | save      | name, extension ("zip", "json"), bytes | the name it was saved under, "" if backed out |
// | saveStart | name, extension                        | a number for the parts, 0 if backed out       |
// | saveWrite | token, bytes                           | null (the parts in order)                     |
// | saveEnd   | token, keep                            | the name it was saved under, "" if dropped    |
// | open      | extension ("json")                     | the bytes the player picked, null if backed out |
// | close     |                                        | (none: the launcher goes back to the mod)     |
import { api, type ToolFile, type ToolRoot } from "./api";

export const PROTOCOL = 1;

/** What the page is told it runs in. */
export interface ToolInfo {
  game: string;
  version: string;
  /** The texture pack's id the game knows the disc by (`rac1-pal`): a pack's manifest names it. */
  packId: string;
  mod: { id: string; name: string; version: string };
  /** "android" or "desktop". */
  system: string;
}

export type ToolRequest =
  | { id: number; call: "info" }
  | { id: number; call: "list"; root: ToolRoot }
  | { id: number; call: "read"; root: ToolRoot; path: string; start?: number; length?: number }
  | { id: number; call: "storeGet"; key: string }
  | { id: number; call: "storeSet"; key: string; value: unknown }
  | { id: number; call: "save"; name: string; extension: "zip" | "json"; bytes: ArrayBuffer }
  | { id: number; call: "saveStart"; name: string; extension: "zip" | "json" }
  | { id: number; call: "saveWrite"; token: number; bytes: ArrayBuffer }
  | { id: number; call: "saveEnd"; token: number; keep: boolean }
  | { id: number; call: "open"; extension: "json" };

const ROOTS: readonly string[] = ["levels", "dump"];
const KEY = /^[a-z0-9_-]{1,64}$/;

const isObject = (v: unknown): v is Record<string, unknown> => typeof v === "object" && v !== null;
const count = (v: unknown): v is number => typeof v === "number" && Number.isSafeInteger(v) && v >= 0;
/** A read's part, when it asks for one; null when start or length is there but not a count. */
function range(args: Record<string, unknown>): { start?: number; length?: number } | null {
  const bad = (v: unknown) => v !== undefined && v !== null && !count(v);
  if (bad(args.start) || bad(args.length)) return null;
  return { ...(count(args.start) && { start: args.start }), ...(count(args.length) && { length: args.length }) };
}

/** The most a stored value takes, as JSON (core/src/mod_tools.rs, STORE_MAX). */
const STORE_MAX = 1 << 20;

/** `value` as plain JSON (what the store keeps), or undefined when it is not JSON or too large. */
function plainJson(value: unknown): unknown {
  try {
    const text = JSON.stringify(value ?? null) as string | undefined; // undefined for a function
    return text !== undefined && text.length <= STORE_MAX ? JSON.parse(text) : undefined;
  } catch {
    return undefined; // a cycle, or a BigInt
  }
}
const isToken = (v: unknown): v is number => typeof v === "number" && Number.isSafeInteger(v) && v > 0;
const text = (v: unknown, most = 512): v is string => typeof v === "string" && v.length > 0 && v.length <= most;

/** A message from the page as one of the calls, or why it is not one (null: not a message of ours). */
export function readRequest(data: unknown): ToolRequest | { id: number; error: string } | null {
  if (!isObject(data) || data.openrac !== PROTOCOL || "ok" in data) return null;
  const id = data.id;
  if (typeof id !== "number" || !Number.isSafeInteger(id)) return null;
  const args = isObject(data.args) ? data.args : {};
  const refuse = (error: string) => ({ id, error });
  switch (data.call) {
    case "info":
      return { id, call: "info" };
    case "list":
      return ROOTS.includes(args.root as string)
        ? { id, call: "list", root: args.root as ToolRoot }
        : refuse("root is levels or dump");
    case "read": {
      if (!ROOTS.includes(args.root as string)) return refuse("root is levels or dump");
      if (!text(args.path, 1024)) return refuse("no path");
      const part = range(args);
      if (!part) return refuse("start and length are whole numbers from 0");
      return { id, call: "read", root: args.root as ToolRoot, path: args.path, ...part };
    }
    case "storeGet":
      return text(args.key) && KEY.test(args.key) ? { id, call: "storeGet", key: args.key } : refuse("a bad key");
    case "storeSet": {
      if (!text(args.key) || !KEY.test(args.key)) return refuse("a bad key");
      const value = plainJson(args.value);
      if (value === undefined) return refuse("the value is not JSON, or larger than 1 MB");
      return { id, call: "storeSet", key: args.key, value };
    }
    case "save":
      if (args.extension !== "zip" && args.extension !== "json") return refuse("extension is zip or json");
      if (!(args.bytes instanceof ArrayBuffer) || args.bytes.byteLength === 0) return refuse("no bytes to save");
      return text(args.name, 200)
        ? { id, call: "save", name: args.name, extension: args.extension, bytes: args.bytes }
        : refuse("no name");
    case "saveStart":
      if (args.extension !== "zip" && args.extension !== "json") return refuse("extension is zip or json");
      return text(args.name, 200)
        ? { id, call: "saveStart", name: args.name, extension: args.extension }
        : refuse("no name");
    case "saveWrite":
      if (!isToken(args.token)) return refuse("a bad token");
      if (!(args.bytes instanceof ArrayBuffer) || args.bytes.byteLength === 0) return refuse("no bytes to save");
      return { id, call: "saveWrite", token: args.token, bytes: args.bytes };
    case "saveEnd":
      if (!isToken(args.token)) return refuse("a bad token");
      return { id, call: "saveEnd", token: args.token, keep: args.keep === true };
    case "open":
      return args.extension === "json" ? { id, call: "open", extension: "json" } : refuse("extension is json");
    default:
      return refuse(`no call ${JSON.stringify(data.call)}`);
  }
}

/** The calls' work, so a test can stand in for the launcher. */
export interface ToolBackend {
  list(game: string, version: string, root: ToolRoot): Promise<ToolFile[]>;
  read(
    game: string,
    version: string,
    root: ToolRoot,
    path: string,
    start?: number,
    length?: number,
  ): Promise<ArrayBuffer>;
  storeGet(id: string, key: string): Promise<unknown>;
  storeSet(id: string, key: string, value: unknown): Promise<unknown>;
  save(id: string, name: string, extension: string, bytes: Uint8Array): Promise<string>;
  saveStart(id: string, name: string, extension: string): Promise<number>;
  saveWrite(token: number, bytes: Uint8Array): Promise<unknown>;
  saveEnd(token: number, keep: boolean): Promise<string>;
  open(extension: string): Promise<ArrayBuffer>;
}

export const launcherBackend: ToolBackend = {
  list: api.modToolList,
  read: api.modToolRead,
  storeGet: api.modToolStoreGet,
  storeSet: api.modToolStoreSet,
  save: api.modToolSave,
  saveStart: api.modToolSaveStart,
  saveWrite: api.modToolSaveWrite,
  saveEnd: api.modToolSaveEnd,
  open: api.modToolOpen,
};

/** A backend that remembers the files a page has not finished saving, so that closing the page
 *  drops them (nothing half-written is left). A page writes only to the files it started. */
export function trackSaves(backend: ToolBackend): ToolBackend & { dropUnfinished(): void } {
  const open = new Set<number>();
  const mine = (token: number) => {
    if (!open.has(token)) throw new Error("no such file being saved");
  };
  return {
    ...backend,
    async saveStart(id, name, extension) {
      const token = await backend.saveStart(id, name, extension);
      if (token) open.add(token);
      return token;
    },
    async saveWrite(token, bytes) {
      mine(token);
      try {
        return await backend.saveWrite(token, bytes);
      } catch (e) {
        open.delete(token); // the launcher dropped it (src-tauri/src/mod_tool.rs)
        throw e;
      }
    },
    async saveEnd(token, keep) {
      mine(token);
      open.delete(token);
      return backend.saveEnd(token, keep);
    },
    dropUnfinished() {
      for (const token of open) void backend.saveEnd(token, false).catch(() => undefined);
      open.clear();
    },
  };
}

/** The value a call answers with. */
export async function answer(request: ToolRequest, info: ToolInfo, backend: ToolBackend): Promise<unknown> {
  const { game, version } = info;
  switch (request.call) {
    case "info":
      return info;
    case "list":
      return backend.list(game, version, request.root);
    case "read":
      return backend.read(game, version, request.root, request.path, request.start, request.length);
    case "storeGet":
      return (await backend.storeGet(info.mod.id, request.key)) ?? null;
    case "storeSet":
      await backend.storeSet(info.mod.id, request.key, request.value);
      return null;
    case "save":
      return backend.save(info.mod.id, request.name, request.extension, new Uint8Array(request.bytes));
    case "saveStart":
      return backend.saveStart(info.mod.id, request.name, request.extension);
    case "saveWrite":
      await backend.saveWrite(request.token, new Uint8Array(request.bytes));
      return null;
    case "saveEnd":
      return backend.saveEnd(request.token, request.keep);
    case "open": {
      const bytes = await backend.open(request.extension);
      return bytes.byteLength ? bytes : null;
    }
  }
}

/** The reply to a message: the answer, or the error; with the bytes to hand over, not copy. */
export async function reply(
  data: unknown,
  info: ToolInfo,
  backend: ToolBackend,
): Promise<{ message: Record<string, unknown>; transfer: Transferable[] } | null> {
  const request = readRequest(data);
  if (!request) return null;
  if ("error" in request) {
    return { message: { openrac: PROTOCOL, id: request.id, ok: false, error: request.error }, transfer: [] };
  }
  try {
    const value = await answer(request, info, backend);
    const transfer = value instanceof ArrayBuffer ? [value] : [];
    return { message: { openrac: PROTOCOL, id: request.id, ok: true, value }, transfer };
  } catch (e) {
    const error = typeof e === "string" ? e : e instanceof Error ? e.message : String(e);
    return { message: { openrac: PROTOCOL, id: request.id, ok: false, error }, transfer: [] };
  }
}

/** The address a mod's page is served at (src-tauri/src/mod_tool.rs), each part escaped. On Windows
 *  and Android the scheme is reached as http://openrac-mod.localhost/. */
export function pageUrl(base: string, game: string, id: string, page: string): string {
  const parts = [game, id, ...page.split("/")].map(encodeURIComponent);
  return `${base.replace(/\/+$/, "")}/${parts.join("/")}`;
}

/** The site a page at `url` is: `http://openrac-mod.localhost`, or `openrac-mod://localhost` (the
 *  URL API gives a custom scheme's address no origin, "null", which postMessage cannot target). */
export function pageOrigin(url: string): string {
  const u = new URL(url);
  return u.protocol === "http:" || u.protocol === "https:" ? u.origin : `${u.protocol}//${u.host}`;
}
