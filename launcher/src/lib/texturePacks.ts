// How a game's Texture packs section talks about a pack, in a player's words. No Svelte or Tauri
// here, so it can be tested.
import type { TexturePackPreview, TexturePackView } from "./api";
import { grouped } from "./format";

/** "1 texture", "1 342 textures". */
export function textureCount(n: number): string {
  return `${grouped(n)} ${n === 1 ? "texture" : "textures"}`;
}

/** The pack's line: what it holds and whether the game uses it. */
export function packSummary(view: TexturePackView): string {
  if (!view.packCount) {
    return "Empty for now. Put your pictures in it, or add a pack someone shared.";
  }
  const animated = view.packAnimated ? `, ${grouped(view.packAnimated)} of them animated` : "";
  const state = view.enabled ? "The game uses it from its next start." : "Off: the game shows its own pictures.";
  return `${textureCount(view.packCount)}${animated}. ${state}`;
}

/** What a picked pack holds and what adding it does. */
export function previewSummary(p: TexturePackPreview): string {
  const name = p.name ? `“${p.name}”` : "This pack";
  const animated = p.animated ? ` (${grouped(p.animated)} animated)` : "";
  const folders = p.folders.map((f) => `${f.label} ${grouped(f.count)}`).join(", ");
  let mine = "";
  if (p.current) {
    mine = p.replacing
      ? ` Your pack has ${textureCount(p.current)}; ${grouped(p.replacing)} of them would be replaced if you add it to yours.`
      : ` Your pack has ${textureCount(p.current)}; none of them would be replaced.`;
  }
  return `${name} has ${textureCount(p.textures)}${animated}: ${folders}.${mine}`;
}
