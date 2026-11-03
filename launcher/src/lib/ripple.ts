// Android's touch feedback: a press on a control sends a wave out from the finger, clipped to the
// control's shape (app.css, .ripples and .ripple), as the buttons of Android's own apps do. Not on
// the navigation rail (Header.svelte), whose highlight slides to the tab pressed instead.

const CONTROLS = 'button, .button, [role="menuitem"], label.option, label.setting';

function press(event: PointerEvent) {
  const target = event.target instanceof Element ? event.target.closest<HTMLElement>(CONTROLS) : null;
  if (target) wave(target, event.clientX, event.clientY);
}

/** A controller's A on `target` (pad.ts): the same wave, from the control's middle. */
export function padRipple(target: HTMLElement) {
  if (!target.matches(CONTROLS)) return;
  const box = target.getBoundingClientRect();
  wave(target, (box.left + box.right) / 2, (box.top + box.bottom) / 2);
}

function wave(target: HTMLElement, x: number, y: number) {
  if ((target as HTMLButtonElement).disabled || target.closest(".site-header")) return;
  const box = target.getBoundingClientRect();
  // Wide enough to cover the control from any point inside it.
  const size = 2 * Math.hypot(box.width, box.height);
  if (getComputedStyle(target).position === "static") target.style.position = "relative";
  const clip = document.createElement("span");
  clip.className = "ripples";
  clip.setAttribute("aria-hidden", "true");
  const ring = document.createElement("span");
  ring.className = "ripple";
  ring.style.left = `${x - box.left}px`;
  ring.style.top = `${y - box.top}px`;
  ring.style.width = ring.style.height = `${size}px`;
  clip.append(ring);
  target.append(clip);
  const done = () => {
    clip.remove();
  };
  ring.addEventListener("animationend", done);
  // Without animations (reduced motion) there is no end to wait for.
  setTimeout(done, 800);
}

export function startRipples() {
  document.addEventListener("pointerdown", press, { passive: true });
}
