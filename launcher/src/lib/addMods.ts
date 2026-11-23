// Adding what the player chose or dropped on the Mods tab: a texture pack's .zip goes into the
// player's own texture pack (core/src/texture_packs.rs, added to it), a native mod to the game's mods
// (src-tauri/src/lib.rs, plugins_install), a data pack to its overrides packs (extractor/overrides.py,
// the version's overrides-add action), a shared map (.openrac-map) to OpenRAC Editor's import, which
// makes its project from the player's own disc (the editor's editor-import-map action). The player
// never has to know which is which.
import { api, type VersionView } from "./api";
import { jobs, runChain } from "./app.svelte";
import { added, TEXTURES, withName } from "./modsText";
import { refusesPack, textureCount } from "./texturePacks";
import { toolFailure } from "./toolErrors";

export interface Added {
  /** What happened, in a sentence. */
  text: string;
  failed: boolean;
  /** A native mod was added (the mods' list is read again). */
  plugin: boolean;
  /** A pack was added (the packs' list is read again). */
  pack: boolean;
}

/** Why the last add stopped, in a player's words (lib/toolErrors.ts): never the tool's own. */
function whyStopped(): string {
  const job = jobs.list
    .filter((j) => j.scope.kind === "version" && j.actionId === "overrides-add")
    .at(-1);
  const failure = toolFailure(job?.lines ?? []);

  return failure.line.charAt(0).toLowerCase() + failure.line.slice(1);
}

const nameOf = (path: string) => path.split(/[\\/]/).at(-1) ?? path;

/** A shared map made in OpenRAC Editor (its tools/shared_map.py). */
export const isMap = (path: string) => path.toLowerCase().endsWith(".openrac-map");

/** A shared map imported by OpenRAC Editor's tools, or why it cannot be. */
async function addMap(v: VersionView, path: string): Promise<Added> {
  const result: Added = { text: "", failed: true, plugin: false, pack: false };
  const importer = v.actions.find((a) => a.id === "editor-import-map" && a.thisPlatform) ?? null;

  if (!importer?.runnable) return { ...result, text: added.mapNeedsEditor(nameOf(path)) };
  const ok = await runChain({ kind: "version", key: v.key }, [importer], { path });

  if (!ok) {
    const job = jobs.list.filter((j) => j.scope.kind === "version" && j.actionId === "editor-import-map").at(-1);
    const failure = toolFailure(job?.lines ?? []);

    return { ...result, text: added.packFailed(nameOf(path), failure.line) };
  }

  return { ...result, text: added.map(nameOf(path)), failed: false, pack: true };
}

/** A texture pack's .zip added to the player's own pack, or why it is refused; null when the file
 *  is not a texture pack at all (a mod, an overrides pack), to be added as one. */
async function addTexturePack(v: VersionView, path: string): Promise<Added | null> {
  if (!path.toLowerCase().endsWith(".zip")) return null;
  let preview;

  try {
    preview = await api.texturePacksPreview(v.game, v.name, path);
  } catch (e) {
    const why = typeof e === "string" ? e : e instanceof Error ? e.message : String(e);

    // A texture pack that cannot be added (another game's, say): said as it is. Anything else is
    // not a texture pack, and is added as what it is.
    if (refusesPack(why)) {
      return { text: withName(nameOf(path), why), failed: true, plugin: false, pack: false };
    }

    return null;
  }

  try {
    const done = await api.texturePacksImport(v.game, v.name, path, "merge");

    return {
      text: TEXTURES.added(textureCount(done.textures), preview.replacing),
      failed: false,
      plugin: false,
      pack: true,
    };
  } catch (e) {
    return {
      text: withName(nameOf(path), typeof e === "string" ? e : String(e)),
      failed: true,
      plugin: false,
      pack: false,
    };
  }
}

/** Adds each path in turn, stopping at the first that fails; what the last one came to. */
export async function addMods(v: VersionView, paths: string[]): Promise<Added> {
  const result: Added = { text: "", failed: false, plugin: false, pack: false };
  const adder = v.actions.find((a) => a.id === "overrides-add" && a.thisPlatform) ?? null;
  // A folder of several mods (a repository's checkout) is each of its mods.
  const each: string[] = [];

  for (const path of paths) {
    if (isMap(path)) {
      const map = await addMap(v, path);

      if (map.failed) return map;
      Object.assign(result, { text: map.text, pack: true });
      continue;
    }

    try {
      each.push(...(await api.pluginsModFolders(path)));
    } catch (e) {
      return {
        ...result,
        text: withName(nameOf(path), typeof e === "string" ? e : String(e)),
        failed: true,
      };
    }
  }

  let count = 0;

  for (const path of each) {
    const name = nameOf(path);
    const texture = await addTexturePack(v, path);

    if (texture) {
      if (texture.failed) return { ...result, text: texture.text, failed: true };
      result.pack = true;
      result.text = texture.text;
      continue;
    }

    let installed;

    try {
      installed = await api.pluginsInstall(v.game, v.name, path);
    } catch (e) {
      return {
        ...result,
        text: withName(name, typeof e === "string" ? e : String(e)),
        failed: true,
      };
    }

    if (installed.kind === "plugin") {
      result.plugin = true;
      count++;
      result.text = installed.replaced
        ? added.replaced(installed.name, installed.version)
        : installed.enabled
          ? added.on(installed.name)
          : added.off(installed.name);
      continue;
    }

    if (!adder?.runnable) {
      return { ...result, text: added.packHereNot(name), failed: true };
    }

    const ok = await runChain({ kind: "version", key: v.key }, [adder], { path });

    if (!ok) {
      const why = whyStopped();

      return { ...result, text: added.packFailed(name, why), failed: true };
    }

    result.pack = true;
    result.text = added.pack(name);
  }

  if (count > 1) result.text = added.several(count);

  return result;
}
