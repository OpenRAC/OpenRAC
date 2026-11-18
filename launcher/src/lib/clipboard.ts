// Copying text for the player. Every Copy button (Activity, Develop's console, a step's command,
// the crash notice, the reports list) writes to the clipboard the same way and, when the browser
// refuses, says so in an error toast in its own words: what the player can do instead.
import { toast } from "./app.svelte";

/**
 * Puts `text` on the clipboard. True when it is there; false when the browser refused (no
 * clipboard, no permission), after `refused` has been shown as an error toast. A caller that
 * shows "Copied" does so only on true.
 */
export async function copyText(text: string, refused: string): Promise<boolean> {
  try {
    await navigator.clipboard.writeText(text);
    return true;
  } catch {
    // Linux's WebKitGTK refuses the async clipboard once the click's gesture has passed, as it has
    // after the text is read from the launcher (a report). The webview's own copy command still
    // works there.
    if (copyCommand(text)) return true;
    toast(refused, "error");
    return false;
  }
}

/** Copies `text` through a hidden text field and the document's copy command. True when copied. */
function copyCommand(text: string): boolean {
  if (typeof document === "undefined") return false;
  // Deprecated, but the one copy WebKitGTK still allows after the gesture.
  // eslint-disable-next-line @typescript-eslint/no-deprecated
  const command = document.execCommand.bind(document) as ((name: string) => boolean) | undefined;
  if (typeof command !== "function") return false;
  const field = document.createElement("textarea");
  field.value = text;
  field.setAttribute("readonly", "");
  field.style.position = "fixed";
  field.style.opacity = "0";
  document.body.appendChild(field);
  const focused = document.activeElement as HTMLElement | null;
  try {
    field.select();
    return command("copy");
  } catch {
    return false;
  } finally {
    field.remove();
    focused?.focus();
  }
}
