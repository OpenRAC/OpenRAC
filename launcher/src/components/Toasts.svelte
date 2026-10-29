<script lang="ts">
  // What the launcher says for a moment, at the window's corner. Information goes after a few
  // seconds and waits while the pointer or the focus is on it; an error stays until it is
  // dismissed, and is read out at once (an assertive region), not after what is being read.
  import { api, onAndroid } from "$lib/api";
  import { app, dismissToast, holdToasts, guard, type BakeNotice } from "$lib/app.svelte";
  import { hidePaths } from "$lib/paths";
  import { reduced } from "$lib/motion";
  import { flip } from "svelte/animate";
  import { cubicOut } from "svelte/easing";
  import { fade, fly } from "svelte/transition";
  import Icon from "./Icon.svelte";

  // Android: in from below, out by fading, and the others slide into the place one leaves. A
  // desktop's arrive as they did (app.css .rise) and go at once.
  const still = reduced || !onAndroid;
  const into = { y: 16, opacity: 0, duration: still ? 0 : 240, easing: cubicOut };
  const away = { duration: still ? 0 : 160 };
  const move = { duration: still ? 0 : 220, easing: cubicOut };

  function openLog(bake?: BakeNotice) {
    if (bake?.failure.log) void guard(api.openBakeLog(bake.version, bake.failure.log));
  }

  const infos = $derived(app.toasts.filter((t) => t.kind !== "error"));
  const errors = $derived(app.toasts.filter((t) => t.kind === "error"));

  // The pointer and the focus each hold the toasts, and each lets them go: the two are counted
  // apart, so that moving the pointer out while the focus is still on them (or the other way
  // round) does not let a message go before it is read.
  let pointerOver = false;
  let focusWithin = false;
  const hold = () => {
    holdToasts(pointerOver || focusWithin);
  };
</script>

<div
  class="toasts"
  role="presentation"
  onpointerenter={() => {
    pointerOver = true;
    hold();
  }}
  onpointerleave={() => {
    pointerOver = false;
    hold();
  }}
  onfocusin={() => {
    focusWithin = true;
    hold();
  }}
  onfocusout={() => {
    focusWithin = false;
    hold();
  }}
>
  <div class="list" role="alert">
    {#each errors as t (t.id)}
      <div class="toast error" class:rise={!onAndroid} in:fly={into} out:fade={away} animate:flip={move}>
        <Icon name="warn" size={16} />
        <div class="grow message">
          <strong>{t.text}</strong>
          {#if t.bake}
            <p>{t.bake.failure.explanation}</p>
            <p class="fix"><strong>Try this:</strong> {t.bake.failure.suggestion}</p>
            <details class="error-details">
              <summary>Error details</summary>
              {#if t.bake.failure.stage}<p>{t.bake.failure.stage}</p>{/if}
              <pre>{hidePaths(t.bake.failure.detail).text}</pre>
            </details>
            <div class="actions">
              {#if t.bake.failure.log && !onAndroid}
                <button
                  class="small"
                  onclick={() => {
                    openLog(t.bake);
                  }}>Open log</button
                >
              {/if}
              <button
                class="ghost small"
                onclick={() => {
                  app.page = "tasks";
                }}>View details</button
              >
            </div>
            <small>{t.bake.failure.level ? `${t.bake.failure.level} - ` : ""}{t.bake.failure.code}</small>
          {/if}
        </div>
        <button
          class="ghost small"
          onclick={() => {
            dismissToast(t.id);
          }}
          aria-label="Dismiss"><Icon name="x" size={14} /></button
        >
      </div>
    {/each}
  </div>
  <div class="list" role="status" aria-live="polite">
    {#each infos as t (t.id)}
      <div class="toast" class:rise={!onAndroid} in:fly={into} out:fade={away} animate:flip={move}>
        <Icon name="info" size={16} />
        <span class="grow">{t.text}</span>
        <button
          class="ghost small"
          onclick={() => {
            dismissToast(t.id);
          }}
          aria-label="Dismiss"><Icon name="x" size={14} /></button
        >
      </div>
    {/each}
  </div>
</div>

<style>
  .toasts {
    position: fixed;
    right: 20px;
    bottom: calc(20px + var(--nav-bottom));
    display: flex;
    flex-direction: column;
    gap: 8px;
    /* Over the full-screen launch and running screens (LaunchScreen, GameRunning). */
    z-index: 70;
    max-width: 420px;
  }

  /* Not display: contents, which takes a live region out of what a screen reader hears. */
  .list {
    display: flex;
    flex-direction: column;
    align-items: inherit;
    gap: 8px;
  }

  .toast {
    display: flex;
    gap: 10px;
    align-items: flex-start;
    background: var(--panel-hi);
    border: 1px solid var(--line-strong);
    border-radius: var(--radius-sm);
    padding: 10px 8px 10px 14px;
    font-size: 0.875rem;
    box-shadow: var(--shadow-sm);
  }

  .message {
    min-width: 0;
  }
  .message p {
    margin: 6px 0;
    line-height: 1.45;
  }
  .error-details {
    margin-top: 8px;
  }
  .error-details summary {
    cursor: pointer;
  }
  .error-details pre {
    max-height: 160px;
    overflow: auto;
    white-space: pre-wrap;
    overflow-wrap: anywhere;
    font-size: 0.75rem;
  }
  .message > strong {
    display: block;
  }
  .message small {
    display: block;
    margin-top: 8px;
    color: var(--soft);
  }
  .actions {
    display: flex;
    flex-wrap: wrap;
    gap: 8px;
    margin-top: 12px;
  }
  .toast .actions button {
    margin: 0;
    padding: 6px 10px;
  }

  .toast > :global(svg) {
    flex: none;
    margin-top: 2px;
    color: var(--amber);
  }

  .toast.error {
    border-color: color-mix(in srgb, var(--err) 60%, transparent);
  }

  .toast.error > :global(svg) {
    color: var(--err);
  }

  .toast button {
    flex: none;
    margin: -4px 0;
    padding: 4px 6px;
  }

  /* Android: centred over the bottom of the page, as its snackbars are. */
  :global(html[data-platform="android"]) .toasts {
    right: 16px;
    left: calc(var(--nav-left) + 16px);
    bottom: calc(16px + var(--nav-bottom));
    align-items: center;
    max-width: none;
    pointer-events: none;
  }

  :global(html[data-platform="android"]) .toast {
    align-items: center;
    max-width: 560px;
    border-radius: 12px;
    pointer-events: auto;
  }

  :global(html[data-platform="android"]) .toast > :global(svg) {
    margin-top: 0;
  }

  /* Its Dismiss keeps a finger's size (app.css gives small buttons 44 px) without making the toast
     a line taller than its text: the button reaches over the toast's padding. */
  :global(html[data-platform="android"]) .toast button {
    min-height: 40px;
    min-width: 40px;
    margin: -10px -4px -10px 0;
  }
</style>
