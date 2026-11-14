<script lang="ts">
  // Make your own → Models and textures → Interface textures: the HUD, the menus, the font and the
  // effects, rebuilt from the player's disc as the game makes them and named as texture packs name
  // them (docs/port/TEXTURE_PACKS.md, "Export without playing"). The export reads a run (which
  // textures the game draws and where their bytes are on the disc): the project's, shown in green,
  // and the player's own, which "Make a run" plays here (core/src/interface_runs.rs,
  // port/tools/interface_runs/run.py).
  import Icon from "$components/Icon.svelte";
  import { api, ask, type ActionView, type InterfaceRunsView, type VersionView } from "$lib/api";
  import { app, cancel, guardShown, jobs, runChain, toast } from "$lib/app.svelte";
  import { INTERFACE } from "$lib/modsText";

  let { version: v }: { version: VersionView } = $props();

  const CATEGORIES = [
    { id: "font", label: INTERFACE.font },
    { id: "interface", label: INTERFACE.hud },
    { id: "effects", label: INTERFACE.effects },
  ] as const;

  const scope = $derived({ kind: "version" as const, key: v.key });
  const action = (id: string): ActionView | null => v.actions.find((a) => a.id === id && a.thisPlatform) ?? null;
  const exporter = $derived(action("interface-export"));
  const runner = $derived(action("interface-run"));

  let view = $state<InterfaceRunsView | null>(null);
  let failed = $state<string | null>(null);
  /** The kinds ticked; all that can be exported at first. */
  let chosen = $state<Record<string, boolean>>({ font: true, interface: true, effects: true });
  let busy = $state<"export" | "run" | null>(null);
  let said = $state<{ text: string; failed: boolean } | null>(null);

  async function refresh() {
    try {
      view = await api.interfaceRuns(v.game, v.name);
      failed = null;
    } catch (e) {
      failed = String(e);
    }
  }

  // Again for another version, and when a run or an export ends (a new run file).
  $effect(() => {
    if (v.key && busy === null) void refresh();
  });

  const available = (at: number) => view?.sources[at] !== undefined && view.sources[at] !== "none";
  /** The export action's option: the kinds ticked that a run has, in its order (core option()). */
  const option = $derived(
    CATEGORIES.filter((c, at) => chosen[c.id] && available(at))
      .map((c) => c.id)
      .join(","),
  );

  function date(made: string | null | undefined): string {
    if (!made) return "";
    const day = new Date(`${made}T12:00:00Z`);
    return Number.isNaN(day.getTime()) ? made : day.toLocaleDateString(undefined, { dateStyle: "medium" });
  }

  function status(at: number): { text: string; tone: "ok" | "own" | "none" } {
    const source = view?.sources[at];
    if (source === "official" && view?.official) {
      return { text: INTERFACE.official(date(view.official.made), view.official.counts[at] ?? 0), tone: "ok" };
    }
    if (source === "local" && view?.local) {
      return { text: INTERFACE.yours(date(view.local.made), view.local.counts[at] ?? 0), tone: "own" };
    }
    return { text: view?.runnable && runner ? INTERFACE.noneRun : INTERFACE.noneYet, tone: "none" };
  }

  /** The newest job of an action here. */
  const last = (id: string) =>
    jobs.list.filter((j) => j.scope.kind === "version" && j.scope.key === v.key && j.actionId === id).at(-1);

  /** The JSON events a job printed, newest last. */
  function events(id: string): Record<string, unknown>[] {
    const out: Record<string, unknown>[] = [];
    for (const l of last(id)?.lines ?? []) {
      if (!l.line.startsWith("{")) continue;
      try {
        out.push(JSON.parse(l.line) as Record<string, unknown>);
      } catch {
        // Not an event: a line of the program's log.
      }
    }
    return out;
  }

  /** "Route 12 of 54", while a run plays. */
  const progress = $derived.by(() => {
    if (busy !== "run") return null;
    const route = events("interface-run").findLast((e) => e.event === "route");
    const start = events("interface-run").findLast((e) => e.event === "start");
    const of = Number(route?.of ?? start?.routes ?? 0);
    return of ? INTERFACE.routeOf(Number(route?.done ?? 0), of) : INTERFACE.starting;
  });

  async function exportTextures() {
    if (!exporter || !option) return;
    busy = "export";
    said = null;
    const ok = await runChain(scope, [exporter], { option });
    busy = null;
    const done = events("interface-export").findLast((e) => e.event === "done") as
      { written?: Record<string, number>; skipped?: Record<string, number> } | undefined;
    const error = events("interface-export").findLast((e) => e.event === "error");
    const folder = last("interface-export")?.outputs[0] ?? null;
    if (ok && done && folder) {
      const written = Object.values(done.written ?? {}).reduce((a, b) => a + b, 0);
      const skipped = Object.values(done.skipped ?? {}).reduce((a, b) => a + b, 0);
      said = { text: INTERFACE.exported(written, skipped), failed: false };
      await guardShown(api.openOutput(folder));
    } else {
      said = { text: typeof error?.message === "string" ? error.message : INTERFACE.exportStopped, failed: true };
    }
  }

  async function makeRun() {
    if (!runner) return;
    if (!(await ask(INTERFACE.runAsk, INTERFACE.runYes))) return;
    busy = "run";
    said = null;
    const ok = await runChain(scope, [runner]);
    busy = null;
    await refresh();
    if (!ok) {
      const error = events("interface-run").findLast((e) => e.event === "error");
      said = { text: typeof error?.message === "string" ? error.message : INTERFACE.runStopped, failed: true };
    } else {
      const failedRoutes = view?.local && !view.local.clean;
      said = { text: failedRoutes ? INTERFACE.runPartly : INTERFACE.runDone, failed: false };
      toast(INTERFACE.runDone);
    }
  }

  async function stopRun() {
    const job = last("interface-run");
    if (job) await cancel(job);
  }

  async function share() {
    if (!view) return;
    await guardShown(api.openOutput(view.shareFile));
    await guardShown(api.openUrl(INTERFACE.uploadUrl(view.id)));
  }
</script>

{#if exporter}
  <section class="interface" aria-labelledby="interface-title">
    <h3 id="interface-title">{INTERFACE.title}</h3>
    <p class="muted lead">{INTERFACE.lead}</p>
    {#if failed}
      <p class="failed">{failed}</p>
    {/if}
    <ul class="kinds">
      {#each CATEGORIES as c, at (c.id)}
        {@const s = status(at)}
        <li>
          <label class:off={!available(at)}>
            <input type="checkbox" bind:checked={chosen[c.id]} disabled={!available(at) || busy !== null} />
            {c.label}
          </label>
          <span class="chip {s.tone}">
            {#if s.tone === "ok"}<Icon name="check" size={14} />{/if}{s.text}
          </span>
        </li>
      {/each}
    </ul>
    {#if view?.local && !view.local.current}
      <p class="muted">{INTERFACE.outOfDate}</p>
    {/if}
    {#each view?.unreadable ?? [] as why (why)}
      <p class="muted">{INTERFACE.unreadable(why)}</p>
    {/each}
    <div class="controls">
      <button disabled={!exporter.runnable || !option || busy !== null} onclick={() => void exportTextures()}>
        {#if busy === "export"}<span class="spinner"></span>{:else}<Icon
            name="external"
            size={16}
          />{/if}{INTERFACE.export}
      </button>
      {#if runner && view?.runnable}
        {#if busy === "run"}
          <button class="ghost" onclick={() => void stopRun()}><Icon name="x" size={16} />{INTERFACE.stop}</button>
        {:else}
          <button class="ghost" disabled={!runner.runnable || busy !== null} onclick={() => void makeRun()}
            ><Icon name="build" size={16} />{view.local ? INTERFACE.runAgain : INTERFACE.run}</button
          >
        {/if}
      {/if}
    </div>
    <p class="muted" class:failed={said?.failed} class:done={said && !said.failed}>
      {#if busy === "export"}
        {INTERFACE.exporting}
      {:else if busy === "run"}
        {progress} · {INTERFACE.runLong}
      {:else if said}
        {said.text}
      {:else if !exporter.runnable}
        {INTERFACE.notReady}
      {:else if !option}
        {view?.runnable && runner ? INTERFACE.nothingYetRun : INTERFACE.nothingYet}
      {:else}
        {INTERFACE.exportLine}
      {/if}
    </p>
    {#if view?.suggestSharing && busy === null}
      <div class="share">
        <p>{INTERFACE.shareLead}</p>
        <button onclick={() => void share()}><Icon name="upload" size={16} />{INTERFACE.share}</button>
      </div>
    {/if}
    {#if app.config?.developer}
      <p class="muted small">{INTERFACE.developer(v.key.replace("/", "-"))}</p>
    {/if}
  </section>
{/if}

<style>
  .interface {
    display: flex;
    flex-direction: column;
    gap: 8px;
    margin-top: 12px;
    padding-top: 12px;
    border-top: 1px solid var(--line);
  }

  h3,
  p {
    margin: 0;
  }

  .lead,
  .small {
    font-size: 0.875rem;
  }

  .kinds {
    list-style: none;
    margin: 0;
    padding: 0;
    display: flex;
    flex-direction: column;
    gap: 6px;
  }

  .kinds li {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    justify-content: space-between;
    gap: 6px 12px;
  }

  label {
    display: flex;
    align-items: center;
    gap: 8px;
  }

  label.off {
    opacity: 0.6;
  }

  .chip {
    display: inline-flex;
    align-items: center;
    gap: 4px;
    font-size: 0.8125rem;
    padding: 2px 8px;
    border-radius: 999px;
    border: 1px solid var(--line);
  }

  .chip.ok {
    color: var(--ok);
    border-color: var(--ok);
  }

  .chip.own {
    color: var(--amber);
    border-color: var(--amber);
  }

  .chip.none {
    color: var(--dim);
  }

  .controls {
    display: flex;
    flex-wrap: wrap;
    gap: 8px;
  }

  .share {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 8px 12px;
    padding: 10px 12px;
    border: 1px solid var(--ok);
    border-radius: 8px;
  }

  .share p {
    flex: 1 1 240px;
  }

  .failed {
    color: var(--err);
  }

  .done {
    color: var(--ok);
  }
</style>
