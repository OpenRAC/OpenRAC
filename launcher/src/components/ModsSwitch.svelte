<script lang="ts">
  // Whether Play brings the mods (config.playWithMods). Under Play on the desktop's game page: a
  // pill, its switch and the state now ("On: 3 mods" or "Off: the original game"), which leads to
  // the Mods tab. Beside the disc on Android's Games (`quiet`): two ways, Original and Mods, the
  // one that plays filled, with X's glyph (X swaps them).
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
  const modsOk = $derived(supports(app.info, "mods"));

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

{#if quiet}
  <!-- Android: two ways, the one that plays filled; X swaps them, the Mods way when on opens the
       Mods tab. -->
  <div class="ways" role="radiogroup" aria-label={WITH_MODS.ways} title={WITH_MODS.hint}>
    <span class="glyph" aria-hidden="true">X</span>
    <span class="choice">
      <button
        role="radio"
        aria-checked={!on}
        tabindex={on ? -1 : 0}
        disabled={!modsOk}
        onclick={() => void setWithMods(false)}>{WITH_MODS.wayOriginal}</button
      >
      <button
        role="radio"
        aria-checked={on}
        tabindex={on ? 0 : -1}
        disabled={!modsOk}
        onclick={() => {
          if (on) onopen();
          else void setWithMods(true);
        }}
        >{WITH_MODS.wayMods(count)}{#if on}<Icon name="next" size={12} />{/if}</button
      >
    </span>
  </div>
{:else}
  <div class="with-mods">
    <label title={WITH_MODS.hint}
      >{WITH_MODS.label}<input
        type="checkbox"
        role="switch"
        class="switch"
        checked={on}
        disabled={!modsOk}
        onchange={(e) => void setWithMods(e.currentTarget.checked)}
      /></label
    >
    {#if on}
      <button class="link" onclick={onopen}>{WITH_MODS.count(count)}<Icon name="next" size={12} /></button>
    {:else}
      <span class="dim">{WITH_MODS.original}</span>
    {/if}
  </div>
{/if}

<style>
  /* The desktop's pill: its switch, then the state ("On: 3 mods", "Off: the original game"). */
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

  /* Beside the disc: X's glyph under A's, then the two ways, small and filled. */
  .ways {
    display: inline-flex;
    align-items: center;
    gap: 10px;
    padding-left: 12px;
  }

  .choice {
    display: inline-flex;
    gap: 2px;
    padding: 3px;
    border-radius: 11px;
    background: var(--control-card);
  }

  /* Smaller than a finger's 44 px (app.css): the whole strip is the target, beside A's button. */
  :global(html[data-platform="android"]) .choice button,
  .choice button {
    display: inline-flex;
    align-items: center;
    gap: 2px;
    min-height: 0;
    height: 30px;
    padding: 0 12px;
    border-radius: 8px;
    background: none;
    box-shadow: none;
    color: var(--soft);
    font-size: 0.8125rem;
    font-weight: 600;
    transition:
      background var(--fast),
      color var(--fast);
  }

  :global(html) .choice button[aria-checked="true"] {
    background: var(--amber);
    color: var(--bar-ink-deep);
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
