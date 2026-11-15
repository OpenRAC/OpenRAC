<script lang="ts">
  // The Going Commando save editor: one slot of the memory card, opened from the Save files list.
  // It shows only what core/src/save_edit_rac2.rs proved by a save the game wrote (format:
  // docs/RAC2_SAVE_FORMAT.md), and saves through write_save_rac2, which refuses while the game runs
  // with the card and backs the card up first. Looks and behaves as rac1's SaveEditor.svelte:
  // changes are marked and listed above Save, Reset puts the save back as read, Undo after a save
  // restores the backup, and leaving with changes asks first.
  import Icon from "$components/Icon.svelte";
  import ModSaveData from "$components/ModSaveData.svelte";
  import SaveEditorRac2Advanced2 from "$components/SaveEditorRac2Advanced2.svelte";
  import { api, type Rac2SaveFields, type Rac2SaveView, type SaveBackupInfo } from "$lib/api";
  import { toast } from "$lib/app.svelte";
  import { inertPage, portal } from "$lib/portal";
  import { fold } from "$lib/motion";
  import {
    ARMOR_LEVELS,
    armorName,
    changed,
    editList,
    fieldErrors,
    ownedCount,
    platinumCount,
    setOwned,
    setVersion,
    SHIP_PAINTS,
    flagGroups,
    selectedVersion,
    versionHow,
    versionLabel,
    versionName,
    type Rac2Section,
  } from "$lib/saveEditRac2";
  import {
    activeTiers,
    attachments,
    choiceName,
    pickVersion,
    pickedValue,
    tierOptions,
    valueOf,
    type Attached,
    type ItemAttachments,
  } from "$lib/modSaveAttach";
  import { ModSaveStore } from "$lib/modSaveStore.svelte";
  import { onMount } from "svelte";
  import { slide } from "svelte/transition";

  let {
    serial,
    slot,
    playing = false,
    onclose,
  }: {
    serial: string;
    slot: number;
    /** The launcher is running the game, which writes the card too: saving waits until it is closed. */
    playing?: boolean;
    /** `true` when the card changed (saved or undone), so the list reads it again. */
    onclose: (changedCard: boolean) => void;
  } = $props();

  let view = $state<Rac2SaveView | null>(null);
  let fields = $state<Rac2SaveFields | null>(null);
  let loadError = $state<string | null>(null);
  let saveError = $state<string | null>(null);
  let busy = $state(false);
  let saved = $state<SaveBackupInfo | null>(null);
  let touchedCard = false;

  const errors = $derived(view && fields ? fieldErrors(view, fields) : {});
  /** The mods' records of this save; their attached fields show in the weapon rows. */
  // svelte-ignore state_referenced_locally
  const mods = new ModSaveStore(serial, slot);
  const itemIds = $derived(new Set(view?.items.map((i) => i.id) ?? []));
  const attached = $derived(mods.mods ? attachments(mods.mods, itemIds) : new Map<number, ItemAttachments>());
  const modEdits = $derived(
    (mods.mods ?? []).flatMap((m) =>
      m.fields
        .filter((f) => mods.get(m.id, f.key) !== f.value)
        .map((f): { section: Rac2Section; text: string } => ({
          section: "weapons",
          text: `${m.name}: ${f.name} ${choiceName(f, f.value)} → ${choiceName(f, mods.get(m.id, f.key))}`,
        })),
    ),
  );
  const cardDirty = $derived(!!view && !!fields && changed(view, fields));
  const dirty = $derived(cardDirty || modEdits.length > 0);
  const canSave = $derived(dirty && !busy && !playing && Object.keys(errors).length === 0);
  const edits = $derived([...(view && fields ? editList(view, fields) : []), ...modEdits]);
  const editedSections = $derived(new Set(edits.map((e) => e.section)));
  const changeCount = $derived(Math.max(edits.length, dirty ? 1 : 0));
  let asking = $state(false);
  let reviewing = $state(false);
  let modalEl = $state<HTMLElement | null>(null);
  let bodyEl = $state<HTMLElement | null>(null);
  const weapons = $derived(view?.items.filter((i) => i.kind === "weapon") ?? []);
  const gadgets = $derived(view?.items.filter((i) => i.kind === "gadget") ?? []);

  async function load() {
    loadError = null;
    try {
      const read = await api.readSaveRac2(serial, slot);
      view = read;
      fields = structuredClone(read.fields);
    } catch (e) {
      loadError = String(e);
    }
  }

  async function save() {
    if (!view || !fields || !canSave) return;
    busy = true;
    saveError = null;
    try {
      if (cardDirty) {
        const result = await api.writeSaveRac2(serial, slot, view.fingerprint, $state.snapshot(fields));
        view = result.view;
        fields = structuredClone(result.view.fields);
        saved = result.backup;
        touchedCard = true;
      }
      // Each mod's record by its own path, after the card.
      await mods.saveAll();
    } catch (e) {
      saveError = String(e);
    } finally {
      busy = false;
    }
  }

  async function undo() {
    if (!saved) return;
    busy = true;
    saveError = null;
    try {
      await api.restoreBackup(serial, saved.name);
      toast(`Slot ${String(slot + 1)} is back as it was before the edit.`, "info");
      onclose(true);
    } catch (e) {
      saveError = String(e);
    } finally {
      busy = false;
    }
  }

  function close() {
    if (busy) return;
    if (dirty) asking = true;
    else onclose(touchedCard);
  }

  async function saveAndClose() {
    await save();
    if (!saveError) onclose(true);
  }

  function reset() {
    if (!view) return;
    fields = $state.snapshot(view.fields);
    mods.resetAll();
    reviewing = false;
  }

  function escape() {
    if (asking) asking = false;
    else if (reviewing) reviewing = false;
    else close();
  }

  function item(id: number) {
    return fields?.items.find((i) => i.id === id);
  }

  function itemEdited(id: number): boolean {
    const a = view?.fields.items.find((i) => i.id === id);
    const b = item(id);
    const modChanged = [...(attached.get(id)?.version ?? []), ...(attached.get(id)?.row ?? [])].some(
      (x) => valueOf(mods.values, x) !== x.field.value,
    );
    return (
      modChanged ||
      (!!a && !!b && (a.owned !== b.owned || a.ammo !== b.ammo || a.xp !== b.xp || a.version !== b.version))
    );
  }

  /** The mods' names for a stacked list of attached fields, for the mod tag. */
  function modNames(list: Attached[]): string {
    return [...new Set(list.map((x) => x.mod.name))].join(", ");
  }

  function planetEdited(id: number): boolean {
    const a = view?.fields.planets.find((p) => p.id === id);
    const b = fields?.planets.find((p) => p.id === id);
    return !!a && !!b && JSON.stringify(a) !== JSON.stringify(b);
  }

  function focusOnShow(node: HTMLElement) {
    node.focus();
  }

  /** Advanced: progress flags and the like, behind a switch that starts off for each slot. */
  let advanced = $state(false);
  const sections = $derived<[Rac2Section, string][]>([
    ["ratchet", "Ratchet"],
    ["weapons", "Weapons"],
    ["gadgets", "Gadgets and items"],
    ["planets", "Planets"],
    ["skills", "Skill points"],
    ...(advanced ? ([["advanced", "Advanced"]] as [Rac2Section, string][]) : []),
  ]);
  const sectionLabel: Record<Rac2Section, string> = {
    ratchet: "Ratchet",
    weapons: "Weapons",
    gadgets: "Gadgets and items",
    planets: "Planets",
    skills: "Skill points",
    advanced: "Advanced",
  };
  let current = $state<Rac2Section>("ratchet");
  let target: Rac2Section | null = null;

  function jump(key: Rac2Section) {
    target = key;
    current = key;
    document.getElementById(`se2-${key}`)?.scrollIntoView({ block: "start", behavior: "smooth" });
  }

  function spy() {
    if (!bodyEl) return;
    const line = bodyEl.getBoundingClientRect().top + 60;
    let at: Rac2Section = "ratchet";
    for (const [key] of sections) {
      const el = document.getElementById(`se2-${key}`);
      if (el && el.getBoundingClientRect().top <= line) at = key;
    }
    const end = bodyEl.scrollTop + bodyEl.clientHeight >= bodyEl.scrollHeight - 2;
    current = end ? (target ?? (advanced ? "advanced" : "skills")) : at;
  }

  $effect(() => {
    const el = bodyEl;
    if (!el) return;
    const free = () => (target = null);
    el.addEventListener("wheel", free, { passive: true });
    el.addEventListener("pointerdown", free);
    return () => {
      el.removeEventListener("wheel", free);
      el.removeEventListener("pointerdown", free);
    };
  });

  const storyWarning =
    "Taking away something the story needs, like the Swingshot or the Heli-Pack, can leave Ratchet stuck. What he has in hand or on stays.";

  const changedMark = (a: unknown, b: unknown) => JSON.stringify(a) !== JSON.stringify(b);

  onMount(() => {
    const back = document.activeElement instanceof HTMLElement ? document.activeElement : null;
    const release = inertPage();
    modalEl?.focus();
    void load();
    void mods.load();
    return () => {
      release();
      back?.focus();
    };
  });
</script>

<svelte:window
  onkeydown={(e) => {
    if (e.key === "Escape") escape();
    else if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === "s") {
      e.preventDefault();
      if (canSave) void save();
    }
  }}
/>

<div class="overlay" role="presentation" use:portal>
  <button class="backdrop" onclick={close} tabindex="-1" aria-label="Close the save editor"></button>
  <div
    class="modal rise"
    role="dialog"
    aria-modal="true"
    aria-labelledby="save-editor-title"
    tabindex="-1"
    bind:this={modalEl}
  >
    <div class="head">
      <div class="grow">
        <h2 id="save-editor-title">
          {view
            ? `Slot ${String(slot + 1)} · ${view.planetName ?? "Unknown planet"} · ${view.fields.bolts.toLocaleString("en-US")} bolts`
            : `Slot ${String(slot + 1)}`}
        </h2>
        {#if view?.savedAt}
          <p class="sub">Saved {view.savedAt}</p>
        {/if}
      </div>
      <button class="ghost small" onclick={close} disabled={busy} aria-label="Close">
        <Icon name="x" size={16} />
      </button>
    </div>

    <div class="body" id="save-editor-body" bind:this={bodyEl} onscroll={spy}>
      {#if loadError}
        <p class="problem"><Icon name="warn" size={16} />{loadError}</p>
      {:else if !view || !fields}
        <p class="muted row"><span class="spinner"></span>Reading the save…</p>
      {:else}
        <nav class="jump" aria-label="Sections">
          {#each sections as [key, label] (key)}
            <button
              class="ghost small"
              class:on={current === key}
              aria-current={current === key ? "location" : undefined}
              onclick={() => {
                jump(key);
              }}
              >{label}{#if editedSections.has(key)}<span class="mark" role="img" aria-label="changed"
                ></span>{/if}</button
            >
          {/each}
        </nav>

        <section id="se2-ratchet" aria-labelledby="se2-ratchet-h">
          <h3 id="se2-ratchet-h">Ratchet</h3>
          {#each [["bolts", "Bolts"], ["raritanium", "Raritanium"]] as const as [key, label] (key)}
            <div class="line">
              <label for={`se2-${key}`}
                >{label}{#if fields[key] !== view.fields[key]}<span class="mark" role="img" aria-label="changed"
                  ></span>{/if}</label
              >
              <div class="field">
                {#if key === "bolts"}<span class="bolt"><Icon name="bolt" size={16} /></span>{/if}
                <input
                  id={`se2-${key}`}
                  type="number"
                  min="0"
                  max={view.maxBolts}
                  step="1"
                  bind:value={fields[key]}
                  aria-invalid={!!errors[key]}
                />
                <button
                  class="ghost small"
                  onclick={() => {
                    if (view && fields) fields[key] = view.maxBolts;
                  }}
                  disabled={fields[key] === view.maxBolts}>Max</button
                >
                {#if errors[key]}<span class="err-text">{errors[key]}</span>{/if}
              </div>
            </div>
          {/each}
          <div class="line">
            <label for="se2-nanotech"
              >Nanotech experience{#if fields.nanotechXp !== view.fields.nanotechXp}<span
                  class="mark"
                  role="img"
                  aria-label="changed"
                ></span>{/if}</label
            >
            <div class="field">
              <input
                id="se2-nanotech"
                type="number"
                min="0"
                max={view.maxXp}
                step="1"
                bind:value={fields.nanotechXp}
                aria-invalid={!!errors.nanotechXp}
              />
              <span class="hint"
                >Health in the save: {view.maxHealth}. The game works it out again from the experience and the boosts.</span
              >
              {#if errors.nanotechXp}<span class="err-text">{errors.nanotechXp}</span>{/if}
            </div>
          </div>
          <div class="line">
            <span id="se2-boosts"
              >Nanotech Boosts{#if changedMark(fields.boosts, view.fields.boosts)}<span
                  class="mark"
                  role="img"
                  aria-label="changed"
                ></span>{/if}</span
            >
            <div class="skills" role="group" aria-labelledby="se2-boosts">
              {#each fields.boosts as on, k (k)}
                <button
                  class="skill"
                  class:on
                  class:edited={on !== view.fields.boosts[k]}
                  aria-pressed={on}
                  aria-label={`Nanotech Boost ${String(k + 1)}`}
                  onclick={() => {
                    if (fields) fields.boosts[k] = !on;
                  }}>{k + 1}</button
                >
              {/each}
            </div>
          </div>
          <div class="line">
            <label for="se2-armor"
              >Armour{#if fields.armor !== view.fields.armor}<span class="mark" role="img" aria-label="changed"
                ></span>{/if}</label
            >
            <div class="field">
              <select id="se2-armor" bind:value={fields.armor}>
                {#each Array.from({ length: ARMOR_LEVELS }, (_, k) => k) as k (k)}
                  <option value={k}>{k === 0 ? "None" : `Level ${String(k)}`}</option>
                {/each}
              </select>
              <span class="hint">The armour worn, {armorName(view.fields.armor)} in the save.</span>
            </div>
          </div>
          <div class="line">
            <label for="se2-paint"
              >Ship paint{#if fields.shipPaint !== view.fields.shipPaint}<span
                  class="mark"
                  role="img"
                  aria-label="changed"
                ></span>{/if}</label
            >
            <div class="field">
              <select id="se2-paint" bind:value={fields.shipPaint}>
                {#each Array.from({ length: SHIP_PAINTS }, (_, k) => k) as k (k)}
                  <option value={k}>Paint job {k + 1}{k === 0 ? " (the ship's own)" : ""}</option>
                {/each}
              </select>
            </div>
          </div>
        </section>

        <section id="se2-weapons" aria-labelledby="se2-weapons-h">
          <h3 id="se2-weapons-h">
            Weapons <span class="count"
              >{ownedCount(view, fields, "weapon")[0]} of {ownedCount(view, fields, "weapon")[1]}</span
            >
          </h3>
          <p class="note">
            Outside challenge mode a V3 still earns experience but never upgrades to V4: pick V4 here instead. A new
            version comes with full ammo, and with the experience the game's own upgrade leaves.
          </p>
          <ul class="rows weapons">
            {#each weapons as w (w.id)}
              {@const s = item(w.id)}
              {#if s}
                {@const at = attached.get(w.id)}
                <li>
                  <label class="own" title={w.worn ? "In Ratchet's hand: it stays" : undefined}>
                    <input
                      type="checkbox"
                      checked={s.owned}
                      disabled={w.worn && s.owned}
                      onchange={(e) => {
                        if (view && fields) setOwned(view, fields, w.id, e.currentTarget.checked);
                      }}
                    />
                    {versionName(view, fields, w.id) ?? w.name}{#if itemEdited(w.id)}<span
                        class="mark"
                        role="img"
                        aria-label="changed"
                      ></span>{/if}
                  </label>
                  {#if (at?.version.length ?? 0) > 0 && at}
                    {@const v = selectedVersion(view, fields, w.id)}
                    {@const tiers = activeTiers(mods.values, at.version)}
                    <span class="version-stack">
                      <select
                        class="version"
                        class:mega={v?.challengeOnly}
                        class:modded={tiers.length > 0}
                        class:off={!s.owned}
                        aria-label={`${w.name} version`}
                        title={w.worn
                          ? "In Ratchet's hand: switch weapons in the game and save to change its version"
                          : tiers.length > 0
                            ? `From ${tiers[0]?.attached.mod.name ?? "a mod"}: saved in the mod's own file`
                            : v
                              ? versionHow(v)
                              : undefined}
                        value={pickedValue(mods.values, at.version, s.version)}
                        disabled={!s.owned || w.worn}
                        aria-invalid={!!errors[`version-${String(w.id)}`]}
                        onchange={(e) => {
                          if (view && fields)
                            pickVersion(view, fields, mods.values, w.id, at.version, e.currentTarget.value);
                        }}
                      >
                        {#if w.versions.length === 0}<option value="g:0">{w.name}</option>{/if}
                        {#each w.versions as option, k (k)}
                          <option value={`g:${String(k)}`}
                            >{versionLabel(option)}{option.name !== w.name ? ` ${option.name}` : ""}</option
                          >
                        {/each}
                        {#each at.version as a (`${a.mod.id}:${a.field.key}`)}
                          <optgroup label={a.mod.name}>
                            {#each tierOptions([a]) as o (o.value)}
                              {#if o.kind === "mod"}<option value={o.value}>{o.name}</option>{/if}
                            {/each}
                          </optgroup>
                        {/each}
                      </select>
                      <span class="mod-tag" title={`Tiers from ${modNames(at.version)}`}>{modNames(at.version)}</span>
                      {#if tiers.length > 1}
                        <span class="err-text small"
                          >Mods disagree: {tiers
                            .map((t) => `${t.attached.mod.name} ${choiceName(t.attached.field, t.tier)}`)
                            .join(", ")}</span
                        >
                      {/if}
                    </span>
                  {:else if w.versions.length > 1 && s.version !== null}
                    {@const v = selectedVersion(view, fields, w.id)}
                    <select
                      class="version"
                      class:mega={v?.challengeOnly}
                      class:off={!s.owned}
                      aria-label={`${w.name} version`}
                      title={w.worn
                        ? "In Ratchet's hand: switch weapons in the game and save to change its version"
                        : v
                          ? versionHow(v)
                          : undefined}
                      value={s.version}
                      disabled={!s.owned || w.worn}
                      aria-invalid={!!errors[`version-${String(w.id)}`]}
                      onchange={(e) => {
                        if (view && fields) setVersion(view, fields, w.id, Number(e.currentTarget.value));
                      }}
                    >
                      {#each w.versions as option, k (k)}
                        <option value={k}
                          >{versionLabel(option)}{option.name !== w.name ? ` ${option.name}` : ""}</option
                        >
                      {/each}
                    </select>
                  {:else}
                    <span></span>
                  {/if}
                  {#if (selectedVersion(view, fields, w.id)?.maxAmmo ?? w.maxAmmo) !== null && s.ammo !== null}
                    {@const maxAmmo = selectedVersion(view, fields, w.id)?.maxAmmo ?? w.maxAmmo ?? 0}
                    <span class="ammo" class:off={!s.owned}>
                      <input
                        type="number"
                        min="0"
                        max={maxAmmo}
                        step="1"
                        aria-label={`${w.name} ammo`}
                        bind:value={s.ammo}
                        aria-invalid={!!errors[`ammo-${String(w.id)}`]}
                      />
                      <span class="dim">/ {maxAmmo}</span>
                      <button class="ghost small" onclick={() => (s.ammo = maxAmmo)} disabled={s.ammo === maxAmmo}
                        >Fill</button
                      >
                    </span>
                  {:else}
                    <span class="dim no-ammo">No ammo</span>
                  {/if}
                  {#if w.maxXp !== null && s.xp !== null}
                    {@const v = selectedVersion(view, fields, w.id)}
                    {@const next = v ? w.versions[(s.version ?? 0) + 1] : undefined}
                    <span
                      class="ammo xp"
                      class:off={!s.owned}
                      title={next?.how === "experience"
                        ? `Becomes ${next.name} ${versionLabel(next)} at ${(next.stepXp ?? 0).toLocaleString("en-US")}`
                        : undefined}
                    >
                      <input
                        type="number"
                        min={v?.minXp ?? 0}
                        max={v?.maxXp ?? w.maxXp}
                        step="1"
                        aria-label={`${w.name} experience`}
                        bind:value={s.xp}
                        aria-invalid={!!errors[`xp-${String(w.id)}`]}
                      />
                      <span class="dim">XP</span>
                    </span>
                  {:else}
                    <span></span>
                  {/if}
                  {#each attached.get(w.id)?.row ?? [] as a (`${a.mod.id}:${a.field.key}`)}
                    <span class="mod-row">
                      <span class="mod-tag">{a.mod.name}</span>
                      {#if a.field.choices && a.field.choices.length > a.field.max - a.field.min}
                        <select
                          aria-label={`${w.name}: ${a.field.name}`}
                          value={mods.get(a.mod.id, a.field.key)}
                          onchange={(e) => {
                            mods.set(a.mod.id, a.field.key, Number(e.currentTarget.value));
                          }}
                        >
                          {#each a.field.choices as c, i (i)}
                            {#if i >= a.field.min && i <= a.field.max}<option value={i}>{a.field.name}: {c}</option
                              >{/if}
                          {/each}
                        </select>
                      {:else}
                        <input
                          type="number"
                          min={a.field.min}
                          max={a.field.max}
                          step="1"
                          aria-label={`${w.name}: ${a.field.name}`}
                          value={mods.get(a.mod.id, a.field.key)}
                          onchange={(e) => {
                            mods.set(a.mod.id, a.field.key, Number(e.currentTarget.value));
                          }}
                        />
                      {/if}
                    </span>
                  {/each}
                </li>
              {/if}
            {/each}
          </ul>
          {#each Object.entries(errors).filter(([k]) => k.startsWith("version-")) as [k, message] (k)}
            <p class="err-text">
              {view.items.find((i) => `version-${String(i.id)}` === k)?.name}: {message}.
            </p>
          {/each}
          {#if Object.keys(errors).some((k) => k.startsWith("ammo-") || k.startsWith("xp-"))}
            <p class="err-text">
              Ammo goes up to what the version holds; experience stays under the next upgrade, which the game makes
              itself, and at least at the step a version upgraded at.
            </p>
          {/if}
        </section>

        <section id="se2-gadgets" aria-labelledby="se2-gadgets-h">
          <h3 id="se2-gadgets-h">
            Gadgets and items <span class="count"
              >{ownedCount(view, fields, "gadget")[0]} of {ownedCount(view, fields, "gadget")[1]}</span
            >
          </h3>
          <div class="chips">
            {#each gadgets as g (g.id)}
              {@const s = item(g.id)}
              {#if s}
                <label class="chip" class:on={s.owned}>
                  <input
                    type="checkbox"
                    checked={s.owned}
                    disabled={g.worn && s.owned}
                    onchange={(e) => {
                      if (view && fields) setOwned(view, fields, g.id, e.currentTarget.checked);
                    }}
                  />
                  {g.name}{#if itemEdited(g.id)}<span class="mark" role="img" aria-label="changed"></span>{/if}
                </label>
              {/if}
            {/each}
          </div>
          <p class="note">{storyWarning}</p>
        </section>

        <section id="se2-planets" aria-labelledby="se2-planets-h">
          <h3 id="se2-planets-h">
            Planets <span class="count">{platinumCount(fields)} platinum bolts</span>
          </h3>
          <ul class="rows planets">
            {#each view.planets as p (p.id)}
              {@const s = fields.planets.find((x) => x.id === p.id)}
              {#if s}
                <li>
                  <span class="planet">
                    <b
                      >{p.name}{#if planetEdited(p.id)}<span class="mark" role="img" aria-label="changed"
                        ></span>{/if}</b
                    >
                  </span>
                  <label class="own">
                    <input type="checkbox" bind:checked={s.available} />
                    On the ship's map
                  </label>
                  <label class="own">
                    <input type="checkbox" bind:checked={s.visited} />
                    Visited
                  </label>
                  <span class="bolts" role="group" aria-label={`Platinum bolts on ${p.name}`}>
                    {#each s.platinum as found, k (k)}
                      <button
                        class="goldbolt"
                        class:on={found}
                        aria-pressed={found}
                        aria-label={`Platinum bolt ${String(k + 1)} on ${p.name}`}
                        onclick={() => (s.platinum[k] = !found)}
                      >
                        <Icon name="bolt" size={14} />
                      </button>
                    {/each}
                  </span>
                </li>
              {/if}
            {/each}
          </ul>
          <p class="note">
            Each level keeps four places for platinum bolts; the game counts every one found (40 at most), less those
            spent on weapon mods.
          </p>
        </section>

        <section id="se2-skills" aria-labelledby="se2-skills-h">
          <h3 id="se2-skills-h">
            Skill points <span class="count"
              >{fields.skillPoints.filter(Boolean).length} of {fields.skillPoints.length}</span
            >
            <span class="grow"></span>
            <button
              class="ghost small"
              onclick={() => {
                if (fields) fields.skillPoints = fields.skillPoints.map(() => true);
              }}>All</button
            >
            <button
              class="ghost small"
              onclick={() => {
                if (fields) fields.skillPoints = fields.skillPoints.map(() => false);
              }}>None</button
            >
          </h3>
          <div class="skills" role="group" aria-label="Skill points">
            {#each fields.skillPoints as on, k (k)}
              <button
                class="skill"
                class:on
                class:edited={on !== view.fields.skillPoints[k]}
                aria-pressed={on}
                aria-label={`Skill point ${String(k + 1)}`}
                onclick={() => {
                  if (fields) fields.skillPoints[k] = !on;
                }}>{k + 1}</button
              >
            {/each}
          </div>
          <p class="note">Numbered in the order the game keeps them.</p>
        </section>

        <ModSaveData store={mods} {itemIds} {playing} />

        <section id="se2-advanced-switch" aria-label="Advanced editing">
          <label class="own switch">
            <input type="checkbox" bind:checked={advanced} />
            Advanced: story and progress flags
          </label>
          {#if !advanced && editedSections.has("advanced")}
            <p class="note">Advanced changes are still in the list of changes.</p>
          {/if}
        </section>

        {#if advanced}
          <section id="se2-advanced" aria-labelledby="se2-advanced-h" transition:slide={fold}>
            <h3 id="se2-advanced-h">Advanced</h3>
            <p class="running">
              <Icon name="warn" size={16} />These change where the story stands. A flag that does not match the rest of
              the save can replay a scene, skip one, or leave a door shut; your card is backed up first.
            </p>
            {#each flagGroups(view) as group (group.planet)}
              <h4 class="group">{group.planet}</h4>
              <ul class="rows flags">
                {#each group.flags as info (info.id)}
                  {@const s = fields.flags.find((x) => x.id === info.id)}
                  {#if s}
                    <li>
                      <label class="own">
                        <input type="checkbox" bind:checked={s.on} />
                        {info.name}{#if s.on !== view.fields.flags.find((x) => x.id === info.id)?.on}<span
                            class="mark"
                            role="img"
                            aria-label="changed"
                          ></span>{/if}
                      </label>
                      <span class="hint"
                        >{#if info.confidence === "fromCode"}<span
                            class="unverified"
                            title="Read from the game's code; this one flag was not tried in the game">unverified</span
                          >{/if}{info.description}</span
                      >
                    </li>
                  {/if}
                {/each}
              </ul>
            {/each}
            <h4 class="group">More</h4>
            {#key view.fingerprint}
              <SaveEditorRac2Advanced2 {serial} {slot} {playing} onsaved={() => void load()} />
            {/key}
          </section>
        {/if}
      {/if}
    </div>

    <div class="foot">
      {#if saveError}
        <p class="problem"><Icon name="warn" size={16} />{saveError}</p>
      {/if}
      {#if playing && dirty}
        <p class="running">
          <Icon name="info" size={16} />The game is running and writes this memory card too: close it, then save.
        </p>
      {/if}
      {#if reviewing && edits.length && !asking}
        <ul class="review" aria-label="Your changes" transition:slide={fold}>
          {#each edits as e, i (i)}
            <li>
              <button
                class="ghost small"
                onclick={() => {
                  jump(e.section);
                }}><span class="where">{sectionLabel[e.section]}</span>{e.text}</button
              >
            </li>
          {/each}
        </ul>
      {/if}
      {#if asking}
        <div class="row" role="alertdialog" aria-label="Close without saving?">
          <span class="grow"
            >Close without saving your {changeCount === 1 ? "change" : `${String(changeCount)} changes`}?</span
          >
          <button class="ghost" use:focusOnShow onclick={() => (asking = false)} disabled={busy}>Keep editing</button>
          <button
            class="discard"
            onclick={() => {
              onclose(touchedCard);
            }}
            disabled={busy}>Discard</button
          >
          {#if canSave}
            <button class="primary" onclick={saveAndClose} disabled={busy}>
              {#if busy}<span class="spinner"></span>{:else}<Icon name="save" size={16} />{/if}Save and close
            </button>
          {/if}
        </div>
      {:else if saved && !dirty}
        <div class="row">
          <span class="grow ok"
            ><Icon name="save" size={16} />Saved. Your card was backed up first: Undo puts it back.</span
          >
          <button class="ghost" onclick={undo} disabled={busy}>Undo</button>
          <button
            onclick={() => {
              onclose(true);
            }}
            disabled={busy}>Close</button
          >
        </div>
      {:else}
        <div class="row">
          {#if dirty}
            <button
              class="ghost small changes"
              aria-expanded={reviewing}
              onclick={() => (reviewing = !reviewing)}
              title={reviewing ? "Hide the list" : "See what you changed"}
              ><span class="mark" aria-hidden="true"></span>{changeCount === 1
                ? "1 change"
                : `${String(changeCount)} changes`}<span class="caret" class:open={reviewing} aria-hidden="true"
              ></span></button
            >
            <span class="grow dim backup">Backed up before writing</span>
            <button class="ghost" onclick={reset} disabled={busy} title="Undo every change you made here">Reset</button>
          {:else}
            <span class="grow dim">Your memory card is backed up before anything is written.</span>
          {/if}
          <button class="ghost" onclick={close} disabled={busy}>{dirty ? "Cancel" : "Close"}</button>
          <button
            class="primary"
            onclick={save}
            disabled={!canSave}
            title={playing ? "Close the game first" : "Save (Ctrl+S)"}
          >
            {#if busy}<span class="spinner"></span>{:else}<Icon name="save" size={16} />{/if}Save
          </button>
        </div>
      {/if}
    </div>
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
    max-width: 980px;
    height: calc(100vh - 48px);
    background: var(--card);
    --control: var(--control-card);
    --control-hi: var(--control-card-hi);
    border-radius: var(--radius);
    box-shadow: var(--shadow);
    display: flex;
    flex-direction: column;
    overflow: hidden;
  }

  /* Focus lands on the dialog itself when it opens; its own controls show focus as usual. */
  .modal:focus {
    outline: none;
  }

  .head,
  .foot {
    padding: 16px 22px;
    background: var(--card-hi);
    --control: var(--control-card);
    --control-hi: var(--control-card-hi);
  }

  .head {
    display: flex;
    align-items: flex-start;
    gap: 12px;
    border-bottom: 1px solid var(--line);
  }

  .head h2 {
    margin: 0;
    overflow-wrap: anywhere;
  }

  .sub {
    margin: 4px 0 0;
    font-size: 0.8125rem;
    color: var(--soft);
  }

  .foot {
    border-top: 1px solid var(--line);
    display: flex;
    flex-direction: column;
    gap: 10px;
  }

  .foot .row {
    flex-wrap: wrap;
    gap: 8px;
    font-size: 0.8125rem;
  }

  /* Changed: an amber dot where the change is, on its section's link and before the count. */
  .mark {
    display: inline-block;
    flex: none;
    width: 7px;
    height: 7px;
    margin-left: 7px;
    border-radius: 50%;
    background: var(--amber);
    box-shadow: none;
    vertical-align: middle;
  }

  .own .mark,
  .chip .mark,
  .changes .mark {
    margin-left: 0;
  }

  .changes {
    color: var(--amber);
    font-weight: 600;
  }

  .caret {
    width: 0;
    height: 0;
    border-left: 4px solid transparent;
    border-right: 4px solid transparent;
    border-bottom: 5px solid currentcolor;
    transition: transform 0.15s;
  }

  .caret.open {
    transform: rotate(180deg);
  }

  .review {
    max-height: 168px;
    overflow-y: auto;
    padding: 4px;
    border-radius: var(--radius-sm);
    background: var(--card);
    --control: var(--control-card);
    --control-hi: var(--control-card-hi);
  }

  .review button {
    width: 100%;
    white-space: normal;
    justify-content: flex-start;
    gap: 12px;
    font-weight: 400;
    text-align: left;
    color: var(--text);
  }

  .where {
    flex: 0 1 8rem;
    min-width: 0;
    color: var(--dim);
  }

  .discard {
    color: var(--err);
  }

  .running {
    display: flex;
    align-items: flex-start;
    gap: 8px;
    margin: 0;
    font-size: 0.8125rem;
    color: var(--amber);
  }

  @media (max-width: 900px) {
    .backup {
      visibility: hidden;
    }
  }

  /* A short window keeps its height for the save itself. */
  @media (max-height: 640px) {
    .overlay {
      padding: 12px;
    }

    .modal {
      height: calc(100vh - 24px);
    }

    .head,
    .foot {
      padding: 10px 22px;
    }
  }

  .ok {
    display: inline-flex;
    align-items: center;
    gap: 8px;
    color: var(--ok);
  }

  /* The sections reflow by the body's own width (the window less the dialog's margins). */
  .body {
    container-type: inline-size;
    flex: 1;
    min-height: 0;
    overflow-y: auto;
    padding: 0 22px 18px;
    display: flex;
    flex-direction: column;
    gap: 18px;
  }

  /* The section links stay at the top of the scrolling body. */
  .jump {
    position: sticky;
    top: 0;
    z-index: 2;
    display: flex;
    flex-wrap: wrap;
    gap: 4px;
    margin: 0 -22px;
    padding: 8px 18px;
    background: var(--control);
    border-bottom: 1px solid var(--line);
  }

  section {
    scroll-margin-top: 52px;
  }

  .jump button.on {
    color: var(--text);
    background: var(--on-fill);
  }

  section h3 {
    display: flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 10px;
    margin: 14px 0 8px;
  }

  .count {
    font-family: var(--font-sans);
    font-size: 0.8125rem;
    font-weight: 500;
    color: var(--soft);
  }

  ul {
    margin: 0;
    padding: 0;
    list-style: none;
  }

  .line {
    display: grid;
    grid-template-columns: minmax(0, 120px) minmax(0, 1fr);
    align-items: center;
    gap: 12px;
    padding: 10px 2px;
    border-top: 1px solid var(--line);
  }

  .line:last-child {
    border-bottom: 1px solid var(--line);
  }

  .field {
    display: flex;
    align-items: center;
    gap: 10px;
    flex-wrap: wrap;
  }

  select {
    font: inherit;
    font-size: 0.875rem;
    background: var(--control);
    color: var(--text);
    border-radius: var(--radius-sm);
    padding: 6px 10px;
    max-width: 100%;
  }

  .hint {
    flex: 1 1 12rem;
    min-width: 0;
    font-size: 0.75rem;
    color: var(--dim);
  }

  .bolt {
    color: var(--amber);
    display: inline-flex;
  }

  input[type="number"] {
    font: inherit;
    font-size: 0.875rem;
    font-variant-numeric: tabular-nums;
    width: 130px;
    background: var(--control);
    color: var(--text);
    border-radius: var(--radius-sm);
    padding: 6px 10px;
  }

  input[type="number"][aria-invalid="true"] {
    box-shadow: inset 0 0 0 2px var(--err);
  }

  input[type="checkbox"] {
    accent-color: var(--amber);
    margin: 0;
  }

  .err-text {
    font-size: 0.8125rem;
    color: var(--err);
  }

  .chip {
    display: inline-flex;
    align-items: center;
    gap: 7px;
    font-size: 0.8125rem;
    border-radius: var(--radius-sm);
    padding: 5px 12px;
    cursor: pointer;
    color: var(--soft);
    background: var(--control);
  }

  .chip.on {
    color: var(--text);
    background: var(--on-fill);
  }

  .rows {
    display: grid;
    grid-template-columns: minmax(0, 1fr) auto auto;
    column-gap: 12px;
  }

  .rows li {
    display: grid;
    grid-column: 1 / -1;
    grid-template-columns: subgrid;
    align-items: center;
    gap: 12px;
    padding: 7px 2px;
    border-top: 1px solid var(--line);
  }

  .rows li:last-child {
    border-bottom: 1px solid var(--line);
  }

  .own {
    min-width: 0;
    overflow-wrap: anywhere;
    display: inline-flex;
    align-items: center;
    gap: 9px;
    cursor: pointer;
  }

  .ammo {
    flex-wrap: wrap;
    display: inline-flex;
    align-items: center;
    gap: 8px;
    font-size: 0.8125rem;
  }

  .ammo input {
    width: 80px;
  }

  .ammo.off {
    opacity: 0.55;
  }

  .no-ammo {
    font-size: 0.8125rem;
  }

  .chips {
    display: flex;
    flex-wrap: wrap;
    gap: 8px;
  }

  .note {
    margin: 8px 0 0;
    font-size: 0.75rem;
    color: var(--dim);
  }

  .rows.flags {
    grid-template-columns: minmax(0, 1fr) minmax(0, 1.4fr);
  }

  /* Weapons: name, version, ammo, experience. */
  .rows.weapons {
    grid-template-columns: minmax(0, 1fr) auto auto auto;
  }

  .version-stack {
    display: inline-flex;
    flex-wrap: wrap;
    align-items: center;
    gap: 6px;
    min-width: 0;
  }

  .version.modded {
    border: 1px solid var(--lav);
  }

  .mod-tag {
    font-size: 0.6875rem;
    font-weight: 600;
    padding: 1px 6px;
    border-radius: 999px;
    color: var(--lav);
    border: 1px solid var(--lav);
    white-space: nowrap;
  }

  .mod-row {
    grid-column: 1 / -1;
    display: inline-flex;
    align-items: center;
    gap: 8px;
    padding-left: 28px;
  }

  .err-text.small {
    font-size: 0.75rem;
  }

  .version {
    font-size: 0.8125rem;
    padding: 5px 8px;
  }

  .version.mega {
    color: var(--gold);
    background: var(--on-fill);
  }

  .version.off {
    opacity: 0.55;
  }

  .unverified {
    display: inline-block;
    margin-right: 6px;
    padding: 0 6px;
    background: var(--ink-deep);
    border-radius: var(--radius-xs);
    font-size: 0.6875rem;
    color: var(--soft);
  }

  .group {
    margin: 12px 0 6px;
    font-size: 0.875rem;
  }

  .switch {
    font-size: 0.875rem;
  }

  .rows.planets {
    grid-template-columns: minmax(0, 1fr) auto auto auto;
  }

  .planet {
    min-width: 0;
    overflow-wrap: anywhere;
    display: flex;
    flex-direction: column;
    line-height: 1.3;
  }

  .bolts {
    display: inline-flex;
    gap: 6px;
    justify-self: end;
  }

  .goldbolt,
  .skill {
    padding: 0;
    width: 28px;
    height: 28px;
    justify-content: center;
    color: var(--dim);
  }

  .goldbolt.on {
    color: var(--bar-ink-deep);
    background: var(--gold);
    box-shadow: none;
    background: var(--on-fill);
  }

  @container (max-width: 560px) {
    .line {
      grid-template-columns: minmax(0, 1fr);
      gap: 6px;
    }

    .rows,
    .rows.planets {
      grid-template-columns: repeat(3, auto) minmax(0, 1fr);
    }

    .rows li > :first-child {
      grid-column: 1 / -1;
    }

    .rows.flags {
      grid-template-columns: minmax(0, 1fr);
    }
  }

  .skills {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(34px, 1fr));
    gap: 6px;
  }

  .skill {
    width: auto;
    font-size: 0.75rem;
    font-variant-numeric: tabular-nums;
  }

  .skill.on {
    color: var(--bar-ink-deep);
    background: var(--amber);
    box-shadow: none;
    background: var(--on-fill);
  }

  /* A skill point changed: the same amber dot, in the corner. */
  .skill.edited {
    position: relative;
  }

  .skill.edited::after {
    content: "";
    position: absolute;
    top: -2px;
    right: -2px;
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background: var(--amber);
    box-shadow: 0 0 0 2px var(--panel);
  }

  .problem {
    display: flex;
    align-items: flex-start;
    gap: 8px;
    margin: 0;
    font-size: 0.8125rem;
    color: var(--err);
  }
</style>
