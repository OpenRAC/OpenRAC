<script lang="ts">
  // A game's page: start this game without the launcher's window, from Steam (its own entry, in
  // core/src/steam.rs) or from a desktop shortcut (core/src/shortcut.rs). Both start the launcher
  // with --play. Not shown on Android and macOS; the desktop shortcut is Linux's.
  import { api, errorText, type SteamState } from "$lib/api";
  import { app } from "$lib/app.svelte";
  import { shortcutOptions } from "$lib/shortcuts";
  import { SHORTCUT_TEXT as T, STEAM_TEXT as S } from "$lib/modsText";
  import Reason from "./Reason.svelte";

  let { version }: { version: string } = $props();

  const options = $derived(shortcutOptions(app.info?.platform));
  let steam = $state<SteamState>("noSteam");
  let busy = $state<"steam" | "desktop" | null>(null);
  let said = $state<{ text: string; failed: boolean } | null>(null);

  $effect(() => {
    const key = version;
    steam = "noSteam";
    said = null;
    if (!options.steam) return;
    void api.steamState(key).then(
      (found) => {
        if (key === version) steam = found;
      },
      () => undefined,
    );
  });

  async function addToSteam() {
    busy = "steam";
    said = null;
    try {
      steam = await api.steamAdd(version);
      said = { text: S.added, failed: false };
    } catch (e) {
      said = { text: errorText(e), failed: true };
    }
    busy = null;
  }

  async function createShortcut() {
    busy = "desktop";
    said = null;
    try {
      said = { text: T.created(await api.shortcutCreate(version)), failed: false };
    } catch (e) {
      said = { text: errorText(e), failed: true };
    }
    busy = null;
  }
</script>

{#if steam !== "noSteam"}
  <div class="setting">
    <span class="text">
      <strong>{S.title}</strong>
      <small
        >{steam === "flatpakSteam"
          ? S.flatpak
          : steam === "noUser"
            ? S.noUser
            : steam === "added"
              ? T.steamIn
              : T.steamLine}</small
      >
    </span>
    <button
      disabled={busy !== null || steam === "flatpakSteam" || steam === "noUser" || steam === "added"}
      onclick={() => void addToSteam()}
    >
      {#if busy === "steam"}<span class="spinner"></span>{/if}{steam === "added"
        ? S.inSteam
        : steam === "outdated"
          ? S.update
          : S.add}
    </button>
  </div>
{/if}
{#if options.desktop}
  <div class="setting">
    <span class="text">
      <strong>{T.desktopTitle}</strong>
      <small>{T.desktopLine}</small>
    </span>
    <button disabled={busy !== null} onclick={() => void createShortcut()}>
      {#if busy === "desktop"}<span class="spinner"></span>{/if}{T.create}
    </button>
  </div>
{/if}
{#if said}<Reason text={said.text} tone={said.failed ? "err" : "ok"} role="status" />{/if}
