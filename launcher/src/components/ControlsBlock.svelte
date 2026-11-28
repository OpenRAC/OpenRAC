<script lang="ts">
  // Settings, Controls: every key in one place, as the game menu's Controls tab has them. The
  // game's shortcuts (F12 for a screenshot, F11 for full screen...), kept for every game in the
  // global controls file (core key_shortcuts.rs), and the way to a game's Keyboard and mouse page
  // (its keys, mouse and gyro). A shortcut's cap, pressed, waits for the next key with its Ctrl,
  // Shift or Alt; Escape cancels, Delete or Backspace clears it.
  import { onDestroy } from "svelte";

  import { api, errorText, type ShortcutRow } from "$lib/api";
  import { app } from "$lib/app.svelte";
  import {
    shortcutClashes,
    shortcutKeys,
    shortcutLabel,
    shortcutOfKey,
    shortcutValue,
  } from "$lib/keyShortcuts";

  import Icon from "./Icon.svelte";

  let { onkeyboard }: { onkeyboard: (version: string) => void } = $props();

  let rows = $state<ShortcutRow[]>([]);
  let failed = $state<string | null>(null);
  /** The cap waiting for a key: its shortcut and which of its keys (its count: a new one). */
  let waiting = $state<{ id: string; slot: number } | null>(null);

  void api.shortcutsView().then(
    (found) => (rows = found),
    (e: unknown) => (failed = errorText(e)),
  );

  const developer = $derived(app.config?.developer ?? false);
  const sections = $derived(["Game", "Developer"].filter((s) => s === "Game" || developer));
  const clashes = $derived(shortcutClashes(rows));

  async function save(id: string, keys: string | null) {
    try {
      rows = await api.shortcutsSet(id, keys);
      failed = null;
    } catch (e) {
      failed = errorText(e);
    }
  }

  async function resetAll() {
    try {
      rows = await api.shortcutsReset();
      failed = null;
    } catch (e) {
      failed = errorText(e);
    }
  }

  function take(e: KeyboardEvent) {
    if (!waiting) return;
    e.preventDefault();
    e.stopPropagation();
    const { id, slot } = waiting;
    const row = rows.find((r) => r.id === id);

    if (!row) return;
    const keys = shortcutKeys(row.keys);

    if (e.code === "Escape") {
      waiting = null;

      return;
    }

    if (e.code === "Delete" || e.code === "Backspace") {
      waiting = null;
      void save(id, shortcutValue(keys.filter((_, i) => i !== slot)));

      return;
    }

    const key = shortcutOfKey(e);

    if (!key) return; // a modifier alone: it waits for the key
    waiting = null;
    const next = [...keys];

    next[slot] = key;
    void save(id, shortcutValue([...new Set(next)]));
  }

  $effect(() => {
    if (!waiting) return;
    window.addEventListener("keydown", take, true);

    return () => {
      window.removeEventListener("keydown", take, true);
    };
  });
  onDestroy(() => {
    window.removeEventListener("keydown", take, true);
  });

  /** The game Keyboard and mouse opens for: the one picked, else the first (Android picks none
   *  until its shelf is used). */
  const version = $derived.by(() => {
    const all = app.library?.games.flatMap((g) => g.versions) ?? [];

    return all.find((v) => v.key === app.version) ?? all[0] ?? null;
  });
</script>

<section class="panel controls" id="controls">
  <h2>Controls</h2>

  <div class="setting">
    <span class="text">
      <strong>Keyboard and mouse</strong>
      <small
        >Your keys, the mouse and the gyro{version ? ` for ${version.title}` : ""}. A controller's
        buttons are set in the game's menu, under Controls.</small
      >
    </span>
    <button
      class="small"
      disabled={!version}
      onclick={() => {
        if (version) onkeyboard(version.key);
      }}>Open<Icon name="next" size={14} /></button
    >
  </div>

  {#if failed}
    <p class="err note" role="alert">{failed}</p>
  {/if}

  {#each sections as section (section)}
    <h3>{section === "Game" ? "Shortcuts" : "Developer shortcuts"}</h3>
    {#each rows.filter((r) => r.section === section) as row (row.id)}
      {@const keys = shortcutKeys(row.keys)}
      <div class="setting shortcut">
        <span class="text">
          <strong>{row.name}</strong>
          {#if waiting?.id === row.id}
            <small class="warn" role="status"
              >Press a key, with Ctrl, Shift or Alt if you like. Escape cancels, Delete clears.</small
            >
          {:else if clashes[row.id]}
            <small class="warn">Also {clashes[row.id]?.join(", ")}</small>
          {/if}
        </span>
        <div class="caps" role="group" aria-label={row.name}>
          {#each [0, 1] as slot (slot)}
            {@const key = keys[slot]}
            {@const busy = waiting?.id === row.id && waiting.slot === slot}
            {#if key !== undefined || slot === keys.length || busy}
              <span class="cap-wrap">
                <button
                  class="cap"
                  class:waiting={busy}
                  class:empty={!key && !busy}
                  class:clash={!!key && !!clashes[row.id]}
                  aria-label={busy
                    ? "Press a key"
                    : key
                      ? `${row.name}: ${shortcutLabel(key)}. Change`
                      : `${row.name}: add a key`}
                  onclick={() => (waiting = { id: row.id, slot })}
                  >{#if busy}Press a key…{:else if key}{shortcutLabel(key)}{:else}<span class="add"
                      >{slot === 0 ? "None" : "Add"}</span
                    >{/if}</button
                >{#if key && !busy}<button
                    class="clear"
                    aria-label={`Remove ${shortcutLabel(key)} from ${row.name}`}
                    title={`Remove ${shortcutLabel(key)}`}
                    onclick={() =>
                      void save(row.id, shortcutValue(keys.filter((_, i) => i !== slot)))}
                    ><Icon name="x" size={14} /></button
                  >{/if}
              </span>
            {:else}
              <span></span>
            {/if}
          {/each}
          <button
            class="default"
            disabled={row.keys === row.defaults}
            aria-label={`${row.name}: back to ${row.defaults.split(",").map(shortcutLabel).join(" or ")}`}
            title={row.keys === row.defaults
              ? "The default"
              : `Back to ${row.defaults.split(",").map(shortcutLabel).join(" or ")}`}
            onclick={() => void save(row.id, null)}><Icon name="reset" size={16} /></button
          >
        </div>
      </div>
    {/each}
  {/each}

  {#if rows.some((r) => r.keys !== r.defaults)}
    <div class="end">
      <button class="small" onclick={() => void resetAll()}>Shortcuts back to the defaults</button>
    </div>
  {/if}
</section>

<style>
  h3 {
    margin: 14px 0 0;
  }

  /* The same caps as a Keyboard and mouse bind (KeyBindSetting.svelte): two columns, the same on
     every row, then the way back to the default. */
  .caps {
    flex: none;
    display: grid;
    grid-template-columns: repeat(2, 156px) 40px;
    align-items: center;
    gap: 8px;
  }

  .cap-wrap {
    display: flex;
    align-items: stretch;
  }

  .cap {
    flex: 1;
    min-width: 0;
    overflow: hidden;
    text-overflow: ellipsis;
    white-space: nowrap;
    text-align: center;
    justify-content: center;
    min-height: 40px;
    padding: 6px 12px;
    font-family: var(--font-mono);
    font-size: 0.875rem;
    font-weight: 600;
    color: var(--text);
    background: var(--control);
    border-radius: var(--radius-sm);
    cursor: pointer;
  }

  .cap-wrap:has(.clear) .cap {
    border-top-right-radius: 0;
    border-bottom-right-radius: 0;
  }

  .cap.empty {
    background: transparent;
    box-shadow: none;
    border: 1px dashed var(--line-field);
  }

  .cap.waiting {
    font-family: var(--font-sans);
    font-weight: 500;
    color: var(--amber);
    box-shadow: none;
    background: var(--on-fill);
    animation: pulse 1.2s ease-in-out infinite;
  }

  .cap.clash {
    background: var(--on-fill);
    color: var(--amber);
  }

  .add {
    font-family: var(--font-sans);
    font-weight: 400;
    color: var(--dim);
  }

  .clear,
  .default {
    display: grid;
    place-items: center;
    min-height: 40px;
    padding: 0 8px;
    color: var(--soft);
    background: var(--control);
    cursor: pointer;
  }

  .clear {
    min-width: 34px;
    border-radius: 0 var(--radius-sm) var(--radius-sm) 0;
  }

  .default {
    width: 40px;
    padding: 0;
    border-radius: var(--radius-sm);
  }

  .default:disabled {
    opacity: 0.35;
    cursor: default;
  }

  @media (hover: hover) {
    :global(html:where(:not([data-input="pad"]))) .cap:hover,
    :global(html:where(:not([data-input="pad"]))) .default:hover:not(:disabled) {
      background: var(--on-fill);
      color: var(--amber);
    }

    :global(html:where(:not([data-input="pad"]))) .clear:hover {
      color: var(--err);
    }
  }

  .cap:focus-visible,
  .clear:focus-visible,
  .default:focus-visible {
    outline: 2px solid var(--amber);
    outline-offset: 2px;
  }

  @media (max-width: 560px) {
    .shortcut {
      flex-wrap: wrap;
    }

    .caps {
      grid-template-columns: repeat(2, minmax(0, 1fr)) 40px;
      width: 100%;
    }
  }

  /* Android's 44 px finger height, as every other button there (app.css). */
  :global(html[data-platform="android"]) .cap,
  :global(html[data-platform="android"]) .clear,
  :global(html[data-platform="android"]) .default {
    min-height: 44px;
  }

  :global(html[data-platform="android"]) .caps {
    grid-template-columns: repeat(2, 156px) 44px;
  }

  :global(html[data-platform="android"]) .default {
    width: 44px;
  }

  @keyframes pulse {
    50% {
      opacity: 0.75;
    }
  }

  @media (prefers-reduced-motion: reduce) {
    .cap.waiting {
      animation: none;
    }
  }

  .warn {
    color: var(--warn) !important;
  }

  .end {
    display: flex;
    justify-content: flex-end;
    padding-top: 8px;
  }
</style>
