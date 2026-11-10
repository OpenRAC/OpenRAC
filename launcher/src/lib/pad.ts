// The launcher with a controller, without a mouse or a touch screen: the d-pad or the left stick
// moves the focus to the nearest control in that direction, A presses it, B goes back (closes a
// menu or a dialog, then returns to the games), LB and RB change tabs, Y opens what a control has
// besides (a game's page), X is the page's own shortcut (a game's mods). On Android the activity
// hands the controller's buttons to `window.__openracPad` (MainActivity.kt); on a desktop they are
// read with the Gamepad API.
// Nothing here knows a page but the tabs' bar: it reads the document as it is.

export type PadButton = "up" | "down" | "left" | "right" | "accept" | "back" | "prev" | "next" | "more" | "extra";
type Direction = "up" | "down" | "left" | "right";

const DIRECTIONS: readonly string[] = ["up", "down", "left", "right"];
const BUTTONS: readonly string[] = [...DIRECTIONS, "accept", "back", "prev", "next", "more", "extra"];

/** A held direction moves again after REPEAT_DELAY_MS, then every REPEAT_MS. */
const REPEAT_DELAY_MS = 350;
const REPEAT_MS = 90;

// A row of tabs or radios (tabs.ts), and a menu's items (Menu.svelte), are left out of Tab's order
// and moved through with the arrows; the controller reaches each of them, and A takes the one it
// is on.
const ROVING = '[role="tablist"] [role="tab"], [role="radiogroup"] [role="radio"], [role="menu"] [role="menuitem"]';
const FOCUSABLE = `button, a[href], input, select, textarea, summary, [tabindex]:not([tabindex="-1"]), ${ROVING}`;

/** What B and Android's Back do once nothing is open; true when they did something. */
let backHome: () => boolean = () => false;

/** The buttons held, with the timer that repeats a direction. */
const held = new Map<PadButton, ReturnType<typeof setTimeout> | undefined>();
/** The control focused before a menu opened, focused again when it closes. */
let opener: HTMLElement | null = null;

function visible(el: HTMLElement): boolean {
  if (el.getClientRects().length === 0) return false;
  // What a closed <details> holds keeps its boxes in Chromium (Android's WebView) but cannot take
  // the focus: picked as the next control, it left the focus where it was.
  const folded = el.closest("details:not([open])");
  if (folded && !(el.tagName === "SUMMARY" && el.parentElement === folded)) return false;
  if (typeof el.checkVisibility === "function") return el.checkVisibility({ visibilityProperty: true });
  const style = getComputedStyle(el);
  return style.visibility !== "hidden" && style.display !== "none";
}

/** A tab, a radio or a menu item that the arrows move to (ROVING): not in the Tab order, but the
 *  controller goes to it. */
function rovingItem(el: HTMLElement): boolean {
  return el.matches(ROVING);
}

function usable(el: HTMLElement): boolean {
  return (
    !(el as HTMLButtonElement).disabled &&
    (el.tabIndex !== -1 || rovingItem(el)) &&
    !el.closest("[inert], [aria-hidden=true], [data-pad-skip]") &&
    visible(el)
  );
}

/** Where the focus may go: the open menu, else the topmost dialog, else the page. */
export function scope(doc: Document = document): HTMLElement {
  const menu = doc.querySelector<HTMLElement>('[role="menu"]');
  if (menu && visible(menu)) return menu;
  const dialogs = [...doc.querySelectorAll<HTMLElement>('[role="dialog"][aria-modal="true"]')].filter(visible);
  return dialogs.at(-1) ?? doc.body;
}

function focusables(root: HTMLElement): HTMLElement[] {
  return [...root.querySelectorAll<HTMLElement>(FOCUSABLE)].filter(usable);
}

interface Box {
  left: number;
  right: number;
  top: number;
  bottom: number;
}

/** Whether `to` is that way from `from`: past its edge, or at least with its centre beyond it by
 *  more than a pixel (a button centred in the same row, a fraction higher, is not above it). */
function ahead(from: Box, to: Box, dir: Direction): boolean {
  const centre = (a: number, b: number) => (a + b) / 2;
  switch (dir) {
    case "down":
      return centre(to.top, to.bottom) > centre(from.top, from.bottom) + 1 && to.top >= from.top + 1;
    case "up":
      return centre(to.top, to.bottom) < centre(from.top, from.bottom) - 1 && to.bottom <= from.bottom - 1;
    case "right":
      return centre(to.left, to.right) > centre(from.left, from.right) + 1 && to.left >= from.left + 1;
    case "left":
      return centre(to.left, to.right) < centre(from.left, from.right) - 1 && to.right <= from.right - 1;
  }
}

/** The gap from `from` to `to`'s near edge going `dir` (0 when they overlap), and to its far edge. */
function gaps(from: Box, to: Box, dir: Direction): { near: number; far: number } {
  switch (dir) {
    case "down":
      return { near: Math.max(0, to.top - from.bottom), far: to.bottom - from.bottom };
    case "up":
      return { near: Math.max(0, from.top - to.bottom), far: from.top - to.top };
    case "right":
      return { near: Math.max(0, to.left - from.right), far: to.right - from.right };
    case "left":
      return { near: Math.max(0, from.left - to.right), far: from.left - to.left };
  }
}

/**
 * How far `to` is from `from` going `dir`, or null when it is not that way: the gap along the
 * direction counts thirteen times as much as how far its centre is off to the side (squared, as
 * Android's own focus search weighs them), so that the next row is the row under this one, and
 * in it the control nearest this one's middle.
 */
export function distance(from: Box, to: Box, dir: Direction): number | null {
  if (!ahead(from, to, dir)) return null;
  const vertical = dir === "up" || dir === "down";
  const side = vertical
    ? (to.left + to.right) / 2 - (from.left + from.right) / 2
    : (to.top + to.bottom) / 2 - (from.top + from.bottom) / 2;
  const gap = gaps(from, to, dir).near;
  return 13 * gap * gap + side * side;
}

/** Whether `to` is in line with `from` across `dir`: beside it for left and right, above or below
 *  it for up and down. */
function inLine(from: Box, to: Box, dir: Direction): boolean {
  return dir === "up" || dir === "down"
    ? to.left < from.right && to.right > from.left
    : to.top < from.bottom && to.bottom > from.top;
}

/** Whether `a`, in line with `from`, wins over `b`, which is not, whatever their distances: always
 *  sideways (the next control in the row), and up or down unless `b` is wholly nearer than `a`
 *  (a control in the row in between, off to the side, comes first rather than being skipped). */
function lineWins(from: Box, a: Box, b: Box, dir: Direction): boolean {
  if (!inLine(from, a, dir) || inLine(from, b, dir)) return false;
  if (dir === "left" || dir === "right") return true;
  return gaps(from, a, dir).near < gaps(from, b, dir).far;
}

/** The control nearest `from` going `dir` among `items`, or null. */
export function nearest<T>(from: Box, items: { item: T; box: Box }[], dir: Direction): T | null {
  let best: { item: T; box: Box; d: number } | null = null;
  for (const { item, box } of items) {
    const d = distance(from, box, dir);
    if (d === null) continue;
    if (!best || lineWins(from, box, best.box, dir) || (!lineWins(from, best.box, box, dir) && d < best.d))
      best = { item, box, d };
  }
  return best?.item ?? null;
}

/** The control last focused on the page, focused again when a dialog or a menu closes. */
let lastOnPage: HTMLElement | null = null;

/** The page shown: what <main> holds (App.svelte), another element for each page. */
function pageNow(): Element | null {
  return document.querySelector("main")?.firstElementChild ?? null;
}

/** Where the focus was last: the control, its place, and the scope and the page it was in. When
 *  the control goes away (a press that changed the page, a row deleted, a dialog closed), the
 *  focus goes on from there (keep), so that the next A presses something rather than only
 *  bringing the focus back. */
let spot: { el: HTMLElement; box: DOMRect; root: HTMLElement; page: Element | null } | null = null;

function note(el: HTMLElement) {
  spot = { el, box: el.getBoundingClientRect(), root: scope(), page: pageNow() };
}

/** The control the focus was last on, while it is still in `root`. */
function spotIn(root: HTMLElement): HTMLElement | null {
  return spot?.root === root && spot.el.isConnected && root.contains(spot.el) ? spot.el : null;
}

/** Room under the control for what floats over the bottom of the page (the toasts): scrolled to,
 *  it stops above them rather than under them (app.css, --pad-aside). */
function clearAside(el: HTMLElement) {
  const aside = document.querySelector<HTMLElement>("[data-pad-aside]")?.getBoundingClientRect();
  const box = el.getBoundingClientRect();
  const under =
    aside && aside.height > 0 && !el.closest("[data-pad-aside]") && box.left < aside.right && box.right > aside.left;
  document.documentElement.style.setProperty("--pad-aside", `${under ? Math.max(0, innerHeight - aside.top) : 0}px`);
}

/** Scrolling to the focus glides, unless the player asked for less motion or holds a direction
 *  (each step then jumps, so that a glide does not fall behind the next). */
const glide = typeof matchMedia === "function" && !matchMedia("(prefers-reduced-motion: reduce)").matches;
let repeating = false;

function markFocus(el: HTMLElement | null) {
  document.querySelector("[data-pad-focus]")?.removeAttribute("data-pad-focus");
  el?.setAttribute("data-pad-focus", "");
}

function show(el: HTMLElement) {
  if (!el.closest('[role="dialog"], [role="menu"]')) lastOnPage = el;
  parked = false;
  clearAside(el);
  el.focus({ preventScroll: true });
  markFocus(el);
  // app.css gives the focus a margin, so its ring is not cut by the edge it scrolls to.
  el.scrollIntoView({ block: "nearest", inline: "nearest", behavior: glide && !repeating ? "smooth" : "auto" });
  note(el);
}

function current(items: HTMLElement[]): HTMLElement | null {
  // WebKit may decline programmatic DOM focus or leave it on the preceding control.
  if (document.documentElement.dataset.input === "pad") {
    const marked = items.find((el) => el.hasAttribute("data-pad-focus"));
    if (marked) return marked;
  }
  const active = document.activeElement;
  return active instanceof HTMLElement && items.includes(active) ? active : null;
}

/** The first control to focus in a scope: in a page, the one focused last, else the first in its
 *  main content; in a dialog, its main button. */
function start(root: HTMLElement, items: HTMLElement[]): HTMLElement | undefined {
  if (root === document.body) {
    if (lastOnPage && items.includes(lastOnPage)) return lastOnPage;
    const main = document.querySelector("main");
    const inMain = items.filter((el) => main?.contains(el));
    // The page's own pick first: the game shown (the side list's, or its tile on Android); else
    // its first control that is not a text field, which would open the keyboard on Android.
    const picked =
      inMain.find((el) => el.matches('[aria-current]:not([aria-current="false"])')) ??
      inMain.find((el) => !textEntry(el)) ??
      inMain[0];
    if (picked) return picked;
  }
  return items.find((el) => el.matches("[autofocus], .primary, .btn-primary")) ?? items[0];
}

function scrollable(el: HTMLElement | null): HTMLElement | null {
  for (let at = el; at; at = at.parentElement) {
    const style = getComputedStyle(at);
    if (/(auto|scroll)/.test(style.overflowY) && at.scrollHeight > at.clientHeight) return at;
  }
  return document.scrollingElement as HTMLElement | null;
}

/** The control nearest `box` among `items`, centre to centre. */
function closest(box: DOMRect, items: HTMLElement[]): HTMLElement | undefined {
  const x = (box.left + box.right) / 2;
  const y = (box.top + box.bottom) / 2;
  let best: HTMLElement | undefined;
  let score = Infinity;
  for (const el of items) {
    const b = el.getBoundingClientRect();
    const d = ((b.left + b.right) / 2 - x) ** 2 + ((b.top + b.bottom) / 2 - y) ** 2;
    if (d < score) {
      score = d;
      best = el;
    }
  }
  return best;
}

function move(dir: Direction) {
  const root = scope();
  const items = focusables(root);
  let from = current(items);
  if (!from) {
    // The focus put away (B closed the keyboard) or waiting for a busy button: the move goes on
    // from that control, which is still where it was.
    const still = spotIn(root);
    if (!still || !visible(still)) {
      const first = start(root, items);
      if (first) show(first);
      return;
    }
    from = still;
  }
  // A range, and a list to pick from, move their value sideways.
  const sideways = (dir === "left" || dir === "right") && from === current(items);
  if (sideways && from instanceof HTMLInputElement && from.type === "range") {
    if (dir === "left") from.stepDown();
    else from.stepUp();
    changed(from);
    return;
  }
  if (sideways && from instanceof HTMLSelectElement) {
    step(from, dir === "left" ? -1 : 1);
    return;
  }
  const box = from.getBoundingClientRect();
  const candidates = (list: HTMLElement[]) =>
    list.filter((el) => el !== from).map((el) => ({ item: el, box: el.getBoundingClientRect() }));
  // What floats over the page (the toasts, [data-pad-aside]) is reached only past the page's end
  // that way; from there, the page is as near as ever.
  const aside = (el: HTMLElement) => el.closest("[data-pad-aside]") !== null;
  const inAside = aside(from);
  const reachable = items.filter((el) => crosses(from, el, dir));
  const next = steer(
    from,
    nearest(box, candidates(reachable.filter((el) => inAside || !aside(el))), dir) ??
      (inAside ? null : nearest(box, candidates(reachable.filter(aside)), dir)),
    dir,
    items,
    (part) => nearest(box, candidates(reachable.filter((el) => part.contains(el))), dir),
  );
  if (!next && from !== document.activeElement && usable(from) && !textEntry(from)) {
    show(from);
    return;
  }
  if (next) {
    show(next);
    return;
  }
  // Nothing further that way: the text there scrolls into view instead.
  if (dir === "up" || dir === "down") {
    const area = scrollable(from);
    area?.scrollBy({ top: (dir === "down" ? 1 : -1) * area.clientHeight * 0.4, behavior: "smooth" });
  }
}

/**
 * Where a move that leaves a part of the page lands, when that part says so: `data-pad-<dir>` on
 * it holds a selector (the first control it matches takes the focus) or "none" (the focus stays).
 * A move inside the part is left as it is (a control of the part that way wins over a nearer one
 * outside it), and a selector that matches nothing leaves the move to the nearest control. Without it, the nearest control across the page could be anything: down
 * at the foot of the game's page went into the game list beside it. Into the tabs' bar the move
 * lands on the tab open.
 */
function steer(
  from: HTMLElement,
  next: HTMLElement | null,
  dir: Direction,
  items: HTMLElement[],
  within: (part: HTMLElement) => HTMLElement | null,
): HTMLElement | null {
  // Into the tabs' bar from the page: the tab open, not whichever is nearest (the logo, Settings).
  const bar = document.querySelector<HTMLElement>(BAR);
  if (next && bar?.contains(next) && !bar.contains(from)) {
    const open = items.find((el) => bar.contains(el) && el.matches("[aria-current]") && visible(el));
    if (open) return open;
  }
  const part = from.closest<HTMLElement>(`[data-pad-${dir}]`);
  if (!part || (next && part.contains(next))) return next;
  // A control of the part that way, even far to the side, comes before leaving it.
  const inside = within(part);
  if (inside) return inside;
  const rule = part.getAttribute(`data-pad-${dir}`) ?? "";
  if (rule === "none") return null;
  return items.find((el) => el !== from && el.matches(rule) && visible(el)) ?? next;
}

function escape(target: Element) {
  target.dispatchEvent(new KeyboardEvent("keydown", { key: "Escape", code: "Escape", bubbles: true }));
}

/** The launcher's tabs: Android's rail on the left (its bar at the bottom on a narrow window), the
 *  desktop's bar at the top. */
const BAR = "header.site-header";

/**
 * Whether the focus may go from `from` to `to` going `dir`, as far as the tabs' bar goes: into it
 * and out of it only across it (left and right for Android's rail, up and down for a bar along
 * the window's edge), and along it only inside it. Up in the settings jumped to the Games tab of
 * the rail beside them, as the tab was nearer than the control above at the far side.
 */
function crosses(from: HTMLElement, to: HTMLElement, dir: Direction): boolean {
  const bar = document.querySelector<HTMLElement>(BAR);
  if (!bar) return true;
  const fromBar = bar.contains(from);
  if (fromBar === bar.contains(to)) return true;
  const { width, height } = bar.getBoundingClientRect();
  const across = height > width ? dir === "left" || dir === "right" : dir === "up" || dir === "down";
  return across;
}

/** Tells the page that `el`'s value changed, as a player's own change does. */
function changed(el: HTMLElement) {
  el.dispatchEvent(new Event("input", { bubbles: true }));
  el.dispatchEvent(new Event("change", { bubbles: true }));
}

/** The choice `by` along from `select`'s own, round from one end to the other, past those that
 *  cannot be chosen. */
function step(select: HTMLSelectElement, by: number) {
  const n = select.options.length;
  for (let i = 1; i < n; i++) {
    const at = (((select.selectedIndex + by * i) % n) + n) % n;
    if (!select.options[at]?.disabled) {
      select.selectedIndex = at;
      changed(select);
      return;
    }
  }
}

/** A's press shows on the control as a finger's or the mouse's does (app.css `.pad-press`, as
 *  :active; on Android a ripple from its middle, ripple.ts). */
function pressed(el: HTMLElement) {
  el.classList.add("pad-press");
  setTimeout(() => {
    el.classList.remove("pad-press");
  }, 140);
  onPress(el);
}

function accept() {
  const items = focusables(scope());
  const el = current(items);
  if (!el) {
    // Nothing focused (the focus waits for a busy button, or was put away): it comes back first.
    const was = spotIn(scope());
    const first = was && usable(was) ? was : start(scope(), items);
    if (first) show(first);
    return;
  }
  if (el instanceof HTMLSelectElement) {
    // Android forwards controller keys to its native picker. On desktop and iOS the system
    // picker cannot receive our navigation events: A takes the next choice, as right does.
    if (!nativePicker) {
      step(el, 1);
      return;
    }
    try {
      el.showPicker();
    } catch {
      el.focus();
    }
    return;
  }
  if (el.getAttribute("aria-haspopup") === "menu") {
    opener = el;
    el.click();
    // The menu is drawn on the next frame: its first item takes the focus.
    requestAnimationFrame(() => {
      const menu = scope();
      if (menu.getAttribute("role") === "menu") focusables(menu)[0]?.focus();
    });
    return;
  }
  pressed(el);
  // A control can take A for itself: `padpress`, cancelled.
  if (!el.dispatchEvent(new CustomEvent("padpress", { cancelable: true }))) return;
  el.click();
  // A choice in a menu closes it: back to the button that opened it.
  if (el.getAttribute("role") === "menuitem") restoreOpener();
}

/** Y: what the focused control offers besides A, as `padmore`; it bubbles, so a page can answer
 *  for all its controls. */
function more() {
  const el = current(focusables(scope()));
  el?.dispatchEvent(new CustomEvent("padmore", { bubbles: true }));
}

/** X: the page's own shortcut, as `padextra` on the document (Android's Games: the game's mods). */
function extra() {
  document.dispatchEvent(new CustomEvent("padextra"));
}

function restoreOpener() {
  const el = opener;
  opener = null;
  requestAnimationFrame(() => {
    if (el?.isConnected && usable(el)) show(el);
  });
}

/** Whether `el` takes typed text (and so the keyboard on Android). */
function textEntry(el: Element | null): el is HTMLInputElement | HTMLTextAreaElement {
  return (
    el instanceof HTMLTextAreaElement ||
    (el instanceof HTMLInputElement && !["range", "checkbox", "radio", "button", "submit"].includes(el.type))
  );
}

/** B, or Android's Back: true when it did something here (else Android leaves the launcher). */
export function back(): boolean {
  const active = document.activeElement;
  // Typing: B puts the keyboard away first.
  if (textEntry(active)) {
    // The field keeps its place: the next move goes on from it (move), and the focus is not
    // brought back to it (keep), which would open the keyboard again.
    parked = true;
    active.blur();
    return true;
  }
  const root = scope();
  if (root !== document.body) {
    escape(active instanceof HTMLElement && root.contains(active) ? active : root);
    if (root.getAttribute("role") === "menu") restoreOpener();
    return true;
  }
  return backHome();
}

/** LB and RB: the tab before or after the one open. */
function tab(step: number) {
  const tabs = [...document.querySelectorAll<HTMLButtonElement>("header nav .tab")].filter(
    (tab) => !tab.disabled && visible(tab),
  );
  if (!tabs.length || scope() !== document.body) return;
  const at = tabs.findIndex((t) => t.classList.contains("on"));
  const next = tabs[(at + step + tabs.length) % tabs.length];
  // The new page starts at its content (keep, once it is there).
  leaving = { page: pageNow(), until: performance.now() + PAGE_WAIT_MS };
  setTimeout(schedule, PAGE_WAIT_MS);
  lastOnPage = null;
  spot = null;
  (document.activeElement as HTMLElement | null)?.blur();
  next?.click();
}

/** While a tab's page comes in (App.svelte slides it in, its code fetched the first time): the page
 *  it is leaving, and until when the focus waits for the next one. */
let leaving: { page: Element | null; until: number } | null = null;
const PAGE_WAIT_MS = 1500;

/** The focus put away on purpose (B closing the keyboard): keep leaves it so. */
let parked = false;

/**
 * With the controller in use, the focus stays on something the next press acts on. When the
 * control with it goes away or is closed over (a page changed by a press, a dialog opened or
 * closed, a row deleted), the focus goes to the control in its place: back to the same control
 * if it is shown again, the one nearest where it was on the same page, else where the page or
 * the dialog starts. Before, the next A only brought the focus back, and a second A was needed.
 */
function keep() {
  if (document.documentElement.dataset.input !== "pad" || holding || parked) return;
  const root = scope();
  const active = document.activeElement;
  if (active instanceof HTMLElement && active !== document.body && root.contains(active) && usable(active)) return;
  if (leaving) {
    if (pageNow() === leaving.page && performance.now() < leaving.until) return;
    leaving = null;
  }
  const items = focusables(root);
  if (!items.length) return;
  const lost = spotIn(root);
  if (lost && usable(lost)) {
    show(lost);
    return;
  }
  // Disabled while it works (a build, an install): the focus waits for it.
  if (lost && (lost as HTMLButtonElement).disabled && visible(lost)) return;
  const samePage = spot?.root === root && spot.page !== null && spot.page === pageNow();
  const near = spot && samePage ? closest(spot.box, items) : null;
  const next = near ?? start(root, items);
  if (next) show(next);
}

let scheduled = false;

/** Runs keep once the page has settled: after the changes of this frame, not between them. */
function schedule() {
  if (scheduled) return;
  scheduled = true;
  requestAnimationFrame(() => {
    scheduled = false;
    keep();
  });
}

function act(button: PadButton) {
  switch (button) {
    case "up":
    case "down":
    case "left":
    case "right":
      move(button);
      break;
    case "accept":
      accept();
      break;
    case "back":
      back();
      break;
    case "prev":
      tab(-1);
      break;
    case "next":
      tab(1);
      break;
    case "more":
      more();
      break;
    case "extra":
      extra();
      break;
  }
}

/** The controller is in use: focus is drawn and the pointer's hover is not (app.css); a touch or
 *  the mouse moved or pressed brings them back. */
function usingPad(on: boolean) {
  const mode = on ? "pad" : "pointer";
  if (!on) markFocus(null);
  if (document.documentElement.dataset.input !== mode) document.documentElement.dataset.input = mode;
}

/** Whether the host forwards controller keys to a native select picker (Android). */
let nativePicker = false;

/** What else shows A's press (ripple.ts on Android); set by startPad. */
let onPress: (el: HTMLElement) => void = () => undefined;

/** While a controls page waits for a press to bind (`holdPad(true)`), the controller does not move
 *  the focus or press anything: the press is the page's. */
let holding = false;

/** Holds the controller's navigation back while a press is captured, or lets it go again. What is
 *  held as it is let go acts only once let go and pressed again. */
export function holdPad(on: boolean) {
  holding = on;
  releasePad();
}

/** Says whether the controller's navigation is held back (holdPad). */
export function padHeld(): boolean {
  return holding;
}

/** A button pressed (`down:accept`) or let go (`up:accept`). A press while held is ignored, so
 *  that a hat seen both as keys and as an axis acts once. */
/** Where the buttons go instead of moving the focus, while a mod's own page is shown (ModTool.svelte):
 *  the page moves its own focus. */
let routed: ((button: PadButton, down: boolean) => void) | null = null;

/** Sends every press and let-go to `to` instead of the launcher's navigation, or (null) back to it. */
export function routePad(to: ((button: PadButton, down: boolean) => void) | null) {
  routed = to;
  releasePad();
}

function padEvent(event: string) {
  if (holding) return;
  const [kind, name] = event.split(":");
  if (!name || !BUTTONS.includes(name)) return;
  const button = name as PadButton;
  if (routed) {
    if (kind === "down" || kind === "up") routed(button, kind === "down");
    return;
  }
  if (kind === "down") {
    if (held.has(button)) return;
    usingPad(true);
    // A press of its own: the focus put away by the last one comes back where it is needed.
    parked = false;
    act(button);
    const repeat = () => {
      repeating = true;
      act(button);
      repeating = false;
      held.set(button, setTimeout(repeat, REPEAT_MS));
    };
    held.set(button, DIRECTIONS.includes(button) ? setTimeout(repeat, REPEAT_DELAY_MS) : undefined);
  } else if (kind === "up") {
    clearTimeout(held.get(button));
    held.delete(button);
  }
}

/** Every held button let go: the launcher lost the window (the game, a picker, a system dialog)
 *  and their ups go elsewhere. Without it the next press of a button held then was ignored, and a
 *  direction held then went on moving the focus. */
function releasePad() {
  for (const timer of held.values()) clearTimeout(timer);
  held.clear();
}

// ---- a desktop's controllers, through the Gamepad API ----

/** The standard mapping's buttons (w3c.github.io/gamepad), and the left stick past half way. */
const STANDARD: [number, PadButton][] = [
  [0, "accept"],
  [1, "back"],
  [4, "prev"],
  [5, "next"],
  [3, "more"],
  [2, "extra"],
  [9, "accept"],
  [12, "up"],
  [13, "down"],
  [14, "left"],
  [15, "right"],
];

/**
 * The buttons down on the controllers in `pads` now; with `acting`, those pressed or let go since
 * `previous` are acted on. Without `acting` (the launcher's window is not the one in front) they are
 * only noted: the browser reports a controller whichever window has the focus, and a press meant
 * for the game also moved and pressed things in the launcher behind it. A button still down when
 * the launcher comes back to the front acts only once it is let go and pressed again.
 */
export function pollGamepads(
  previous: Set<PadButton>,
  pads: readonly (Gamepad | null)[] = navigator.getGamepads(),
  acting = true,
): Set<PadButton> {
  const now = new Set<PadButton>();
  for (const pad of pads) {
    if (pad?.mapping !== "standard") continue;
    for (const [index, button] of STANDARD) if (pad.buttons[index]?.pressed) now.add(button);
    const [x = 0, y = 0] = pad.axes;
    if (x <= -0.5) now.add("left");
    if (x >= 0.5) now.add("right");
    if (y <= -0.5) now.add("up");
    if (y >= 0.5) now.add("down");
  }
  if (!acting) return now;
  for (const button of now) if (!previous.has(button)) padEvent(`down:${button}`);
  for (const button of previous) if (!now.has(button)) padEvent(`up:${button}`);
  return now;
}

/**
 * Starts reading the controller. `home` is what B does once nothing is open (back to the games);
 * true when it did something. Desktop reads the Gamepad API; Android and iOS send native events.
 * Only Android forwards controller keys to a native select picker.
 */
export function startPad(
  home: () => boolean,
  options: { readGamepads: boolean; nativePicker?: boolean; press?: (el: HTMLElement) => void },
) {
  const { readGamepads, press } = options;
  backHome = home;
  nativePicker = options.nativePicker ?? false;
  if (press) onPress = press;
  const w = window as unknown as {
    __openracPad?: typeof padEvent;
    __openracPadRelease?: typeof releasePad;
    __openracBack?: typeof back;
  };
  w.__openracPad = padEvent;
  w.__openracPadRelease = releasePad;
  w.__openracBack = back;
  window.addEventListener("blur", releasePad);
  document.addEventListener("visibilitychange", () => {
    if (document.hidden) releasePad();
  });
  window.addEventListener("pointerdown", () => {
    usingPad(false);
  });
  // The mouse moved by the player: the browser also sends a move that did not move (a page that
  // scrolled under a pointer standing still), which must not take the focus ring away.
  window.addEventListener("pointermove", (e) => {
    if (e.pointerType === "mouse" && Math.abs(e.movementX) + Math.abs(e.movementY) > 2) usingPad(false);
  });
  // The focus follows what is focused another way (the mouse, a dialog's own first control), and
  // stays on something whatever goes away (keep).
  document.addEventListener("focusin", (e) => {
    if (e.target instanceof HTMLElement && e.target.matches(FOCUSABLE) && usable(e.target)) {
      note(e.target);
      if (document.documentElement.dataset.input === "pad") markFocus(e.target);
    }
  });
  new MutationObserver(schedule).observe(document.body, {
    childList: true,
    subtree: true,
    attributes: true,
    attributeFilter: ["disabled", "inert", "open", "hidden", "aria-hidden"],
  });
  document.addEventListener("focusout", schedule);
  if (!readGamepads || typeof navigator.getGamepads !== "function") return;
  let pressed = new Set<PadButton>();
  let polling = false;
  const poll = () => {
    pressed = pollGamepads(pressed, navigator.getGamepads(), document.hasFocus() && !holding);
    if (navigator.getGamepads().some((p) => p)) requestAnimationFrame(poll);
    else polling = false;
  };
  window.addEventListener("gamepadconnected", () => {
    if (polling) return;
    polling = true;
    requestAnimationFrame(poll);
  });
}
