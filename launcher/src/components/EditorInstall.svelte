<script lang="ts">
  // The Level editor tab's OpenRAC Editor section, under Your levels (EditorFlow): which editor is
  // there (a download, a Mac app or a folder the player picked; core/src/editor_install.rs), a newer
  // one, and download, choose or remove. The level projects are never touched here.
  import { onMount } from "svelte";

  import { api } from "$lib/api";
  import {
    app,
    chooseEditor,
    forgetEditor,
    guardShown,
    installEditor,
    readEditor,
    removeEditor,
  } from "$lib/app.svelte";

  import Icon from "./Icon.svelte";

  let newest = $state<string | null>(null);
  let looking = $state(false);
  let lookFailed = $state(false);

  const installed = $derived(app.editor?.version ?? null);
  // An app the player installed, or a folder they picked: the launcher neither downloads into it
  // nor removes it.
  const own = $derived(!!app.editor?.installedApp || !!app.editor?.chosen);
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

<section class="program" aria-labelledby="editor-install-title">
  <h3 id="editor-install-title">OpenRAC Editor</h3>
  <p class="muted">
    The program that changes your levels. The launcher downloads it, or uses one you have.
  </p>
  <p class="status dim">
    {#if installed}
      <Icon name="check" size={14} />
      {#if app.editor?.chosen}OpenRAC Editor {installed} from
        <span class="mono">{app.editor.folder}</span>.{:else if app.editor?.installedApp}OpenRAC
        Editor {installed} is installed.{:else}OpenRAC Editor {installed} is downloaded.{/if}
    {:else}
      <Icon name="info" size={14} /> Not downloaded.
    {/if}
    {#if app.editor?.problem}<span class="warn">{app.editor.problem}.</span>{/if}
    {#if looking}· Looking…{:else if newer}· {newest} is out.{:else if newest && installed}· You
      have the latest.{:else if lookFailed}· Could not check.{/if}
  </p>
  <div class="row">
    {#if !own}
      <button
        class={`small${!installed || newer ? " primary" : ""}`}
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
    {/if}
    {#if app.editor?.chosen}
      <button class="small" onclick={() => void forgetEditor()}
        ><Icon name="x" size={14} />Stop using this folder</button
      >
    {:else}
      <button class="small" onclick={() => void chooseEditor()}
        ><Icon name="folder" size={14} />Choose its folder</button
      >
    {/if}
    {#if installed && !own}
      <button
        class="small ghost"
        disabled={app.editorDownloading}
        onclick={() => void removeEditor()}><Icon name="trash" size={14} />Remove</button
      >
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
  /* As Your levels above it: a line over it, the title close over its text. */
  .program {
    display: flex;
    flex-direction: column;
    gap: 4px;
    border-top: 1px solid var(--line);
    padding-top: 14px;
  }
  .program > h3 {
    font-size: 1rem;
  }
  .program > p {
    margin: 0;
    font-size: 0.8125rem;
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
    margin-top: 8px;
  }
</style>
