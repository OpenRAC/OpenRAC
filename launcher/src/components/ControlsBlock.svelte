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

  const version = $derived(
    app.library?.games.flatMap((g) => g.versions).find((v) => v.key === app.version) ?? null,
  );
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
        <span class="caps">
          {#each keys as key, slot (key)}
            <button
              class="cap"
              class:waiting={waiting?.id === row.id && waiting.slot === slot}
              aria-label={`${row.name}: ${shortcutLabel(key)}. Change`}
              onclick={() => (waiting = { id: row.id, slot })}>{shortcutLabel(key)}</button
            >
          {/each}
          {#if keys.length < 2}
            <button
              class="cap add"
              class:waiting={waiting?.id === row.id && waiting.slot === keys.length}
              aria-label={keys.length ? `${row.name}: add a key` : `${row.name}: none. Set a key`}
              onclick={() => (waiting = { id: row.id, slot: keys.length })}
              >{keys.length ? "+" : "None"}</button
            >
          {/if}
          {#if row.keys !== row.defaults}
            <button
              class="ghost small"
              title={`Back to ${row.defaults.split(",").map(shortcutLabel).join(" or ")}`}
              onclick={() => void save(row.id, null)}>Default</button
            >
          {/if}
        </span>
      </div>
    {/each}
  {/each}

  {#if rows.some((r) => r.keys !== r.defaults)}
    <div class="end">
      <button class="ghost small" onclick={() => void resetAll()}
        >Shortcuts back to the defaults</button
      >
    </div>
  {/if}
</section>

<style>
  h3 {
    margin: 14px 0 0;
  }

  .caps {
    display: flex;
    flex: none;
    flex-wrap: wrap;
    gap: 6px;
    align-items: center;
    justify-content: flex-end;
  }

  .cap {
    min-width: 44px;
    padding: 4px 10px;
    font-family: var(--mono, monospace);
    font-size: 0.8125rem;
  }

  .cap.add {
    color: var(--soft);
  }

  .cap.waiting {
    outline: 2px solid var(--warn);
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
