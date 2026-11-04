<script lang="ts">
  // A mod's own page (core/src/mod_tools.rs), opened with its Open button (ModPage.svelte): the way
  // back, and the page in a frame of its own, served from the mod's folder (src-tauri/src/mod_tool.rs).
  // The frame is another site to the launcher: it cannot reach the launcher's page or its commands,
  // only send the messages lib/modTool.ts answers. While it is shown, the controller's buttons go to
  // the page as messages ({ openrac: 1, event: "pad", button, down }), and the page moves its own
  // focus; B is the page's too, and its "close" call (or Back here, or Escape outside the frame)
  // returns to the mod's page.
  import { onDestroy, onMount } from "svelte";
  import { convertFileSrc } from "@tauri-apps/api/core";
  import { inTauri, onAndroid, type PluginView } from "$lib/api";
  import { launcherBackend, pageOrigin, pageUrl, PROTOCOL, reply, type ToolInfo } from "$lib/modTool";
  import { routePad } from "$lib/pad";
  import * as T from "$lib/modsText";
  import Icon from "./Icon.svelte";
  import Reason from "./Reason.svelte";

  let {
    mod,
    game,
    version,
    onclose,
  }: {
    mod: PluginView & { tool: string };
    game: string;
    version: string;
    onclose: () => void;
  } = $props();

  let frame = $state<HTMLIFrameElement | null>(null);
  let failure = $state<string | null>(null);

  const info = $derived<ToolInfo>({
    game,
    version,
    packId: `${game}-${version}`,
    mod: { id: mod.id, name: mod.name, version: mod.version },
    system: onAndroid ? "android" : "desktop",
  });
  // The scheme's root as this system reaches it: openrac-mod://localhost/, or
  // http://openrac-mod.localhost/ on Windows and Android.
  const src = $derived(inTauri ? pageUrl(convertFileSrc("", "openrac-mod"), game, mod.id, mod.tool) : null);

  // The page's site: messages are taken only from it and sent only to it, never to a page the
  // frame was sent away to. (A custom scheme's URL has no origin in the URL API: it is written out.)
  const origin = $derived(src ? pageOrigin(src) : null);

  function post(message: Record<string, unknown>, transfer: Transferable[] = []) {
    if (origin) frame?.contentWindow?.postMessage(message, origin, transfer);
  }

  function onMessage(e: MessageEvent) {
    void handle(e);
  }

  async function handle(e: MessageEvent) {
    if (e.source !== frame?.contentWindow || e.origin !== origin) return;
    const data: unknown = e.data;
    const asked = data as { openrac?: unknown; call?: unknown } | null;
    if (asked?.openrac === PROTOCOL && asked.call === "close") {
      onclose();
      return;
    }
    const answer = await reply(data, info, launcherBackend);
    if (!answer) return;
    if (answer.message.ok === false) failure = T.MOD_TOOL.failed(String(answer.message.error));
    post(answer.message, answer.transfer);
  }

  onMount(() => {
    window.addEventListener("message", onMessage);
    routePad((button, down) => {
      post({ openrac: PROTOCOL, event: "pad", button, down });
    });
    const key = (e: KeyboardEvent) => {
      if (e.key === "Escape" && !e.defaultPrevented) {
        e.preventDefault();
        onclose();
      }
    };
    window.addEventListener("keydown", key);
    return () => {
      window.removeEventListener("keydown", key);
    };
  });

  onDestroy(() => {
    window.removeEventListener("message", onMessage);
    routePad(null);
  });
</script>

<div class="mod-tool">
  <nav class="crumbs" aria-label={T.MOD_PAGE.crumbsLabel}>
    <button class="back" aria-label={T.MOD_TOOL.backTo(mod.name)} title={T.MOD_TOOL.backTo(mod.name)} onclick={onclose}
      ><Icon name="arrow" size={18} /></button
    >
    <ol>
      <li><button class="crumb" onclick={onclose}>{mod.name}</button></li>
      <li aria-current="page"><Icon name="next" size={14} /><span>{T.MOD_PAGE.openTool}</span></li>
    </ol>
  </nav>
  {#if failure}<Reason text={failure} tone="err" role="status" />{/if}
  {#if src}
    <!-- allow-same-origin keeps the page its own site (openrac-mod), never the launcher's: it lets
         the page load its own files and workers; scripts run, nothing else (no popups, forms or
         leaving the frame). -->
    <iframe
      bind:this={frame}
      class="page"
      title={T.MOD_TOOL.frameLabel(mod.name)}
      {src}
      sandbox="allow-scripts allow-same-origin"
      allow="gamepad 'none'; camera 'none'; microphone 'none'; geolocation 'none'"
      referrerpolicy="no-referrer"
    ></iframe>
  {:else}
    <p class="panel dim">{T.MOD_TOOL.onlyInApp}</p>
  {/if}
</div>

<style>
  .mod-tool {
    display: flex;
    flex-direction: column;
    gap: var(--page-gap, 16px);
    min-height: 0;
  }

  /* Where you are: back, then the mod › its page (as ModPage.svelte's). */
  .crumbs {
    display: flex;
    flex-direction: row;
    align-items: center;
    gap: 10px;
  }

  .crumbs ol {
    display: flex;
    align-items: center;
    gap: 6px;
    margin: 0;
    padding: 0;
    list-style: none;
    font-size: 0.875rem;
    min-width: 0;
  }

  .crumbs li {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    min-width: 0;
  }

  .crumbs li[aria-current="page"] {
    color: var(--text);
    font-weight: 600;
  }

  .crumbs li[aria-current="page"] :global(svg) {
    color: var(--dim);
    flex: none;
  }

  .back {
    width: 40px;
    height: 40px;
    padding: 0;
    justify-content: center;
    flex: none;
  }

  .crumb {
    border: 0;
    background: none;
    padding: 6px 4px;
    border-radius: 6px;
    color: var(--amber);
    font-weight: 600;
  }

  /* The page takes what the window has left under the crumbs. */
  .page {
    width: 100%;
    height: calc(100vh - 180px);
    min-height: 420px;
    border: 0;
    border-radius: 12px;
    background: var(--panel);
  }
</style>
