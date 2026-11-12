// The mods' records of one save, shared by the save editor's native rows (attached fields,
// modSaveAttach.ts) and the mods' own sections (ModSaveData.svelte). Each mod's record is its own
// file, written by write_mod_save_data_rac2 (refused while the game runs), never the card.
import { api, type ModSaveData } from "./api";
import type { ModValues } from "./modSaveAttach";

export class ModSaveStore {
  mods = $state<ModSaveData[] | null>(null);
  values = $state<ModValues>({});
  loadError = $state<string | null>(null);
  busy = $state<string | null>(null);
  readonly serial: string;
  readonly slot: number;

  constructor(serial: string, slot: number) {
    this.serial = serial;
    this.slot = slot;
  }

  async load(): Promise<void> {
    this.loadError = null;
    try {
      const read = await api.readModSaveDataRac2(this.serial, this.slot);
      this.mods = read;
      for (const m of read) this.take(m);
    } catch (e) {
      this.loadError = String(e);
    }
  }

  get(id: string, key: string): number {
    return this.values[id]?.[key] ?? 0;
  }

  set(id: string, key: string, value: number): void {
    this.values[id] = { ...(this.values[id] ?? {}), [key]: value };
  }

  take(read: ModSaveData): void {
    this.values[read.id] = Object.fromEntries(read.fields.map((f) => [f.key, f.value]));
  }

  changedKeys(m: ModSaveData): string[] {
    return m.fields.filter((f) => this.get(m.id, f.key) !== f.value).map((f) => f.key);
  }

  /** The mods with unsaved changes. */
  dirtyMods(): ModSaveData[] {
    return (this.mods ?? []).filter((m) => this.changedKeys(m).length > 0);
  }

  /** Writes mod `m`'s changed fields; throws the command's error. */
  async save(m: ModSaveData): Promise<void> {
    const keys = this.changedKeys(m);
    if (keys.length === 0) return;
    this.busy = m.id;
    try {
      const read = await api.writeModSaveDataRac2(
        this.serial,
        this.slot,
        m.id,
        Object.fromEntries(keys.map((k) => [k, this.get(m.id, k)])),
      );
      this.mods = (this.mods ?? []).map((x) => (x.id === read.id ? read : x));
      this.take(read);
    } finally {
      this.busy = null;
    }
  }

  /** Writes every mod with changes, each by its own path. */
  async saveAll(): Promise<void> {
    for (const m of this.dirtyMods()) await this.save(m);
  }

  resetAll(): void {
    for (const m of this.mods ?? []) this.take(m);
  }
}
