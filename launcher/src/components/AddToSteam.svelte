<script lang="ts">
  // Settings: the launcher as a game in the player's Steam library (core/src/steam.rs). Not shown
  // without Steam. Steam must be closed to add it: the button says so, and writes nothing.
  import { api, errorText, type SteamState } from "$lib/api";
  import { STEAM_TEXT as T } from "$lib/modsText";
  import Reason from "./Reason.svelte";

  let steam = $state<SteamState>("noSteam");
  let busy = $state(false);
  let said = $state<{ text: string; failed: boolean } | null>(null);

  $effect(() => {
    void api.steamState().then(
      (found) => {
        steam = found;
      },
      () => undefined,
    );
  });

  async function add() {
    busy = true;
    said = null;
    try {
      steam = await api.steamAdd();
      said = { text: T.added, failed: false };
    } catch (e) {
      said = { text: errorText(e), failed: true };
    }
    busy = false;
  }
</script>

{#if steam !== "noSteam"}
  <div class="setting">
    <span class="text">
      <strong>{T.title}</strong>
      <small
        >{steam === "flatpakSteam"
          ? T.flatpak
          : steam === "noUser"
            ? T.noUser
            : steam === "added"
              ? T.inLibrary
              : T.line}</small
      >
    </span>
    <button
      disabled={busy || steam === "flatpakSteam" || steam === "noUser" || steam === "added"}
      onclick={() => void add()}
    >
      {#if busy}<span class="spinner"></span>{/if}{steam === "added"
        ? T.inSteam
        : steam === "outdated"
          ? T.update
          : T.add}
    </button>
  </div>
  {#if said}<Reason text={said.text} tone={said.failed ? "err" : "ok"} role="status" />{/if}
{/if}
