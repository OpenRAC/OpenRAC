<script lang="ts">
  // Whether Play brings the mods (config.playWithMods): a switch, and beside it what it means, the
  // number of mods on or "original game", which leads to the Mods tab. Under Play on the desktop's
  // game page (a pill) and beside the disc on Android's Games (`quiet`: a line, the switch with X's
  // glyph: X turns it on Games).
  import type { VersionView } from "$lib/api";
  import { app } from "$lib/app.svelte";
  import { supports } from "$lib/capabilities";
  import { modsOnCount, setWithMods } from "$lib/gameMenu.svelte";
  import { WITH_MODS } from "$lib/modsText";
  import Icon from "./Icon.svelte";

  let {
    version,
    setUp,
    onopen,
    quiet = false,
  }: { version: VersionView; setUp: boolean; onopen: () => void; quiet?: boolean } = $props();

  const on = $derived(app.config?.playWithMods ?? false);

  /** How many mods are on for the game; null until read (or where they cannot be). */
  let count = $state<number | null>(null);
  $effect(() => {
    const shown = version;
    count = null;
    if (!setUp) return;
    let current = true;
    void modsOnCount(shown).then((n) => {
      if (current) count = n;
    });
    return () => {
      current = false;
    };
  });
</script>

<div class="with-mods" class:quiet>
  <label title={WITH_MODS.hint}
    >{#if quiet}<span class="glyph" aria-hidden="true">X</span>{/if}{WITH_MODS.label}<input
      type="checkbox"
      role="switch"
      class="switch"
      checked={on}
      disabled={!supports(app.info, "mods")}
      onchange={(e) => void setWithMods(e.currentTarget.checked)}
    /></label
  >
  {#if quiet}
    <button class="link" class:dim={!on} onclick={onopen}
      >{on ? WITH_MODS.count(count) : WITH_MODS.original}<Icon name="next" size={12} /></button
    >
  {:else if on}
    <button class="link" onclick={onopen}>{WITH_MODS.count(count)}<Icon name="next" size={12} /></button>
  {:else}
    <span class="dim">{WITH_MODS.original}</span>
  {/if}
</div>

<style>
  /* A quiet pill: its switch, then the count or "original game". */
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

  .link {
    display: inline-flex;
    align-items: center;
    gap: 2px;
  }

  /* Beside the disc: a line, not a button, smaller than the buttons above it. */
  .quiet {
    margin: 0;
    padding: 0 0 0 4px;
    background: none;
    font-size: clamp(0.8125rem, 2vh, 0.9375rem);
  }

  .quiet label {
    font-weight: 600;
    color: var(--text);
  }

  .quiet .link {
    gap: 4px;
    min-height: 32px;
    padding: 0 10px;
    border-radius: 99px;
    background: none;
    box-shadow: none;
    color: var(--text);
  }

  .quiet .link.dim {
    color: var(--dim);
  }

  /* As the shelf's A and Y: the letter in the middle of its circle, whatever the font around it. */
  .glyph {
    flex: none;
    display: inline-grid;
    place-items: center;
    width: 22px;
    height: 22px;
    margin-right: -2px;
    border-radius: 50%;
    background: var(--control-card);
    color: var(--text);
    font-family: var(--font-sans);
    font-size: 0.6875rem;
    font-weight: 800;
    line-height: 1;
  }
</style>
