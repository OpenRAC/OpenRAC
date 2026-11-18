// How the Mods tab's Texture packs section talks about a pack, in a player's words. No Svelte or Tauri
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
    if (view.pcsx2Path) return view.enabled ? "PCSX2 pack selected for the next game start." : "Texture packs are off.";
    return "Empty for now. Put your pictures in it, or add a pack someone shared.";
  }
  const animated = view.packAnimated ? `, ${grouped(view.packAnimated)} of them animated` : "";
  const state = view.enabled ? "The game uses it from its next start." : "Off: the game shows its own pictures.";
  return `${textureCount(view.packCount)}${animated}. ${state}`;
}

/** What a picked pack holds and what replacing the player's pack with it does. */
export function replaceSummary(p: TexturePackPreview): string {
  const name = p.name ? `“${p.name}”` : "This pack";
  const animated = p.animated ? ` (${grouped(p.animated)} animated)` : "";
  const mine = p.current ? ` Your pack (${textureCount(p.current)}) is kept beside it.` : "";
  return `${name} has ${textureCount(p.textures)}${animated}. It becomes your texture pack.${mine}`;
}

/** The starts of the refusals (core/src/texture_packs.rs, scan) that mean a .zip is a texture pack
 *  but cannot be added. Any other refusal ("holds no texture pictures", pictures not named after a
 *  texture) means it is something else, a mod or an overrides pack. */
const PACK_REFUSALS = [
  "This pack is for ",
  "The pack has two pictures for the same texture",
  "The animation folder ",
  "The pack's manifest cannot be read",
];

/** Whether a preview's refusal is of a texture pack (said to the player) or of another kind of file. */
export function refusesPack(why: string): boolean {
  return PACK_REFUSALS.some((start) => why.startsWith(start));
}
