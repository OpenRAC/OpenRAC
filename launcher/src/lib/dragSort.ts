// Moving a row of a list by dragging its grip (My mods' gameplay mods and packs): a mouse, a pen or
// a finger. The rows are the list's children; a grip is an element with `data-grip` inside one.
// The row follows the pointer, a line shows where it will land, and on release the list is told
// the row's old and new place. A pad moves rows through the row's menu instead (Move up, Move to
// the top...), so nothing here listens to keys.

export interface DragSortOptions {
  /** Whether rows can be moved now (load order, nothing searched). */
  enabled: boolean;
  /** A row was dropped: its place before and after. */
  onmove: (from: number, to: number) => void;
}

/** Where a row dropped at Y lands among rows whose middles are MIDDLES (the dragged one's left out). */
export function dropIndex(middles: number[], from: number, y: number): number {
  let to = 0;
  middles.forEach((middle, i) => {
    if (i !== from && y > middle) to = i < from ? i + 1 : i;
  });
  return Math.max(0, Math.min(middles.length - 1, to));
}

export function dragSort(list: HTMLElement, initial: DragSortOptions) {
  let options = initial;
  let drag: { row: HTMLElement; from: number; startY: number; pointer: number; to: number } | null = null;

  const rows = () => Array.from(list.children) as HTMLElement[];

  function mark(to: number | null) {
    for (const [i, row] of rows().entries()) {
      row.classList.toggle("drop-above", to !== null && drag !== null && i === to && to < drag.from);
      row.classList.toggle("drop-below", to !== null && drag !== null && i === to && to > drag.from);
    }
  }

  function down(e: PointerEvent) {
    if (!options.enabled || e.button !== 0) return;
    const grip = (e.target as HTMLElement | null)?.closest("[data-grip]");
    if (!grip || !list.contains(grip)) return;
    const row = rows().find((r) => r.contains(grip));
    if (!row) return;
    e.preventDefault();
    const from = rows().indexOf(row);
    drag = { row, from, startY: e.clientY, pointer: e.pointerId, to: from };
    (grip as HTMLElement).setPointerCapture(e.pointerId);
    row.classList.add("dragging");
  }

  function move(e: PointerEvent) {
    if (drag?.pointer !== e.pointerId) return;
    drag.row.style.transform = `translateY(${e.clientY - drag.startY}px)`;
    const middles = rows().map((r) => {
      const box = r.getBoundingClientRect();
      // The dragged row's own box moves with it: its place in the list is where it started.
      return r === drag?.row ? Number.NaN : box.top + box.height / 2;
    });
    drag.to = dropIndex(middles, drag.from, e.clientY);
    mark(drag.to);
  }

  function up(e: PointerEvent) {
    if (drag?.pointer !== e.pointerId) return;
    const { row, from, to } = drag;
    row.style.transform = "";
    row.classList.remove("dragging");
    mark(null);
    drag = null;
    if (to !== from) options.onmove(from, to);
  }

  list.addEventListener("pointerdown", down);
  list.addEventListener("pointermove", move);
  list.addEventListener("pointerup", up);
  list.addEventListener("pointercancel", up);
  return {
    update(next: DragSortOptions) {
      options = next;
    },
    destroy() {
      list.removeEventListener("pointerdown", down);
      list.removeEventListener("pointermove", move);
      list.removeEventListener("pointerup", up);
      list.removeEventListener("pointercancel", up);
    },
  };
}
