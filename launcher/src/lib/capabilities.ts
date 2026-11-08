/** Optional limits supplied by a native host. Existing desktop and Android hosts retain their features. */
export type Capability = "mods" | "saveEditing" | "saveTransfer" | "openFolders" | "folderScan" | "updates";

export function supports(
  host: { capabilities?: Partial<Record<Capability, boolean>> } | null | undefined,
  feature: Capability,
): boolean {
  return host?.capabilities?.[feature] ?? true;
}
