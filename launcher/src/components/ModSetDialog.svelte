<script lang="ts">
  // The window both mod code dialogs are: a head, a body that scrolls, and a row of actions.
  import type { Snippet } from "svelte";
  import { modal } from "$lib/portal";
  import { MOD_CODES as T } from "$lib/modsText";
  import Icon, { type IconName } from "./Icon.svelte";

  let {
    id,
    icon,
    title,
    subtitle = null,
    busy = false,
    onclose,
    children,
    actions,
  }: {
    id: string;
    icon: IconName;
    title: string;
    subtitle?: string | null;
    /** Working: it cannot be closed now. */
    busy?: boolean;
    onclose: () => void;
    children: Snippet;
    actions: Snippet;
  } = $props();

  function close() {
    if (!busy) onclose();
  }
</script>

<svelte:window
  onkeydown={(e) => {
    if (e.key === "Escape") close();
  }}
/>

<div class="overlay" role="presentation" use:modal>
  <button class="backdrop" onclick={close} tabindex="-1" aria-label={T.close}></button>
  <div class="modal rise" role="dialog" aria-modal="true" aria-labelledby={`${id}-title`} tabindex="-1">
    <div class="header">
      <Icon name={icon} size={20} />
      <div class="titles">
        <h2 id={`${id}-title`}>{title}</h2>
        {#if subtitle}<p>{subtitle}</p>{/if}
      </div>
    </div>
    <div class="body">{@render children()}</div>
    <div class="actions">{@render actions()}</div>
  </div>
</div>

<style>
  .overlay {
    position: fixed;
    inset: 0;
    z-index: 100;
    display: flex;
    align-items: center;
    justify-content: center;
    padding: 24px;
  }

  .backdrop {
    position: absolute;
    inset: 0;
    width: 100%;
    height: 100%;
    border: none;
    border-radius: 0;
    padding: 0;
    background: color-mix(in srgb, var(--ink-deep) 75%, transparent);
    backdrop-filter: blur(4px);
    cursor: default;
  }

  .modal {
    position: relative;
    z-index: 1;
    width: 100%;
    max-width: 540px;
    max-height: 100%;
    background: var(--panel);
    border: 1px solid var(--line-strong);
    border-radius: var(--radius);
    box-shadow: var(--shadow);
    display: flex;
    flex-direction: column;
    overflow: hidden;
  }

  .modal:focus {
    outline: none;
  }

  .header {
    display: flex;
    align-items: center;
    gap: 12px;
    padding: 16px 24px;
    border-bottom: 1px solid var(--line);
    background: var(--panel-hi);
    color: var(--amber);
  }

  .titles {
    min-width: 0;
  }

  h2 {
    margin: 0;
    font-size: 1.125rem;
    font-family: var(--font-head);
    font-weight: 600;
    color: var(--text);
    overflow-wrap: anywhere;
  }

  p {
    margin: 2px 0 0;
    font-size: 0.8125rem;
    color: var(--soft);
  }

  .body {
    padding: 20px 24px;
    display: flex;
    flex-direction: column;
    gap: 16px;
    min-height: 0;
    overflow-y: auto;
    overscroll-behavior: contain;
  }

  .actions {
    display: flex;
    align-items: center;
    flex-wrap: wrap;
    justify-content: flex-end;
    gap: 12px;
    padding: 14px 24px;
    border-top: 1px solid var(--line);
    background: var(--panel-hi);
  }

  @media (max-width: 560px) {
    .overlay {
      padding: 12px;
    }

    .body,
    .header,
    .actions {
      padding-inline: 16px;
    }
  }
</style>
