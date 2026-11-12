<script module lang="ts">
  /** The last count read for each version: shown at once when the page comes back, which read it
   *  again and blinked "enabled" to "enabled: 3". */
  const lastCount: Record<string, number | null> = {};
</script>

<script lang="ts">
  // Whether Play brings the mods (config.playWithMods): a switch, and beside it what it means, the
  // state, "enabled: 3" or "disabled", which leads to the Mods tab. One pill under Play on the
  // desktop's game page and beside the disc on Android's Games (X turns it there).
  import type { VersionView } from "$lib/api";
  import { app } from "$lib/app.svelte";
  import { supports } from "$lib/capabilities";
  import { modsOnCount, setWithMods } from "$lib/gameMenu.svelte";
  import { WITH_MODS } from "$lib/modsText";
  import Icon from "./Icon.svelte";

  let { version, setUp, onopen }: { version: VersionView; setUp: boolean; onopen: () => void } = $props();

  const on = $derived(app.config?.playWithMods ?? false);

  /** How many mods are on for the game; null until read (or where they cannot be). */
  let count = $state<number | null>(null);
  $effect(() => {
    const shown = version;
    count = lastCount[shown.key] ?? null;
    if (!setUp) return;
    let current = true;
    void modsOnCount(shown).then((n) => {
      lastCount[shown.key] = n;
      if (current) count = n;
    });
    return () => {
      current = false;
    };
  });
</script>

<div class="with-mods">
  <label title={WITH_MODS.hint}
    >{WITH_MODS.label}<input
      type="checkbox"
      role="switch"
      class="switch"
      checked={on}
      disabled={!supports(app.info, "mods")}
      onchange={(e) => void setWithMods(e.currentTarget.checked)}
    /></label
  >
  {#if on}
    <button class="link" onclick={onopen}>{WITH_MODS.count(count)}<Icon name="next" size={12} /></button>
  {:else}
    <span class="dim">{WITH_MODS.original}</span>
  {/if}
</div>

<style>
  /* A quiet pill: its switch, then "enabled: 3" or "disabled". */
  .with-mods {
    display: inline-flex;
    align-items: center;
    gap: 10px;
    margin-top: 12px;
    padding: 6px 14px;
    border-radius: var(--radius-sm);
    background: var(--control);
    font-size: 0.875rem;
    color: var(--soft);
  }

  label {
    display: inline-flex;
    align-items: center;
    gap: 10px;
    cursor: pointer;
  }

  /* Words in the pill, not a button in it: a press only dims them. As a button, its own box lit
     up inside the pill on a tap, as the page slid away. The padding keeps a thumb's target. */
  .link {
    display: inline-flex;
    align-items: center;
    gap: 2px;
    min-height: 0;
    margin: -8px -10px;
    padding: 8px 10px;
    background: none;
    color: inherit;
    font-weight: 500;
    transition: opacity var(--fast);
  }

  .link:active {
    filter: none;
    opacity: 0.7;
  }
</style>
