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

/** The nearest box that scrolls around EL, or the page's own. */
function scrollerOf(el: HTMLElement): HTMLElement {
  for (let at = el.parentElement; at; at = at.parentElement) {
    const { overflowY } = getComputedStyle(at);
    if ((overflowY === "auto" || overflowY === "scroll") && at.scrollHeight > at.clientHeight) return at;
  }
  return (document.scrollingElement as HTMLElement | null) ?? document.documentElement;
}

/** How close to a scrolling box's edge the pointer scrolls it, and how far a frame. */
const EDGE = 48;
const STEP = 12;

export function dragSort(list: HTMLElement, initial: DragSortOptions) {
  let options = initial;
  let drag: {
    row: HTMLElement;
    from: number;
    to: number;
    pointer: number;
    startY: number;
    y: number;
    scroller: HTMLElement;
    startScroll: number;
    frame: number;
  } | null = null;

  const rows = () => Array.from(list.children) as HTMLElement[];

  function mark(to: number | null) {
    for (const [i, row] of rows().entries()) {
      row.classList.toggle("drop-above", to !== null && drag !== null && i === to && to < drag.from);
      row.classList.toggle("drop-below", to !== null && drag !== null && i === to && to > drag.from);
    }
  }

  /** The row under the pointer, and where it would land; the list scrolls by itself near an edge. */
  function follow() {
    if (!drag) return;
    const { scroller } = drag;
    const box =
      scroller === document.scrollingElement
        ? { top: 0, bottom: window.innerHeight }
        : scroller.getBoundingClientRect();
    if (drag.y < box.top + EDGE) scroller.scrollTop -= STEP;
    else if (drag.y > box.bottom - EDGE) scroller.scrollTop += STEP;
    // The row follows the pointer, and the page under it when it scrolls.
    drag.row.style.transform = `translateY(${drag.y - drag.startY + scroller.scrollTop - drag.startScroll}px)`;
    const middles = rows().map((r) => {
      const b = r.getBoundingClientRect();
      // The dragged row's own box moves with it: its place in the list is where it started.
      return r === drag?.row ? Number.NaN : b.top + b.height / 2;
    });
    drag.to = dropIndex(middles, drag.from, drag.y);
    mark(drag.to);
    drag.frame = requestAnimationFrame(follow);
  }

  function down(e: PointerEvent) {
    if (drag || !options.enabled || e.button !== 0) return;
    const grip = (e.target as HTMLElement | null)?.closest("[data-grip]");
    if (!grip || !list.contains(grip)) return;
    const row = rows().find((r) => r.contains(grip));
    if (!row) return;
    e.preventDefault();
    const from = rows().indexOf(row);
    const scroller = scrollerOf(list);
    drag = {
      row,
      from,
      to: from,
      pointer: e.pointerId,
      startY: e.clientY,
      y: e.clientY,
      scroller,
      startScroll: scroller.scrollTop,
      frame: 0,
    };
    (grip as HTMLElement).setPointerCapture(e.pointerId);
    row.classList.add("dragging");
    drag.frame = requestAnimationFrame(follow);
  }

  function move(e: PointerEvent) {
    if (drag?.pointer === e.pointerId) drag.y = e.clientY;
  }

  /** The drag ends: dropped (the list is told) or cancelled (the row goes back). */
  function end(drop: boolean) {
    if (!drag) return;
    const { row, from, to, frame } = drag;
    cancelAnimationFrame(frame);
    row.style.transform = "";
    row.classList.remove("dragging");
    mark(null);
    drag = null;
    if (drop && to !== from) options.onmove(from, to);
  }

  const up = (e: PointerEvent) => {
    if (drag?.pointer === e.pointerId) end(true);
  };
  const cancel = (e: PointerEvent) => {
    if (drag?.pointer === e.pointerId) end(false);
  };

  list.addEventListener("pointerdown", down);
  list.addEventListener("pointermove", move);
  list.addEventListener("pointerup", up);
  list.addEventListener("pointercancel", cancel);
  return {
    update(next: DragSortOptions) {
      options = next;
      // Moving was turned off (a search began): a drag under way goes back.
      if (!next.enabled) end(false);
    },
    destroy() {
      end(false);
      list.removeEventListener("pointerdown", down);
      list.removeEventListener("pointermove", move);
      list.removeEventListener("pointerup", up);
      list.removeEventListener("pointercancel", cancel);
    },
  };
}
