<script lang="ts">
  // Add a mod, the Mods tab's first panel: one field for a link, and a file or a folder beside it,
  // or a .zip or folder dropped on the page. The player never says which kind it is: a native or
  // script mod goes to the gameplay mods, a texture and model pack to the packs (lib/addMods.ts).
  // A link is read first (core/src/remote.rs): what it holds shows under the field, each mod with
  // whether it fits this computer's game and its Install button, before anything is installed.
  // The field takes a friend's mod code too, and so does a paste anywhere on the page: the code's
  // review opens (lib/modCodes), which changes nothing before its Apply.
  // With developer tools on, the preview adds the commit read and each mod's folder, asset and discs.
  import { onMount } from "svelte";
  import {
    api,
    inTauri,
    onAndroid,
    onDrop,
    pickFolder,
    pickZip,
    type RemoteMod,
    type RemotePreview,
    type VersionView,
  } from "$lib/api";
  import { addMods, type Added } from "$lib/addMods";
  import { app } from "$lib/app.svelte";
  import { looksLikeCode } from "$lib/modCodes";
  import { openCode } from "$lib/modCodes.svelte";
  import { openLink } from "$lib/deepLink.svelte";
  import { hubLink, isSiteLink } from "$lib/siteLinks";
  import { modHint, modKind } from "$lib/plugins";
  import * as T from "$lib/modsText";
  import Icon from "./Icon.svelte";
  import Reason from "./Reason.svelte";

  let {
    version: v,
    links = true,
    files = true,
    onadded = () => undefined,
  }: {
    version: VersionView;
    /** Whether a link can be read for this game (it has a port that plays). */
    links?: boolean;
    /** Whether a file or folder can be added here. */
    files?: boolean;
    /** Something was added: which lists to read again. */
    onadded?: (added: Pick<Added, "plugin" | "pack">) => void;
  } = $props();

  const developer = $derived(app.config?.developer ?? false);

  let link = $state("");
  let reading = $state(false);
  let preview = $state<RemotePreview | null>(null);
  let previewError = $state<string | null>(null);
  let installing = $state<string | null>(null);
  let adding = $state(false);
  let dropping = $state(false);
  /** What a file or folder being added is at: copied in (Android, with its share when known), then
   *  installed. Shown under the buttons from the moment the picker closes until the add says. */
  let busy = $state<{ text: string; share: number | null } | null>(null);
  /** What the last add said, in a sentence. */
  let said = $state<{ text: string; failed: boolean } | null>(null);

  const message = (e: unknown) => (typeof e === "string" ? e : String(e));
  const nameOf = (path: string) => path.split(/[\\/]/).at(-1) ?? path;

  async function readLink(e: SubmitEvent) {
    e.preventDefault();
    if (!link.trim()) return;
    if (looksLikeCode(link)) {
      openCode(link, v);
      link = "";
      return;
    }
    preview = null;
    previewError = null;
    said = null;
    // A mod's page on the website goes to the same change set as its Install button.
    const hub = hubLink(link);
    if (hub) {
      link = "";
      void openLink(hub);
      return;
    }
    if (isSiteLink(link)) {
      previewError = T.ADD.notModPage;
      return;
    }
    reading = true;
    try {
      preview = await api.pluginsPreview(v.game, v.name, link.trim());
    } catch (err) {
      previewError = message(err);
    }
    reading = false;
  }

  async function installRemote(m: RemoteMod) {
    if (!preview) return;
    installing = m.id;
    try {
      const done = await api.pluginsInstallRemote(v.game, v.name, link.trim(), m.id);
      if (done.kind === "pack") {
        // A data pack's files, downloaded: the packs take them (overrides-add).
        const added = await addMods(v, [done.path]);
        said = { text: added.text, failed: added.failed };
        onadded({ plugin: false, pack: added.pack });
      } else {
        said = {
          text: done.replaced
            ? T.installedFrom.replaced(done.name, done.version, preview.label)
            : done.enabled
              ? T.installedFrom.on(done.name, preview.label)
              : T.installedFrom.off(done.name, preview.label),
          failed: false,
        };
        onadded({ plugin: true, pack: false });
      }
      if (preview.mods.length > 1) {
        // A repository of several mods (openrac-mods): the preview stays for the others, this one
        // now installed.
        preview = { ...preview, mods: preview.mods.map((x) => (x.id === m.id ? { ...x, installed: m.version } : x)) };
      } else {
        preview = null;
        link = "";
      }
    } catch (err) {
      said = { text: T.withName(m.name, message(err)), failed: true };
    }
    installing = null;
  }

  /** Adds to `target`, the game the player was on when they picked or dropped (not the one shown
   *  once a long copy ends). */
  async function add(paths: string[], target: VersionView = v) {
    adding = true;
    said = null;
    busy = {
      text: paths.length === 1 ? T.ADD.adding(nameOf(paths[0] ?? "")) : T.ADD.addingSeveral(paths.length),
      share: null,
    };
    try {
      const added = await addMods(target, paths);
      said = { text: added.text, failed: added.failed };
      onadded({ plugin: added.plugin, pack: added.pack });
    } catch (err) {
      said = { text: message(err), failed: true };
    }
    busy = null;
    adding = false;
  }

  /** A picker's answer, or what stopped it said under the buttons (a copy that failed on Android,
   *  a picker that could not open): never a click that seems to do nothing. */
  async function picked(pick: () => Promise<string | null>) {
    const target = v;
    said = null;
    let path: string | null;
    try {
      path = await pick();
    } catch (err) {
      path = null;
      said = { text: message(err), failed: true };
    }
    if (path) await add([path], target);
    else {
      busy = null;
      adding = false;
    }
  }

  function addZip() {
    // Android copies the file into the app first: said from its first event, as the picker closes.
    return picked(() =>
      pickZip(T.ADD.pickZip, (copy) => {
        adding = true;
        busy = { text: T.ADD.copying(copy.name), share: copy.total > 0 ? Math.min(1, copy.copied / copy.total) : null };
      }),
    );
  }

  function addFolder() {
    return picked(async () => (inTauri ? await pickFolder(T.ADD.pickFolder) : "/home/you/Downloads/lock-strafe"));
  }

  /** A code pasted anywhere but in a field: there is only one thing it can mean. */
  function pasted(e: ClipboardEvent) {
    const target = e.target as HTMLElement | null;
    if (!links || target?.closest("input, textarea, [contenteditable]")) return;
    const text = e.clipboardData?.getData("text") ?? "";
    if (looksLikeCode(text)) openCode(text, v);
  }

  onMount(() => {
    if (!files) return;
    let stop: (() => void) | undefined;
    // The listener is registered when the promise is answered, which may be after this panel is
    // closed: it is then taken away at once, or a file dropped later would start an add for a
    // game the player has left.
    let closed = false;
    void onDrop(
      (paths) => {
        dropping = false;
        // One add at a time: a drop while one runs is let go, as the buttons are greyed then.
        if (!adding) void add(paths);
      },
      (over) => (dropping = over),
    ).then((off) => {
      if (closed) off();
      else stop = off;
    });
    return () => {
      closed = true;
      stop?.();
    };
  });
</script>

<svelte:document onpaste={pasted} />

<section class="panel add" class:dropping aria-label={T.ADD.title}>
  <div class="row head">
    <h2 class="grow">{T.ADD.title}</h2>
  </div>

  <div class="adds">
    {#if links}
      <form class="by-link" onsubmit={(e) => void readLink(e)}>
        <label class="grow">
          <span class="sr-only">{T.ADD.linkLabel}</span>
          <input
            type="text"
            inputmode="url"
            spellcheck="false"
            autocomplete="off"
            placeholder={T.ADD.linkPlaceholder}
            bind:value={link}
          />
        </label>
        <button type="submit" disabled={reading || !link.trim()}
          >{#if reading}<span class="spinner"></span>{/if}{T.ADD.go}</button
        >
      </form>
    {/if}
    {#if files}
      <div class="files">
        <button disabled={adding} onclick={() => void addZip()}
          >{#if adding}<span class="spinner"></span>{:else}<Icon name="library" size={16} />{/if}{T.ADD.file}</button
        >
        {#if !onAndroid}
          <!-- Android's picker gives no folder the launcher can read: a .zip only. -->
          <button disabled={adding} onclick={() => void addFolder()}
            ><Icon name="folder" size={16} />{T.ADD.folder}</button
          >
        {/if}
      </div>
    {/if}
  </div>
  {#if files && inTauri && !onAndroid}<p class="dim hint">{dropping ? T.ADD.dropHere : T.ADD.drop}</p>{/if}

  {#if busy}
    <div class="busy" role="status">
      <p>
        <span class="spinner"></span><span class="grow">{busy.text}</span>{#if busy.share !== null}<span
            class="mono dim">{Math.round(busy.share * 100)}%</span
          >{/if}
      </p>
      <div class="track" aria-hidden="true">
        {#if busy.share !== null}<i style:width={`${busy.share * 100}%`}></i>{:else}<i class="moving"></i>{/if}
      </div>
    </div>
  {/if}
  {#if previewError}<Reason text={previewError} tone="err" role="status" />{/if}
  {#if said}<Reason text={said.text} tone={said.failed ? "err" : "ok"} role="status" />{/if}

  {#if preview}
    {@const from = T.fromPublisher(preview.label, preview.owner)}
    <div class="preview" aria-label={T.modsAria(preview.label)}>
      <div class="row from">
        <p class="grow">
          {from.before}<strong>{from.label}</strong>{from.after}{#if developer}
            <span class="mono dim"
              >· {preview.reference}{#if preview.commit}&nbsp;({preview.commit.slice(0, 10)}){/if}</span
            >{/if}
        </p>
        <button class="small ghost" aria-label={T.ADD.close} title={T.ADD.close} onclick={() => (preview = null)}
          ><Icon name="x" size={14} /></button
        >
      </div>
      {#if preview.note}<p class="dim note">{preview.note}</p>{/if}
      {#if preview.mods.some((m) => m.kind === "native" && m.compatible)}
        <p class="caution"><Icon name="info" size={16} /><span>{T.PREVIEW.nativeWarning}</span></p>
      {/if}
      <ul>
        {#each preview.mods as m (m.id)}
          {@const kind = modKind(m.kind)}
          <li class:unfit={!m.compatible}>
            <div class="line">
              <div class="grow">
                <div class="name">
                  {m.name}<span class="meta"
                    >{m.version}{#if m.author}&nbsp;· {m.author}{/if}</span
                  >
                </div>
                {#if m.description}<div class="dim desc">{m.description}</div>{/if}
                <div class="meta tags">
                  <span class={`kind ${kind.tone}`} title={modHint(m.kind)}><span class="dot"></span>{kind.text}</span
                  >{#if m.installed}<span class="dim">&nbsp;· {T.installedAlready(m.installed)}</span>{/if}
                </div>
              </div>
              {#if m.compatible}
                <button disabled={installing !== null} onclick={() => void installRemote(m)}
                  >{#if installing === m.id}<span class="spinner"></span>{:else}<Icon
                      name="check"
                      size={16}
                    />{/if}{m.installed === m.version
                    ? T.PREVIEW.reinstall
                    : m.installed
                      ? T.updateTo(m.version)
                      : T.PREVIEW.install}</button
                >
              {:else}
                <span class="state err"><span class="dot"></span>{T.COMPATIBLE_LABEL.no.text}</span>
              {/if}
            </div>
            {#if !m.compatible && m.why}<div class="why"><Reason text={m.why} tone="err" /></div>{/if}
            {#if developer}
              <div class="dev dim">
                <code>{m.id}</code>{#if m.path}&nbsp;· <span class="mono">{m.path}/</span>{/if}{#if m.asset}&nbsp;·
                  <span class="mono">{m.asset}</span> ({m.tag}){/if}{#if m.api}&nbsp;· {T.PANEL.interface} {m.api}{/if}
                · {Object.entries(m.games)
                  .map(([g, d]) => `${g}: ${d.join(", ")}`)
                  .join("; ")}
              </div>
            {/if}
          </li>
        {/each}
      </ul>
    </div>
  {/if}
</section>

<style>
  .add {
    display: flex;
    flex-direction: column;
    gap: 10px;
    transition: border-color var(--fast) var(--ease);
  }

  /* A file held over the page: the panel it lands in says so. */
  .add.dropping {
    background: var(--on-fill);
    color: var(--amber);
    border-style: dashed;
  }

  h2 {
    font-size: 1rem;
  }

  .adds {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 10px;
  }

  .by-link {
    flex: 1 1 360px;
    display: flex;
    align-items: center;
    gap: 8px;
  }

  .by-link input {
    padding-block: 9px;
  }

  .by-link input:focus-visible {
    outline: 2px solid var(--amber);
    outline-offset: 2px;
  }

  .files {
    display: flex;
    gap: 8px;
  }

  .adds button {
    padding: 8px 16px;
  }

  /* A file being added: what it is at and a bar, as a job's progress reads elsewhere. */
  .busy {
    display: flex;
    flex-direction: column;
    gap: 6px;
    animation: rise var(--medium) var(--ease) both;
  }

  .busy p {
    display: flex;
    align-items: center;
    gap: 8px;
    margin: 0;
    font-size: 0.8125rem;
    color: var(--body);
  }

  .busy .spinner {
    color: var(--amber);
  }

  .hint {
    margin: -2px 0 0;
    font-size: 0.8125rem;
  }

  .sr-only {
    position: absolute;
    width: 1px;
    height: 1px;
    overflow: hidden;
    clip-path: inset(50%);
    white-space: nowrap;
  }

  /* What a link holds, on the panel's darker ground until it is installed. */
  .preview {
    display: flex;
    flex-direction: column;
    gap: 8px;
    margin-top: 2px;
    padding: 12px 16px 4px;
    border-radius: var(--radius-sm);
    background: var(--ink-deep);
    animation: rise var(--medium) var(--ease) both;
  }

  .from p {
    margin: 0;
    font-size: 0.875rem;
    color: var(--soft);
  }

  .from strong {
    color: var(--text);
  }

  .note {
    margin: 0;
    font-size: 0.8125rem;
  }

  /* Informs, does not alarm: the info mark and the body's colour, the amber kept for the mods it means. */
  .caution {
    display: flex;
    align-items: flex-start;
    gap: 8px;
    margin: 0;
    font-size: 0.8125rem;
    color: var(--body);
  }

  .caution :global(svg) {
    flex: none;
    color: var(--amber);
    margin-top: 1px;
  }

  ul {
    margin: 0;
    padding: 0;
    list-style: none;
  }

  li {
    padding: 11px 0;
    border-top: 1px solid var(--line);
  }

  .line {
    display: flex;
    align-items: center;
    gap: 12px;
  }

  .name {
    font-size: 0.875rem;
    font-weight: 600;
  }

  li.unfit .name {
    color: var(--dim);
  }

  .meta {
    margin-left: 0.5em;
    font-size: 0.8125rem;
    font-weight: 400;
    color: var(--dim);
  }

  .tags {
    margin: 3px 0 0;
  }

  .desc {
    margin-top: 1px;
    font-size: 0.8125rem;
  }

  .kind,
  .state {
    display: inline-flex;
    align-items: center;
    gap: 7px;
    font-size: 0.8125rem;
    color: var(--dim);
    white-space: nowrap;
  }

  .kind.warn {
    color: var(--warn);
  }

  .kind.ok .dot {
    background: var(--ok);
  }

  .dot {
    width: 7px;
    height: 7px;
    border-radius: 50%;
    background: currentColor;
  }

  .state {
    flex: none;
  }

  .state.err {
    color: var(--err);
  }

  .why {
    margin: 6px 0 0;
    font-size: 0.8125rem;
  }

  .dev {
    margin-top: 6px;
    font-size: 0.75rem;
    overflow-wrap: anywhere;
  }
</style>
