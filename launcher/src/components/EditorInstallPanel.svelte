<script lang="ts">
  // Settings' Level editor: OpenRAC Editor, a program of its own downloaded from its releases
  // (core/src/editor_install.rs), or a Mac app the player installed. The launcher carries none of
  // it; this says which one is there, looks for a newer one, and downloads or removes a download.
  // The level projects are never touched here.
  import { onMount } from "svelte";

  import { api } from "$lib/api";
  import { app, guardShown, installEditor, readEditor, removeEditor } from "$lib/app.svelte";

  import Icon from "./Icon.svelte";

  let newest = $state<string | null>(null);
  let looking = $state(false);
  let lookFailed = $state(false);

  const installed = $derived(app.editor?.version ?? null);
  const newer = $derived(!!newest && !!installed && newest !== installed && installed !== "dev");

  async function look() {
    looking = true;
    lookFailed = false;

    try {
      newest = await api.editorNewest();
    } catch {
      lookFailed = true;
    } finally {
      looking = false;
    }
  }

  onMount(() => {
    void readEditor();
  });
</script>

<section class="panel" aria-labelledby="editor-install-title">
  <h2 id="editor-install-title">Level editor</h2>
  <p class="muted">OpenRAC Editor changes your disc's levels. It is a separate program.</p>
  <p class="status dim">
    {#if installed}
      <Icon name="check" size={14} />
      {app.editor?.installedApp
        ? `OpenRAC Editor ${installed} is installed.`
        : `OpenRAC Editor ${installed} is downloaded.`}
    {:else}
      <Icon name="info" size={14} /> Not downloaded.
    {/if}
    {#if app.editor?.problem}<span class="warn">{app.editor.problem}.</span>{/if}
    {#if looking}· Looking…{:else if newer}· {newest} is out.{:else if newest && installed}· You
      have the latest.{:else if lookFailed}· Could not check.{/if}
  </p>
  <div class="row">
    {#if !app.editor?.installedApp}
      <button
        class="small primary"
        disabled={app.editorDownloading}
        onclick={() => void installEditor()}
      >
        {#if app.editorDownloading}<span class="spinner"></span>Downloading…{:else}<Icon
            name="download"
            size={14}
          />{installed
            ? newer
              ? `Update to ${newest}`
              : "Download again"
            : "Download OpenRAC Editor"}{/if}
      </button>
    {/if}
    {#if installed}
      <button class="small" disabled={looking} onclick={() => void look()}
        ><Icon name="refresh" size={14} />Check for a newer one</button
      >
      {#if !app.editor?.installedApp}
        <button
          class="small ghost"
          disabled={app.editorDownloading}
          onclick={() => void removeEditor()}><Icon name="trash" size={14} />Remove</button
        >
      {/if}
    {/if}
    {#if app.editor}
      <button
        class="small ghost"
        onclick={() => void guardShown(api.openUrl(app.editor?.repository ?? ""))}
        ><Icon name="external" size={14} />Releases</button
      >
    {/if}
  </div>
</section>

<style>
  /* As Settings' other panels: the title close over its text, the rest spaced evenly. */
  .panel {
    display: flex;
    flex-direction: column;
    gap: 12px;
  }
  .panel h2 {
    margin-bottom: -8px;
  }
  .status {
    font-size: 0.8125rem;
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 6px;
  }
  .warn {
    color: var(--warn);
  }
  .row {
    display: flex;
    flex-wrap: wrap;
    gap: 8px;
  }
</style>
