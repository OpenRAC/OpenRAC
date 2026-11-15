<script lang="ts">
  // Make your own → Models and textures → Interface textures: the HUD, the menus, the font and the
  // effects, rebuilt from the player's disc as the game makes them and named as texture packs name
  // them (docs/port/TEXTURE_PACKS.md, "Export without playing"). The export reads the player's run
  // (which textures the game draws and where their bytes are on the disc), which "Make" plays here in
  // a few minutes (core/src/interface_runs.rs, port/tools/interface_runs/run.py).
  import Icon from "$components/Icon.svelte";
  import Menu from "$components/Menu.svelte";
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
  /** What this panel started, and for which version (another version shown meanwhile is not busy). */
  let busy = $state<{ kind: "export" | "run"; key: string } | null>(null);
  /** The kinds the export takes: each one a run has, unless switched off. */
  let chosen = $state<Record<string, boolean>>({ font: true, interface: true, effects: true });
  let said = $state<{ text: string; failed: boolean } | null>(null);

  async function refresh() {
    // Another version may be chosen while this one is read: its answer is not this one's.
    const key = v.key;
    try {
      const read = await api.interfaceRuns(v.game, v.name);
      if (key !== v.key) return;
      view = read;
      failed = null;
    } catch (e) {
      if (key === v.key) failed = String(e);
    }
  }

  /** A job of an action here that is queued or running: one started before this panel was shown
   *  (another tab, the panel closed and opened) counts too. */
  const active = (id: string) =>
    jobs.list.some(
      (j) =>
        j.scope.kind === "version" &&
        j.scope.key === v.key &&
        j.actionId === id &&
        (j.state === "queued" || j.state === "running"),
    );
  const working = $derived<"export" | "run" | null>(
    (busy?.key === v.key ? busy.kind : null) ??
      (active("interface-run") ? "run" : active("interface-export") ? "export" : null),
  );

  // Again for another version, and when a run or an export ends (a new run file).
  $effect(() => {
    if (v.key && working === null) void refresh();
  });

  // Another version: what was said of the last one goes.
  $effect(() => {
    if (v.key) said = null;
  });

  const canRun = $derived(Boolean(runner && view?.runnable));
  const available = (at: number) => view?.available[at] === true;
  /** The export action's option: the kinds switched on that a run has, in their order (core option()). */
  const option = $derived(
    CATEGORIES.filter((c, at) => chosen[c.id] && available(at))
      .map((c) => c.id)
      .join(","),
  );
  /** A kind a run here would make: the player's run is not whole and current in it. */
  function makeable(at: number): boolean {
    if (!canRun) return false;
    const l = view?.local;
    return !(l && l.current && l.complete[at]);
  }
  const missing = $derived(CATEGORIES.filter((_, at) => makeable(at)).map((c) => c.id));

  function date(made: string | null | undefined): string {
    if (!made) return "";
    const day = new Date(`${made}T12:00:00Z`);
    return Number.isNaN(day.getTime()) ? made : day.toLocaleDateString(undefined, { month: "short", day: "numeric" });
  }

  /** What the run holds of a kind, in a few words: whole (a tick), part way, or none. */
  function status(at: number): { text: string; tone: "ok" | "own" | "none" } {
    const l = view?.local;
    if (l && !l.current) return { text: INTERFACE.outOfDate, tone: "none" };
    if (available(at) && l) {
      const n = l.counts[at] ?? 0;
      return l.complete[at]
        ? { text: INTERFACE.made(date(l.kindMade[at] ?? l.made), n), tone: "ok" }
        : { text: INTERFACE.partly(n), tone: "own" };
    }
    return { text: INTERFACE.none, tone: "none" };
  }

  /** The newest job of an action here. */
  const last = (id: string, key = v.key) =>
    jobs.list.filter((j) => j.scope.kind === "version" && j.scope.key === key && j.actionId === id).at(-1);

  /** The JSON events a job printed, newest last. */
  function events(id: string, key = v.key): Record<string, unknown>[] {
    const out: Record<string, unknown>[] = [];
    for (const l of last(id, key)?.lines ?? []) {
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
    if (working !== "run") return null;
    const route = events("interface-run").findLast((e) => e.event === "route");
    const start = events("interface-run").findLast((e) => e.event === "start");
    const of = Number(route?.of ?? start?.routes ?? 0);
    return of ? INTERFACE.routeOf(Number(route?.done ?? 0), of) : INTERFACE.starting;
  });

  async function exportTextures() {
    if (!exporter || !option) return;
    const key = v.key;
    busy = { kind: "export", key };
    said = null;
    const ok = await runChain(scope, [exporter], { option });
    busy = null;
    const done = events("interface-export", key).findLast((e) => e.event === "done") as
      { written?: Record<string, number>; skipped?: Record<string, number> } | undefined;
    const error = events("interface-export", key).findLast((e) => e.event === "error");
    const folder = last("interface-export", key)?.outputs[0] ?? null;
    if (key !== v.key) {
      // Another version is shown now: the folder still opens, nothing is said of it here.
      if (ok && done && folder) await guardShown(api.openOutput(folder));
      return;
    }
    if (ok && done && folder) {
      const written = Object.values(done.written ?? {}).reduce((a, b) => a + b, 0);
      const skipped = Object.values(done.skipped ?? {}).reduce((a, b) => a + b, 0);
      said = { text: INTERFACE.exported(written, skipped), failed: false };
      await guardShown(api.openOutput(folder));
    } else {
      said = { text: typeof error?.message === "string" ? error.message : INTERFACE.exportStopped, failed: true };
    }
  }

  /** Plays the routes KINDS need (run.py --kinds), added to the player's run; ["fresh"] makes it all anew. */
  async function makeRun(kinds: string[]) {
    if (!runner || kinds.length === 0) return;
    const value =
      kinds[0] === "fresh"
        ? "fresh"
        : CATEGORIES.filter((c) => kinds.includes(c.id))
            .map((c) => c.id)
            .join(",");
    if (!(await ask(INTERFACE.runAsk, INTERFACE.runYes))) return;
    const key = v.key;
    busy = { kind: "run", key };
    said = null;
    const ok = await runChain(scope, [runner], { option: value });
    busy = null;
    if (key !== v.key) return;
    await refresh();
    if (!ok) {
      // Stopped by the player, or ended by an error run.py named.
      const error = events("interface-run", key).findLast((e) => e.event === "error");
      const message = typeof error?.message === "string" && error.message !== "stopped" ? error.message : null;
      said = { text: message ?? INTERFACE.runStopped, failed: true };
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
</script>

{#if exporter}
  <section class="interface" aria-labelledby="interface-title">
    <h3 id="interface-title">{INTERFACE.title}</h3>
    <p class="muted lead">{INTERFACE.lead}</p>
    {#if failed}
      <p class="failed">{failed}</p>
    {/if}
    {#each view?.unreadable ?? [] as why (why)}
      <!-- A run file that cannot be read (cut short, another format): a new run writes it anew. -->
      <p class="failed small">{why}</p>
    {/each}
    <ul class="kinds">
      {#each CATEGORIES as c, at (c.id)}
        {@const s = status(at)}
        <li>
          <input
            type="checkbox"
            role="switch"
            class="switch"
            aria-label={INTERFACE.exportKind(c.label)}
            checked={chosen[c.id] && available(at)}
            onchange={(e) => (chosen[c.id] = e.currentTarget.checked)}
            disabled={!available(at) || working !== null}
          />
          <span class="name" class:off={!available(at)}>{c.label}</span>
          <span class="state {s.tone}">
            {#if s.tone === "ok"}<Icon name="check" size={14} />{:else}<span class="dot"></span>{/if}{s.text}
          </span>
          {#if makeable(at) && working !== "run"}
            <button class="small" disabled={!runner?.runnable || working !== null} onclick={() => void makeRun([c.id])}
              >{view?.local?.counts[at] ? INTERFACE.finish : INTERFACE.make}</button
            >
          {/if}
        </li>
      {/each}
    </ul>
    <div class="controls">
      <button disabled={!exporter.runnable || !option || working !== null} onclick={() => void exportTextures()}>
        {#if working === "export"}<span class="spinner"></span>{:else}<Icon
            name="external"
            size={16}
          />{/if}{INTERFACE.export}
      </button>
      {#if working === "run"}
        <button class="ghost" onclick={() => void stopRun()}><Icon name="x" size={16} />{INTERFACE.stop}</button>
      {:else if missing.length > 1}
        <button class="ghost" disabled={!runner?.runnable || working !== null} onclick={() => void makeRun(missing)}
          ><Icon name="build" size={16} />{INTERFACE.makeMissing(missing.length)}</button
        >
      {/if}
      {#if canRun && view?.local && working === null && runner?.runnable}
        <Menu items={[{ label: INTERFACE.makeAgain, icon: "build", run: () => void makeRun(["fresh"]) }]} />
      {/if}
    </div>
    <p class="muted line" class:failed={said?.failed} class:done={said && !said.failed}>
      {#if working === "export"}
        {INTERFACE.exporting}
      {:else if working === "run"}
        {progress} · {INTERFACE.runLong}
      {:else if said}
        {said.text}
      {:else if !exporter.runnable}
        {INTERFACE.notReady}
      {:else if !option}
        {canRun ? INTERFACE.nothingYetRun : INTERFACE.nothingYet}
      {:else}
        {INTERFACE.exportLine}
      {/if}
    </p>
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
  .line,
  .small {
    font-size: 0.875rem;
  }

  /* Rows with a rule between them, as every list in a panel (STYLE.md). */
  .kinds {
    list-style: none;
    margin: 0;
    padding: 0;
  }

  .kinds li {
    display: grid;
    grid-template-columns: auto minmax(0, 1fr) auto auto;
    align-items: center;
    gap: 12px;
    min-height: 40px;
    padding: 4px 0;
  }

  .kinds li + li {
    border-top: 1px solid var(--line);
  }

  .kinds li:not(:has(button)) {
    grid-template-columns: auto minmax(0, 1fr) auto;
  }

  .name.off {
    color: var(--dim);
  }

  /* A state is a dot (or a tick) and a few words. */
  .state {
    display: inline-flex;
    align-items: center;
    gap: 6px;
    font-size: 0.875rem;
    white-space: nowrap;
  }

  .dot {
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background: currentColor;
  }

  .state.ok {
    color: var(--ok);
  }

  .state.own {
    color: var(--amber);
  }

  .state.none {
    color: var(--dim);
  }

  .controls {
    display: flex;
    flex-wrap: wrap;
    gap: 6px;
  }

  .failed {
    color: var(--err);
  }

  .done {
    color: var(--ok);
  }
</style>
